#include "core.h"
#include "uiscreen-rosanewgame.h"
#include "rosadifficulty.h"
#include "configmanager.h"
#include "uifactory.h"

UIScreenRosaNewGame::UIScreenRosaNewGame()
:	m_SaveSlotIndex( 0 )
,	m_SaveSlotInfos()
,	m_Y( 0.0f )
,	m_ButtonWidgetDefinitionName()
,	m_NewGameActionDefinitionName()
,	m_Archetype()
,	m_ConfirmOverwriteAction()
,	m_YBase( 0.0f )
,	m_YStep( 0.0f )
,	m_X( 0.0f )
{
}

UIScreenRosaNewGame::~UIScreenRosaNewGame()
{
}

void UIScreenRosaNewGame::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Flush();

	UIScreen::InitializeFromDefinition( DefinitionName );

	const bool FilterQuickAndAutosaves = true;	// ZETANOTE: For Zeta, I'll have a quicksave slot for dev but should never show it. And there won't be autosave slots.
	m_SaveSlotInfos.Clear();
	RosaSaveLoad::GetSaveSlotInfos( m_SaveSlotInfos, FilterQuickAndAutosaves );

	InitializeRules();

	const uint NumSaveSlots = m_SaveSlotInfos.Size();
	for( m_SaveSlotIndex = 0; m_SaveSlotIndex < NumSaveSlots; ++m_SaveSlotIndex )
	{
		// ZETANOTE/HACKHACK: Removing this offset because quicksaves and autosaves will not be visible.
		const float Offset = 0.0f;
		//const RosaSaveLoad::SSaveSlotInfo& SaveSlotInfo = m_SaveSlotInfos[ m_SaveSlotIndex ];
		//const float Offset = ( SaveSlotInfo.m_Type == RosaSaveLoad::ESST_Quicksave ) ? 0.0f : ( ( SaveSlotInfo.m_Type == RosaSaveLoad::ESST_Autosave ) ? 0.5f : 1.0f );

		m_Y = m_YBase + ( Offset + static_cast<float>( m_SaveSlotIndex ) ) * m_YStep;

		CreateNewGameActionDefinition();
		CreateButtonWidgetDefinition();
		CreateButtonWidget();
	}

	UpdateRender();
	ResetFocus();
	RefreshWidgets();
}

void UIScreenRosaNewGame::InitializeRules()
{
	MAKEHASH( m_Name );

	STATICHASH( Rules );
	const SimpleString UsingRules = ConfigManager::GetString( sRules, "", sm_Name );

	MAKEHASH( UsingRules );

	STATICHASH( Archetype );
	m_Archetype = ConfigManager::GetString( sArchetype, "", sUsingRules );

	STATICHASH( ConfirmOverwriteAction );
	m_ConfirmOverwriteAction = ConfigManager::GetString( sConfirmOverwriteAction, "", sUsingRules );

	STATICHASH( ParentHYBase );
	m_YBase = ConfigManager::GetFloat( sParentHYBase, 0.0f, sUsingRules );

	STATICHASH( ParentHYStep );
	m_YStep = ConfigManager::GetFloat( sParentHYStep, 0.0f, sUsingRules );

	STATICHASH( ParentWX );
	m_X = ConfigManager::GetFloat( sParentWX, 0.0f, sUsingRules );
}

void UIScreenRosaNewGame::CreateNewGameActionDefinition()
{
	const RosaSaveLoad::SSaveSlotInfo& SaveSlotInfo = m_SaveSlotInfos[ m_SaveSlotIndex ];

	m_NewGameActionDefinitionName = SimpleString::PrintF( "_NewGameAction%d", m_SaveSlotIndex );

	MAKEHASH( m_NewGameActionDefinitionName );

	STATICHASH( ActionType );
	ConfigManager::SetString( sActionType, "RosaGoToLevel", sm_NewGameActionDefinitionName );

	STATICHASH( NewGame );
	ConfigManager::SetBool( sNewGame, true, sm_NewGameActionDefinitionName );

	STATICHASH( NewGameSaveSlot );
	ConfigManager::SetString( sNewGameSaveSlot, SaveSlotInfo.m_SlotName.CStr(), sm_NewGameActionDefinitionName );

	STATICHASH( NewGameConfirmOverwriteAction );
	ConfigManager::SetString( sNewGameConfirmOverwriteAction, m_ConfirmOverwriteAction.CStr(), sm_NewGameActionDefinitionName );
}

void UIScreenRosaNewGame::CreateButtonWidgetDefinition()
{
	const RosaSaveLoad::SSaveSlotInfo& SaveSlotInfo = m_SaveSlotInfos[ m_SaveSlotIndex ];

	m_ButtonWidgetDefinitionName	= SimpleString::PrintF( "_NewGameButton%d", m_SaveSlotIndex );

	SimpleString ButtonString;
	if( SaveSlotInfo.m_Type == RosaSaveLoad::ESST_Quicksave )
	{
		STATICHASH( QuicksaveSlot );
		ButtonString = ConfigManager::GetLocalizedString( sQuicksaveSlot, "" );
	}
	else if( SaveSlotInfo.m_Type == RosaSaveLoad::ESST_Autosave )
	{
		STATICHASH( AutosaveSlot );
		ButtonString = SimpleString::PrintF( ConfigManager::GetLocalizedString( sAutosaveSlot, "" ), SaveSlotInfo.m_TypeIndex + 1 );
	}
	else if( SaveSlotInfo.m_Type == RosaSaveLoad::ESST_Save )
	{
		STATICHASH( SaveSlot );
		ButtonString = SimpleString::PrintF( ConfigManager::GetLocalizedString( sSaveSlot, "" ), SaveSlotInfo.m_TypeIndex + 1 );
	}
	else
	{
		// We shouldn't get level skips in here
		WARN;
	}

	if( SaveSlotInfo.m_Empty )
	{
		STATICHASH( EmptyGame );
		ButtonString += ConfigManager::GetLocalizedString( sEmptyGame, "" );
	}
	else
	{
		// Construct descriptive string from slot info
		// Description uses MM/DD/YYYY 24:00:00 format; it would be nice to localize this format, but no big deal

		MAKEHASHFROM( LevelName, SaveSlotInfo.m_LevelName );
		const SimpleString	LevelName		= ConfigManager::GetLocalizedString( sLevelName, "" );
		STATICHASH( SaveLoadArc );
		const SimpleString	ArcFormat		= ConfigManager::GetLocalizedString( sSaveLoadArc, "" );
		const SimpleString	ArcProgress		= SimpleString::PrintF( ArcFormat.CStr(), SaveSlotInfo.m_Legacy, SaveSlotInfo.m_Season, SaveSlotInfo.m_Episode );
		const SimpleString	SlotDescription	= SimpleString::PrintF( "%s - %s - %d/%d/%d %d:%02d:%02d",
												LevelName.CStr(),
												ArcProgress.CStr(),
												SaveSlotInfo.m_DateM, SaveSlotInfo.m_DateD, SaveSlotInfo.m_DateY,
												SaveSlotInfo.m_TimeH, SaveSlotInfo.m_TimeM, SaveSlotInfo.m_TimeS );
		ButtonString += SlotDescription;
	}

	MAKEHASH( m_ButtonWidgetDefinitionName );

	STATICHASH( UIWidgetType );
	ConfigManager::SetString( sUIWidgetType, "Text", sm_ButtonWidgetDefinitionName );

	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_Archetype.CStr(), sm_ButtonWidgetDefinitionName );

	STATICHASH( String );
	ConfigManager::SetString( sString, ButtonString.CStr(), sm_ButtonWidgetDefinitionName );

	STATICHASH( IsLiteral );
	ConfigManager::SetBool( sIsLiteral, true, sm_ButtonWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_X, sm_ButtonWidgetDefinitionName );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, m_Y, sm_ButtonWidgetDefinitionName );

	STATICHASH( Focus );
	ConfigManager::SetBool( sFocus, true, sm_ButtonWidgetDefinitionName );

	STATICHASH( NumActions );
	ConfigManager::SetInt( sNumActions, 1, sm_ButtonWidgetDefinitionName );

	STATICHASH( Action0 );
	ConfigManager::SetString( sAction0, m_NewGameActionDefinitionName.CStr(), sm_ButtonWidgetDefinitionName );
}

void UIScreenRosaNewGame::CreateButtonWidget()
{
	UIWidget* const pButtonWidget = UIFactory::CreateWidget( m_ButtonWidgetDefinitionName, this, NULL );
	ASSERT( pButtonWidget );

	AddWidget( pButtonWidget );
}

/*virtual*/ void UIScreenRosaNewGame::Pushed()
{
	// ZETANOTE: I've replaced difficulty buttons with save slots,
	// so this is going away.
	// Highlight whichever difficulty we're currently on
	//const uint MenuDifficulty = RosaDifficulty::GetMenuDifficulty();
	//m_FocusedWidget = m_FocusWidgets[ MenuDifficulty ];

	UIScreen::Pushed();

	// Reinitialize whenever this screen is pushed to get latest save slot infos.
	InitializeFromDefinition( m_Name );
}
