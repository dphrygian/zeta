#include "core.h"
#include "rodinbtnodetimeout.h"
#include "configmanager.h"
#include "Components/wbcomprodinblackboard.h"
#include "idatastream.h"

RodinBTNodeTimeout::RodinBTNodeTimeout()
:	m_TimeoutPE()
,	m_BlackboardKey()
,	m_IgnoreIfFailed( false )
,	m_NextCanExecuteTime( 0.0f )
,	m_OldNextCanExecuteTime( 0.0f )
{
}

RodinBTNodeTimeout::~RodinBTNodeTimeout()
{
}

void RodinBTNodeTimeout::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	RodinBTNodeDecorator::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( TimeoutPE );
	m_TimeoutPE.InitializeFromDefinition( ConfigManager::GetString( sTimeoutPE, "", sDefinitionName ) );

	STATICHASH( BlackboardKey );
	m_BlackboardKey = ConfigManager::GetHash( sBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( IgnoreIfFailed );
	m_IgnoreIfFailed = ConfigManager::GetBool( sIgnoreIfFailed, false, sDefinitionName );
}

/*virtual*/ RodinBTNode::ETickStatus RodinBTNodeTimeout::Tick( const float DeltaTime )
{
	// We only need to apply the timeout when the child is starting.
	if( m_ChildStatus != ETS_None )
	{
		return RodinBTNodeDecorator::Tick( DeltaTime );
	}

	WBEntity* const					pEntity		= GetEntity();
	WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );

	if( m_BlackboardKey )
	{
		DEVASSERT( pBlackboard );
		m_NextCanExecuteTime = pBlackboard->GetFloat( m_BlackboardKey );
	}

	const float Time = GetTime();
	if( Time < m_NextCanExecuteTime )
	{
		// In all my use cases so far, success has been the expected behavior here.
		// If that ever changes, I can return failure and override the result with another decorator.
		// (Or, use a ConditionTimeout inside a sequence, that's the actual pattern I've used before.)
		return ETS_Success;
	}

	WBParamEvaluator::SPEContext Context;
	Context.m_Entity = pEntity;
	m_TimeoutPE.Evaluate( Context );

	m_OldNextCanExecuteTime	= m_NextCanExecuteTime;	// Store the old value in case we need to restore it on child failure
	m_NextCanExecuteTime	= Time + m_TimeoutPE.GetFloat();
	if( m_BlackboardKey )
	{
		pBlackboard->SetFloat( m_BlackboardKey, m_NextCanExecuteTime );
	}

	return RodinBTNodeDecorator::Tick( DeltaTime );
}

void RodinBTNodeTimeout::OnChildCompleted( RodinBTNode* pChildNode, ETickStatus TickStatus )
{
	RodinBTNodeDecorator::OnChildCompleted( pChildNode, TickStatus );

	if( m_IgnoreIfFailed && TickStatus == ETS_Fail )
	{
		// Restore the old value because we failed
		m_NextCanExecuteTime = m_OldNextCanExecuteTime;
		if( m_BlackboardKey )
		{
			WBEntity* const					pEntity		= GetEntity();
			WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
			DEVASSERT( pBlackboard );
			pBlackboard->SetFloat( m_BlackboardKey, m_NextCanExecuteTime );
		}
	}
}
