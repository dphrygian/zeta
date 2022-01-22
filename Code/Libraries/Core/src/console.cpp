#include "core.h"
#include "console.h"

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>

Console* Console::m_Instance = NULL;

Console::Console()
{
	AllocConsole();

	FILE* pStream;
	CHECK( 0 == freopen_s( &pStream, "CONIN$", "r", stdin ) );
	CHECK( 0 == freopen_s( &pStream, "CONOUT$", "w", stdout ) );
	CHECK( 0 == freopen_s( &pStream, "CONOUT$", "w", stderr ) );

	DEBUGPRINTF( "Console initialized\n" );
}

Console::~Console()
{
	FreeConsole();

	// NOTE: No longer restoring stdin/stdout/stderr; there doesn't seem to be a guaranteed way to do that and I don't need it.
}

Console* Console::GetInstance()
{
	if( !m_Instance )
	{
		m_Instance = new Console;
	}
	return m_Instance;
}

void Console::DeleteInstance()
{
	SafeDelete( m_Instance );
}

bool Console::IsOpen()
{
	return ( m_Instance != NULL );
}

void Console::Toggle()
{
	if( IsOpen() )
	{
		DeleteInstance();
	}
	else
	{
		GetInstance();
	}
}

HWND Console::GetHWnd()
{
	return GetConsoleWindow();
}

void Console::SetPos( const int X, const int Y ) const
{
	SetWindowPos( GetConsoleWindow(), HWND_NOTOPMOST, X, Y, 0, 0, SWP_NOSIZE );
}
