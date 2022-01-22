#include "core.h"
#include "wbactionrosaaddobjective.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaAddObjective::WBActionRosaAddObjective()
:	m_ObjectiveTag()
{
}

WBActionRosaAddObjective::~WBActionRosaAddObjective()
{
}

/*virtual*/ void WBActionRosaAddObjective::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Objective );
	m_ObjectiveTag = ConfigManager::GetHash( sObjective, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaAddObjective::Execute()
{
	WBAction::Execute();

	WB_MAKE_EVENT( AddObjective, NULL );
	WB_SET_AUTO( AddObjective, Hash, ObjectiveTag, m_ObjectiveTag );
	WB_DISPATCH_EVENT( GetEventManager(), AddObjective, RosaGame::GetPlayer() );
}
