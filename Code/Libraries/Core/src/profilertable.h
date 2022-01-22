#ifndef PROFILERTABLE_H
#define PROFILERTABLE_H

#if DO_PROFILING

#include "map.h"
#include "arraystack.h"
#include "hashedstring.h"

class IDataStream;

typedef Map< class HashedString, class ProfilerTableEntry > ProfilerMap;

class ProfilerTableEntry
{
public:
	ProfilerTableEntry();
	~ProfilerTableEntry();

	const char*		m_Name;
	__int64			m_TotalInclusiveTime;
	__int64			m_TotalExclusiveTime;
	__int64			m_MaxInclusiveTime;
	__int64			m_MaxExclusiveTime;
	int				m_NumCalls;
	int				m_CallstackDepth;
	HashedString	m_CallstackHash;
	HashedString	m_CallstackParentHash;
};

class ProfilerTable
{
public:
	ProfilerTable();
	~ProfilerTable();

	void				AddEntries( const char* Name, const HashedString& HashedName, const HashedString& HashStackName, __int64 InclusiveTime, __int64 ExclusiveTime );

	void				Dump( const IDataStream& Stream );
	void				DumpStack( const IDataStream& Stream, const bool Minimal );
	void				Flush();

	const ProfilerMap&	GetEntries();

private:
	ProfilerMap			m_Entries;

	void				DumpPlainEntries( const IDataStream& Stream );
	void				DumpPlainEntry( const IDataStream& Stream, const ProfilerTableEntry& Entry );

	void				DumpStackEntries( const IDataStream& Stream, const HashedString& ParentHash, const bool Minimal );
	void				DumpStackEntry( const IDataStream& Stream, const ProfilerTableEntry& Entry, const bool Minimal );

public:	// Public for ease of access in ProfilerHook ctor/dtor
	ArrayStack< __int64 >		m_Overhead;		// Subtracted time due to profiler overhead
	ArrayStack< __int64 >		m_Inclusive;	// Subtracted time due to inclusive profiled time

	// DLP 30 Apr 2019: Adding a hierarchical display
	ArrayStack<HashedString>	m_HashStack;	// Function hashes up the callstack, mixed together
};

#endif // DO_PROFILING

#endif // PROFILERTABLE_H
