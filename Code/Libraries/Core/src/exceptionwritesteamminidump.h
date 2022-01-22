#ifndef EXCEPTIONWRITESTEAMMINIDUMP_H
#define EXCEPTIONWRITESTEAMMINIDUMP_H

#if BUILD_WINDOWS && BUILD_STEAM

#include <Windows.h>

namespace ExceptionWriteSteamMinidump
{
	void WriteSteamMinidump( const DWORD ExceptionCode, EXCEPTION_POINTERS* const pExceptionPointers, const uint BuildNumber );
}

#endif // BUILD_WINDOWS && BUILD_STEAM

#endif // EXCEPTIONWRITESTEAMMINIDUMP_H
