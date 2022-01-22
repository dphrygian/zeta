#include "core.h"
#include "wbcomprosasensorpatrol.h"
#include "Components/wbcomprodinblackboard.h"
#include "wbcomprosapatrolpath.h"
#include "wbcomprosatransform.h"
#include "configmanager.h"

WBCompRosaSensorPatrol::WBCompRosaSensorPatrol()
:	m_MoveOutputBlackboardKey()
,	m_TurnOutputBlackboardKey()
{
}

WBCompRosaSensorPatrol::~WBCompRosaSensorPatrol()
{
}

/*virtual*/ void WBCompRosaSensorPatrol::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( MoveOutputBlackboardKey );
	m_MoveOutputBlackboardKey = ConfigManager::GetInheritedHash( sMoveOutputBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( TurnOutputBlackboardKey );
	m_TurnOutputBlackboardKey = ConfigManager::GetInheritedHash( sTurnOutputBlackboardKey, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBCompRosaSensorPatrol::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnInitializedQueued );
	STATIC_HASHED_STRING( SetNextPatrolNode );
	STATIC_HASHED_STRING( SetNearestPatrolNode );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitializedQueued )
	{
		PromoteCurrentPatrolNode();
	}
	else if( EventName == sSetNextPatrolNode )
	{
		WBCompRosaPatrolPath* const pPatrolPath = WB_GETCOMP( GetEntity(), RosaPatrolPath );
		DEVASSERT( pPatrolPath );

		pPatrolPath->SetNextNode();
		PromoteCurrentPatrolNode();
	}
	else if( EventName == sSetNearestPatrolNode )
	{
		WBEntity* const				pEntity		= GetEntity();
		DEVASSERT( pEntity );

		WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		WBCompRosaPatrolPath* const	pPatrolPath	= WB_GETCOMP( pEntity, RosaPatrolPath );
		DEVASSERT( pPatrolPath );

		pPatrolPath->SetNearestNode( pTransform->GetLocation() );
		PromoteCurrentPatrolNode();
	}
}

void WBCompRosaSensorPatrol::PromoteCurrentPatrolNode() const
{
	WBEntity* const					pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaPatrolPath* const		pPatrolPath	= WB_GETCOMP( pEntity, RosaPatrolPath );
	DEVASSERT( pPatrolPath );

	// Bypass knowledge and write directly to the blackboard (because there's nothing
	// for a thinker to select from and we know exactly what we want to do with this)
	WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
	DEVASSERT( pBlackboard );

	Vector							PatrolNodeLocation;
	Angles							PatrolNodeOrientation;

	if( pPatrolPath->HasPatrolNodes() )
	{
		PatrolNodeLocation		= pPatrolPath->GetCurrentNodeLocation();
		PatrolNodeOrientation	= pPatrolPath->GetCurrentNodeOrientation();
	}
	else
	{
		// Use the spawn point, if any; else, use current location so we don't fail
		STATIC_HASHED_STRING( SpawnLocation );
		const Vector SpawnLocation		= pBlackboard->GetVector( sSpawnLocation );

		STATIC_HASHED_STRING( SpawnOrientation );
		const Angles SpawnOrientation	= pBlackboard->GetAngles( sSpawnOrientation );

		if( !SpawnLocation.IsZero() )
		{
			PatrolNodeLocation		= SpawnLocation;
			PatrolNodeOrientation	= SpawnOrientation;
		}
		else
		{
			WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
			PatrolNodeLocation		= pTransform->GetLocation();
			PatrolNodeOrientation	= pTransform->GetOrientation();
		}
	}

	DEVASSERT( !PatrolNodeLocation.IsZero() );
	pBlackboard->SetVector( m_MoveOutputBlackboardKey, PatrolNodeLocation );
	pBlackboard->SetAngles( m_TurnOutputBlackboardKey, PatrolNodeOrientation );
}
