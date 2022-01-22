#include "core.h"
#include "reversehash.h"
#include "hashedstring.h"
#include "simplestring.h"
#include "map.h"

static bool								gReverseHashEnabled = false;
static Map<HashedString, SimpleString>	gReverseHashMap;

#if BUILD_DEV
static SimpleString*					gReversedHashString = NULL;
#endif

void ReverseHash::Initialize()
{
	gReverseHashEnabled = true;

#if BUILD_DEV
	ASSERT( NULL == gReversedHashString );
	gReversedHashString = new SimpleString;
#endif
}

void ReverseHash::ShutDown()
{
	gReverseHashEnabled = false;
	gReverseHashMap.Clear();

#if BUILD_DEV
	SafeDelete( gReversedHashString );
#endif
}

bool ReverseHash::IsEnabled()
{
	return gReverseHashEnabled;
}

void ReverseHash::RegisterHash( const HashedString& Hash, const SimpleString& String )
{
	if( gReverseHashEnabled )
	{
#if BUILD_DEV
		const Map<HashedString, SimpleString>::Iterator HashIter = gReverseHashMap.Search( Hash );
		if( HashIter.IsValid() )
		{
			const SimpleString& ExistingString = HashIter.GetValue();
			if( ExistingString != String )
			{
				PRINTF( "ReverseHash: Hash collision detected between \"%s\" and \"%s\"!\n", ExistingString.CStr(), String.CStr() );
			}
		}
#endif

		gReverseHashMap[ Hash ] = String;
	}
	else
	{
		// Even though it's safe, this shouldn't be called if the reverse hash isn't enabled,
		// because it could be wasting time doing the SimpleString construction.
		WARNDESC( "ReverseHash: Not enabled." );
	}
}

bool ReverseHash::IsRegistered( const HashedString& Hash )
{
	return gReverseHashEnabled && gReverseHashMap.Search( Hash ).IsValid();
}

SimpleString ReverseHash::ReversedHash( const HashedString& Hash )
{
	if( gReverseHashEnabled )
	{
		const Map<HashedString, SimpleString>::Iterator HashIter = gReverseHashMap.Search( Hash );
		if( HashIter.IsValid() )
		{
			return HashIter.GetValue();
		}
		else
		{
			WARN;
			return SimpleString( "(Hash)" );
		}
	}
	else
	{
		WARNDESC( "ReverseHash: Not enabled." );
		return SimpleString( "" );
	}
}

// NOTE: Doesn't account for fragmentation, which is probably a big part of string memory.
void ReverseHash::ReportSize()
{
	if( gReverseHashEnabled )
	{
		uint MemSize = 0;
		FOR_EACH_MAP( HashIter, gReverseHashMap, HashedString, SimpleString )
		{
			const SimpleString& ReverseHashString = HashIter.GetValue();
			MemSize += ReverseHashString.Length() + 1;
		}

		PRINTF( "Reverse hash size:\n" );
		PRINTF( "  Num strings: %d\n", gReverseHashMap.Size() );
		PRINTF( "  Memory size: %d\n", MemSize );
	}
}

#if BUILD_DEV
// For viewing in watch window
const char* ReversedHash( const HashedString& Hash )
{
	ASSERT( gReversedHashString );
	*gReversedHashString = ReverseHash::ReversedHash( Hash );
	return gReversedHashString->CStr();
}

const char* ReversedHash( const uint Hash )
{
	ASSERT( gReversedHashString );
	*gReversedHashString = ReverseHash::ReversedHash( Hash );
	return gReversedHashString->CStr();
}
#endif
