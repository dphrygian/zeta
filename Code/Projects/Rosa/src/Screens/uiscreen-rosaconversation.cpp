#include "core.h"
#include "uiscreen-rosaconversation.h"
#include "rosagame.h"
#include "wbeventmanager.h"
#include "iuiinputmap.h"
#include "uimanager.h"
#include "wbworld.h"
#include "rosaframework.h"
#include "keyboard.h"
#include "mouse.h"

UIScreenRosaConversation::UIScreenRosaConversation()
{
}

UIScreenRosaConversation::~UIScreenRosaConversation()
{
}

/*virtual*/ UIScreen::ETickReturn UIScreenRosaConversation::TickInput()
{
	IUIInputMap* const	pInputMap	= m_UIManager->GetUIInputMap();
	Keyboard* const		pKeyboard	= m_UIManager->GetKeyboard();
	Mouse* const		pMouse		= m_UIManager->GetMouse();

	DEVASSERT( pInputMap );
	DEVASSERT( pKeyboard );
	DEVASSERT( pMouse );

	if( pInputMap->OnAccept() ||
		pInputMap->OnCancel() )
	{
		SkipConversationLine();
		return ETR_None;
	}

	// Use number keys to make dialogue choices (HACKHACK: hard-coded to 5 choices to match data)
	if( pKeyboard->OnRise( Keyboard::EB_1 ) || pKeyboard->OnRise( Keyboard::EB_Num1 ) ) { SelectConversationChoice( 0 ); }
	if( pKeyboard->OnRise( Keyboard::EB_2 ) || pKeyboard->OnRise( Keyboard::EB_Num2 ) ) { SelectConversationChoice( 1 ); }
	if( pKeyboard->OnRise( Keyboard::EB_3 ) || pKeyboard->OnRise( Keyboard::EB_Num3 ) ) { SelectConversationChoice( 2 ); }
	if( pKeyboard->OnRise( Keyboard::EB_4 ) || pKeyboard->OnRise( Keyboard::EB_Num4 ) ) { SelectConversationChoice( 3 ); }
	if( pKeyboard->OnRise( Keyboard::EB_5 ) || pKeyboard->OnRise( Keyboard::EB_Num5 ) ) { SelectConversationChoice( 4 ); }

	// Let mouse buttons skip lines if we're not clicking on a button
	// ROSANOTE: In Neon, either left or right mouse button would advance convos,
	// because they were longer and I would alternate taps to skip through.
	// For Rosa, I'm only using LMB (yes, this supports swapped mouse buttons).
	if( NULL == m_ClickedWidget )
	{
		if( pKeyboard->OnRise( LEFT_MOUSE_KEY ) ||
			pMouse->OnRise( LEFT_MOUSE_BUTTON ) )
		{
			SkipConversationLine();
			return ETR_None;
		}
	}

	return UIScreen::TickInput();
}

void UIScreenRosaConversation::SkipConversationLine()
{
	// Tell conversation system to skip to end of text
	RosaGame* const		pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( SkipConversationLine, NULL );
	WB_DISPATCH_EVENT( pEventManager, SkipConversationLine, pGame );
}

void UIScreenRosaConversation::SelectConversationChoice( uint ChoiceIndex )
{
	RosaGame* const		pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( SelectConversationChoice, NULL );
	WB_LOG_EVENT( SelectConversationChoice );
	WB_SET_AUTO( SelectConversationChoice, Int, ChoiceIndex, ChoiceIndex );
	WB_DISPATCH_EVENT( pEventManager, SelectConversationChoice, pGame );
}
