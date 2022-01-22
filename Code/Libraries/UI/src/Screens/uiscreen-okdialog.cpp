#include "core.h"
#include "uiscreen-okdialog.h"
#include "Widgets/uiwidget-text.h"
#include "configmanager.h"

UIScreenOKDialog::UIScreenOKDialog()
{
	InitializeFromDefinition( "OKDialog" );
}

UIScreenOKDialog::~UIScreenOKDialog() {}

void UIScreenOKDialog::SetParameters(
	bool							PauseGame,
	const SimpleString&				OKString,
	const SimpleString&				OKDynamicString,
	const HashedString&				OKEvent,
	const SUICallback&				OKCallback,
	const Array<WBAction*>* const	pOKActions )
{
	m_PausesGame = PauseGame;

	// HACKY: Referencing widgets directly by index
	UIWidgetText* pDialog = static_cast< UIWidgetText* >( m_RenderWidgets[1] );
	UIWidgetText* pOK = static_cast< UIWidgetText* >( m_RenderWidgets[2] );

	// NOTE: This bypasses the "IsLiteral" property of the widget and always uses localized
	pDialog->m_String = ConfigManager::GetLocalizedString( OKString, "" );
	pDialog->m_DynamicString = ConfigManager::GetLocalizedString( OKDynamicString, "" );
	pDialog->InitializeFromDefinition( pDialog->m_Name );

	pOK->m_EventName	= OKEvent;
	pOK->m_Callback		= OKCallback;
	pOK->m_OwnsActions	= false;
	if( pOKActions )
	{
		pOK->m_Actions	= *pOKActions;
	}
	else
	{
		pOK->m_Actions.Clear();
	}
}
