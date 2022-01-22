#include "core.h"
#include "wbactionrosasavegame.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosasaveload.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "Common/uimanagercommon.h"
#include "Screens/uiscreen-rosasave.h"
#include "wbactionfactory.h"

WBActionRosaSaveGame::WBActionRosaSaveGame()
:	m_SaveSlot()
,	m_ConfirmOverwriteAction( NULL )
,	m_SaveToPendingSlot( false )
{
}

WBActionRosaSaveGame::~WBActionRosaSaveGame()
{
	SafeDelete( m_ConfirmOverwriteAction );
}

/*virtual*/ void WBActionRosaSaveGame::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( SaveSlot );
	m_SaveSlot = ConfigManager::GetString( sSaveSlot, "", sDefinitionName );

	STATICHASH( ConfirmOverwriteAction );
	const SimpleString ConfirmOverwriteActionDef = ConfigManager::GetString( sConfirmOverwriteAction, "", sDefinitionName );
	m_ConfirmOverwriteAction = WBActionFactory::Create( ConfirmOverwriteActionDef );

	STATICHASH( SaveToPendingSlot );
	m_SaveToPendingSlot = ConfigManager::GetBool( sSaveToPendingSlot, false, sDefinitionName );

	ASSERT( m_ConfirmOverwriteAction != NULL || m_SaveToPendingSlot );
}

/*virtual*/ void WBActionRosaSaveGame::Execute()
{
	WBAction::Execute();

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	ASSERT( pGame );

	RosaSaveLoad* const		pSaveLoad	= pGame->GetSaveLoad();
	ASSERT( pSaveLoad );

	if( m_SaveToPendingSlot )
	{
		SaveSlot( pSaveLoad->GetPendingSaveSlot() );
	}
	else if( pSaveLoad->IsSlotFull( m_SaveSlot ) )
	{
		pSaveLoad->SetPendingSaveSlot( m_SaveSlot );

		// NOTE: Not pushing context onto action stack. See if that's a problem?
		ASSERT( m_ConfirmOverwriteAction );
		m_ConfirmOverwriteAction->Execute();
	}
	else
	{
		// Save immediately
		SaveSlot( m_SaveSlot );
	}
}

void WBActionRosaSaveGame::SaveSlot( const SimpleString& Slot )
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	ASSERT( pGame );

	UIManagerCommon* const		pUIManager	= pFramework->GetUIManager();
	ASSERT( pUIManager );

	RosaSaveLoad* const		pSaveLoad	= pGame->GetSaveLoad();
	ASSERT( pSaveLoad );

	pSaveLoad->SaveSlot( Slot );

	// Refresh the save game screen in case it is open
	STATIC_HASHED_STRING( SaveGameScreen );
	UIScreenRosaSave* const pSaveScreen = pUIManager->GetScreen<UIScreenRosaSave>( sSaveGameScreen );
	pSaveScreen->RefreshSlots();
}
