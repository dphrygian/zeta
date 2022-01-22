#include "core.h"
#include "rodinbtnoderosamoveto.h"
#include "configmanager.h"
#include "Components/wbcomprosaaimotion.h"
#include "Components/wbcomprodinblackboard.h"
#include "idatastream.h"

RodinBTNodeRosaMoveTo::RodinBTNodeRosaMoveTo()
:	m_MoveTargetBlackboardKey()
,	m_TurnTargetBlackboardKey()
,	m_Stance()
,	m_Cautious( false )
,	m_Wander( false )
,	m_WanderTargetDistance( 0.0f )
,	m_Flee( false )
,	m_FleeTargetDistance( 0.0f )
,	m_TetherLocationPE()
,	m_TetherDistance( 0.0f )
,	m_TetherDistanceZ( 0.0f )
,	m_ApproachDistance( 0.0f )
,	m_ApproachDistancePE()
,	m_UseActualTargetLocation( false )
,	m_ReachedThresholdMin( 0.0f )
,	m_ReachedThresholdMax( 0.0f )
,	m_FlyingDeflectionRadius( 0.0f )
,	m_PostDeflectionEndTime( 0.0f )
,	m_FlyingDestinationOffsetZ( 0.0f )
,	m_Teleport( false )
,	m_TeleportDelay( 0.0f )
,	m_TeleportDelayRemaining( 0.0f )
,	m_MoveState( EMTS_Begin )
#if BUILD_DEV
,	m_WarnMaxSteps( false )
#endif
{
}

RodinBTNodeRosaMoveTo::~RodinBTNodeRosaMoveTo()
{
}

void RodinBTNodeRosaMoveTo::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( MoveTargetBlackboardKey );
	m_MoveTargetBlackboardKey = ConfigManager::GetHash( sMoveTargetBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( TurnTargetBlackboardKey );
	m_TurnTargetBlackboardKey = ConfigManager::GetHash( sTurnTargetBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( Stance );
	m_Stance = ConfigManager::GetHash( sStance, HashedString::NullString, sDefinitionName );

	STATICHASH( Cautious );
	m_Cautious = ConfigManager::GetBool( sCautious, false, sDefinitionName );

	STATICHASH( Wander );
	m_Wander = ConfigManager::GetBool( sWander, false, sDefinitionName );

	STATICHASH( WanderTargetDistance );
	m_WanderTargetDistance = ConfigManager::GetFloat( sWanderTargetDistance, 0.0f, sDefinitionName );

	STATICHASH( Flee );
	m_Flee = ConfigManager::GetBool( sFlee, false, sDefinitionName );

	STATICHASH( FleeTargetDistance );
	m_FleeTargetDistance = ConfigManager::GetFloat( sFleeTargetDistance, 0.0f, sDefinitionName );

	STATICHASH( TetherLocationPE );
	m_TetherLocationPE.InitializeFromDefinition( ConfigManager::GetString( sTetherLocationPE, "", sDefinitionName ) );

	STATICHASH( TetherDistance );
	m_TetherDistance = ConfigManager::GetFloat( sTetherDistance, 0.0f, sDefinitionName );

	STATICHASH( TetherDistanceZ );
	m_TetherDistanceZ = ConfigManager::GetFloat( sTetherDistanceZ, 0.0f, sDefinitionName );

	STATICHASH( ApproachDistance );
	m_ApproachDistance = ConfigManager::GetFloat( sApproachDistance, 0.0f, sDefinitionName );

	STATICHASH( ApproachDistancePE );
	m_ApproachDistancePE.InitializeFromDefinition( ConfigManager::GetString( sApproachDistancePE, "", sDefinitionName ) );

	STATICHASH( UseActualTargetLocation );
	m_UseActualTargetLocation = ConfigManager::GetBool( sUseActualTargetLocation, false, sDefinitionName );

	STATICHASH( ReachedThresholdMin );
	m_ReachedThresholdMin = ConfigManager::GetFloat( sReachedThresholdMin, 0.0f, sDefinitionName );

	STATICHASH( ReachedThresholdMax );
	m_ReachedThresholdMax = ConfigManager::GetFloat( sReachedThresholdMax, 0.0f, sDefinitionName );

	STATICHASH( DeflectionRadius );
	m_FlyingDeflectionRadius = ConfigManager::GetFloat( sDeflectionRadius, 0.0f, sDefinitionName );

	STATICHASH( PostDeflectionEndTime );
	m_PostDeflectionEndTime = ConfigManager::GetFloat( sPostDeflectionEndTime, 0.0f, sDefinitionName );

	STATICHASH( FlyingDestinationOffsetZ );
	m_FlyingDestinationOffsetZ = ConfigManager::GetFloat( sFlyingDestinationOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( TeleportDelay );
	m_TeleportDelay = ConfigManager::GetFloat( sTeleportDelay, 0.0f, sDefinitionName );

	STATICHASH( Teleport );
	m_Teleport = ConfigManager::GetBool( sTeleport, m_TeleportDelay > 0.0f, sDefinitionName );

#if BUILD_DEV
	STATICHASH( WarnMaxSteps );
	m_WarnMaxSteps = ConfigManager::GetBool( sWarnMaxSteps, false, sDefinitionName );
#endif
}

void RodinBTNodeRosaMoveTo::OnStart()
{
	m_MoveState					= EMTS_Begin;
	m_TeleportDelayRemaining	= m_TeleportDelay;
}

void RodinBTNodeRosaMoveTo::OnFinish()
{
	WBCompRosaAIMotion* pAIMotion = WB_GETCOMP( GetEntity(), RosaAIMotion );
	ASSERT( pAIMotion );

	if( m_MoveState == EMTS_StartedMove )
	{
		pAIMotion->StopMove();
	}
}

RodinBTNode::ETickStatus RodinBTNodeRosaMoveTo::Tick( const float DeltaTime )
{
	WBEntity* const		pEntity		= GetEntity();
	WBCompRosaAIMotion*	pAIMotion	= WB_GETCOMP( pEntity, RosaAIMotion );
	DEVASSERT( pAIMotion );

	if( m_MoveState == EMTS_Begin )
	{
		WBParamEvaluator::SPEContext Context;
		Context.m_Entity = pEntity;
		m_TetherLocationPE.Evaluate( Context );

		if( m_TetherLocationPE.IsEvaluated() )
		{
			pAIMotion->SetTether( m_TetherLocationPE.GetVector(), m_TetherDistance, m_TetherDistanceZ );
		}
		else
		{
			pAIMotion->UnsetTether();
		}

		if( m_Wander )
		{
#if BUILD_DEV
			pAIMotion->SetWarnMaxSteps( m_WarnMaxSteps );
#endif
			pAIMotion->SetTurnTarget( false, Angles() );
			pAIMotion->SetStance( m_Stance );
			pAIMotion->SetCautious( m_Cautious );
			pAIMotion->SetReachedThreshold( m_ReachedThresholdMin, m_ReachedThresholdMax );
			pAIMotion->StartWander( m_WanderTargetDistance );
			m_MoveState = EMTS_StartedMove;

			return ETS_Running;
		}
		else
		{
			m_ApproachDistancePE.Evaluate( Context );
			const float ApproachDistance = ( m_ApproachDistancePE.GetType() == WBParamEvaluator::EPT_Float ) ? m_ApproachDistancePE.GetFloat() : m_ApproachDistance;

			WBCompRodinBlackboard* const	pAIBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
			DEVASSERT( pAIBlackboard );

			const WBEvent::EType			TargetType		= pAIBlackboard->GetType( m_MoveTargetBlackboardKey );

			if( TargetType == WBEvent::EWBEPT_Vector )
			{
				// Move to location

				const Vector	MoveTarget		= pAIBlackboard->GetVector( m_MoveTargetBlackboardKey );
				const bool		UseTurnTarget	= ( m_TurnTargetBlackboardKey != HashedString::NullString );
				const Angles	TurnTarget		= pAIBlackboard->GetAngles( m_TurnTargetBlackboardKey );

#if BUILD_DEV
				pAIMotion->SetWarnMaxSteps( m_WarnMaxSteps );
#endif
				pAIMotion->SetTurnTarget( UseTurnTarget, TurnTarget );
				pAIMotion->SetStance( m_Stance );
				pAIMotion->SetCautious( m_Cautious );
				pAIMotion->SetReachedThreshold( m_ReachedThresholdMin, m_ReachedThresholdMax );
				pAIMotion->SetDeflectionRadius( m_FlyingDeflectionRadius );
				pAIMotion->SetPostDeflectionEndTime( m_PostDeflectionEndTime );
				pAIMotion->SetFlyingDestinationOffsetZ( m_FlyingDestinationOffsetZ );
				pAIMotion->StartMove( MoveTarget, ApproachDistance );

				m_MoveState = EMTS_StartedMove;

				return ETS_Running;
			}
			else if( TargetType == WBEvent::EWBEPT_Entity )
			{
				// Move to entity

				WBEntity* const pMoveTargetEntity = pAIBlackboard->GetEntity( m_MoveTargetBlackboardKey );
				if( !pMoveTargetEntity )
				{
					return ETS_Fail;
				}

#if BUILD_DEV
				pAIMotion->SetWarnMaxSteps( m_WarnMaxSteps );
#endif
				pAIMotion->SetTurnTarget( false, Angles() );
				pAIMotion->SetStance( m_Stance );
				pAIMotion->SetCautious( m_Cautious );
				pAIMotion->SetReachedThreshold( m_ReachedThresholdMin, m_ReachedThresholdMax );
				pAIMotion->SetDeflectionRadius( m_FlyingDeflectionRadius );
				pAIMotion->SetPostDeflectionEndTime( m_PostDeflectionEndTime );
				pAIMotion->SetFlyingDestinationOffsetZ( m_FlyingDestinationOffsetZ );

				if( m_Flee )
				{
					pAIMotion->StartFlee( pMoveTargetEntity, m_FleeTargetDistance );
				}
				else
				{
					pAIMotion->StartFollow( pMoveTargetEntity, ApproachDistance, m_UseActualTargetLocation );
				}

				m_MoveState = EMTS_StartedMove;

				return ETS_Running;
			}
		}
	}
	else
	{
		DEVASSERT( m_MoveState == EMTS_StartedMove );

		if( m_Teleport )
		{
			m_TeleportDelayRemaining -= DeltaTime;
			if( m_TeleportDelayRemaining <= 0.0f )
			{
				// Try teleport and succeed or fail depending
				return pAIMotion->TeleportFinishMove() ? ETS_Success : ETS_Fail;
			}
		}

		// DLP 30 Dec 2018: Requiring the animation resource back is a hack for interrupted ranged attacks in pursue/attack parallel
		if( pAIMotion->IsMoving() || !pAIMotion->HasAnimationResource() )
		{
			return ETS_Running;
		}
		else if( pAIMotion->DidMoveSucceed() )
		{
			return ETS_Success;
		}
	}

	return ETS_Fail;
}
