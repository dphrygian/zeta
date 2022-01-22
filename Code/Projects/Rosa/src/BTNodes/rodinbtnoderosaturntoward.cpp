#include "core.h"
#include "rodinbtnoderosaturntoward.h"
#include "configmanager.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosaaimotion.h"
#include "Components/wbcomprodinblackboard.h"
#include "idatastream.h"

RodinBTNodeRosaTurnToward::RodinBTNodeRosaTurnToward()
:	m_TurnTargetBlackboardKey()
,	m_TurnState( ETTS_Begin )
{
}

RodinBTNodeRosaTurnToward::~RodinBTNodeRosaTurnToward()
{
}

void RodinBTNodeRosaTurnToward::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( TurnTargetBlackboardKey );
	m_TurnTargetBlackboardKey = ConfigManager::GetHash( sTurnTargetBlackboardKey, HashedString::NullString, sDefinitionName );
}

void RodinBTNodeRosaTurnToward::OnStart()
{
	m_TurnState = ETTS_Begin;
}

void RodinBTNodeRosaTurnToward::OnFinish()
{
	WBCompRosaAIMotion* pAIMotion = WB_GETCOMP( GetEntity(), RosaAIMotion );
	ASSERT( pAIMotion );

	if( m_TurnState == ETTS_StartedTurn )
	{
		pAIMotion->StopMove();
	}
}

RodinBTNode::ETickStatus RodinBTNodeRosaTurnToward::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	WBEntity* const		pEntity		= GetEntity();
	WBCompRosaAIMotion*	pAIMotion	= WB_GETCOMP( pEntity, RosaAIMotion );
	ASSERT( pAIMotion );

	if( m_TurnState == ETTS_Begin )
	{
		WBCompRodinBlackboard* const	pAIBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
		ASSERT( pAIBlackboard );

		const WBEvent::EType			TargetType		= pAIBlackboard->GetType( m_TurnTargetBlackboardKey );

		if( TargetType == WBEvent::EWBEPT_Vector )
		{
			// Turn toward location

			const Vector TurnTarget = pAIBlackboard->GetVector( m_TurnTargetBlackboardKey );

			pAIMotion->SetTurnTarget( false, Angles() );
			pAIMotion->StartTurn( TurnTarget );
			m_TurnState = ETTS_StartedTurn;

			return ETS_Running;
		}
		else if( TargetType == WBEvent::EWBEPT_Entity )
		{
			// Turn toward entity

			WBEntity* const pTurnTargetEntity = pAIBlackboard->GetEntity( m_TurnTargetBlackboardKey );
			if( !pTurnTargetEntity )
			{
				return ETS_Fail;
			}

			WBCompRosaTransform* const pTransform = pTurnTargetEntity->GetTransformComponent<WBCompRosaTransform>();
			if( !pTransform )
			{
				return ETS_Fail;
			}

			const Vector TurnTarget = pTransform->GetLocation();

			pAIMotion->SetTurnTarget( false, Angles() );
			pAIMotion->StartTurn( TurnTarget );
			m_TurnState = ETTS_StartedTurn;

			return ETS_Running;
		}
		else if( TargetType == WBEvent::EWBEPT_Angles )
		{
			// Turn toward target orientation

			const Angles TargetOrientation = pAIBlackboard->GetAngles( m_TurnTargetBlackboardKey );

			pAIMotion->SetTurnTarget( true, TargetOrientation );
			pAIMotion->StartTurn( Vector() );
			m_TurnState = ETTS_StartedTurn;

			return ETS_Running;
		}
	}
	else
	{
		ASSERT( m_TurnState == ETTS_StartedTurn );

		if( pAIMotion->IsMoving() )
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
