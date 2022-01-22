#include "core.h"
#include "uiscreen-rosalanguages.h"
#include "configmanager.h"
#include "uifactory.h"
#include "array.h"
#include "packstream.h"

UIScreenRosaLanguages::UIScreenRosaLanguages()
:	m_LanguageIndex( 0 )
,	m_Languages()
,	m_Y( 0.0f )
,	m_ButtonWidgetDefinitionName()
,	m_SelectLanguageActionDefinitionName()
,	m_Archetype()
,	m_YBase( 0.0f )
,	m_YStep( 0.0f )
,	m_X( 0.0f )
{
}

UIScreenRosaLanguages::~UIScreenRosaLanguages()
{
}

void UIScreenRosaLanguages::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Unused( DefinitionName );

	Flush();

	UIScreen::InitializeFromDefinition( DefinitionName );

	m_Languages.Clear();

	// Always push English because yeah.
	m_Languages.PushBack( "English" );

	Array<SimpleString> RosaLanguageFiles;
	PackStream::StaticGetFilesInFolder_Master( ".", true, ".rosalanguage", RosaLanguageFiles );
	FOR_EACH_ARRAY( LanguageFileIter, RosaLanguageFiles, SimpleString )
	{
		const SimpleString& LanguageFile = LanguageFileIter.GetValue();

		// Reload the config file so it overrides any previous Languages values
		ConfigManager::Load( PackStream( LanguageFile.CStr() ) );

		STATICHASH( NumLanguages );
		const uint NumLanguages = ConfigManager::GetInt( sNumLanguages );
		for( uint LanguageIndex = 0; LanguageIndex < NumLanguages; ++LanguageIndex )
		{
			const SimpleString Language = ConfigManager::GetSequenceString( "Language%d", LanguageIndex, "" );
			m_Languages.PushBackUnique( Language );
		}
	}

	InitializeRules();

	const uint NumLanguages = m_Languages.Size();
	for( m_LanguageIndex = 0; m_LanguageIndex < NumLanguages; ++m_LanguageIndex )
	{
		m_Y = m_YBase + m_LanguageIndex * m_YStep;

		CreateSelectLanguageActionDefinition();
		CreateButtonWidgetDefinition();
		CreateButtonWidget();
	}

	UpdateRender();
	ResetFocus();
	RefreshWidgets();
}

void UIScreenRosaLanguages::InitializeRules()
{
	MAKEHASH( m_Name );

	STATICHASH( Rules );
	const SimpleString UsingRules = ConfigManager::GetString( sRules, "", sm_Name );

	MAKEHASH( UsingRules );

	STATICHASH( Archetype );
	m_Archetype = ConfigManager::GetString( sArchetype, "", sUsingRules );

	STATICHASH( ParentHYBase );
	m_YBase = ConfigManager::GetFloat( sParentHYBase, 0.0f, sUsingRules );

	STATICHASH( ParentHYStep );
	m_YStep = ConfigManager::GetFloat( sParentHYStep, 0.0f, sUsingRules );

	STATICHASH( ParentWX );
	m_X = ConfigManager::GetFloat( sParentWX, 0.0f, sUsingRules );
}

void UIScreenRosaLanguages::CreateButtonWidgetDefinition()
{
	const SimpleString& Language = m_Languages[ m_LanguageIndex ];

	m_ButtonWidgetDefinitionName	= SimpleString::PrintF( "_LanguageButton%d", m_LanguageIndex );

	MAKEHASH( Language );
	const SimpleString ButtonString = ConfigManager::GetLocalizedString( sLanguage, Language.CStr() );

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
	ConfigManager::SetString( sAction0, m_SelectLanguageActionDefinitionName.CStr(), sm_ButtonWidgetDefinitionName );
}

void UIScreenRosaLanguages::CreateSelectLanguageActionDefinition()
{
	const SimpleString& Language = m_Languages[ m_LanguageIndex ];

	m_SelectLanguageActionDefinitionName = SimpleString::PrintF( "_LanguageAction%d", m_LanguageIndex );

	MAKEHASH( m_SelectLanguageActionDefinitionName );

	STATICHASH( ActionType );
	ConfigManager::SetString( sActionType, "RosaSetLanguage", sm_SelectLanguageActionDefinitionName );

	STATICHASH( Language );
	ConfigManager::SetString( sLanguage, Language.CStr(), sm_SelectLanguageActionDefinitionName );
}

void UIScreenRosaLanguages::CreateButtonWidget()
{
	UIWidget* const pButtonWidget = UIFactory::CreateWidget( m_ButtonWidgetDefinitionName, this, NULL );
	ASSERT( pButtonWidget );

	AddWidget( pButtonWidget );
}

/*virtual*/ void UIScreenRosaLanguages::Pushed()
{
	UIScreen::Pushed();

	// Reinitialize whenever this screen is pushed in case new languages have been loaded.
	InitializeFromDefinition( m_Name );
}
