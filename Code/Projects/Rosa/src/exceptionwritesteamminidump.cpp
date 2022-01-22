#include "core.h"
#include "exceptionwritesteamminidump.h"

#if BUILD_WINDOWS && BUILD_STEAM

#include "simplestring.h"

#include "steam/steam_api.h"

void ExceptionWriteSteamMinidump::WriteSteamMinidump( const DWORD ExceptionCode, EXCEPTION_POINTERS* const pExceptionPointers, const uint BuildNumber )
{
	// NOTE: Even if we're out of memory, PrintF shouldn't fail because ExceptionHandler disables our allocator
	const SimpleString MiniDumpComment = SimpleString::PrintF( "NEON STRUCT: Desperation Column Build %d", BuildNumber );
	SteamAPI_SetMiniDumpComment( MiniDumpComment.CStr() );
	SteamAPI_WriteMiniDump( ExceptionCode, pExceptionPointers, 0 );
}

#endif // BUILD_WINDOWS && BUILD_STEAM
