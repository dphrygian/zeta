#include "core.h"
#include "wbactionrosagotolevel.h"
#include "rosaframework.h"
#include "rosacampaign.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosasaveload.h"
#include "wbactionfactory.h"

WBActionRosaGoToLevel::WBActionRosaGoToLevel()
:	m_GoToNextLevel( false )
,	m_GoToPrevLevel( false )
,	m_GoToHubLevel( false )
,	m_NewGame( false )
#if ROSA_USE_ACTIVESAVESLOT
,	m_NewGameSaveSlot()
,	m_NewGameConfirmOverwriteAction( NULL )
,	m_NewGameUseActiveSlot( false )
#endif
,	m_Immediate( false )
,	m_Level()
,	m_TeleportLabel()
{
}

WBActionRosaGoToLevel::~WBActionRosaGoToLevel()
{
#if ROSA_USE_ACTIVESAVESLOT
	SafeDelete( m_NewGameConfirmOverwriteAction );
#endif
}

/*virtual*/ void WBActionRosaGoToLevel::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( GoToNextLevel );
	m_GoToNextLevel = ConfigManager::GetBool( sGoToNextLevel, false, sDefinitionName );

	STATICHASH( GoToPrevLevel );
	m_GoToPrevLevel = ConfigManager::GetBool( sGoToPrevLevel, false, sDefinitionName );

	STATICHASH( GoToHubLevel );
	m_GoToHubLevel = ConfigManager::GetBool( sGoToHubLevel, false, sDefinitionName );

	STATICHASH( NewGame );
	m_NewGame = ConfigManager::GetBool( sNewGame, false, sDefinitionName );

#if ROSA_USE_ACTIVESAVESLOT
	STATICHASH( NewGameSaveSlot );
	m_NewGameSaveSlot = ConfigManager::GetString( sNewGameSaveSlot, "", sDefinitionName );

	STATICHASH( NewGameConfirmOverwriteAction );
	const SimpleString NewGameConfirmOverwriteActionDef = ConfigManager::GetString( sNewGameConfirmOverwriteAction, "", sDefinitionName );
	m_NewGameConfirmOverwriteAction = WBActionFactory::Create( NewGameConfirmOverwriteActionDef );

	STATICHASH( NewGameUseActiveSlot );
	m_NewGameUseActiveSlot = ConfigManager::GetBool( sNewGameUseActiveSlot, false, sDefinitionName );

	DEVASSERT( !m_NewGame || ( m_NewGameSaveSlot != "" && m_NewGameConfirmOverwriteAction != NULL ) || m_NewGameUseActiveSlot );
#endif

	STATICHASH( Immediate );
	m_Immediate = ConfigManager::GetBool( sImmediate, false, sDefinitionName );

	STATICHASH( Level );
	m_Level = ConfigManager::GetString( sLevel, "", sDefinitionName );

	STATICHASH( TeleportLabel );
	m_TeleportLabel = ConfigManager::GetHash( sTeleportLabel, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaGoToLevel::Execute()
{
	WBAction::Execute();

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaGame* const			pGame			= pFramework->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= GetEventManager();
	ASSERT( pEventManager );

	if( m_GoToNextLevel )
	{
		WB_MAKE_EVENT( GoToNextLevel, NULL );
		WB_LOG_EVENT( GoToNextLevel );
		WB_SET_AUTO( GoToNextLevel, Hash, TeleportLabel, m_TeleportLabel );
		WB_DISPATCH_EVENT( pEventManager, GoToNextLevel, pGame );
	}
	else if( m_GoToPrevLevel )
	{
		WB_MAKE_EVENT( GoToPrevLevel, NULL );
		WB_LOG_EVENT( GoToPrevLevel );
		WB_SET_AUTO( GoToPrevLevel, Hash, TeleportLabel, m_TeleportLabel );
		WB_DISPATCH_EVENT( pEventManager, GoToPrevLevel, pGame );
	}
	else if( m_GoToHubLevel )
	{
		WB_MAKE_EVENT( GoToHubLevel, NULL );
		WB_LOG_EVENT( GoToHubLevel );
		WB_DISPATCH_EVENT( pEventManager, GoToHubLevel, pGame );
	}
	else if( m_NewGame )
	{
#if ROSA_USE_ACTIVESAVESLOT
		DEVASSERT( m_NewGameSaveSlot != "" || m_NewGameUseActiveSlot );

		RosaSaveLoad* const		pSaveLoad	= pGame->GetSaveLoad();
		ASSERT( pSaveLoad );

		bool					GoToLevel	= false;
		if( m_NewGameUseActiveSlot )
		{
			GoToLevel = true;
		}
		else
		{
			// ZETAHACK
			// NOTE: This sets the active save slot *before* loading the level;
			// and if the slot is full and the user cancels the overwrite prompt,
			// this active slot will remain. This shouldn't be a problem as long
			// as this action is only ever used from non-saving worlds like the
			// title screen, but it is a bug waiting to happen.
			pSaveLoad->SetActiveSaveSlot( m_NewGameSaveSlot );

			if( pSaveLoad->IsSlotFull( m_NewGameSaveSlot ) )
			{
				ASSERT( m_NewGameConfirmOverwriteAction );
				m_NewGameConfirmOverwriteAction->Execute();
			}
			else
			{
				GoToLevel = true;
			}
		}

		if( GoToLevel )
#endif
		{
			WB_MAKE_EVENT( GoToInitialLevel, NULL );
			WB_LOG_EVENT( GoToInitialLevel );
			WB_SET_AUTO( GoToInitialLevel, Hash, TeleportLabel, m_TeleportLabel );
			WB_DISPATCH_EVENT( pEventManager, GoToInitialLevel, pGame );
		}
	}
	else
	{
		WB_MAKE_EVENT( GoToLevel, NULL );
		WB_LOG_EVENT( GoToLevel );
		WB_SET_AUTO( GoToLevel, Hash, Level, m_Level );
		WB_SET_AUTO( GoToLevel, Hash, TeleportLabel, m_TeleportLabel );
		WB_DISPATCH_EVENT( pEventManager, GoToLevel, pGame );
	}

	if( m_Immediate )
	{
		WB_MAKE_EVENT( GoToLevelImmediate, NULL );
		WB_LOG_EVENT( GoToLevelImmediate );
		WB_DISPATCH_EVENT( pEventManager, GoToLevelImmediate, pGame );
	}
}
