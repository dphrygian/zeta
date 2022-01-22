#ifndef CUSTOMNEW_H
#define CUSTOMNEW_H

#include "hashedstring.h"
#include <stddef.h>	// Required for BUILD_LINUX, at least.
#include <new>		// Required for BUILD_LINUX, at least.

// Early allocations through Cocoa framework makes allocator fail on Mac, or something.
// And Steam allocates things through my allocator in Linux, which it doesn't do on Windows (I think ).
// I think for Neon and future projects, I'll just use my allocator for Windows, and maybe only for dev.

// 13 Jan 2018: Disabling allocator because it causes problems with threads launched by SoLoud.
// TODO: Figure out if compiling SoLoud separately could resolve this? It's useful for tracking leaks.
#define USE_ALLOCATOR ( 0 && BUILD_WINDOWS )	// VS2015: Something is using a new form of operator new, I think; track this down later!

class Allocator;

void* operator new( const size_t Size, Allocator& Alloc );
void* operator new( const size_t Size, Allocator& Alloc, const uint Alignment );
void* operator new( const size_t Size, Allocator& Alloc, const HashedString& Tag );
void* operator new( const size_t Size, Allocator& Alloc, const HashedString& Tag, const uint Alignment );
void* operator new[]( const size_t Size, Allocator& Alloc );
void* operator new[]( const size_t Size, Allocator& Alloc, const uint Alignment );
void* operator new[]( const size_t Size, Allocator& Alloc, const HashedString& Tag );
void* operator new[]( const size_t Size, Allocator& Alloc, const HashedString& Tag, const uint Alignment );

// These forms aren't actually used.
// They are just here to fix warnings about there not being deletes to match news.
void operator delete( void* const pObj, Allocator& Alloc );
void operator delete( void* const pObj, Allocator& Alloc, const uint Alignment );
void operator delete( void* const pObj, Allocator& Alloc, const HashedString& Tag );
void operator delete( void* const pObj, Allocator& Alloc, const HashedString& Tag, const uint Alignment );
void operator delete[]( void* const pObj, Allocator& Alloc );
void operator delete[]( void* const pObj, Allocator& Alloc, const uint Alignment );
void operator delete[]( void* const pObj, Allocator& Alloc, const HashedString& Tag );
void operator delete[]( void* const pObj, Allocator& Alloc, const HashedString& Tag, const uint Alignment );

#if USE_ALLOCATOR

#if BUILD_WINDOWS
#define NEW_THROW
#define DELETE_THROW
#else
#define NEW_THROW		throw( std::bad_alloc )
#define DELETE_THROW	throw()
#endif

// Override global new/delete
void* operator new( size_t Size ) NEW_THROW;		// Allocates from the default allocator
void* operator new[]( size_t Size ) NEW_THROW;		// Allocates from the default allocator
void operator delete( void* pObj ) DELETE_THROW;
void operator delete[]( void* pObj ) DELETE_THROW;

void* operator new( size_t Size, uint Alignment );			// Allocates from the default allocator
void* operator new[]( size_t Size, uint Alignment );		// Allocates from the default allocator
void operator delete( void* pObj, uint Alignment );			// Never needs to be called, just in case the compiler wants it automatically
void operator delete[]( void* pObj, uint Alignment );		// Never needs to be called, just in case the compiler wants it automatically

void* operator new( size_t Size, const HashedString& Tag );		// Allocates from the default allocator
void* operator new[]( size_t Size, const HashedString& Tag );	// Allocates from the default allocator
void operator delete( void* pObj, const HashedString& Tag );	// Never needs to be called, just in case the compiler wants it automatically
void operator delete[]( void* pObj, const HashedString& Tag );	// Never needs to be called, just in case the compiler wants it automatically

void* operator new( size_t Size, const HashedString& Tag, uint Alignment );		// Allocates from the default allocator
void* operator new[]( size_t Size, const HashedString& Tag, uint Alignment );	// Allocates from the default allocator
void operator delete( void* pObj, const HashedString& Tag, uint Alignment );	// Never needs to be called, just in case the compiler wants it automatically
void operator delete[]( void* pObj, const HashedString& Tag, uint Alignment );	// Never needs to be called, just in case the compiler wants it automatically

#endif // USE_ALLOCATOR

#endif // CUSTOMNEW_H
