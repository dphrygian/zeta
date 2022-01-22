#include "core.h"
#include "xinputcontroller.h"
#include "mathcore.h"
#include "configmanager.h"

#if BUILD_WINDOWS_NO_SDL
#include <Windows.h>
#include <xinput.h>
#endif

#if USE_DIRECTINPUT
#include "mouse.h"

BOOL CALLBACK DI_EnumDevicesCallback( LPCDIDEVICEINSTANCE pDeviceInstance, LPVOID pRef )
{
	LPDIRECTINPUT8 const pDirectInput = Mouse::GetDirectInput();
	ASSERT( pDirectInput );

	XInputController* const pThis = static_cast<XInputController*>( pRef );
	ASSERT( pThis );

	PRINTF( "Enumerated DI device %s\n", pDeviceInstance->tszProductName );

	// Create the device
	LPDIRECTINPUTDEVICE8 pDevice = NULL;
	HRESULT hr = pDirectInput->CreateDevice( pDeviceInstance->guidInstance, &pDevice, NULL );
	if( FAILED( hr ) )
	{
		PRINTF( " Failed to create device.\n" );
		return DIENUM_CONTINUE;
	}

	// Try to set it to receive a DIJOYSTATE2 structure
	hr = pDevice->SetDataFormat( &c_dfDIJoystick2 );
	if( FAILED( hr ) )
	{
		PRINTF( " Failed to set data format.\n" );
		return DIENUM_CONTINUE;
	}

	hr = pDevice->SetCooperativeLevel( pThis->m_hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND );
	if( FAILED( hr ) )
	{
		PRINTF( " Failed to set cooperative level.\n" );
		return DIENUM_CONTINUE;
	}

	// Set desired axis ranges to match XInput
	DIPROPRANGE AxisRange;
	AxisRange.diph.dwSize		= sizeof( DIPROPRANGE );
	AxisRange.diph.dwHeaderSize	= sizeof( DIPROPHEADER );
	AxisRange.diph.dwHow		= DIPH_BYOFFSET;

	DWORD Axes[] = { DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RZ, DIJOFS_RX, DIJOFS_RY };
	const uint NumAxes = sizeof( Axes ) / sizeof( DWORD );
	for( uint Axis = 0; Axis < NumAxes; ++Axis )
	{
		AxisRange.diph.dwObj	= Axes[ Axis ];
		AxisRange.lMin			= -32768;
		AxisRange.lMax			= 32767;

		hr = pDevice->SetProperty( DIPROP_RANGE, &AxisRange.diph );
		if( FAILED( hr ) )
		{
			PRINTF( " Failed to set axis range property (%d/%d).\n", Axis, Axes[ Axis ] );

			pDevice->GetProperty( DIPROP_RANGE, &AxisRange.diph );
			PRINTF( " Using range %d-%d instead.\n", AxisRange.lMin, AxisRange.lMax );
		}
	}

	DWORD Triggers[] = { DIJOFS_RX, DIJOFS_RY };
	const uint NumTriggers = sizeof( Triggers ) / sizeof( DWORD );
	for( uint Trigger = 0; Trigger < NumTriggers; ++Trigger )
	{
		AxisRange.diph.dwObj	= Triggers[ Trigger ];
		AxisRange.lMin			= 0;
		AxisRange.lMax			= 255;

		hr = pDevice->SetProperty( DIPROP_RANGE, &AxisRange.diph );
		if( FAILED( hr ) )
		{
			PRINTF( " Failed to set trigger range property (%d/%d).\n", Trigger, Triggers[ Trigger ] );

			pDevice->GetProperty( DIPROP_RANGE, &AxisRange.diph );
			PRINTF( " Using range %d-%d instead.\n", AxisRange.lMin, AxisRange.lMax );
		}
	}

	// Stop after we get the first device.
	// NOTE: This precludes multiple DI devices. I don't care right now.
	pThis->m_DI_Controller = pDevice;

	PRINTF( " Successfully initialized device.\n" );
	return DIENUM_STOP;
}
#endif

#include <memory.h>

#if USE_DIRECTINPUT
XInputController::XInputController( HWND hWnd, uint Port /*= 0*/ )
#else
XInputController::XInputController( uint Port /*= 0*/ )
#endif
:	m_Port( Port )
#if USE_DIRECTINPUT
,	m_DI_Controller( NULL )
,	m_hWnd( hWnd )
#endif
#if BUILD_SDL
,	m_Controller( NULL )
#endif
,	m_LeftThumbDeadZone( 0.0f )
,	m_RightThumbDeadZone( 0.0f )
,	m_LeftThumbSaturationPoint( 0.0f )
,	m_RightThumbSaturationPoint( 0.0f )
,	m_TriggerDeadZone( 0.0f )
,	m_TriggerSaturationPoint( 0.0f )
,	m_LeftThumbBoolThreshold( 0.0f )
,	m_RightThumbBoolThreshold( 0.0f )
,	m_LeftTriggerBoolThreshold( 0.0f )
,	m_RightTriggerBoolThreshold( 0.0f )
,	m_ReceivedInputThisTick( false )
,	m_WasConnectedThisTick( false )
{
	memset( &m_CurrentState, 0, sizeof( SXInputState ) );
	memset( &m_LastState, 0, sizeof( SXInputState ) );

	STATICHASH( LeftThumbDeadZone );
	STATICHASH( RightThumbDeadZone );
	STATICHASH( LeftThumbSaturationPoint );
	STATICHASH( RightThumbSaturationPoint );
	STATICHASH( TriggerDeadZone );
	STATICHASH( TriggerSaturationPoint );
	STATICHASH( LeftThumbBoolThreshold );
	STATICHASH( RightThumbBoolThreshold );
	STATICHASH( LeftTriggerBoolThreshold );
	STATICHASH( RightTriggerBoolThreshold );

	ConfigManager::Bind( &m_LeftThumbDeadZone,			sLeftThumbDeadZone,			0.2f );
	ConfigManager::Bind( &m_RightThumbDeadZone,			sRightThumbDeadZone,		0.2f );
	ConfigManager::Bind( &m_LeftThumbSaturationPoint,	sLeftThumbSaturationPoint,	0.9f );
	ConfigManager::Bind( &m_RightThumbSaturationPoint,	sRightThumbSaturationPoint,	0.9f );
	ConfigManager::Bind( &m_TriggerDeadZone,			sTriggerDeadZone,			0.1f );
	ConfigManager::Bind( &m_TriggerSaturationPoint,		sTriggerSaturationPoint,	0.9f );

	ConfigManager::Bind( &m_LeftThumbBoolThreshold,		sLeftThumbSaturationPoint,	1.0f );
	ConfigManager::Bind( &m_RightThumbBoolThreshold,	sRightThumbSaturationPoint,	1.0f );
	ConfigManager::Bind( &m_LeftTriggerBoolThreshold,	sTriggerDeadZone,			1.0f );
	ConfigManager::Bind( &m_RightTriggerBoolThreshold,	sTriggerSaturationPoint,	1.0f );

#if USE_DIRECTINPUT
	STATICHASH( DirectInput );
	if( ConfigManager::GetBool( sDirectInput ) )
	{
		LPDIRECTINPUT8 const pDirectInput = Mouse::GetDirectInput();
		if( pDirectInput )
		{
			pDirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL, &DI_EnumDevicesCallback, this, DIEDFL_ALLDEVICES );
		}
	}
#endif

#if BUILD_SDL
	// HACKHACK: Removed the stuff dealing with ports to try to support
	// gamepads on e.g. /dev/input/js2 in Linux. Not sure it'll work, but
	// now I'm just grabbing the first valid joystick and ignoring the port
	// number.
	const uint	NumJoysticks	= SDL_NumJoysticks();
	for( uint JoystickIndex = 0; JoystickIndex < NumJoysticks; ++JoystickIndex )
	{
		if( SDL_IsGameController( JoystickIndex ) )
		{
			SDL_GameController* pController = SDL_GameControllerOpen( JoystickIndex );
			if( pController )
			{
				m_Controller = pController;
				break;
			}
		}
	}
#endif
}

XInputController::~XInputController()
{
	ConfigManager::Unbind( &m_LeftThumbDeadZone );
	ConfigManager::Unbind( &m_RightThumbDeadZone );
	ConfigManager::Unbind( &m_LeftThumbSaturationPoint );
	ConfigManager::Unbind( &m_RightThumbSaturationPoint );
	ConfigManager::Unbind( &m_TriggerDeadZone );
	ConfigManager::Unbind( &m_TriggerSaturationPoint );

	ConfigManager::Unbind( &m_LeftThumbBoolThreshold );
	ConfigManager::Unbind( &m_RightThumbBoolThreshold );
	ConfigManager::Unbind( &m_LeftTriggerBoolThreshold );
	ConfigManager::Unbind( &m_RightTriggerBoolThreshold );

#if USE_DIRECTINPUT
	if( m_DI_Controller )
	{
		m_DI_Controller->Unacquire();
		SafeRelease( m_DI_Controller );
	}
#endif

#if BUILD_SDL
	if( m_Controller )
	{
		SDL_GameControllerClose( m_Controller );
	}
#endif
}

void XInputController::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	m_LastState = m_CurrentState;

	m_WasConnectedThisTick = false;

#if BUILD_WINDOWS_NO_SDL
	XINPUT_STATE XInputState;
	if( XInputGetState( m_Port, &XInputState ) == ERROR_SUCCESS )
	{
		m_WasConnectedThisTick = true;

#define SET_BUTTON( EB, XI ) m_CurrentState.m_Buttons[ EB ] = ( XInputState.Gamepad.wButtons & XI ) != 0
		SET_BUTTON( EB_A, XINPUT_GAMEPAD_A );
		SET_BUTTON( EB_B, XINPUT_GAMEPAD_B );
		SET_BUTTON( EB_X, XINPUT_GAMEPAD_X );
		SET_BUTTON( EB_Y, XINPUT_GAMEPAD_Y );
		SET_BUTTON( EB_Up, XINPUT_GAMEPAD_DPAD_UP );
		SET_BUTTON( EB_Down, XINPUT_GAMEPAD_DPAD_DOWN );
		SET_BUTTON( EB_Left, XINPUT_GAMEPAD_DPAD_LEFT );
		SET_BUTTON( EB_Right, XINPUT_GAMEPAD_DPAD_RIGHT );
		SET_BUTTON( EB_Start, XINPUT_GAMEPAD_START );
		SET_BUTTON( EB_Back, XINPUT_GAMEPAD_BACK );
		SET_BUTTON( EB_LeftBumper, XINPUT_GAMEPAD_LEFT_SHOULDER );
		SET_BUTTON( EB_RightBumper, XINPUT_GAMEPAD_RIGHT_SHOULDER );
		SET_BUTTON( EB_LeftThumb, XINPUT_GAMEPAD_LEFT_THUMB );
		SET_BUTTON( EB_RightThumb, XINPUT_GAMEPAD_RIGHT_THUMB );
#undef SET_BUTTON

#define SET_ANALOG_BUTTON( EB, XI, RANGE, CMP, TH ) m_CurrentState.m_Buttons[ EB ] = ( static_cast<float>( XInputState.Gamepad.XI ) / RANGE ) CMP TH
		SET_ANALOG_BUTTON( EB_LeftThumbUp, sThumbLY, 32768.0f, >=, m_LeftThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_LeftThumbDown, sThumbLY, 32768.0f, <=, -m_LeftThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_LeftThumbLeft, sThumbLX, 32768.0f, <=, -m_LeftThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_LeftThumbRight, sThumbLX, 32768.0f, >=, m_LeftThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightThumbUp, sThumbRY, 32768.0f, >=, m_RightThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightThumbDown, sThumbRY, 32768.0f, <=, -m_RightThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightThumbLeft, sThumbRX, 32768.0f, <=, -m_RightThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightThumbRight, sThumbRX, 32768.0f, >=, m_RightThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_LeftTrigger, bLeftTrigger, 255.0f, >=, m_LeftTriggerBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightTrigger, bRightTrigger, 255.0f, >=, m_RightTriggerBoolThreshold );
#undef SET_ANALOG_BUTTON

#define SET_AXIS( EA, XI, RANGE ) m_CurrentState.m_Axes[ EA ] = static_cast<float>( XInputState.Gamepad.XI ) / RANGE
		SET_AXIS( EA_LeftThumbX, sThumbLX, 32768.0f );
		SET_AXIS( EA_LeftThumbY, sThumbLY, 32768.0f );
		SET_AXIS( EA_RightThumbX, sThumbRX, 32768.0f );
		SET_AXIS( EA_RightThumbY, sThumbRY, 32768.0f );
		SET_AXIS( EA_LeftTrigger, bLeftTrigger, 255.0f );
		SET_AXIS( EA_RightTrigger, bRightTrigger, 255.0f );
#undef SET_AXIS

		ApplyDeadZones();
	}
#if USE_DIRECTINPUT
	else if( m_DI_Controller )
	{
		// We couldn't poll an XInput device, try DirectInput

		m_DI_Controller->Poll();

		DIJOYSTATE2 JoyState;
		HRESULT hr = m_DI_Controller->GetDeviceState( sizeof( DIJOYSTATE2 ), &JoyState );

		// Like mouse, try to reacquire if polling fails
		while( FAILED( hr ) )
		{
			hr = m_DI_Controller->Acquire();
			while( hr == DIERR_INPUTLOST )
			{
				hr = m_DI_Controller->Acquire();
			}

			if( FAILED( hr ) )
			{
				// Couldn't acquire for some reason
				return;
			}

			// Else, successfully reacquired device! Continue as before.
			m_DI_Controller->Poll();
			m_DI_Controller->GetDeviceState( sizeof( DIJOYSTATE2 ), &JoyState );
		}

		m_WasConnectedThisTick = true;

		// Assume a DualShock 4 controller
#define SET_BUTTON( EB, DS ) m_CurrentState.m_Buttons[ EB ] = ( JoyState.rgbButtons[ DS ] & 0x80 ) != 0
		SET_BUTTON( EB_X, 0 );
		SET_BUTTON( EB_A, 1 );
		SET_BUTTON( EB_B, 2 );
		SET_BUTTON( EB_Y, 3 );
		SET_BUTTON( EB_LeftBumper, 4 );
		SET_BUTTON( EB_RightBumper, 5 );
		SET_BUTTON( EB_Back, 8 );
		SET_BUTTON( EB_Start, 9 );
		SET_BUTTON( EB_LeftThumb, 10 );
		SET_BUTTON( EB_RightThumb, 11 );
#undef SET_BUTTON

#define SET_DPAD( EB, POV, VAL ) m_CurrentState.m_Buttons[ EB ] = ( JoyState.rgdwPOV[ POV ] == VAL )
		SET_DPAD( EB_Up, 0, 0 );
		SET_DPAD( EB_Right, 0, 9000 );
		SET_DPAD( EB_Down, 0, 18000 );
		SET_DPAD( EB_Left, 0, 27000 );
#undef SET_DPAD

#define SET_ANALOG_BUTTON( EB, AXIS, RANGE, CMP, TH ) m_CurrentState.m_Buttons[ EB ] = ( static_cast<float>( JoyState.AXIS ) / RANGE ) CMP TH
		SET_ANALOG_BUTTON( EB_LeftThumbUp, lY, 32768.0f, <=, -m_LeftThumbBoolThreshold );		// Y is inverted from XInput
		SET_ANALOG_BUTTON( EB_LeftThumbDown, lY, 32768.0f, >=, m_LeftThumbBoolThreshold );		// Y is inverted from XInput
		SET_ANALOG_BUTTON( EB_LeftThumbLeft, lX, 32768.0f, <=, -m_LeftThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_LeftThumbRight, lX, 32768.0f, >=, m_LeftThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightThumbUp, lRz, 32768.0f, <=, -m_RightThumbBoolThreshold );	// Y is inverted from XInput
		SET_ANALOG_BUTTON( EB_RightThumbDown, lRz, 32768.0f, >=, m_RightThumbBoolThreshold );	// Y is inverted from XInput
		SET_ANALOG_BUTTON( EB_RightThumbLeft, lZ, 32768.0f, <=, -m_RightThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightThumbRight, lZ, 32768.0f, >=, m_RightThumbBoolThreshold );
		SET_ANALOG_BUTTON( EB_LeftTrigger, lRx, 255.0f, >=, m_LeftTriggerBoolThreshold );
		SET_ANALOG_BUTTON( EB_RightTrigger, lRy, 255.0f, >=, m_RightTriggerBoolThreshold );
#undef SET_ANALOG_BUTTON

#define SET_AXIS( EA, AXIS, RANGE, FAC ) m_CurrentState.m_Axes[ EA ] = ( FAC * static_cast<float>( JoyState.AXIS ) ) / RANGE
		SET_AXIS( EA_LeftThumbX, lX, 32768.0f, 1.0f );
		SET_AXIS( EA_LeftThumbY, lY, 32768.0f, -1.0f );		// Y is inverted from XInput
		SET_AXIS( EA_RightThumbX, lZ, 32768.0f, 1.0f );
		SET_AXIS( EA_RightThumbY, lRz, 32768.0f, -1.0f );	// Y is inverted from XInput
		SET_AXIS( EA_LeftTrigger, lRx, 255.0f, 1.0f );
		SET_AXIS( EA_RightTrigger, lRy, 255.0f, 1.0f );
#undef SET_AXIS

		ApplyDeadZones();
	}
#endif
#endif // BUILD_WINDOWS_NO_SDL

#if BUILD_SDL
	if( m_Controller )
	{
		SDL_GameControllerUpdate();
		if( SDL_GameControllerGetAttached( m_Controller ) )
		{
			m_WasConnectedThisTick = true;

#define SET_BUTTON( EB, SDLB ) m_CurrentState.m_Buttons[ EB ] = SDL_GameControllerGetButton( m_Controller, SDLB ) != 0
			SET_BUTTON( EB_A,			SDL_CONTROLLER_BUTTON_A );
			SET_BUTTON( EB_B,			SDL_CONTROLLER_BUTTON_B );
			SET_BUTTON( EB_X,			SDL_CONTROLLER_BUTTON_X );
			SET_BUTTON( EB_Y,			SDL_CONTROLLER_BUTTON_Y );
			SET_BUTTON( EB_Up,			SDL_CONTROLLER_BUTTON_DPAD_UP );
			SET_BUTTON( EB_Down,		SDL_CONTROLLER_BUTTON_DPAD_DOWN );
			SET_BUTTON( EB_Left,		SDL_CONTROLLER_BUTTON_DPAD_LEFT );
			SET_BUTTON( EB_Right,		SDL_CONTROLLER_BUTTON_DPAD_RIGHT );
			SET_BUTTON( EB_Start,		SDL_CONTROLLER_BUTTON_START );
			SET_BUTTON( EB_Back,		SDL_CONTROLLER_BUTTON_BACK );
			SET_BUTTON( EB_LeftBumper,	SDL_CONTROLLER_BUTTON_LEFTSHOULDER );
			SET_BUTTON( EB_RightBumper,	SDL_CONTROLLER_BUTTON_RIGHTSHOULDER );
			SET_BUTTON( EB_LeftThumb,	SDL_CONTROLLER_BUTTON_LEFTSTICK );
			SET_BUTTON( EB_RightThumb,	SDL_CONTROLLER_BUTTON_RIGHTSTICK );
#undef SET_BUTTON

#define SET_ANALOG_BUTTON( EB, SDLA, RANGE, CMP, TH ) m_CurrentState.m_Buttons[ EB ] = ( static_cast<float>( SDL_GameControllerGetAxis( m_Controller, SDLA ) ) / RANGE ) CMP TH
			SET_ANALOG_BUTTON( EB_LeftThumbUp,		SDL_CONTROLLER_AXIS_LEFTY,			32768.0f,	<=, -m_LeftThumbBoolThreshold );	// Y is inverted from XInput
			SET_ANALOG_BUTTON( EB_LeftThumbDown,	SDL_CONTROLLER_AXIS_LEFTY,			32768.0f,	>=, m_LeftThumbBoolThreshold );		// Y is inverted from XInput
			SET_ANALOG_BUTTON( EB_LeftThumbLeft,	SDL_CONTROLLER_AXIS_LEFTX,			32768.0f,	<=, -m_LeftThumbBoolThreshold );
			SET_ANALOG_BUTTON( EB_LeftThumbRight,	SDL_CONTROLLER_AXIS_LEFTX,			32768.0f,	>=, m_LeftThumbBoolThreshold );
			SET_ANALOG_BUTTON( EB_RightThumbUp,		SDL_CONTROLLER_AXIS_RIGHTY,			32768.0f,	<=, -m_RightThumbBoolThreshold );	// Y is inverted from XInput
			SET_ANALOG_BUTTON( EB_RightThumbDown,	SDL_CONTROLLER_AXIS_RIGHTY,			32768.0f,	>=, m_RightThumbBoolThreshold );	// Y is inverted from XInput
			SET_ANALOG_BUTTON( EB_RightThumbLeft,	SDL_CONTROLLER_AXIS_RIGHTX,			32768.0f,	<=, -m_RightThumbBoolThreshold );
			SET_ANALOG_BUTTON( EB_RightThumbRight,	SDL_CONTROLLER_AXIS_RIGHTX,			32768.0f,	>=, m_RightThumbBoolThreshold );
			SET_ANALOG_BUTTON( EB_LeftTrigger,		SDL_CONTROLLER_AXIS_TRIGGERLEFT,	32768.0f,	>=, m_LeftTriggerBoolThreshold );
			SET_ANALOG_BUTTON( EB_RightTrigger,		SDL_CONTROLLER_AXIS_TRIGGERRIGHT,	32768.0f,	>=, m_RightTriggerBoolThreshold );
#undef SET_ANALOG_BUTTON

#define SET_AXIS( EA, SDLA, RANGE, FAC ) m_CurrentState.m_Axes[ EA ] = ( FAC * static_cast<float>( SDL_GameControllerGetAxis( m_Controller, SDLA ) ) ) / RANGE
			SET_AXIS( EA_LeftThumbX,	SDL_CONTROLLER_AXIS_LEFTX,			32768.0f, 1.0f );
			SET_AXIS( EA_LeftThumbY,	SDL_CONTROLLER_AXIS_LEFTY,			32768.0f, -1.0f );	// Y is inverted from XInput
			SET_AXIS( EA_RightThumbX,	SDL_CONTROLLER_AXIS_RIGHTX,			32768.0f, 1.0f );
			SET_AXIS( EA_RightThumbY,	SDL_CONTROLLER_AXIS_RIGHTY,			32768.0f, -1.0f );	// Y is inverted from XInput
			SET_AXIS( EA_LeftTrigger,	SDL_CONTROLLER_AXIS_TRIGGERLEFT,	32768.0f, 1.0f );
			SET_AXIS( EA_RightTrigger,	SDL_CONTROLLER_AXIS_TRIGGERRIGHT,	32768.0f, 1.0f );
#undef SET_AXIS

			ApplyDeadZones();
		}
	}
#endif

	m_ReceivedInputThisTick = false;
	for( uint ButtonIndex = 1; ButtonIndex < EB_Max && !m_ReceivedInputThisTick; ++ButtonIndex )
	{
		if( IsHigh( ButtonIndex ) )
		{
			m_ReceivedInputThisTick = true;
		}
	}
	for( uint AxisIndex = 1; AxisIndex < EA_Max && !m_ReceivedInputThisTick; ++AxisIndex )
	{
		if( GetPosition( AxisIndex ) != 0.0f )
		{
			m_ReceivedInputThisTick = true;
		}
	}
}

void XInputController::ApplyDeadZones()
{
	ApplyDeadZone( EA_LeftThumbX,	m_LeftThumbDeadZone,	m_LeftThumbSaturationPoint );
	ApplyDeadZone( EA_LeftThumbY,	m_LeftThumbDeadZone,	m_LeftThumbSaturationPoint );
	ApplyDeadZone( EA_RightThumbX,	m_RightThumbDeadZone,	m_RightThumbSaturationPoint );
	ApplyDeadZone( EA_RightThumbY,	m_RightThumbDeadZone,	m_RightThumbSaturationPoint );
	ApplyDeadZone( EA_LeftTrigger,	m_TriggerDeadZone,		m_TriggerSaturationPoint );
	ApplyDeadZone( EA_RightTrigger,	m_TriggerDeadZone,		m_TriggerSaturationPoint );
}

void XInputController::ApplyDeadZone( EAxes Axis, float DeadZone, float SaturationPoint )
{
	m_CurrentState.m_Axes[ Axis ] = Sign( m_CurrentState.m_Axes[ Axis ] ) * Saturate( InvLerp( Abs( m_CurrentState.m_Axes[ Axis ] ), DeadZone, SaturationPoint ) );
}

bool XInputController::IsHigh( uint Signal )
{
	DEVASSERT( Signal > EB_None );
	DEVASSERT( Signal < EB_Max );
	return m_CurrentState.m_Buttons[ Signal ];
}

bool XInputController::IsLow( uint Signal )
{
	DEVASSERT( Signal > EB_None );
	DEVASSERT( Signal < EB_Max );
	return !m_CurrentState.m_Buttons[ Signal ];
}

bool XInputController::OnRise( uint Signal )
{
	DEVASSERT( Signal > EB_None );
	DEVASSERT( Signal < EB_Max );
	return m_CurrentState.m_Buttons[ Signal ] && !m_LastState.m_Buttons[ Signal ];
}

bool XInputController::OnFall( uint Signal )
{
	DEVASSERT( Signal > EB_None );
	DEVASSERT( Signal < EB_Max );
	return !m_CurrentState.m_Buttons[ Signal ] && m_LastState.m_Buttons[ Signal ];
}

float XInputController::GetPosition( uint Axis )
{
	DEVASSERT( Axis > EA_None );
	DEVASSERT( Axis < EA_Max );
	return m_CurrentState.m_Axes[ Axis ];
}

float XInputController::GetVelocity( uint Axis )
{
	DEVASSERT( Axis > EA_None );
	DEVASSERT( Axis < EA_Max );
	return m_CurrentState.m_Axes[ Axis ] - m_LastState.m_Axes[ Axis ];
}

void XInputController::SetVibration( c_uint16 Left, c_uint16 Right )
{
#if BUILD_WINDOWS_NO_SDL
	XINPUT_VIBRATION Vibration;
	Vibration.wLeftMotorSpeed = Left;
	Vibration.wRightMotorSpeed = Right;
	XInputSetState( m_Port, &Vibration );
#endif
#if BUILD_SDL
	// TODO SDL LATER: Support feedback. (Not used in Rosa.)
	Unused( Left );
	Unused( Right );
#endif
}

void XInputController::SetFeedback( float Low, float High )
{
	Low = Saturate( Low );
	High = Saturate( High );
	SetVibration( ( c_uint16 )( Low * 65535.0f ), ( c_uint16 )( High * 65535.0f ) );
}
