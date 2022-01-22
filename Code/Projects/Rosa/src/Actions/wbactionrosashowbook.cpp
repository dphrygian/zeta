#include "core.h"
#include "wbactionrosashowbook.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "Common/uimanagercommon.h"
#include "uistack.h"
#include "uiscreen.h"
#include "Widgets/uiwidget-text.h"

WBActionRosaShowBook::WBActionRosaShowBook()
:	m_BookString()
,	m_BookStringPE()
,	m_IsDynamic( false )
,	m_BookScreen()
{
}

WBActionRosaShowBook::~WBActionRosaShowBook()
{
}

/*virtual*/ void WBActionRosaShowBook::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( BookString );
	m_BookString = ConfigManager::GetString( sBookString, "", sDefinitionName );

	STATICHASH( BookStringPE );
	const SimpleString BookStringPEDef = ConfigManager::GetString( sBookStringPE, "", sDefinitionName );
	m_BookStringPE.InitializeFromDefinition( BookStringPEDef );

	STATICHASH( IsDynamic );
	m_IsDynamic = ConfigManager::GetBool( sIsDynamic, false, sDefinitionName );

	STATICHASH( BookScreen );
	m_BookScreen = ConfigManager::GetHash( sBookScreen, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaShowBook::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_BookStringPE.Evaluate( PEContext );
	const SimpleString BookString = ( m_BookStringPE.GetType() == WBParamEvaluator::EPT_String ) ? m_BookStringPE.GetString() : m_BookString;

	ShowBookScreen( BookString, m_IsDynamic, m_BookScreen );
}

/*static*/ void WBActionRosaShowBook::ShowBookScreen( const SimpleString& BookString, const bool IsDynamic, const HashedString BookScreenOverride )
{
	RosaFramework* const pFramework = RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	UIManager* const pUIManager = pFramework->GetUIManager();
	DEVASSERT( pUIManager );

	UIStack* const pUIStack = pUIManager->GetUIStack();
	DEVASSERT( pUIStack );

	STATIC_HASHED_STRING( BookScreen );
	const HashedString UsingBookScreen = ( BookScreenOverride == HashedString::NullString ) ? sBookScreen : BookScreenOverride;
	UIScreen* const pBookScreen = pUIManager->GetScreen( UsingBookScreen );
	DEVASSERT( pBookScreen );

	STATIC_HASHED_STRING( BookText );
	UIWidgetText* const pBookText = pBookScreen->GetWidget<UIWidgetText>( sBookText );
	DEVASSERT( pBookText );

	MAKEHASH( BookString );

	if( IsDynamic )
	{
		pBookText->m_String			= "";
		pBookText->m_DynamicString	= ConfigManager::GetLocalizedString( BookString, "" );
	}
	else
	{
		pBookText->m_String			= ConfigManager::GetLocalizedString( BookString, "" );
		pBookText->m_DynamicString	= "";
	}
	pBookText->Reinitialize();

	pUIStack->Push( pBookScreen );
}
