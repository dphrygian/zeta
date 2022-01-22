#ifndef HASHEDSTRING_H
#define HASHEDSTRING_H

#define STATIC_HASHED_STRING(name)				\
	PRAGMA( warning( suppress: 4456 ) );		\
	static const HashedString s##name(#name)

class SimpleString;

class HashedString
{
public:
	HashedString();
	HashedString( c_uint32 Hash );
	HashedString( const char* String );
	HashedString( const SimpleString& String );

	HashedString&	operator=( const char* String );
	HashedString&	operator=( const SimpleString& String );

	inline bool		operator==( const HashedString& Other ) const
	{
		return ( m_Hash == Other.m_Hash );
	}

	inline			operator c_uint32() const
	{
		return m_Hash;
	}

	bool	Equals( const HashedString& H ) const;
	bool	IsNull() const;

	c_uint32	GetHash() const;

#if BUILD_DEV
	// For viewing in watch window, using ReverseHash
	const char*	ReversedHash() const;
#endif

	static c_uint32 Hash( const char* const String );

	static const HashedString NullString;

private:
	c_uint32 m_Hash;
};

#endif
