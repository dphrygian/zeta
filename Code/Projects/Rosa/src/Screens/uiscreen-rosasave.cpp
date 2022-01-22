#include "core.h"
#include "uiscreen-rosasave.h"
#include "configmanager.h"
#include "uifactory.h"

UIScreenRosaSave::UIScreenRosaSave()
:	m_SaveSlotIndex( 0 )
,	m_SaveSlotInfos()
,	m_Y( 0.0f )
,	m_ButtonWidgetDefinitionName()
,	m_SaveActionDefinitionName()
,	m_Archetype()
,	m_ConfirmOverwriteAction()
,	m_YBase( 0.0f )
,	m_YStep( 0.0f )
,	m_X( 0.0f )
{
}

UIScreenRosaSave::~UIScreenRosaSave()
{
}

void UIScreenRosaSave::RefreshSlots()
{
	const bool FilterQuickAndAutosaves = true;
	m_SaveSlotInfos.Clear();
	RosaSaveLoad::GetSaveSlotInfos( m_SaveSlotInfos, FilterQuickAndAutosaves );

	InitializeRules();

	const uint NumSaveSlots = m_SaveSlotInfos.Size();
	for( m_SaveSlotIndex = 0; m_SaveSlotIndex < NumSaveSlots; ++m_SaveSlotIndex )
	{
		m_Y = m_YBase + m_SaveSlotIndex * m_YStep;

		CreateSaveActionDefinition();
		CreateButtonWidgetDefinition();
	}

	Reinitialize();
}

void UIScreenRosaSave::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Flush();

	UIScreen::InitializeFromDefinition( DefinitionName );

	const bool FilterQuickAndAutosaves = true;
	m_SaveSlotInfos.Clear();
	RosaSaveLoad::GetSaveSlotInfos( m_SaveSlotInfos, FilterQuickAndAutosaves );

	InitializeRules();

	const uint NumSaveSlots = m_SaveSlotInfos.Size();
	for( m_SaveSlotIndex = 0; m_SaveSlotIndex < NumSaveSlots; ++m_SaveSlotIndex )
	{
		m_Y = m_YBase + m_SaveSlotIndex * m_YStep;

		CreateSaveActionDefinition();
		CreateButtonWidgetDefinition();
		CreateButtonWidget();
	}

	UpdateRender();
	ResetFocus();
	RefreshWidgets();
}

void UIScreenRosaSave::InitializeRules()
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

void UIScreenRosaSave::CreateSaveActionDefinition()
{
	const RosaSaveLoad::SSaveSlotInfo& SaveSlotInfo = m_SaveSlotInfos[ m_SaveSlotIndex ];

	m_SaveActionDefinitionName = SimpleString::PrintF( "_SaveAction%d", m_SaveSlotIndex );

	MAKEHASH( m_SaveActionDefinitionName );

	STATICHASH( ActionType );
	ConfigManager::SetString( sActionType, "RosaSaveGame", sm_SaveActionDefinitionName );

	STATICHASH( SaveSlot );
	ConfigManager::SetString( sSaveSlot, SaveSlotInfo.m_SlotName.CStr(), sm_SaveActionDefinitionName );

	STATICHASH( ConfirmOverwriteAction );
	ConfigManager::SetString( sConfirmOverwriteAction, m_ConfirmOverwriteAction.CStr(), sm_SaveActionDefinitionName );
}

void UIScreenRosaSave::CreateButtonWidgetDefinition()
{
	const RosaSaveLoad::SSaveSlotInfo& SaveSlotInfo = m_SaveSlotInfos[ m_SaveSlotIndex ];

	m_ButtonWidgetDefinitionName	= SimpleString::PrintF( "_SaveButton%d", m_SaveSlotIndex );

	ASSERT( SaveSlotInfo.m_Type == RosaSaveLoad::ESST_Save );

	STATICHASH( SaveSlot );
	SimpleString ButtonString = SimpleString::PrintF( ConfigManager::GetLocalizedString( sSaveSlot, "" ), SaveSlotInfo.m_TypeIndex + 1 );

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
	ConfigManager::SetString( sAction0, m_SaveActionDefinitionName.CStr(), sm_ButtonWidgetDefinitionName );
}

void UIScreenRosaSave::CreateButtonWidget()
{
	UIWidget* const pButtonWidget = UIFactory::CreateWidget( m_ButtonWidgetDefinitionName, this, NULL );
	ASSERT( pButtonWidget );

	AddWidget( pButtonWidget );
}

/*virtual*/ void UIScreenRosaSave::Pushed()
{
	UIScreen::Pushed();

	// Reinitialize whenever this screen is pushed. Probably not actually necessary, but it doesn't hurt.
	InitializeFromDefinition( m_Name );
}
