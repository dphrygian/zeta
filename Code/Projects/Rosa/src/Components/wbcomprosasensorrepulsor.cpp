#include "core.h"
#include "wbcomprosasensorrepulsor.h"
#include "configmanager.h"
#include "mathcore.h"
#include "wbcomponentarrays.h"
#include "wbcomprosatransform.h"
#include "wbcomprosarepulsor.h"
#include "Components/wbcomprodinknowledge.h"
#include "rosaworld.h"
#include "collisioninfo.h"

WBCompRosaSensorRepulsor::WBCompRosaSensorRepulsor()
{
}

WBCompRosaSensorRepulsor::~WBCompRosaSensorRepulsor()
{
}

/*virtual*/ void WBCompRosaSensorRepulsor::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnRepulsed );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnRepulsed )
	{
		STATIC_HASHED_STRING( RepulsorEntity );
		WBEntity* const pRepulsorEntity = Event.GetEntity( sRepulsorEntity );

		HandleRepulsed( pRepulsorEntity );
	}
}

void WBCompRosaSensorRepulsor::HandleRepulsed( WBEntity* const pRepulsorEntity ) const
{
	DEVASSERT( pRepulsorEntity );

	RosaWorld* const				pWorld				= GetWorld();
	DEVASSERT( pWorld );

	WBEntity* const						pEntity				= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const			pTransform			= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRodinKnowledge* const			pKnowledge			= WB_GETCOMP( pEntity, RodinKnowledge );
	DEVASSERT( pKnowledge );

	WBCompRosaTransform* const			pRepulsorTransform	= pRepulsorEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pRepulsorTransform );

	const Vector						CurrentLocation		= pTransform->GetLocation();
	const Vector						RepulsorLocation	= pRepulsorTransform->GetLocation();

	// Do a line check to make sure repulsor entity is accessible (e.g. not on the other side of a door)
	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_UserFlags			= EECF_Nav;
	if( pWorld->LineCheck( CurrentLocation, RepulsorLocation, Info ) )
	{
		// Entity is inaccessible, ignore it
		return;
	}

	DEVASSERT( !RepulsorLocation.IsZero() );

	// Update knowledge
	// ROSANOTE: I'm treating repulsion sensor as seeing entity with 100% certainty, unless I find a reason not to.
	{
		WBCompRodinKnowledge::TKnowledge&	Knowledge		= pKnowledge->UpdateEntity( pRepulsorEntity );

		STATIC_HASHED_STRING( VisionCertainty );
		Knowledge.SetFloat( sVisionCertainty, 1.0f );

		STATIC_HASHED_STRING( LastKnownLocation );
		Knowledge.SetVector( sLastKnownLocation, RepulsorLocation );

		STATIC_HASHED_STRING( LastSeenLocation );
		Knowledge.SetVector( sLastSeenLocation, RepulsorLocation );

		STATIC_HASHED_STRING( LastSeenTime );
		Knowledge.SetFloat( sLastSeenTime, GetTime() );

		STATIC_HASHED_STRING( KnowledgeType );
		STATIC_HASHED_STRING( Target );
		Knowledge.SetHash( sKnowledgeType, sTarget );
	}
}
