#ifndef WBENTITY_H
#define WBENTITY_H

#include "map.h"
#include "multimap.h"
#include "hashedstring.h"
#include "simplestring.h"
#include "iwbeventobserver.h"

class WBComponent;
class WBScene;

class WBEntity : public IWBEventObserver
{
public:
	WBEntity();
	~WBEntity();	// Not virtual, because WorkbenchEntities aren't designed to be inherited.

	void			InitializeFromDefinition( const SimpleString& DefinitionName );
	void			SendOnInitializedEvent();

	void			Tick( const float DeltaTime );

	void			Render() const;

	void			Destroy();
	bool			IsDestroyed() const { return m_Destroyed; }

	uint			GetUID() const { DEVASSERT( m_UID > 0 ); return m_UID; }
	void			SetUID( const uint EntityUID ) { m_UID = EntityUID; }

	uint			GetSceneHandle() const { return m_SceneHandle; }
	void			SetSceneHandle( const uint EntitySceneHandle ) { m_SceneHandle = EntitySceneHandle; }

	WBScene*		GetScene() const { return m_Scene; }	// SRF: Not currently used
	void			SetScene( WBScene* const Scene ) { m_Scene = Scene; }

	WBComponent*	GetComponent( const HashedString& ComponentName ) const;

	WBComponent*			GetTransformComponent() const { return m_TransformComponent; }
	template<class C> C*	GetTransformComponent() const { return static_cast<C*>( GetTransformComponent() ); }
	void					SetTransformComponent( WBComponent* const pTransformComponent );

	WBComponent*			GetOwnerComponent() const { return m_OwnerComponent; }
	template<class C> C*	GetOwnerComponent() const { return static_cast<C*>( GetOwnerComponent() ); }
	void					SetOwnerComponent( WBComponent* const pOwnerComponent );

#if BUILD_DEV
#define WBENTITY_REPORT_PREFIX "    "
	void			Report() const;
	void			DebugRender() const;
	void			DebugRender_Internal() const;	// Implemented in Rosa
	void			SetShouldDebugRender( const bool ShouldDebugRender ) { m_ShouldDebugRender = ShouldDebugRender; }
	void			GoToPrevDebugRenderComponent();
	void			GoToNextDebugRenderComponent();
	bool			ShouldDebugRender() const { return m_ShouldDebugRender; }
	uint			GetDebugRenderLineFeed() const { return m_DebugRenderLineFeed; }
	SimpleString	DebugRenderLineFeed() const;
#endif

	void			Load( const IDataStream& Stream );
	void			Save( const IDataStream& Stream ) const;

	SimpleString	GetName() const { return m_DefinitionName; }
	SimpleString	GetUniqueName() const { return SimpleString::PrintF( "%s_0x%08X", m_DefinitionName.CStr(), m_UID ); }

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );
	virtual uint	GetEntityUID() const { return m_UID; }

	void			AddContextToEvent( WBEvent& Event ) const;

	// Helper to look up a component for a given entity without spawning it
	static SimpleString GetComponentDefinition( const SimpleString& DefinitionName, const HashedString& ComponentType );

private:
	void			AddComponent( const HashedString& ComponentType, const char* ComponentDefinition);

	void			ForwardEventToComponents( const WBEvent& Event ) const;

	SimpleString					m_DefinitionName;
	bool							m_Destroyed;

	uint							m_UID;
	uint							m_SceneHandle;			// SRF: I believe this is the owner scene's handle for this entity, not a handle *to* the scene
	WBScene*						m_Scene;				// SRF: This is only used (along with m_SceneHandle) to remove this entity from the scene in dtor

	Array<WBComponent*>				m_Components;			// All components in their header-defined order
	Map<HashedString, WBComponent*>	m_ComponentsMap;		// All components, mapped by their type name
	Array<WBComponent*>				m_TickComponents;		// Flattened array version of m_TickComponents, for better cache performance
	Multimap<int, WBComponent*>		m_TickComponentsTemp;	// Only the tickable components in their tick order; emptied after flattening
	Array<WBComponent*>				m_RenderComponents;		// Only the renderable components
	WBComponent*					m_TransformComponent;	// For optimization. Not guaranteed to exist. Actual type depends on game.
	WBComponent*					m_OwnerComponent;		// For optimization. Not guaranteed to exist. Actual type depends on game.

#if BUILD_DEBUG
	mutable int						m_IteratingComponentsRefCount;
#endif

#if BUILD_DEV
	bool							m_ShouldDebugRender;			// Config, but also can be set during play
	Array<WBComponent*>				m_DebugRenderComponents;		// Only the components set to ShouldDebugRender
	uint							m_DebugRenderComponentIndex;	// Which component to debug, or all of them if invalid
	bool							m_IgnoreReport;
	mutable uint					m_DebugRenderLineFeed;
#endif
};

#endif // WBENTITY_H
