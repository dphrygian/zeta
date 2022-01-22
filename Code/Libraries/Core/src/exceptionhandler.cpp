#include "core.h"
#include "exceptionhandler.h"
#include "exceptiontrace.h"
#include "exceptionuploadlog.h"
#include "exceptionwritesteamminidump.h"
#include "allocator.h"
#include "configmanager.h"

#include <Windows.h>

#if BUILD_SDL
#include "SDL2/SDL.h"
#endif

#define UPLOADLOG			0
#define WRITESTEAMMINIDUMP	0 && BUILD_FINAL && BUILD_STEAM	// Disabling this for now because it only works on 32-bit Windows builds

static bool		gIsEnabled	= false;

LONG WINAPI CustomUnhandledExceptionFilter( EXCEPTION_POINTERS* pExceptionPointers )
{
	Unused( pExceptionPointers );

	ASSERT( gIsEnabled );

	// In case the exception was due to game being OOM.
	Allocator::GetDefault().Enable( false );

	PRINTF( "Exception handled:\n" );
	EXCEPTION_RECORD* pExceptionRecord = pExceptionPointers->ExceptionRecord;
	const DWORD FirstExceptionCode = pExceptionRecord ? pExceptionRecord->ExceptionCode : 0;
	Unused( FirstExceptionCode );

	while( pExceptionRecord )
	{
		PRINTF( "\tCode: 0x%08X\n", pExceptionRecord->ExceptionCode );
		PRINTF( "\tAddr: 0x%p\n", pExceptionRecord->ExceptionAddress );
		pExceptionRecord = pExceptionRecord->ExceptionRecord;
	}

	ExceptionTrace::PrintTrace();

#if WRITESTEAMMINIDUMP
	STATICHASH( Version );
	STATICHASH( BuildNumber );
	const uint BuildNumber = ConfigManager::GetInt( sBuildNumber, 0, sVersion );
	ExceptionWriteSteamMinidump::WriteSteamMinidump( FirstExceptionCode, pExceptionPointers, BuildNumber );
#endif

#if UPLOADLOG
	static const DWORD kBreakpointCode = 0x80000003;
	if( FirstExceptionCode == kBreakpointCode )
	{
		// Don't upload logs from asserts in dev mode.
	}
	else
	{
		ExceptionUploadLog::UploadLog();
	}
#endif

#if BUILD_SDL
	SDL_Quit();
#endif

	return EXCEPTION_CONTINUE_SEARCH;
}

void ExceptionHandler::Enable()
{
	ASSERT( !gIsEnabled );

	ExceptionTrace::Enable();

	gIsEnabled	= true;
	SetUnhandledExceptionFilter( CustomUnhandledExceptionFilter );
}

void ExceptionHandler::ShutDown()
{
	ASSERT( gIsEnabled );

	gIsEnabled	= false;
	SetUnhandledExceptionFilter( NULL );

	ExceptionTrace::ShutDown();
}
