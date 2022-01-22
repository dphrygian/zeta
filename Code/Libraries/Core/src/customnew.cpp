#include "core.h"
#include "customnew.h"
#include "allocator.h"
#include "allocatorchunk.h"

#include <stdlib.h>

void* operator new( const size_t Size, Allocator& Alloc )
{
	return Alloc.Allocate( static_cast<uint>( Size ) );
}

void* operator new( const size_t Size, Allocator& Alloc, const uint Alignment )
{
	return Alloc.Allocate( static_cast<uint>( Size ), HashedString::NullString, Alignment );
}

void* operator new( const size_t Size, Allocator& Alloc, const HashedString& Tag )
{
	return Alloc.Allocate( static_cast<uint>( Size ), Tag );
}

void* operator new( const size_t Size, Allocator& Alloc, const HashedString& Tag, const uint Alignment )
{
	return Alloc.Allocate( static_cast<uint>( Size ), Tag, Alignment );
}

void* operator new[]( const size_t Size, Allocator& Alloc )
{
	return Alloc.Allocate( static_cast<uint>( Size ) );
}

void* operator new[]( const size_t Size, Allocator& Alloc, const uint Alignment )
{
	return Alloc.Allocate( static_cast<uint>( Size ), HashedString::NullString, Alignment );
}

void* operator new[]( const size_t Size, Allocator& Alloc, const HashedString& Tag )
{
	return Alloc.Allocate( static_cast<uint>( Size ), Tag );
}

void* operator new[]( const size_t Size, Allocator& Alloc, const HashedString& Tag, const uint Alignment )
{
	return Alloc.Allocate( static_cast<uint>( Size ), Tag, Alignment );
}

// These forms aren't actually used.
void operator delete( void* pObj, Allocator& Alloc )
{
	Unused( pObj );
	Unused( Alloc );

	DEBUGBREAKPOINT;
}

void operator delete( void* pObj, Allocator& Alloc, const uint Alignment )
{
	Unused( pObj );
	Unused( Alloc );
	Unused( Alignment );

	DEBUGBREAKPOINT;
}

void operator delete( void* pObj, Allocator& Alloc, const HashedString& Tag )
{
	Unused( pObj );
	Unused( Alloc );
	Unused( Tag );

	DEBUGBREAKPOINT;
}

void operator delete( void* pObj, Allocator& Alloc, const HashedString& Tag, const uint Alignment )
{
	Unused( pObj );
	Unused( Alloc );
	Unused( Tag );
	Unused( Alignment );

	DEBUGBREAKPOINT;
}

void operator delete[]( void* pObj, Allocator& Alloc )
{
	Unused( pObj );
	Unused( Alloc );

	DEBUGBREAKPOINT;
}

void operator delete[]( void* pObj, Allocator& Alloc, const uint Alignment )
{
	Unused( pObj );
	Unused( Alloc );
	Unused( Alignment );

	DEBUGBREAKPOINT;
}

void operator delete[]( void* pObj, Allocator& Alloc, const HashedString& Tag )
{
	Unused( pObj );
	Unused( Alloc );
	Unused( Tag );

	DEBUGBREAKPOINT;
}

void operator delete[]( void* pObj, Allocator& Alloc, const HashedString& Tag, const uint Alignment )
{
	Unused( pObj );
	Unused( Alloc );
	Unused( Tag );
	Unused( Alignment );

	DEBUGBREAKPOINT;
}

#if USE_ALLOCATOR
void* operator new( const size_t Size ) NEW_THROW
{
	void* pObj = NULL;
	if( Allocator::IsEnabled() )
	{
		pObj = Allocator::GetDefault().Allocate( static_cast<uint>( Size ) );
	}
	else
	{
		pObj = malloc( Size );
	}
	DEVASSERT( pObj );
	return pObj;
}

void* operator new[]( const size_t Size ) NEW_THROW
{
	void* pObj = NULL;
	if( Allocator::IsEnabled() )
	{
		pObj = Allocator::GetDefault().Allocate( static_cast<uint>( Size ) );
	}
	else
	{
		pObj = malloc( Size );
	}
	DEVASSERT( pObj );
	return pObj;
}

void operator delete( void* pObj ) DELETE_THROW
{
	if( !pObj )
	{
		return;
	}

	if( Allocator::IsEnabled() )
	{
		Allocator::GetDefault().Free( pObj );
	}
	else
	{
		free( pObj );
	}
}

void operator delete[]( void* pObj ) DELETE_THROW
{
	if( !pObj )
	{
		return;
	}

	if( Allocator::IsEnabled() )
	{
		Allocator::GetDefault().Free( pObj );
	}
	else
	{
		free( pObj );
	}
}

void* operator new( const size_t Size, uint Alignment )
{
	return Allocator::GetDefault().Allocate( static_cast<uint>( Size ), HashedString::NullString, Alignment );
}

void* operator new[]( const size_t Size, uint Alignment )
{
	return Allocator::GetDefault().Allocate( static_cast<uint>( Size ), HashedString::NullString, Alignment );
}

void operator delete( void* pObj, uint Alignment )
{
	Unused( Alignment );

	if( !pObj )
	{
		return;
	}

	// In C++14, this form matches the size_t delete instead of a custom alignment form, urgh
	if (Allocator::IsEnabled())
	{
		Allocator::GetDefault().Free(pObj);
	}
	else
	{
		free(pObj);
	}
}

void operator delete[]( void* pObj, uint Alignment )
{
	Unused( Alignment );

	if( !pObj )
	{
		return;
	}

	// In C++14, this form matches the size_t delete instead of a custom alignment form, urgh
	if (Allocator::IsEnabled())
	{
		Allocator::GetDefault().Free(pObj);
	}
	else
	{
		free(pObj);
	}
}

void* operator new( const size_t Size, const HashedString& Tag )
{
	return Allocator::GetDefault().Allocate( static_cast<uint>( Size ), Tag );
}

void* operator new[]( const size_t Size, const HashedString& Tag )
{
	return Allocator::GetDefault().Allocate( static_cast<uint>( Size ), Tag );
}

void operator delete( void* pObj, const HashedString& Tag )
{
	Unused( Tag );

	if( !pObj )
	{
		return;
	}

	// No need to check if Allocator::IsEnabled(), if this form is used then it should be
	Allocator::GetDefault().Free( pObj );
}

void operator delete[]( void* pObj, const HashedString& Tag )
{
	Unused( Tag );

	if( !pObj )
	{
		return;
	}

	// No need to check if Allocator::IsEnabled(), if this form is used then it should be
	Allocator::GetDefault().Free( pObj );
}

void* operator new( const size_t Size, const HashedString& Tag, uint Alignment )
{
	return Allocator::GetDefault().Allocate( static_cast<uint>( Size ), Tag, Alignment );
}

void* operator new[]( const size_t Size, const HashedString& Tag, uint Alignment )
{
	return Allocator::GetDefault().Allocate( static_cast<uint>( Size ), Tag, Alignment );
}

void operator delete( void* pObj, const HashedString& Tag, uint Alignment )
{
	Unused( Alignment );
	Unused( Tag );

	if( !pObj )
	{
		return;
	}

	// No need to check if Allocator::IsEnabled(), if this form is used then it should be
	Allocator::GetDefault().Free( pObj );
}

void operator delete[]( void* pObj, const HashedString& Tag, uint Alignment )
{
	Unused( Alignment );
	Unused( Tag );

	if( !pObj )
	{
		return;
	}

	// No need to check if Allocator::IsEnabled(), if this form is used then it should be
	Allocator::GetDefault().Free( pObj );
}
#endif // USE_ALLOCATOR
