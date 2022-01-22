#include "core.h"
#include "wbrule.h"
#include "configmanager.h"

WBRule::WBRule()
:	m_Event()
,	m_Additive( false )
,	m_Conditions()
{
}

WBRule::~WBRule()
{
}

void WBRule::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Event );
	m_Event = ConfigManager::GetHash( sEvent, HashedString::NullString, sDefinitionName );

	STATICHASH( Additive );
	m_Additive = ConfigManager::GetBool( sAdditive, false, sDefinitionName );

	STATICHASH( NumConditions );
	const uint NumConditions = ConfigManager::GetInt( sNumConditions, 0, sDefinitionName );
	for( uint ConditionIndex = 0; ConditionIndex < NumConditions; ++ConditionIndex )
	{
		SCondition& Condition = m_Conditions.PushBack();

		Condition.m_ConditionPE.InitializeFromDefinition( ConfigManager::GetSequenceString( "Condition%d", ConditionIndex, "", sDefinitionName ) );
	}
}
