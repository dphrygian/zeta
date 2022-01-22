#include "core.h"
#include "uiscreen-rosakeypad.h"
#include "rosagame.h"
#include "iuiinputmap.h"
#include "uimanager.h"
#include "wbworld.h"
#include "rosaframework.h"
#include "keyboard.h"
#include "xinputcontroller.h"
#include "configmanager.h"
#include "wbactionfactory.h"
#include "uistack.h"

UIScreenRosaKeypad::UIScreenRosaKeypad()
:	m_TargetKeycode( 0 )
,	m_Keycode( 0 )
,	m_KeycodeString()
,	m_NumbersPushed( 0 )
,	m_SuccessActions()
,	m_FailureActions()
,	m_KeycodeEventUID( 0 )
,	m_HandleKeycodeDelay( 0.0f )
,	m_ClosedLightImage()
,	m_OpenLightImage()
{
}

UIScreenRosaKeypad::~UIScreenRosaKeypad()
{
	WBActionFactory::ClearActionArray( m_SuccessActions );
	WBActionFactory::ClearActionArray( m_FailureActions );
}

void UIScreenRosaKeypad::RegisterForEvents()
{
	STATIC_HASHED_STRING( SetKeycode );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sSetKeycode, this, NULL );

	STATIC_HASHED_STRING( PushKeypad );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sPushKeypad, this, NULL );

	STATIC_HASHED_STRING( SucceedKeycode );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sSucceedKeycode, this, NULL );

	STATIC_HASHED_STRING( FailKeycode );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sFailKeycode, this, NULL );
}

/*virtual*/ void UIScreenRosaKeypad::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	UIScreen::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	WBActionFactory::InitializeActionArray( sDefinitionName, "Success", m_SuccessActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "Failure", m_FailureActions );

	STATICHASH( HandleKeycodeDelay );
	m_HandleKeycodeDelay = ConfigManager::GetInheritedFloat( sHandleKeycodeDelay, 0.0f, sDefinitionName );

	STATICHASH( ClosedLightImage );
	m_ClosedLightImage = ConfigManager::GetInheritedHash( sClosedLightImage, HashedString::NullString, sDefinitionName );

	STATICHASH( OpenLightImage );
	m_OpenLightImage = ConfigManager::GetInheritedHash( sOpenLightImage, HashedString::NullString, sDefinitionName );
}

/*virtual*/ UIScreen::ETickReturn UIScreenRosaKeypad::TickInput()
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	Keyboard* const				pKeyboard	= pFramework->GetKeyboard();
	XInputController* const		pController	= pFramework->GetController();

#define TRY_NUMBER(n) \
	if( pKeyboard->OnRise( Keyboard::EB_Num##n ) || pKeyboard->OnRise( Keyboard::EB_##n ) ) \
	{ \
		PushNumber( n ); \
	}

	TRY_NUMBER( 0 );
	TRY_NUMBER( 1 );
	TRY_NUMBER( 2 );
	TRY_NUMBER( 3 );
	TRY_NUMBER( 4 );
	TRY_NUMBER( 5 );
	TRY_NUMBER( 6 );
	TRY_NUMBER( 7 );
	TRY_NUMBER( 8 );
	TRY_NUMBER( 9 );

#undef TRY_NUMBER

	if( pKeyboard->OnRise( Keyboard::EB_Backspace ) ||
		pController->OnRise( XInputController::EB_Back ) )
	{
		PopNumber();
	}

#if BUILD_DEV
	// Alt + H succeeds keypad instantly
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_H ) )
	{
		Succeed();
	}
#endif // BUILD_DEV

	return UIScreen::TickInput();
}

void UIScreenRosaKeypad::ResetCode()
{
	m_Keycode		= 0;
	m_KeycodeString	= "";
	m_NumbersPushed	= 0;

	PublishCode();
}

void UIScreenRosaKeypad::PublishCode()
{
	STATICHASH( RosaKeypad );
	STATICHASH( Keycode );
	ConfigManager::SetString( sKeycode, m_KeycodeString.CStr(), sRosaKeypad );
}

void UIScreenRosaKeypad::PushNumber( uint Number )
{
	if( m_NumbersPushed >= 4 )
	{
		return;
	}

	DEVASSERT( Number < 10 );

	++m_NumbersPushed;
	m_Keycode		= ( m_Keycode * 10 ) + Number;
	m_KeycodeString	= SimpleString::PrintF( "%0*d", m_NumbersPushed, m_Keycode );

	PublishCode();

	if( m_NumbersPushed == 4 )
	{
		HandleKeycode();
	}
}

void UIScreenRosaKeypad::PopNumber()
{
	if( m_NumbersPushed >= 4 || m_NumbersPushed == 0 )
	{
		return;
	}

	--m_NumbersPushed;
	m_Keycode		= m_Keycode / 10;
	m_KeycodeString	= ( m_NumbersPushed > 0 ) ? SimpleString::PrintF( "%0*d", m_NumbersPushed, m_Keycode ) : "";

	PublishCode();
}

void UIScreenRosaKeypad::HandleKeycode()
{
	if( m_Keycode == m_TargetKeycode )
	{
		Succeed();
	}
	else
	{
		Fail();
	}
}

void UIScreenRosaKeypad::Succeed()
{
	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	WB_MAKE_EVENT( SucceedKeycode, NULL );
	m_KeycodeEventUID = WB_QUEUE_EVENT_DELAY( pEventManager, SucceedKeycode, NULL, m_HandleKeycodeDelay );

	// Set the light to green
	{
		STATIC_HASHED_STRING( KeypadScreen );
		STATIC_HASHED_STRING( KeypadLight );

		WB_MAKE_EVENT( SetWidgetImage, NULL );
		WB_SET_AUTO( SetWidgetImage, Hash, Screen, sKeypadScreen );
		WB_SET_AUTO( SetWidgetImage, Hash, Widget, sKeypadLight );
		WB_SET_AUTO( SetWidgetImage, Hash, Image, m_OpenLightImage );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetImage, NULL );
	}
}

void UIScreenRosaKeypad::Fail()
{
	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	WB_MAKE_EVENT( FailKeycode, NULL );
	m_KeycodeEventUID = WB_QUEUE_EVENT_DELAY( pEventManager, FailKeycode, NULL, m_HandleKeycodeDelay );
}

void UIScreenRosaKeypad::SucceedKeycode()
{
	WBActionFactory::ExecuteActionArray( m_SuccessActions, WBEvent(), NULL );

	// Remove, don't pop, in case user has manually closed screen already
	m_UIManager->GetUIStack()->Remove( this );
}

void UIScreenRosaKeypad::FailKeycode()
{
	WBActionFactory::ExecuteActionArray( m_FailureActions, WBEvent(), NULL );

	ResetCode();
}

/*virtual*/ void UIScreenRosaKeypad::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( SetKeycode );
	STATIC_HASHED_STRING( PushKeypad );
	STATIC_HASHED_STRING( SucceedKeycode );
	STATIC_HASHED_STRING( FailKeycode );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetKeycode )
	{
		STATIC_HASHED_STRING( Keycode );
		m_TargetKeycode = Event.GetInt( sKeycode );
		DEVASSERT( m_TargetKeycode < 10000 );
	}
	else if( EventName == sPushKeypad )
	{
		STATIC_HASHED_STRING( Number );
		const uint Number = Event.GetInt( sNumber );
		PushNumber( Number );
	}
	else if( EventName == sSucceedKeycode )
	{
		SucceedKeycode();
	}
	else if( EventName == sFailKeycode )
	{
		FailKeycode();
	}
}

/*virtual*/ void UIScreenRosaKeypad::Pushed()
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	pEventManager->UnqueueEvent( m_KeycodeEventUID );

	ResetCode();

	// Disable frobbing to hide the prompt
	{
		WB_MAKE_EVENT( DisableFrob, NULL );
		WB_DISPATCH_EVENT( pEventManager, DisableFrob, pPlayer );
	}

	// Hide the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
	}

	// Reset the light to red
	{
		STATIC_HASHED_STRING( KeypadScreen );
		STATIC_HASHED_STRING( KeypadLight );

		WB_MAKE_EVENT( SetWidgetImage, NULL );
		WB_SET_AUTO( SetWidgetImage, Hash, Screen, sKeypadScreen );
		WB_SET_AUTO( SetWidgetImage, Hash, Widget, sKeypadLight );
		WB_SET_AUTO( SetWidgetImage, Hash, Image, m_ClosedLightImage );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetImage, NULL );
	}

	UIScreen::Pushed();
}

/*virtual*/ void UIScreenRosaKeypad::Popped()
{
	UIScreen::Popped();

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	// Enable frobbing again
	{
		WB_MAKE_EVENT( EnableFrob, NULL );
		WB_DISPATCH_EVENT( pEventManager, EnableFrob, pPlayer );
	}

	// Show the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, false );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
	}
}
