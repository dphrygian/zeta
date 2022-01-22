#include "core.h"
#include "rodinbtnodeconditiontimeout.h"
#include "configmanager.h"
#include "Components/wbcomprodinblackboard.h"
#include "idatastream.h"

RodinBTNodeConditionTimeout::RodinBTNodeConditionTimeout()
:	m_TimeoutPE()
,	m_BlackboardKey()
,	m_NextCanExecuteTime( 0.0f )
{
}

RodinBTNodeConditionTimeout::~RodinBTNodeConditionTimeout()
{
}

void RodinBTNodeConditionTimeout::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( TimeoutPE );
	m_TimeoutPE.InitializeFromDefinition( ConfigManager::GetString( sTimeoutPE, "", sDefinitionName ) );

	STATICHASH( BlackboardKey );
	m_BlackboardKey = ConfigManager::GetHash( sBlackboardKey, HashedString::NullString, sDefinitionName );
}

/*virtual*/ RodinBTNode::ETickStatus RodinBTNodeConditionTimeout::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

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
		return ETS_Fail;
	}

	WBParamEvaluator::SPEContext Context;
	Context.m_Entity = pEntity;
	m_TimeoutPE.Evaluate( Context );

	m_NextCanExecuteTime = Time + m_TimeoutPE.GetFloat();
	if( m_BlackboardKey )
	{
		pBlackboard->SetFloat( m_BlackboardKey, m_NextCanExecuteTime );
	}

	return ETS_Success;
}
