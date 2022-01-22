#include "core.h"
#include "uiinputmaprosa.h"
#include "rosaframework.h"
#include "keyboard.h"
#include "xinputcontroller.h"
#include "inputsystem.h"

UIInputMapRosa::UIInputMapRosa( RosaFramework* const pFramework )
:	m_Framework( pFramework )
{
}

UIInputMapRosa::~UIInputMapRosa()
{
}

bool UIInputMapRosa::OnNext()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();

	if( pKeyboard->OnRise( Keyboard::EB_Tab ) &&
		!pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) &&
		!pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnPrevious()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();

	if( pKeyboard->OnRise( Keyboard::EB_Tab ) &&
		( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) ||
		  pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnUp()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	XInputController* const pController = m_Framework->GetController();
	InputSystem* const pInputSystem = m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( Forward );
	const uint Keyboard_Forward = pInputSystem->GetBoundKeyboardSignal( sForward );

	if( Keyboard_Forward > Keyboard::EB_None && pKeyboard->OnRise( Keyboard_Forward ) )
	{
		return true;
	}

	if( pKeyboard->OnRise( Keyboard::EB_Up ) )
	{
		return true;
	}

	if( pController->OnRise( XInputController::EB_Up ) ||
		pController->OnRise( XInputController::EB_LeftThumbUp ) ||
		pController->OnRise( XInputController::EB_RightThumbUp ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnDown()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	XInputController* const pController = m_Framework->GetController();
	InputSystem* const pInputSystem = m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( Back );
	const uint Keyboard_Back = pInputSystem->GetBoundKeyboardSignal( sBack );

	if( Keyboard_Back > Keyboard::EB_None && pKeyboard->OnRise( Keyboard_Back ) )
	{
		return true;
	}

	if( pKeyboard->OnRise( Keyboard::EB_Down ) )
	{
		return true;
	}

	if( pController->OnRise( XInputController::EB_Down ) ||
		pController->OnRise( XInputController::EB_LeftThumbDown ) ||
		pController->OnRise( XInputController::EB_RightThumbDown ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnLeft()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	XInputController* const pController = m_Framework->GetController();
	InputSystem* const pInputSystem = m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( Left );
	const uint Keyboard_Left = pInputSystem->GetBoundKeyboardSignal( sLeft );

	if( Keyboard_Left > Keyboard::EB_None && pKeyboard->OnRise( Keyboard_Left ) )
	{
		return true;
	}

	if( pKeyboard->OnRise( Keyboard::EB_Left ) )
	{
		return true;
	}

	if( pController->OnRise( XInputController::EB_Left ) ||
		pController->OnRise( XInputController::EB_LeftThumbLeft ) ||
		pController->OnRise( XInputController::EB_RightThumbLeft ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnRight()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	XInputController* const pController = m_Framework->GetController();
	InputSystem* const pInputSystem = m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( Right );
	const uint Keyboard_Right = pInputSystem->GetBoundKeyboardSignal( sRight );

	if( Keyboard_Right > Keyboard::EB_None && pKeyboard->OnRise( Keyboard_Right ) )
	{
		return true;
	}

	if( pKeyboard->OnRise( Keyboard::EB_Right ) )
	{
		return true;
	}

	if( pController->OnRise( XInputController::EB_Right ) ||
		pController->OnRise( XInputController::EB_LeftThumbRight ) ||
		pController->OnRise( XInputController::EB_RightThumbRight ) )
	{
		return true;
	}

	return false;
}

/*virtual*/ bool UIInputMapRosa::OnPrevPage()
{
	Keyboard* const			pKeyboard		= m_Framework->GetKeyboard();
	XInputController* const	pController		= m_Framework->GetController();
	InputSystem* const		pInputSystem	= m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( LeanLeft );
	const uint Keyboard_LeanLeft = pInputSystem->GetBoundKeyboardSignal( sLeanLeft );
	if( Keyboard_LeanLeft != Keyboard::EB_None && pKeyboard->OnRise( Keyboard_LeanLeft ) )
	{
		return true;
	}

	const uint Controller_LeanLeft = pInputSystem->GetBoundControllerSignal( sLeanLeft );
	if( Controller_LeanLeft > XInputController::EB_None && pController->OnRise( Controller_LeanLeft ) )
	{
		return true;
	}

	return false;
}

/*virtual*/ bool UIInputMapRosa::OnNextPage()
{
	Keyboard* const			pKeyboard		= m_Framework->GetKeyboard();
	XInputController* const	pController		= m_Framework->GetController();
	InputSystem* const		pInputSystem	= m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( LeanRight );
	const uint Keyboard_LeanRight = pInputSystem->GetBoundKeyboardSignal( sLeanRight );
	if( Keyboard_LeanRight != Keyboard::EB_None && pKeyboard->OnRise( Keyboard_LeanRight ) )
	{
		return true;
	}

	const uint Controller_LeanRight = pInputSystem->GetBoundControllerSignal( sLeanRight );
	if( Controller_LeanRight > XInputController::EB_None && pController->OnRise( Controller_LeanRight ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnAccept()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	XInputController* const pController = m_Framework->GetController();

	if( pKeyboard->OnRise( Keyboard::EB_Enter ) &&
		!pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) )
	{
		return true;
	}

	if( pController->OnRise( XInputController::EB_A ) || pController->OnRise( XInputController::EB_Start ) )
	{
		return true;
	}

	return false;
}

bool UIInputMapRosa::OnCancel()
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	XInputController* const pController = m_Framework->GetController();
	InputSystem* const pInputSystem = m_Framework->GetInputSystem();

	STATIC_HASHED_STRING( Frob );
	const uint Keyboard_Frob = pInputSystem->GetBoundKeyboardSignal( sFrob );
	if( Keyboard_Frob > Keyboard::EB_None && pKeyboard->OnRise( Keyboard_Frob ) )
	{
		return true;
	}

	if( pKeyboard->OnRise( Keyboard::EB_Escape ) )
	{
		return true;
	}

	const uint Controller_Frob = pInputSystem->GetBoundControllerSignal( sFrob );
	if( Controller_Frob > XInputController::EB_None && pController->OnRise( Controller_Frob ) )
	{
		return true;
	}

	if( pController->OnRise( XInputController::EB_B ) )
	{
		return true;
	}

	return false;
}
