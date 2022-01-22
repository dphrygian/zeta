#include "core.h"
#include "wbactionrosaloadgame.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "configmanager.h"

WBActionRosaLoadGame::WBActionRosaLoadGame()
:	m_SaveSlot()
{
}

WBActionRosaLoadGame::~WBActionRosaLoadGame()
{
}

/*virtual*/ void WBActionRosaLoadGame::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( SaveSlot );
	m_SaveSlot = ConfigManager::GetString( sSaveSlot, "", sDefinitionName );
}

/*virtual*/ void WBActionRosaLoadGame::Execute()
{
	WBAction::Execute();

	RosaGame* const pGame = RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	pGame->RequestLoadSlot( m_SaveSlot );
}
