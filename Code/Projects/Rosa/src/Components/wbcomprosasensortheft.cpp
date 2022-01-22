#include "core.h"
#include "wbcomprosasensortheft.h"
#include "wbeventmanager.h"
#include "Components/wbcomprodinknowledge.h"
#include "wbcomprosatransform.h"

WBCompRosaSensorTheft::WBCompRosaSensorTheft()
{
	STATIC_HASHED_STRING( OnTheft );
	GetEventManager()->AddObserver( sOnTheft, this );
}

WBCompRosaSensorTheft::~WBCompRosaSensorTheft()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnTheft );
		pEventManager->RemoveObserver( sOnTheft, this );
	}
}

/*virtual*/ void WBCompRosaSensorTheft::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnTheft );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnTheft )
	{
		STATIC_HASHED_STRING( Thief );
		WBEntity* const pThief = Event.GetEntity( sThief );

		HandleTheft( pThief );

		// Also, forward event to this entity for scripting to handle.
		{
			WB_MAKE_EVENT( OnTheftSensed, GetEntity() );
			WB_SET_AUTO( OnTheftSensed, Entity, Thief, pThief );
			WB_DISPATCH_EVENT( GetEventManager(), OnTheftSensed, GetEntity() );
		}
	}
}

void WBCompRosaSensorTheft::HandleTheft( WBEntity* const pThief ) const
{
	WBCompRosaTransform* const			pTransform		= pThief->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRodinKnowledge* const			pKnowledge		= WB_GETCOMP( GetEntity(), RodinKnowledge );
	ASSERT( pKnowledge );

	WBCompRodinKnowledge::TKnowledge&	Knowledge		= pKnowledge->UpdateEntity( pThief );

	STATIC_HASHED_STRING( NeverExpire );
	Knowledge.SetBool( sNeverExpire, true );

	STATIC_HASHED_STRING( RegardAsHostile );
	Knowledge.SetBool( sRegardAsHostile, true );

	STATIC_HASHED_STRING( LastKnownLocation );
	Knowledge.SetVector( sLastKnownLocation, pTransform->GetLocation() );
	ASSERT( !pTransform->GetLocation().IsZero() );

	STATIC_HASHED_STRING( KnowledgeType );
	STATIC_HASHED_STRING( Target );
	Knowledge.SetHash( sKnowledgeType, sTarget );
}
