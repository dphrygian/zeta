#include "core.h"
#include "wbactionrosacompleteobjective.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaCompleteObjective::WBActionRosaCompleteObjective()
:	m_ObjectiveTag()
,	m_Fail( false )
,	m_ForceAdd( false )
{
}

WBActionRosaCompleteObjective::~WBActionRosaCompleteObjective()
{
}

/*virtual*/ void WBActionRosaCompleteObjective::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Objective );
	m_ObjectiveTag = ConfigManager::GetHash( sObjective, HashedString::NullString, sDefinitionName );

	STATICHASH( Fail );
	m_Fail = ConfigManager::GetBool( sFail, false, sDefinitionName );

	STATICHASH( ForceAdd );
	m_ForceAdd = ConfigManager::GetBool( sForceAdd, false, sDefinitionName );
}

/*virtual*/ void WBActionRosaCompleteObjective::Execute()
{
	WBAction::Execute();

	WB_MAKE_EVENT( CompleteObjective, NULL );
	WB_SET_AUTO( CompleteObjective, Hash,	ObjectiveTag,	m_ObjectiveTag );
	WB_SET_AUTO( CompleteObjective, Bool,	Fail,			m_Fail );
	WB_SET_AUTO( CompleteObjective, Bool,	ForceAdd,		m_ForceAdd );
	WB_DISPATCH_EVENT( GetEventManager(), CompleteObjective, RosaGame::GetPlayer() );
}
