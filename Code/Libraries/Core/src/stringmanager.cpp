#include "core.h"
#include "stringmanager.h"
#include "array.h"
#include "configmanager.h"
#include "configparser.h"
#include "mathcore.h"
#include "memorystream.h"
#include "filestream.h"
#include "allocator.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

StringManager*	StringManager::m_Instance = NULL;
Allocator		StringManager::m_AllocatorPermanent( "StringManager/Permanent" );
Allocator		StringManager::m_AllocatorTransient( "StringManager/Transient" );
bool			StringManager::m_UsingAllocator = false;

StringManager::StringManager()
:	m_Strings()
{
}

StringManager::~StringManager()
{
	FlushStrings( ESL_Permanent );
}

const char* StringManager::PrintF( EStringLife Life, const char* FormatString, ... )
{
	va_list Args;

	int Length;
	char* Buffer;

	va_start( Args, FormatString );
	Length = VSPRINTF_COUNT( FormatString, Args ) + 1;
	va_end( Args );

	Buffer = Allocate( Life, Length );

	va_start( Args, FormatString );
	VSPRINTF( Buffer, Length, FormatString, Args );
	va_end( Args );

	AddString( Life, Buffer );

	return Buffer;
}

// Parses strings containing tokens of the form "#{[type]<:context>:[name]}", where type is:
// b: bool (replaced with localized True/False)
// a: bool (replaced with localized Achieved/NotAchieved) (not actually used anymore!)
// !: bool (replaced with localized and recursively parsed Notification_On/Notification_Off)
// X: bool (replaced with localized and recursively parsed Objective_Complete/Objective_Incomplete; no support for failed objectives this way)
// i: signed int
// t: int as 2 digits for time
// x: int as unsigned hex
// f: float
// p: float as int percent (multiplied by 100, percent sign added, no decimal places)
// s: string
// k: localize (doesn't do a config lookup, just localizes the given key (ignores context))
// l: localized string (does a config lookup expecting a loc key and then localizes it)
// n: non-localized config string (does a config lookup expecting a literal string and then recursively parses it as a config string; subsumes the "s" tag)
// c: config string (does a config lookup expecting a loc key, localizes it, and then recursively parses that as a config string; subsumes the "l" tag)
const char* StringManager::ParseConfigString( EStringLife Life, const char* ConfigString )
{
	ASSERT( ConfigString );

	Array< char > ParsedString;

	while( *ConfigString )
	{
		char NextChar = *ConfigString++;
		if( NextChar == '#' )
		{
			// Maybe a token!!
			NextChar = *ConfigString++;
			if( NextChar == '{' )
			{
				// A token!!
				ParseConfigToken( ParsedString, ConfigString );
			}
			else
			{
				ParsedString.PushBack( '#' );
				ParsedString.PushBack( NextChar );
			}
		}
		else
		{
			ParsedString.PushBack( NextChar );
		}
	}
	ParsedString.PushBack( '\0' );

	// Make a permanent copy of the string
	char* TheString = Allocate( Life, ParsedString.Size() );
	memcpy( TheString, ParsedString.GetData(), ParsedString.Size() );
	AddString( Life, TheString );
	return TheString;
}

/*static*/ SimpleString StringManager::ParseConfigString( const char* ConfigString )
{
	ASSERT( ConfigString );

	Array< char > ParsedString;

	while( *ConfigString )
	{
		char NextChar = *ConfigString++;
		if( NextChar == '#' )
		{
			// Maybe a token!!
			NextChar = *ConfigString++;
			if( NextChar == '{' )
			{
				// A token!!
				ParseConfigToken( ParsedString, ConfigString );
			}
			else
			{
				ParsedString.PushBack( '#' );
				ParsedString.PushBack( NextChar );
			}
		}
		else
		{
			ParsedString.PushBack( NextChar );
		}
	}
	ParsedString.PushBack( '\0' );

	return SimpleString( ParsedString );
}

void StringManager::ParseConfigToken( Array< char >& ParsedString, const char*& ConfigString )
{
	Array< char > FirstPartArray;	// Could be name or context
	Array< char > SecondPartArray;	// Name is first part is context
	bool HasContext = false;

	char TypeChar = *ConfigString++;
	ASSERT( *ConfigString == ':' );
	ConfigString++;

	while( *ConfigString )
	{
		char NextNameChar = *ConfigString++;
		if( NextNameChar == ':' )
		{
			HasContext = true;
			break;
		}
		else if( NextNameChar == '}' )
		{
			break;
		}
		else
		{
			FirstPartArray.PushBack( NextNameChar );
		}
	}
	FirstPartArray.PushBack( '\0' );

	if( HasContext )
	{
		while( *ConfigString )
		{
			char NextContextChar = *ConfigString++;
			if( NextContextChar == '}' )
			{
				break;
			}
			else
			{
				SecondPartArray.PushBack( NextContextChar );
			}
		}
	}
	SecondPartArray.PushBack( '\0' );

	SimpleString Name = ( SecondPartArray.Size() > 1 ) ? SecondPartArray.GetData() : FirstPartArray.GetData();
	SimpleString Context = ( SecondPartArray.Size() > 1 ) ? FirstPartArray.GetData() : "";

	const char* AppendString = NULL;

	if( TypeChar == 'b' )
	{
		bool TheBool = ConfigManager::GetBool( Name, false, Context );
		AppendString = ConfigManager::GetLocalizedString( ( TheBool ? "True" : "False" ), "" );
	}
	else if( TypeChar == 'a' )
	{
		bool TheBool = ConfigManager::GetBool( Name, false, Context );
		AppendString = ConfigManager::GetLocalizedString( ( TheBool ? "Achieved" : "NotAchieved" ), "" );
	}
	else if( TypeChar == '!' )
	{
		bool TheBool = ConfigManager::GetBool( Name, false, Context );
		const char* const InnerConfigString = ConfigManager::GetLocalizedString( ( TheBool ? "Notification_On" : "Notification_Off" ), "" );
		AppendString = ParseConfigString( ESL_Transient, InnerConfigString );	// HACKHACK: Re-parse like with the "c" specifier, because notification glyphs are using markup to render white
	}
	else if( TypeChar == 'X' )
	{
		bool TheBool = ConfigManager::GetBool( Name, false, Context );
		const char* const InnerConfigString = ConfigManager::GetLocalizedString( ( TheBool ? "Objective_Complete" : "Objective_Incomplete" ), "" );
		AppendString = ParseConfigString( ESL_Transient, InnerConfigString );	// HACKHACK: Re-parse like with the "c" specifier, because objective glyphs are using markup to render white
	}
	else if( TypeChar == 'i' )
	{
		int TheInt = ConfigManager::GetInt( Name, 0, Context );
		AppendString = PrintF( ESL_Transient, "%d", TheInt );
	}
	else if( TypeChar == 't' )
	{
		int TheInt = ConfigManager::GetInt( Name, 0, Context );
		AppendString = PrintF( ESL_Transient, "%02d", TheInt );
	}
	else if( TypeChar == 'x' )
	{
		int TheInt = ConfigManager::GetInt( Name, 0, Context );
		AppendString = PrintF( ESL_Transient, "0x%08X", TheInt );
	}
	else if( TypeChar == 'f' )
	{
		float TheFloat = ConfigManager::GetFloat( Name, 0.0f, Context );
		AppendString = PrintF( ESL_Transient, "%.2f", TheFloat );
	}
	else if( TypeChar == 'p' )
	{
		float TheFloat = ConfigManager::GetFloat( Name, 0.0f, Context );
		AppendString = PrintF( ESL_Transient, "%d%%", RoundToUInt( 100.0f * TheFloat ) );
	}
	else if( TypeChar == 's' )
	{
		AppendString = ConfigManager::GetString( Name, "", Context );
	}
	else if( TypeChar == 'k' )
	{
		AppendString = ConfigManager::GetLocalizedString( Name, "" );
	}
	else if( TypeChar == 'l' )
	{
#if PARANOID_HASH_CHECK
		const char* const	InnerString	= ConfigManager::GetString( Name, "", Context );
		AppendString = ConfigManager::GetLocalizedString( InnerString, "" );
#else
		const HashedString	InnerName	= ConfigManager::GetHash( Name, "", Context );
		DEBUGASSERT( InnerName == HashedString( ConfigManager::GetString( Name, "", Context ) ) );	// Making sure I didn't break anything switch to GetHash...
		AppendString = ConfigManager::GetLocalizedString( InnerName, "" );
#endif // PARANOID_HASH_CHECK
	}
	else if( TypeChar == 'n' )
	{
		const char* const InnerConfigString = ConfigManager::GetString( Name, "", Context );
		AppendString = ParseConfigString( ESL_Transient, InnerConfigString );
	}
	else if( TypeChar == 'c' )
	{
#if PARANOID_HASH_CHECK
		const char* const	InnerString			= ConfigManager::GetString( Name, "", Context );
		const char* const	InnerConfigString	= ConfigManager::GetLocalizedString( InnerString, "" );
#else
		const HashedString	InnerName			= ConfigManager::GetHash( Name, "", Context );
		DEBUGASSERT( InnerName == HashedString( ConfigManager::GetString( Name, "", Context ) ) );	// Making sure I didn't break anything switch to GetHash...
		const char* const	InnerConfigString	= ConfigManager::GetLocalizedString( InnerName, "" );
#endif // PARANOID_HASH_CHECK
		AppendString = ParseConfigString( ESL_Transient, InnerConfigString );
	}

	ASSERT( AppendString );

	while( *AppendString )
	{
		ParsedString.PushBack( *AppendString++ );
	}
}

/*static*/ bool StringManager::ResolveAndEvaluateConditional( const char* ConditionalString )
{
	// Parse instruction for config vars, if any, and then evaluate as a conditional
	const SimpleString Expression = ParseConfigString( ConditionalString );
	return ConfigParser::EvaluateConditional( MemoryStream( const_cast< char* >( Expression.CStr() ), Expression.Length() + 1 ) );
}

/*static*/ void StringManager::AddString( EStringLife Life, const char* String )
{
	GetInstance()->m_Strings[ Life ].Insert( String );
}

/*static*/ void StringManager::FlushStrings( EStringLife Life )
{
	GetInstance()->_FlushStrings( Life );
}

/*static*/ void StringManager::RemoveString( EStringLife Life, const char* StringToRemove )
{
	GetInstance()->m_Strings[ Life ].Remove( StringToRemove );
}

void StringManager::_FlushStrings( EStringLife Life )
{
	FOR_EACH_MAP( SetIter, m_Strings, EStringLife, Set< const char* > )
	{
		if( SetIter.GetKey() <= Life )
		{
			Set< const char* >& LifeSet = *SetIter;
			FOR_EACH_SET( StringIter, LifeSet, const char* )
			{
				SafeDelete( *StringIter );
			}
			LifeSet.Clear();
		}
	}
}

/*static*/ void	StringManager::Report()
{
	PRINTF( "StringManager report:\n" );
	const Map<EStringLife, Set<const char*>>& Strings = GetInstance()->m_Strings;
	FOR_EACH_MAP( LifetimeIter, Strings, EStringLife, Set<const char*> )
	{
		uint MemoryUsed = 0;
		const Set<const char*>& StringsSet = LifetimeIter.GetValue();
		FOR_EACH_SET( StringsIter, StringsSet, const char* )
		{
			const char* const pString = StringsIter.GetValue();
			const uint StringMemory = static_cast<uint>( strlen( pString ) ) + 1;
			MemoryUsed += StringMemory;
		}

		const char* const pLifetimeName = ( LifetimeIter.GetKey() == EStringLife::ESL_Transient ) ? "Transient" : "Permanent";
		PRINTF( "  %s: %d bytes in %d strings\n", pLifetimeName, MemoryUsed, StringsSet.Size() );
	}
}

StringManager* StringManager::GetInstance()
{
	if( !m_Instance )
	{
		m_Instance = new StringManager;
	}
	return m_Instance;
}

void StringManager::DeleteInstance()
{
	SafeDelete( m_Instance );
}

char* StringManager::Allocate( EStringLife Life, uint Size )
{
	if( m_UsingAllocator )
	{
		return new( GetAllocator( Life ) ) char[ Size ];
	}
	else
	{
		return new char[ Size ];
	}
}

Allocator& StringManager::GetAllocator( EStringLife Life )
{
	if( Life == ESL_Permanent )
	{
		return m_AllocatorPermanent;
	}
	else
	{
		return m_AllocatorTransient;
	}
}

/*static*/ void StringManager::InitializeAllocator( uint Size )
{
	m_AllocatorPermanent.Initialize( Size >> 1 );
	m_AllocatorTransient.Initialize( Size >> 1 );

	m_UsingAllocator = true;
}

/*static*/ void StringManager::ShutDownAllocator()
{
#if BUILD_DEBUG
	m_AllocatorPermanent.Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Append ) );
	m_AllocatorTransient.Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Append ) );
#endif

	DEBUGASSERT( m_AllocatorPermanent.CheckForLeaks() );
	DEBUGASSERT( m_AllocatorTransient.CheckForLeaks() );

	m_AllocatorPermanent.ShutDown();
	m_AllocatorTransient.ShutDown();

	m_UsingAllocator = false;
}

/*static*/ void StringManager::ReportAllocator( const SimpleString& Filename )
{
	Unused( Filename );
#if BUILD_DEBUG
	m_AllocatorPermanent.Report( FileStream( Filename.CStr(), FileStream::EFM_Append ) );
	m_AllocatorTransient.Report( FileStream( Filename.CStr(), FileStream::EFM_Append ) );
#endif
}
