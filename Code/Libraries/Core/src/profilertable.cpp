#include "core.h"

#if DO_PROFILING

#include "idatastream.h"
#include "clock.h"
#include <memory.h>
#include <stdio.h>
#include <string.h>

ProfilerTableEntry::ProfilerTableEntry()
:	m_Name( NULL )
,	m_TotalInclusiveTime( 0 )
,	m_TotalExclusiveTime( 0 )
,	m_MaxInclusiveTime( 0 )
,	m_MaxExclusiveTime( 0 )
,	m_NumCalls( 0 )
,	m_CallstackDepth( 0 )
,	m_CallstackHash( HashedString::NullString )
,	m_CallstackParentHash( HashedString::NullString )
{
}

ProfilerTableEntry::~ProfilerTableEntry()
{
}

ProfilerTable::ProfilerTable()
:	m_Entries()
,	m_Overhead()
,	m_Inclusive()
,	m_HashStack()
{
	m_Overhead.Push( 0 );
	m_Inclusive.Push( 0 );
	m_HashStack.Push( HashedString::NullString );
}

ProfilerTable::~ProfilerTable()
{
	m_Overhead.Pop();
	m_Inclusive.Pop();
	m_HashStack.Pop();
	m_Entries.Clear();
}

void ProfilerTable::AddEntries( const char* Name, const HashedString& HashedName, const HashedString& HashStackName, __int64 InclusiveTime, __int64 ExclusiveTime )
{
	{
		ProfilerTableEntry&	PlainEntry = m_Entries[ HashedName ];

		// Search for an existing entry matching this function sig and update it
		if( PlainEntry.m_NumCalls )
		{
			PlainEntry.m_NumCalls++;
			PlainEntry.m_TotalInclusiveTime		+= InclusiveTime;
			PlainEntry.m_TotalExclusiveTime		+= ExclusiveTime;
			if( InclusiveTime > PlainEntry.m_MaxInclusiveTime )
			{
				PlainEntry.m_MaxInclusiveTime	= InclusiveTime;
			}
			if( ExclusiveTime > PlainEntry.m_MaxExclusiveTime )
			{
				PlainEntry.m_MaxExclusiveTime	= ExclusiveTime;
			}
		}
		else
		{
			// If we get here, a matching entry wasn't found. Add one.
			PlainEntry.m_Name					= Name;
			PlainEntry.m_NumCalls				= 1;
			PlainEntry.m_TotalInclusiveTime		= InclusiveTime;
			PlainEntry.m_TotalExclusiveTime		= ExclusiveTime;
			PlainEntry.m_MaxInclusiveTime		= InclusiveTime;
			PlainEntry.m_MaxExclusiveTime		= ExclusiveTime;
		}
	}

	{
		ProfilerTableEntry&	StackEntry = m_Entries[ HashStackName ];

		// Search for an existing entry matching this function sig and update it
		if( StackEntry.m_NumCalls )
		{
			StackEntry.m_NumCalls++;
			StackEntry.m_TotalInclusiveTime		+= InclusiveTime;
			StackEntry.m_TotalExclusiveTime		+= ExclusiveTime;
			if( InclusiveTime > StackEntry.m_MaxInclusiveTime )
			{
				StackEntry.m_MaxInclusiveTime	= InclusiveTime;
			}
			if( ExclusiveTime > StackEntry.m_MaxExclusiveTime )
			{
				StackEntry.m_MaxExclusiveTime	= ExclusiveTime;
			}
		}
		else
		{
			// If we get here, a matching entry wasn't found. Add one.
			StackEntry.m_Name					= Name;
			StackEntry.m_NumCalls				= 1;
			StackEntry.m_TotalInclusiveTime		= InclusiveTime;
			StackEntry.m_TotalExclusiveTime		= ExclusiveTime;
			StackEntry.m_MaxInclusiveTime		= InclusiveTime;
			StackEntry.m_MaxExclusiveTime		= ExclusiveTime;
			StackEntry.m_CallstackDepth			= m_HashStack.Size();
			StackEntry.m_CallstackHash			= HashStackName;
			StackEntry.m_CallstackParentHash	= m_HashStack.Top(); // We've already popped our own hash
		}
	}
}

void ProfilerTable::DumpStackEntries( const IDataStream& Stream, const HashedString& ParentHash, const bool Minimal )
{
	// Go ahead and deep copy entries just in case the map changes while dumping
	// (and because ArraySortFunc doesn't like pointers, sigh).
	Array<ProfilerTableEntry> ChildEntries;

	// Find children
	FOR_EACH_MAP( EntryIter, m_Entries, HashedString, ProfilerTableEntry )
	{
		const ProfilerTableEntry& Entry = EntryIter.GetValue();
		if( Entry.m_CallstackDepth > 0 &&				// Skip plain entries
			Entry.m_CallstackParentHash == ParentHash )
		{
			ChildEntries.PushBack( Entry );
		}
	}

	// Sort by total inclusive time
	ChildEntries.InsertionSort(
		[]( const ProfilerTableEntry& EntryA, const ProfilerTableEntry& EntryB )
		{
			return EntryA.m_TotalInclusiveTime > EntryB.m_TotalInclusiveTime;
		}
	);

	if( HashedString::NullString == ParentHash )
	{
		// Write header
		if( Minimal )
		{
			Stream.PrintF( "\nFuncSig TotIncTime TotExcTime\n" );
		}
		else
		{
			Stream.PrintF( "\nFuncSig\tTotIncTime\tTotExcTime\tNumCalls\tAvgIncTime\tAvgExcTime\tMaxIncTime\tMaxExcTime\n" );
		}
	}

	FOR_EACH_ARRAY( ChildEntryIter, ChildEntries, ProfilerTableEntry )
	{
		const ProfilerTableEntry& ChildEntry = ChildEntryIter.GetValue();
		DumpStackEntry( Stream, ChildEntry, Minimal );
		DumpStackEntries( Stream, ChildEntry.m_CallstackHash, Minimal );
	}
}

void ProfilerTable::DumpStackEntry( const IDataStream& Stream, const ProfilerTableEntry& Entry, const bool Minimal )
{
	double ClockResolution = Profiler::GetInstance()->m_Clock->GetResolution();

	// Function name prefixed by depth
	for( int Depth = 1; Depth < Entry.m_CallstackDepth; ++Depth )
	{
		if( Minimal )
		{
			Stream.PrintF( " " );
		}
		else
		{
			Stream.PrintF( "    " );
		}
	}
	Stream.PrintF( Entry.m_Name );

	if( Minimal )
	{
		// Total inclusive time in seconds
		Stream.PrintF( " %f", (double)Entry.m_TotalInclusiveTime * ClockResolution );

		// Total exclusive time in seconds
		Stream.PrintF( " %f", (double)Entry.m_TotalExclusiveTime * ClockResolution );
	}
	else
	{
		// Total inclusive time in seconds
		Stream.PrintF( "\t%f", (double)Entry.m_TotalInclusiveTime * ClockResolution );

		// Total exclusive time in seconds
		Stream.PrintF( "\t%f", (double)Entry.m_TotalExclusiveTime * ClockResolution );

		// Number of calls
		Stream.PrintF( "\t%d", Entry.m_NumCalls );

		// Average inclusive time in seconds
		Stream.PrintF( "\t%f", ( (double)Entry.m_TotalInclusiveTime / (double)Entry.m_NumCalls ) * ClockResolution );

		// Average exclusive time in seconds
		Stream.PrintF( "\t%f", ( (double)Entry.m_TotalExclusiveTime / (double)Entry.m_NumCalls ) * ClockResolution );

		// Max inclusive time in seconds
		Stream.PrintF( "\t%f", (double)Entry.m_MaxInclusiveTime * ClockResolution );

		// Max exclusive time in seconds
		Stream.PrintF( "\t%f",  (double)Entry.m_MaxExclusiveTime * ClockResolution );
	}

	Stream.PrintF( "\n" );
}

void ProfilerTable::DumpPlainEntry( const IDataStream& Stream, const ProfilerTableEntry& Entry )
{
	double ClockResolution = Profiler::GetInstance()->m_Clock->GetResolution();

	// Function name
	Stream.PrintF( Entry.m_Name );

	// Average exclusive time in seconds
	Stream.PrintF( "\t%f", ( (double)Entry.m_TotalExclusiveTime / (double)Entry.m_NumCalls ) * ClockResolution );

	// Average inclusive time in seconds
	Stream.PrintF( "\t%f", ( (double)Entry.m_TotalInclusiveTime / (double)Entry.m_NumCalls ) * ClockResolution );

	// Max exclusive time in seconds
	Stream.PrintF( "\t%f",  (double)Entry.m_MaxExclusiveTime * ClockResolution );

	// Max inclusive time in seconds
	Stream.PrintF( "\t%f", (double)Entry.m_MaxInclusiveTime * ClockResolution );

	// Number of calls
	Stream.PrintF( "\t%d", Entry.m_NumCalls );

	// Total inclusive time in cycles
	Stream.PrintF( "\t%I64d", Entry.m_TotalInclusiveTime );

	// Total inclusive time in seconds
	Stream.PrintF( "\t%f", (double)Entry.m_TotalInclusiveTime * ClockResolution );

	// Average inclusive time in cycles
	Stream.PrintF( "\t%f", (double)Entry.m_TotalInclusiveTime / (double)Entry.m_NumCalls );

	// Max inclusive time in cycles
	Stream.PrintF( "\t%f", (double)Entry.m_MaxInclusiveTime );

	// Total exclusive time in cycles
	Stream.PrintF( "\t%I64d", Entry.m_TotalExclusiveTime );

	// Total exclusive time in seconds
	Stream.PrintF( "\t%f", (double)Entry.m_TotalExclusiveTime * ClockResolution );

	// Average exclusive time in cycles
	Stream.PrintF( "\t%f", (double)Entry.m_TotalExclusiveTime / (double)Entry.m_NumCalls );

	// Max exclusive time in cycles
	Stream.PrintF( "\t%f", (double)Entry.m_MaxExclusiveTime );

	Stream.PrintF( "\n" );
}

void ProfilerTable::DumpPlainEntries( const IDataStream& Stream )
{
	// Go ahead and deep copy entries just in case the map changes while dumping
	// (and because ArraySortFunc doesn't like pointers, sigh).
	Array<ProfilerTableEntry> PlainEntries;

	// Find children
	FOR_EACH_MAP( EntryIter, m_Entries, HashedString, ProfilerTableEntry )
	{
		const ProfilerTableEntry& Entry = EntryIter.GetValue();
		if( Entry.m_CallstackDepth == 0 )
		{
			PlainEntries.PushBack( Entry );
		}
	}

	// Sort by total inclusive time
	PlainEntries.InsertionSort(
		[]( const ProfilerTableEntry& EntryA, const ProfilerTableEntry& EntryB )
		{
			return ( (double)EntryA.m_TotalExclusiveTime / (double)EntryA.m_NumCalls ) > ( (double)EntryB.m_TotalExclusiveTime / (double)EntryB.m_NumCalls );
		}
	);

	// Write header
	Stream.PrintF( "FuncSig\tAvgExcTimeSec\tAvgIncTimeSec\tMaxExcTimeSec\tMaxIncTimeSec\tNumCalls\tTotIncTimeCyc\tTotIncTimeSec\tAvgIncTimeCyc\tMaxIncTimeCyc\tTotExcTimeCyc\tTotExcTimeSec\tAvgExcTimeCyc\tMaxExcTimeCyc\n" );

	FOR_EACH_ARRAY( PlainEntryIter, PlainEntries, ProfilerTableEntry )
	{
		const ProfilerTableEntry& PlainEntry = PlainEntryIter.GetValue();
		DumpPlainEntry( Stream, PlainEntry );
	}
}

void ProfilerTable::Dump( const IDataStream& Stream )
{
	// Write the classic format
	DumpPlainEntries( Stream );

	// Write tree starting at root(s)
	DumpStack( Stream, false /*Minimal*/ );
}

void ProfilerTable::DumpStack( const IDataStream& Stream, const bool Minimal )
{
	// Write tree starting at root(s)
	DumpStackEntries( Stream, HashedString::NullString, Minimal );
}

void ProfilerTable::Flush()
{
	m_Entries.Clear();
}

const ProfilerMap& ProfilerTable::GetEntries()
{
	return m_Entries;
}

#endif // DO_PROFILING
