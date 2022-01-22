#ifndef WBCOMPSTATMOD_H
#define WBCOMPSTATMOD_H

#include "wbcomponent.h"
#include "multimap.h"
#include "hashedstring.h"
#include "map.h"
#include "set.h"

// For use when the static hash already exists
#define WB_MODIFY_FLOAT_X( name, var, statmod )									\
	DEVASSERT( ( statmod ) );													\
	const float name##AutoVar = ( statmod )->ModifyFloat( ( var ), s##name )

#define WB_MODIFY_FLOAT( name, var, statmod )									\
	STATIC_HASHED_STRING( name );												\
	DEVASSERT( ( statmod ) );													\
	const float name##AutoVar = ( statmod )->ModifyFloat( ( var ), s##name )

#define WB_MODIFY_FLOAT_SAFE( name, var, statmod )																\
	STATIC_HASHED_STRING( name );																				\
	WBCompStatMod* const name##StatMod = ( statmod );															\
	const float name##Eval = ( var );																			\
	const float name##AutoVar = name##StatMod ? name##StatMod->ModifyFloat( name##Eval, s##name ) : name##Eval

#define WB_MODDED( name ) name##AutoVar

class WBCompStatMod : public WBComponent
{
public:
	WBCompStatMod();
	virtual ~WBCompStatMod();

	DEFINE_WBCOMP( StatMod, WBComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	void			TriggerEvent( const HashedString& Event );
	void			UnTriggerEvent( const HashedString& Event );
	void			AddRefEvent( const HashedString& Event );
	void			ReleaseEvent( const HashedString& Event );
	void			SetEventActive( const HashedString& Event, bool Active );

	float			ModifyFloat( const float Value, const HashedString& StatName );

#if BUILD_DEV
	virtual void	Report() const;
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	enum EModifierFunction
	{
		EMF_None,
		EMF_Add,
		EMF_Multiply,
		EMF_Set,
	};

	void			AddStatMods( const SimpleString& DefinitionName );

	static EModifierFunction	GetModifierFunctionFromString( const HashedString& Function );
	static const char*			GetFunctionString( EModifierFunction Function );

	struct SStatModifier
	{
		SStatModifier();

		bool				m_Active;
		HashedString		m_Event;
		HashedString		m_Stat;
		EModifierFunction	m_Function;
		float				m_Value;
	};

	bool									m_Serialize;			// Config; because I really only want to serialize player stat mods right now (or I did at the time this was written)
	Set<HashedString>						m_NonSerializedEvents;	// Config; events that we ignore when loading (we'll still save them just in case this changes)
	Multimap<HashedString, SStatModifier>	m_StatModMap;			// Config; map of stat names to structure, for fastest lookup when modifying value
	Set<HashedString>						m_ActiveEvents;			// Serialized; all active events, whether refcounted or not
	Map<HashedString, uint>					m_ActiveEventRefCounts;	// Serialized; map of active event names to their refcounts; these are optional, the old Trigger/UnTrigger interface ignores them
};

#endif // WBCOMPSTATMOD_H
