#include "core.h"
#include "hashedstring.h"
#include "simplestring.h"
#include "reversehash.h"

const HashedString HashedString::NullString( "" );

HashedString::HashedString()
:	m_Hash( 0 )
{
}

HashedString::HashedString( c_uint32 Hash )
:	m_Hash( Hash )
{
}

HashedString::HashedString( const char* String )
:	m_Hash( Hash( String ) )
{
}

HashedString::HashedString( const SimpleString& String )
:	m_Hash( Hash( String.CStr() ) )
{
}

HashedString& HashedString::operator=( const char* String )
{
	m_Hash = Hash( String );
	return *this;
}

HashedString& HashedString::operator=( const SimpleString& String )
{
	m_Hash = Hash( String.CStr() );
	return *this;
}

bool HashedString::Equals( const HashedString& H ) const
{
	return ( m_Hash == H.m_Hash );
}

bool HashedString::IsNull() const
{
	return ( m_Hash == 0 );
}

c_uint32 HashedString::GetHash() const
{
	return m_Hash;
}

// I believe this is the PJW/ELF hash. Non-cryptographic.
/*static*/ c_uint32 HashedString::Hash( const char* const String )
{
	if( !String )
	{
		return 0;
	}

	const char* CharIter = String;
	c_uint32 HashValue = 0;
	c_uint32 x;
	char c;
	while( ( c = *CharIter++ ) != 0 )
	{
		HashValue = ( HashValue << 4 ) + c;
		if( ( x = HashValue & 0xF0000000 ) != 0 )
		{
			HashValue ^= ( x >> 24 );
		}
		HashValue &= ~x;
	}

	if( ReverseHash::IsEnabled() )
	{
		ReverseHash::RegisterHash( HashedString( HashValue ), SimpleString( String ) );
	}

	return HashValue;
}

#if BUILD_DEV
// For viewing in watch window
const char* HashedString::ReversedHash() const
{
	static SimpleString StaticString;
	StaticString = ReverseHash::ReversedHash( *this );
	return StaticString.CStr();
}
#endif
