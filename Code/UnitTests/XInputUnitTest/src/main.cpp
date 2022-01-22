#include "core.h"
#include "console.h"
#include "filestream.h"
#include "keyboard.h"
#include "mouse.h"
#include "xinputcontroller.h"
#include "configmanager.h"
#include "allocator.h"

#include <Windows.h>
#include <crtdbg.h>

// Return true when program requests exit
bool PumpMessageQueue()
{
	MSG Msg;
	while( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
	{
		// WM_QUIT is not associated with a window and must be processed here, not WndProc
		if( Msg.message == WM_QUIT )
		{
			return true;
		}

		TranslateMessage( &Msg );
		DispatchMessage( &Msg );
	}

	return false;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	Unused( hInstance );
	Unused( hPrevInstance );
	Unused( lpCmdLine );
	Unused( nCmdShow );

	Allocator::GetDefault().Initialize( 65536 );

	Console::GetInstance();
	Keyboard keyboard;
	Mouse mouse( hInstance, Console::GetInstance()->GetHWnd() );
	XInputController controller( Console::GetInstance()->GetHWnd() );
	float LeftRumble = 0.f;
	float RightRumble = 0.f;
	for(;;)
	{
		if( PumpMessageQueue() )
		{
			break;
		}

		keyboard.Tick( 0.0f );
		mouse.Tick( 0.0f );
		controller.Tick( 0.0f );

		if( keyboard.IsHigh( Keyboard::EB_Escape ) )
		{
			PostQuitMessage(0);
		}

		if( keyboard.OnRise( Keyboard::EB_Minus ) || keyboard.OnRise( Keyboard::EB_NumSubtract ) )
		{
			LeftRumble -= .1f;
			if( LeftRumble < 0.f ) LeftRumble = 0.f;
		}
		if( keyboard.OnRise( Keyboard::EB_Plus ) || keyboard.OnRise( Keyboard::EB_NumAdd ) )
		{
			LeftRumble += .1f;
			if( LeftRumble > 1.f ) LeftRumble = 1.f;
		}
		if( keyboard.OnRise( Keyboard::EB_LeftBrace ) )	// [
		{
			RightRumble -= .1f;
			if( RightRumble < 0.f ) RightRumble = 0.f;
		}
		if( keyboard.OnRise( Keyboard::EB_RightBrace ) )	// ]
		{
			RightRumble += .1f;
			if( RightRumble > 1.f ) RightRumble = 1.f;
		}

		controller.SetFeedback( LeftRumble, RightRumble );

		system( "cls" );
		PRINTF( "Press Esc to exit.\nPress +/- to adjust left (low) rumble.\nPress [/] to adjust right (high) rumble.\n\n" );
		PRINTF( "Left (Low) Rumble: %f\n", LeftRumble );
		PRINTF( "Right (High) Rumble: %f\n\n", RightRumble );
		PRINTF( "Left Thumb:  %f, %f\n", controller.GetPosition( XInputController::EA_LeftThumbX ), controller.GetPosition( XInputController::EA_LeftThumbY ) );
		PRINTF( "Right Thumb: %f, %f\n", controller.GetPosition( XInputController::EA_RightThumbX ), controller.GetPosition( XInputController::EA_RightThumbY ) );
		PRINTF( "Left Trigger:  %f\n", controller.GetPosition( XInputController::EA_LeftTrigger ) );
		PRINTF( "Right Trigger: %f\n\n", controller.GetPosition( XInputController::EA_RightTrigger ) );
#define REPORT_BUTTON( EB, SZ ) if( controller.IsHigh( XInputController::EB ) ) PRINTF( SZ )
		REPORT_BUTTON( EB_Up, "D-Pad Up\n" );
		REPORT_BUTTON( EB_Down, "D-Pad Down\n" );
		REPORT_BUTTON( EB_Left, "D-Pad Left\n" );
		REPORT_BUTTON( EB_Right, "D-Pad Right\n" );
		REPORT_BUTTON( EB_Start, "Start\n" );
		REPORT_BUTTON( EB_Back, "Back\n" );
		REPORT_BUTTON( EB_LeftThumb, "Left Thumb\n" );
		REPORT_BUTTON( EB_LeftBumper, "Left Bumper\n" );
		REPORT_BUTTON( EB_RightThumb, "Right Thumb\n" );
		REPORT_BUTTON( EB_RightBumper, "Right Bumper\n" );
		REPORT_BUTTON( EB_A, "A\n" );
		REPORT_BUTTON( EB_B, "B\n" );
		REPORT_BUTTON( EB_X, "X\n" );
		REPORT_BUTTON( EB_Y, "Y\n" );
		REPORT_BUTTON( EB_LeftThumbUp, "LeftUp\n" );
		REPORT_BUTTON( EB_LeftThumbDown, "LeftDown\n" );
		REPORT_BUTTON( EB_LeftThumbLeft, "LeftLeft\n" );
		REPORT_BUTTON( EB_LeftThumbRight, "LeftRight\n" );
		REPORT_BUTTON( EB_RightThumbUp, "RightUp\n" );
		REPORT_BUTTON( EB_RightThumbDown, "RightDown\n" );
		REPORT_BUTTON( EB_RightThumbLeft, "RightLeft\n" );
		REPORT_BUTTON( EB_RightThumbRight, "RightRight\n" );
		REPORT_BUTTON( EB_LeftTrigger, "Left Trigger\n" );
		REPORT_BUTTON( EB_RightTrigger, "Right Trigger\n" );
#undef REPORT_BUTTON

		for( int i = Keyboard::EB_None + 1; i < Keyboard::EB_Max; ++i )
		{
			if( keyboard.IsHigh(i) )
			{
				PRINTF( "0x%02X\n", i );
			}
		}
	}

	Console::DeleteInstance();
	PrintManager::DeleteInstance();
	ConfigManager::DeleteInstance();

	Allocator::GetDefault().Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Write ) );
	DEBUGASSERT( Allocator::GetDefault().CheckForLeaks() );
	Allocator::GetDefault().ShutDown();

	DEBUGASSERT( _CrtCheckMemory() );
	DEBUGASSERT( !_CrtDumpMemoryLeaks() );

	return 0;
}
