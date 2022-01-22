#include "core.h"
#include "rodinbtnodeparallel.h"
#include "simplestring.h"
#include "configmanager.h"
#include "Components/wbcomprodinbehaviortree.h"
#include "idatastream.h"
#include "mathcore.h"

RodinBTNodeParallel::RodinBTNodeParallel()
:	m_ChildStatuses()
,	m_NumChildrenToSucceed( 0 )
,	m_NumChildrenToFail( 0 )
{
}

RodinBTNodeParallel::~RodinBTNodeParallel()
{
}

void RodinBTNodeParallel::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	RodinBTNodeComposite::InitializeFromDefinition( DefinitionName );

	for( uint ChildIndex = 0; ChildIndex < m_Children.Size(); ++ChildIndex )
	{
		m_ChildStatuses.PushBack( ETS_None );
	}

	STATICHASH( NumChildrenToSucceed );
	STATICHASH( NumChildrenToFail );
	MAKEHASH( DefinitionName );

	// By default, we need all children to succeed, but only one to fail
	m_NumChildrenToSucceed	= ConfigManager::GetInt( sNumChildrenToSucceed, m_Children.Size(), sDefinitionName );
	m_NumChildrenToFail		= ConfigManager::GetInt( sNumChildrenToFail, 1, sDefinitionName );

	// Use negative numbers to indicate number relative to children size (-1 means size, -2 means size-1, etc.)
	if( m_NumChildrenToSucceed < 0 )
	{
		m_NumChildrenToSucceed = ( m_Children.Size() + 1 ) + m_NumChildrenToSucceed;
	}

	if( m_NumChildrenToFail < 0 )
	{
		m_NumChildrenToFail = ( m_Children.Size() + 1 ) + m_NumChildrenToFail;
	}
}

RodinBTNode::ETickStatus RodinBTNodeParallel::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	ETickStatus RetVal			= ETS_Running;

	int NumChildrenSucceeded	= 0;
	int NumChildrenFailed		= 0;

	for( uint ChildIndex = 0; ChildIndex < m_Children.Size(); ++ChildIndex )
	{
		ETickStatus& ChildStatus = m_ChildStatuses[ ChildIndex ];
		if( ChildStatus == ETS_None )
		{
			// Start child running (first time Tick is called)
			m_BehaviorTree->Start( m_Children[ ChildIndex ], this );
			ChildStatus = ETS_Running;
		}
		else if( ChildStatus == ETS_Fail )
		{
			++NumChildrenFailed;
		}
		else if( ChildStatus == ETS_Success )
		{
			++NumChildrenSucceeded;
		}
	}

	// Give failing higher priority, in keeping with the old system of immediately
	// returning if one fail was encountered. Not a huge difference, really.
	if( m_NumChildrenToFail > 0 && NumChildrenFailed >= m_NumChildrenToFail )
	{
		RetVal = ETS_Fail;
	}
	else if( m_NumChildrenToSucceed > 0 && NumChildrenSucceeded >= m_NumChildrenToSucceed )
	{
		RetVal = ETS_Success;
	}

	// Until enough of our children have finished, go back to sleep
	if( RetVal == ETS_Running )
	{
		m_BehaviorTree->Sleep( this );
	}

	return RetVal;
}

void RodinBTNodeParallel::OnStart()
{
	RodinBTNodeComposite::OnStart();

	ASSERT( m_Children.Size() == m_ChildStatuses.Size() );
	for( uint ChildIndex = 0; ChildIndex < m_Children.Size(); ++ChildIndex )
	{
		m_ChildStatuses[ ChildIndex ] = ETS_None;
	}
}

void RodinBTNodeParallel::OnFinish()
{
	RodinBTNodeComposite::OnFinish();

	for( uint ChildIndex = 0; ChildIndex < m_Children.Size(); ++ChildIndex )
	{
		if( m_ChildStatuses[ ChildIndex ] == ETS_Running )
		{
			m_BehaviorTree->Stop( m_Children[ ChildIndex ] );
		}
	}
}

void RodinBTNodeParallel::OnChildCompleted( RodinBTNode* pChildNode, ETickStatus TickStatus )
{
	RodinBTNodeComposite::OnChildCompleted( pChildNode, TickStatus );

	// Keep track of completed status of subtasks
	for( uint ChildIndex = 0; ChildIndex < m_Children.Size(); ++ChildIndex )
	{
		if( m_Children[ ChildIndex ] == pChildNode )
		{
			m_ChildStatuses[ ChildIndex ] = TickStatus;
			break;
		}
	}

	// Only wake if we were sleeping, because multiple things might finish simultaneously and
	// the scheduler doesn't like trying to wake a woken node. When this wakes, it will go back
	// to sleep on the next tick if it's still running.
	if( m_IsSleeping )
	{
		m_BehaviorTree->Wake( this );
	}
}

#if BUILD_DEV
void RodinBTNodeParallel::Report()
{
	RodinBTNode::Report();

	for( uint ChildIndex = 0; ChildIndex < m_Children.Size(); ++ChildIndex )
	{
		m_Children[ ChildIndex ]->Report();
	}
}
#endif
