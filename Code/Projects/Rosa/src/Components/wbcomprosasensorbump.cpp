#include "core.h"
#include "wbcomprosasensorbump.h"
#include "wbeventmanager.h"
#include "Components/wbcomprodinknowledge.h"
#include "wbcomprosatransform.h"

WBCompRosaSensorBump::WBCompRosaSensorBump()
{
}

WBCompRosaSensorBump::~WBCompRosaSensorBump()
{
}

/*virtual*/ void WBCompRosaSensorBump::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnAnyCollision );
	STATIC_HASHED_STRING( FakeBump );	// HACKHACK for Big Bads having special knowledge

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnAnyCollision )
	{
		STATIC_HASHED_STRING( CollidedEntity );
		WBEntity* const pCollidedEntity = Event.GetEntity( sCollidedEntity );

		HandleBump( pCollidedEntity );
	}
	else if( EventName == sFakeBump )
	{
		STATIC_HASHED_STRING( Target );
		WBEntity* const pTarget = Event.GetEntity( sTarget );

		HandleBump( pTarget );
	}
}

void WBCompRosaSensorBump::HandleBump( WBEntity* const pCollidedEntity ) const
{
	if( !pCollidedEntity )
	{
		return;
	}

	WBEntity* const						pEntity				= GetEntity();

	WBCompRosaTransform* const			pCollidedTransform	= pCollidedEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pCollidedTransform );

	WBCompRodinKnowledge* const			pKnowledge			= WB_GETCOMP( pEntity, RodinKnowledge );
	DEVASSERT( pKnowledge );

	WBCompRodinKnowledge::TKnowledge&	Knowledge			= pKnowledge->UpdateEntity( pCollidedEntity );

	const Vector						CollidedLocation	= pCollidedTransform->GetLocation();
	DEVASSERT( !CollidedLocation.IsZero() );

	// ROSANOTE: I'm treating bumping into someone as seeing them with 100% certainty, unless I find a reason not to.
	STATIC_HASHED_STRING( VisionCertainty );
	Knowledge.SetFloat( sVisionCertainty, 1.0f );

	STATIC_HASHED_STRING( LastKnownLocation );
	Knowledge.SetVector( sLastKnownLocation, CollidedLocation );

	STATIC_HASHED_STRING( LastSeenLocation );
	Knowledge.SetVector( sLastSeenLocation, CollidedLocation );

	STATIC_HASHED_STRING( LastSeenTime );
	Knowledge.SetFloat( sLastSeenTime, GetTime() );

	STATIC_HASHED_STRING( KnowledgeType );
	STATIC_HASHED_STRING( Target );
	Knowledge.SetHash( sKnowledgeType, sTarget );
}
