#include "core.h"
#include "wbcomprosahackable.h"
#include "rosagame.h"
#include "wbeventmanager.h"
#include "configmanager.h"

WBCompRosaHackable::WBCompRosaHackable()
:	m_BoardDef()
{
}

WBCompRosaHackable::~WBCompRosaHackable()
{
}

/*virtual*/ void WBCompRosaHackable::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( BoardDef );
	m_BoardDef = ConfigManager::GetInheritedHash( sBoardDef, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBCompRosaHackable::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( StartHack );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sStartHack )
	{
		// Tell the player that they are hacking this entity.
		STATIC_HASHED_STRING( HackTarget );
		WB_MAKE_EVENT( SetVariable, NULL );
		WB_SET_AUTO( SetVariable, Hash, Name, sHackTarget );
		WB_SET_AUTO( SetVariable, Entity, Value, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), SetVariable, RosaGame::GetPlayer() );
	}
}

/*virtual*/ void WBCompRosaHackable::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsHackable, true );
	WB_SET_CONTEXT( Event, Hash, BoardDef, m_BoardDef );
}
