#include "core.h"

#if DO_PROFILING

#include "clock.h"
#include <string.h>

#define GET_PROFILER	Profiler* const	pProfiler	= Profiler::GetInstance()
#define GET_TABLE		ProfilerTable&	TheTable	= ( m_Permanent ? pProfiler->m_Table : pProfiler->m_FrameTable )
#define UPDATE_TIME		pProfiler->m_Clock->Tick()
#define GET_TIME		pProfiler->m_Clock->GetPhysicalCurrentTime()
#define OVERHEAD_STACK	TheTable.m_Overhead
#define INCLUSIVE_STACK	TheTable.m_Inclusive
#define HASH_STACK		TheTable.m_HashStack
#define ADD_ENTRIES		TheTable.AddEntries

ProfilerHook::ProfilerHook( const char* Name, const HashedString& HashedName, bool Permanent )
:	m_Name( Name )
,	m_HashedName( HashedName )
,	m_HashStackName( HashedString::NullString )
,	m_SetupTime( 0 )
,	m_RunningStartTime( 0 )
,	m_RunningTime( 0 )
,	m_Permanent( Permanent )
{
	GET_PROFILER;
	GET_TABLE;

	// Get pre-construction time
	UPDATE_TIME;
	__int64	StartTime = GET_TIME;

	// Push a new time onto the stacks
	OVERHEAD_STACK.Push( 0 );
	INCLUSIVE_STACK.Push( 0 );

	// Negate our actual hash and XOR with the stack; this makes a unique entry
	// based on callstack, which will not conflict with the plain entry
	m_HashStackName = ( HASH_STACK.Top().GetHash() << 1 ) ^ ~m_HashedName.GetHash();
	HASH_STACK.Push( m_HashStackName );

	// Get post-construction time
	UPDATE_TIME;
	m_SetupTime = GET_TIME - StartTime;
	m_RunningStartTime = GET_TIME;
}

ProfilerHook::~ProfilerHook()
{
	GET_PROFILER;
	GET_TABLE;

	// Get pre-destruction time
	UPDATE_TIME;
	__int64 StartTime = GET_TIME;
	m_RunningTime = GET_TIME - m_RunningStartTime;

	// Pop the overhead time off the stack and subtract from this hook's running time
	m_RunningTime -= OVERHEAD_STACK.Top();
	OVERHEAD_STACK.Pop();

	// Pop the inclusive time off the stack
	__int64 InclusiveTime = INCLUSIVE_STACK.Top();
	INCLUSIVE_STACK.Pop();

	// Pop our unique hash off the stack
	HASH_STACK.Pop();

	// Add this hook's running time to the stack
	INCLUSIVE_STACK.Top() += m_RunningTime;

	ADD_ENTRIES( m_Name, m_HashedName, m_HashStackName, m_RunningTime, m_RunningTime - InclusiveTime );

	// Get post-destruction time
	UPDATE_TIME;
	m_SetupTime += GET_TIME - StartTime;

	// Add this hook's setup time to the stack
	OVERHEAD_STACK.Top() += m_SetupTime;
}

#endif // DO_PROFILING
