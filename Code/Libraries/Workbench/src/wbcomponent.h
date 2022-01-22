#ifndef WBCOMPONENT_H
#define WBCOMPONENT_H

// Base class for entity components.

#include "hashedstring.h"
#include "iwbeventobserver.h"
#include "map.h"
#include "array.h"
#include "simplestring.h"

class WBEntity;
class IDataStream;
class WBEvent;
class WBEventManager;

#define DEFINE_WBCOMP_FACTORY( type )				static class WBComponent* Factory()			{ return new WBComp##type; }
typedef class WBComponent* ( *WBCompFactoryFunc )( void );

#define DEFINE_WBCOMP_SUPER( parent )				typedef parent Super;

#define DEFINE_WBCOMP_STATICNAME( type )			static const HashedString& GetStaticName()	{ static const HashedString sName( #type ); return sName; }
#define DEFINE_WBCOMP_NAME							virtual HashedString GetName() const		{ return GetStaticName(); }
#define DEFINE_WBCOMP_STATICREADABLENAME( type )	static const char* GetStaticReadableName()	{ return #type; }
#define DEFINE_WBCOMP_READABLENAME					virtual const char* GetReadableName() const	{ return GetStaticReadableName(); }

#define DEFINE_WBCOMP( type, parent )			\
	DEFINE_WBCOMP_SUPER( parent )				\
	DEFINE_WBCOMP_FACTORY( type )				\
	DEFINE_WBCOMP_STATICNAME( type )			\
	DEFINE_WBCOMP_NAME							\
	DEFINE_WBCOMP_STATICREADABLENAME( type )	\
	DEFINE_WBCOMP_READABLENAME

// This sort of enforces one component per type, but shouldn't prevent manually adding a component with a unique name if I want to do that
#define WB_GETCOMP( entity, type ) ( static_cast<WBComp##type*>( ( entity )->GetComponent( WBComp##type::GetStaticName() ) ) )
#define WB_GETCOMP_SAFE( entity, type ) ( ( entity ) ? ( static_cast<WBComp##type*>( ( entity )->GetComponent( WBComp##type::GetStaticName() ) ) ) : NULL )

// Not used in the usual spawning path; this gives us a component that doesn't actually belong to an entity.
#define WB_CREATECOMP( type, entitydef ) ( static_cast<WBComp##type*>( WBComponent::Create( WBComp##type::GetStaticName(), WBEntity::GetComponentDefinition( ( entitydef ), WBComp##type::GetStaticName() ), NULL ) ) )
#define WB_DESTROYCOMP( component ) do { ( component )->ShutDown(); SafeDelete( ( component ) ); } while ( 0 )

class WBComponent : public IWBEventObserver
{
public:
	WBComponent();
	virtual ~WBComponent();

	typedef Map<HashedString, WBCompFactoryFunc>	TFactoryMap;
	typedef Array<HashedString>						TCompTypes;		// For initialization in the order components are listed in header
	static void					RegisterWBCompFactory( const HashedString& TypeName, WBCompFactoryFunc Factory );
	static const TFactoryMap&	GetWBCompFactories();
	static const TCompTypes&	GetWBCompTypes();

	static void			InitializeBaseFactories();
	static void			ShutDownBaseFactories();

	static WBComponent*	Create( const HashedString& TypeName, const SimpleString& DefinitionName, WBEntity* const pEntity );

	// Expand as needed, there's no constraints on these values.
	enum ETickOrder
	{
		ETO_NoTick,
		ETO_TickFirst,
		ETO_TickSecond,
		ETO_TickDefault,
		ETO_TickLast,
	};

	// Constructor and destructor, effectively; but called when object is fully constructed.
	virtual void	Initialize();	// Called just before InitializeFromDefinition
	virtual void	ShutDown();		// Called just before destruction

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }

	// For fast component iteration
	virtual bool	BelongsInComponentArray() { return false; }

	virtual bool	IsRenderable() { return false; }
	virtual void	Render();

	WBEntity*		GetEntity() const { return m_Entity; }

	// Implemented by DEFINE_WBCOMP macro
	virtual HashedString	GetName() const = 0;
	virtual const char*		GetReadableName() const = 0;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

#if BUILD_DEV
#define WB_REPORT_SPACER "  "
#define WBCOMPONENT_REPORT_PREFIX "      "
#define WBPROPERTY_REPORT_PREFIX "        "
	virtual void	Report() const;
	virtual bool	HasDebugRender() const { return false; }
	virtual void	DebugRender( const bool GroupedRender ) const;
	bool			ShouldDebugRender() const { return m_DEV_ShouldDebugRender && HasDebugRender(); }
	SimpleString	DebugRenderLineFeed() const;
#endif

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

	virtual void	AddContextToEvent( WBEvent& Event ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	float			GetTime() const;
	WBEventManager*	GetEventManager() const;

	void			SetEntity( WBEntity* pEntity ) { DEVASSERT( !m_Entity ); DEVASSERT( pEntity ); m_Entity = pEntity; }

private:
	static TFactoryMap	sm_WBCompFactoryMap;
	static TCompTypes	sm_WBCompTypes;

	WBEntity*			m_Entity;

#if BUILD_DEV
	SimpleString		m_DEV_ReadableName;			// Mostly just here for natvis
	SimpleString		m_DEV_DefinitionName;		// Mostly just here for natvis
	bool				m_DEV_ShouldDebugRender;
#endif
};

#endif // WBCOMPONENT_H
