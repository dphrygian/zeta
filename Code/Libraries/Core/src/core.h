#ifndef CORE_H
#define CORE_H

// This should be the first file included in every .cpp (even before that file's matching header).
// This should be kept a very lightweight include that can go anywhere.

#include "versions.h"

#if BUILD_WINDOWS
	// Requires Windows XP, needed for getting the console window, using SwitchToThread, etc.
	#define _WIN32_WINNT 0x0501
#endif

#if BUILD_WINDOWS
	#define FUNCTION_NAME __FUNCTION__
#else
	#define FUNCTION_NAME "unknown function"	// __func__ exists but is a static variable instead of a string literal, so usage will need to change.
#endif

#if BUILD_WINDOWS
	#define VSPRINTF_COUNT( fmt, args )		_vscprintf( fmt, args )
	#define VSPRINTF( buf, len, fmt, args )	vsprintf_s( buf, len, fmt, args )
	#define FOPEN( file, path, mode )		fopen_s( &file, path, mode )
#else
	#define VSPRINTF_COUNT( fmt, args )		vsnprintf( NULL, 0, fmt, args )
	#define VSPRINTF( buf, len, fmt, args )	vsnprintf( buf, len, fmt, args )
	#define FOPEN( file, path, mode )		file = fopen( path, mode )
#endif

#if !BUILD_WINDOWS
	#define memcpy_s( dst, size, src, count )	do { memcpy( dst, src, count );	Unused( size ); } while(0)
	#define strcpy_s( dst, size, src )			do { strcpy( dst, src );		Unused( size ); } while(0)
#endif

#if BUILD_LINUX
	// Used for raise( SIGINT ), which wasn't even working for me.
	// (Possibly because CustomAssert::ShouldAssert was commented out? I've changed that now.)
	#include <csignal>
#endif

#if BUILD_WINDOWS
	// DLP 22 May 2021: I've decided to re-enable all these warnings, except for nameless structs/unions
	// because that is always intentional. I'm leaving this here as reference for other places where the
	// PRAGMA macro is used now instead (such as allowing shadowing/masking from STATICHASH macro).

	//#pragma warning( disable: 4127 )	// conditional expression is constant
	#pragma warning( disable: 4201 )	// nonstandard extension used : nameless struct/union
	//#pragma warning( disable: 4351 )	// new behavior: elements of array '...' will be default initialized
	//#pragma warning( disable: 4456 )	// declaration of '...' hides previous local declaration
	//#pragma warning( disable: 4457 )	// declaration of '...' hides function parameter
	//#pragma warning( disable: 4458 )	// declaration of '...' hides class member
	//#pragma warning( disable: 4459 )	// declaration of '...' hides global declaration
	//#pragma warning( disable: 4482 )	// nonstandard extension used: enum used in qualified name
#endif

#ifdef _DEBUG
	#define BUILD_DEBUG		1	// Defined in Debug only
	#define BUILD_RELEASE	0	// Defined in Release and Final (same as !BUILD_DEBUG)
#else
	#define BUILD_DEBUG		0	// Defined in Debug only
	#define BUILD_RELEASE	1	// Defined in Release and Final (same as !BUILD_DEBUG)
#endif

#ifdef _FINAL
	#define BUILD_FINAL		1	// Defined in Final only
	#define BUILD_DEV		0	// Defined in Debug and Release (same as !BUILD_FINAL)
#else
	#define BUILD_FINAL		0	// Defined in Final only
	#define BUILD_DEV		1	// Defined in Debug and Release (same as !BUILD_FINAL)
#endif

typedef unsigned int	uint;
typedef unsigned int	c_uint32;	// Renamed to avoid conflicts with other headers
typedef unsigned short	c_uint16;	// Renamed to avoid conflicts with other headers
typedef unsigned char	c_uint8;	// Renamed to avoid conflicts with other headers
typedef int				c_int32;	// Renamed to avoid conflicts with other headers
typedef short			c_int16;	// Renamed to avoid conflicts with other headers
typedef char			c_int8;		// Renamed to avoid conflicts with other headers
typedef unsigned char	byte;

#undef NULL
#define NULL 0

// See http://kernelnewbies.org/FAQ/DoWhile0 for the reason behind this syntax
#define SafeDelete( ptr )		do { if( ( ptr ) ) { delete ( ptr ); ( ptr ) = NULL; } } while(0)
#define SafeDeleteNoNull( ptr )	do { if( ( ptr ) ) { delete ( ptr ); } } while(0)
#define SafeDeleteArray( ptr )	do { if( ( ptr ) ) { delete[] ( ptr ); ( ptr ) = NULL; } } while(0)
#define SafeRelease( ptr )		do { if( ( ptr ) ) { ( ptr )->Release(); ( ptr ) = NULL; } } while(0) // For refcounting interfaces
#if BUILD_WINDOWS
	// The sizeof() thing used to work in older versions of VS, but not anymore
	#define Unused( exp )		do { ( void )( exp ); } while(0)
#else
	#define Unused( exp )		do { ( void )sizeof( ( exp ) ); } while(0)
#endif
#define DoExp( exp )			do { ( exp ); } while(0)
#define DoNothing				do { ( void )0; } while(0)

#if BUILD_DEV
	#if BUILD_WINDOWS
		#define BREAKPOINT		DoExp( __debugbreak() )
	#elif BUILD_LINUX
		#define BREAKPOINT		raise( SIGINT )
	#else
		// TODO PORT LATER: Support breakpoints on other platforms
		#define BREAKPOINT		DoNothing
	#endif
#else
	#define BREAKPOINT			DoNothing
#endif

#if BUILD_DEBUG
	#define DEBUGBREAKPOINT	BREAKPOINT
#else
	#define DEBUGBREAKPOINT	DoNothing
#endif

#if BUILD_DEBUG
	// Don't force inline on debug builds, it can cause warnings
	#define FORCE_INLINE		inline
#else
	#if BUILD_WINDOWS
		#define FORCE_INLINE	__forceinline
	#else
		#define FORCE_INLINE	inline __attribute__((__always_inline__))
	#endif
#endif

// In addition to the MSVC-specific __pragma form, C++11 supports the _Pragma("warning( suppress : 4127 )") form.
// But since I'm only using pragmas for MSVC warnings, I'm not bothering to implement it for other platforms for now.
#if BUILD_WINDOWS
	#define PRAGMA( p )	__pragma( p )
#else
	#define PRAGMA( p )	DoNothing
#endif

// Other includes should go at the end of this file,
// so they can reference anything defined in here
#include "customassert.h"
#include "customnew.h"
#include "printmanager.h"
#include "profiler.h"
#include "exceptiontrace.h"

#if BUILD_WINDOWS
#include "exceptionhandler.h"
#endif

#include "color.h"

#else

#error core.h included twice, check structure. This should be the first include in a .cpp file, and never included from a .h file.

#endif
