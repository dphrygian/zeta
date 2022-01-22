#include "core.h"
#include "wbaction.h"
#include "simplestring.h"
#include "wbactionstack.h"
#include "Components/wbcompowner.h"
#include "wbevent.h"
#include "wbworld.h"

WBAction::WBAction()
{
}

WBAction::~WBAction()
{
}

/*virtual*/ void WBAction::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	// Do nothing
	Unused( DefinitionName );
}

/*virtual*/ void WBAction::Execute()
{
	// Do nothing
}

WBEntity* WBAction::GetEntity() const
{
	WBEntity* const pActingEntity	= WBActionStack::TopActingEntity();

#define PARANOID_ACTION_ENTITY_CHECK 0
#if PARANOID_ACTION_ENTITY_CHECK
	// HACKHACK: Compare EventOwner and ActingEntity, warn if they're different;
	// I want to stop using EventOwner as the default entity of actions.
	STATIC_HASHED_STRING( EventOwner );
	WBEntity* const pEventOwner		= WBActionStack::TopEvent().GetEntity( sEventOwner );

	if( pEventOwner != pActingEntity && pEventOwner != NULL ) // I got tired of this warning when EventOwner was null
	{
		const SimpleString EventName = WBActionStack::TopEvent().GetEventNameString();

		PRINTF( "New implementation of WBAction::GetEntity() returned different result than old.\n" );
		PRINTF( "Event: %s\n", EventName.CStr() );
		PRINTF( "Old version (EventOwner): %s\n", pEventOwner ? pEventOwner->GetUniqueName().CStr() : "None" );
		PRINTF( "New version (ActingEntity): %s\n", pActingEntity ? pActingEntity->GetUniqueName().CStr() : "None" );
		WARNDESC( "New implementation of WBAction::GetEntity() returned different result than old." );
	}
#endif

	return pActingEntity;
}

WBEntity* WBAction::GetTopmostOwner() const
{
	return WBCompOwner::GetTopmostOwner( GetEntity() );
}

WBEventManager* WBAction::GetEventManager() const
{
	return WBWorld::GetInstance()->GetEventManager();
}
