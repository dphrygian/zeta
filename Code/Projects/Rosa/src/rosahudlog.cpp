#include "core.h"
#include "rosahudlog.h"
#include "configmanager.h"
#include "wbworld.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "stringmanager.h"

RosaHUDLog::RosaHUDLog()
:	m_EntryDuration( 0.0f )
,	m_Entries()
{
	InitializeFromDefinition( "RosaHUDLog" );
}

RosaHUDLog::~RosaHUDLog()
{
}

void RosaHUDLog::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntryDuration );
	m_EntryDuration = ConfigManager::GetFloat( sEntryDuration, 0.0f, sDefinitionName );
}

void RosaHUDLog::Clear()
{
	m_Entries.Clear();

	PublishString();
}

/*static*/ void RosaHUDLog::StaticAddMessage( const HashedString& Message )
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pFramework );

	RosaHUDLog* const		pHUDLog		= pGame->GetHUDLog();
	DEVASSERT( pHUDLog );

	const SimpleString		LocString	= ConfigManager::GetLocalizedString( Message, "" );

	pHUDLog->AddEntry( LocString );
}

/*static*/ void RosaHUDLog::StaticAddDynamicMessage( const HashedString& Message )
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pFramework );

	RosaHUDLog* const		pHUDLog		= pGame->GetHUDLog();
	DEVASSERT( pHUDLog );

	const SimpleString		LocString	= ConfigManager::GetLocalizedString( Message, "" );
	const SimpleString		DynString	= StringManager::ParseConfigString( LocString.CStr() );

	pHUDLog->AddEntry( DynString );
}

void RosaHUDLog::AddEntry( const SimpleString& EntryMessage )
{
	SEntry& NewEntry		= m_Entries.PushBack();
	NewEntry.m_Message		= EntryMessage;
	NewEntry.m_ExpireTime	= WBWorld::GetInstance()->GetTime() + m_EntryDuration;

	PublishString();
}

void RosaHUDLog::Tick()
{
	bool		AnyExpirations	= false;
	const float	CurrentTime		= WBWorld::GetInstance()->GetTime();

	FOR_EACH_ARRAY_NOINCR( EntryIter, m_Entries, SEntry )
	{
		const SEntry& Entry = EntryIter.GetValue();
		if( Entry.m_ExpireTime <= CurrentTime )
		{
			m_Entries.Remove( EntryIter );
			AnyExpirations = true;
		}
		else
		{
			// Logs all have same duration and are ordered by time;
			// if we didn't remove this one, we're done iterating.
			break;
		}
	}

	if( AnyExpirations )
	{
		PublishString();
	}
}

void RosaHUDLog::PublishString()
{
	SimpleString ConcatenatedString;

	FOR_EACH_ARRAY( EntryIter, m_Entries, SEntry )
	{
		const SEntry& Entry	= EntryIter.GetValue();
		ConcatenatedString	+= Entry.m_Message;
		ConcatenatedString	+= "\n";
	}

	STATICHASH( RosaHUDLog );
	STATICHASH( Entries );
	ConfigManager::SetString( sEntries, ConcatenatedString.CStr(), sRosaHUDLog );
}
