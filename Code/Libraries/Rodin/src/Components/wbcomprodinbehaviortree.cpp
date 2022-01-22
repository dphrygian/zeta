#include "core.h"
#include "wbcomprodinbehaviortree.h"
#include "configmanager.h"
#include "rodinbtnodefactory.h"
#include "idatastream.h"
#include "wbeventmanager.h"

// If defined, Report will also let each node report itself,
// instead of merely listing the nodes in the tree.
#define REPORT_FULL_TREE 0

WBCompRodinBehaviorTree::WBCompRodinBehaviorTree()
:	m_RootNode( NULL )
,	m_ScheduledNodes()
,	m_TickIterateNodeIndex( 0 )
,	m_Paused( false )
,	m_LookupNodeMap()
#if BUILD_DEV
,	m_Verbose( false )
,	m_BreakNodes()
#endif
{
	m_ScheduledNodes.SetDeflate( false );
}

WBCompRodinBehaviorTree::~WBCompRodinBehaviorTree()
{
	SafeDelete( m_RootNode );
}

void WBCompRodinBehaviorTree::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

#if BUILD_DEV
	STATICHASH( Verbose );
	m_Verbose = ConfigManager::GetInheritedBool( sVerbose, false, sDefinitionName );

	STATICHASH( NumBreakNodes );
	const uint NumBreakNodes = ConfigManager::GetInheritedInt( sNumBreakNodes, 0, sDefinitionName );
	for( uint BreakNodeIndex = 0; BreakNodeIndex < NumBreakNodes; ++BreakNodeIndex )
	{
		const SimpleString BreakNode = ConfigManager::GetInheritedSequenceString( "BreakNode%d", BreakNodeIndex, "", sDefinitionName );
		m_BreakNodes.PushBack( BreakNode );
	}
#endif

	// Optionally add lookup nodes from other sets (or other BT components, ignoring their other properties)
	STATICHASH( NumLookupSets );
	const uint NumLookupSets = ConfigManager::GetInheritedInt( sNumLookupSets, 0, sDefinitionName );
	FOR_EACH_INDEX( LookupSetIndex, NumLookupSets )
	{
		const SimpleString LookupNodeSet = ConfigManager::GetInheritedSequenceString( "LookupSet%d", LookupSetIndex, "", sDefinitionName );
		AddLookupNodeSet( LookupNodeSet );
	}

	// Add local lookup node map last so it overrides anything else
	AddLookupNodeSet( DefinitionName );

	// Recursively construct the entire tree
	STATICHASH( RootNode );
	m_RootNode = RodinBTNodeFactory::Create( ConfigManager::GetInheritedString( sRootNode, "", sDefinitionName ), this );
	DEVASSERT( m_RootNode );
}

void WBCompRodinBehaviorTree::AddLookupNodeSet( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumLookupNodes );
	const uint NumLookupNodes = ConfigManager::GetInheritedInt( sNumLookupNodes, 0, sDefinitionName );
	FOR_EACH_INDEX( LookupNodeIndex, NumLookupNodes )
	{
		const HashedString Key		= ConfigManager::GetInheritedSequenceHash(		"LookupNode%dKey",		LookupNodeIndex, HashedString::NullString,	sDefinitionName );
		const SimpleString Value	= ConfigManager::GetInheritedSequenceString(	"LookupNode%dValue",	LookupNodeIndex, "",						sDefinitionName );

		m_LookupNodeMap.Insert( Key, Value );
	}
}

SimpleString WBCompRodinBehaviorTree::GetLookupNode( const HashedString& Key )
{
	Map<HashedString, SimpleString>::Iterator LookupNodeIter = m_LookupNodeMap.Search( Key );
	return LookupNodeIter.IsValid() ? LookupNodeIter.GetValue() : "";
}

/*virtual*/ void WBCompRodinBehaviorTree::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( PauseBT );
	STATIC_HASHED_STRING( UnpauseBT );
	STATIC_HASHED_STRING( FlushBT );
	STATIC_HASHED_STRING( OnDestroyed );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sPauseBT )
	{
		m_Paused = true;

		// Also flush the scheduler so that running actions clean up.
		Flush();
	}
	else if( EventName == sUnpauseBT )
	{
		m_Paused = false;
	}
	else if( EventName == sFlushBT )
	{
		Flush();
	}
	else if( EventName == sOnDestroyed )
	{
		// Do the same stuff as pausing when we get destroyed.
		// Force BTs to exit cleanly in all cases.
		m_Paused = true;
		Flush();
	}
}

/*virtual*/ void WBCompRodinBehaviorTree::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	if( m_Paused )
	{
		return;
	}

	// (Re)start tree if scheduler is empty
	if( m_ScheduledNodes.Empty() )
	{
		if( m_RootNode )
		{
			Start( m_RootNode );
		}
	}

	// Tick all the scheduled nodes
	for( m_TickIterateNodeIndex = 0; m_TickIterateNodeIndex < m_ScheduledNodes.Size(); ++m_TickIterateNodeIndex )
	{
		SScheduledNode& ScheduledNode = m_ScheduledNodes[ m_TickIterateNodeIndex ];
		ASSERT( ScheduledNode.m_Node );

		if( ScheduledNode.m_Node->m_IsSleeping )
		{
			continue;
		}

		RodinBTNode::ETickStatus TickStatus = ScheduledNode.m_Node->Tick( DeltaTime );
		if( TickStatus != RodinBTNode::ETS_Running )
		{
			Finish( m_TickIterateNodeIndex, TickStatus );
		}
	}
}

void WBCompRodinBehaviorTree::Start( RodinBTNode* pNode, RodinBTNode* pParentNode /*= NULL*/ )
{
	ASSERT( pNode );
	if( !pNode )
	{
		return;
	}

	ASSERT( !pNode->m_IsSleeping );

#if BUILD_DEBUG
	// TODO: Make sure this node isn't already in the scheduled array.
	// While it is valid for two nodes to have a shared child node, it is never valid for them
	// both to be running that child simultaneously (because the node only has one state).
	// 12 Apr 2013: Actually, I don't think that *is* a valid use case; two child nodes can
	// share the same config definition, but they'll be uniquely constructed for each parent.)
	uint NodeIndex = 0;
	if( FindScheduledNode( pNode, NodeIndex ) )
	{
		WARNDESC( "WBCompRodinBehaviorTree::Start: Node pair is already scheduled!" );
	}
#endif

#if BUILD_DEV
	pNode->m_DEV_Depth = pParentNode ? ( pParentNode->m_DEV_Depth + 1 ) : 0;

	if( m_Verbose )
	{
		PRINTF( "BT: Start %s on %s\n", pNode->GetDebugName().CStr(), GetEntity()->GetUniqueName().CStr() );
	}

	if( m_BreakNodes.Search( pNode->GetDebugName() ).IsValid() )
	{
		BREAKPOINT;
	}
#endif

	SScheduledNode ScheduledNode;
	ScheduledNode.m_Node = pNode;
	ScheduledNode.m_ParentNode = pParentNode;

	m_ScheduledNodes.PushBack( ScheduledNode );

	pNode->OnStart();
}

void WBCompRodinBehaviorTree::Stop( RodinBTNode* pNode )
{
	ASSERT( pNode );

	uint NodeIndex = 0;
	if( FindScheduledNode( pNode, NodeIndex ) )
	{
		Finish( NodeIndex, RodinBTNode::ETS_Fail );
	}
	else
	{
		WARNDESC( "Given task was not found in the scheduler." );
	}
}

void WBCompRodinBehaviorTree::Wake( RodinBTNode* pNode )
{
	ASSERT( pNode );
	ASSERT( pNode->m_IsSleeping );

	pNode->m_IsSleeping = false;

	// Move the task to the end of the list
	uint NodeIndex = 0;
	if( FindScheduledNode( pNode, NodeIndex ) )
	{
		// Make a copy, because if we expand the array, a reference into the array won't be valid anymore.
		SScheduledNode WokenNode = m_ScheduledNodes[ NodeIndex ];
		m_ScheduledNodes.PushBack( WokenNode );
		m_ScheduledNodes.Remove( NodeIndex );

		if( NodeIndex <= m_TickIterateNodeIndex )
		{
			--m_TickIterateNodeIndex;
		}
	}
}

void WBCompRodinBehaviorTree::Sleep( RodinBTNode* pNode )
{
	ASSERT( pNode );
	ASSERT( !pNode->m_IsSleeping );

	pNode->m_IsSleeping = true;
}

void WBCompRodinBehaviorTree::Flush()
{
#if BUILD_DEV
	if( m_Verbose )
	{
		PRINTF( "BT: Flush %s\n", GetEntity()->GetUniqueName().CStr() );
	}
#endif

	if( m_ScheduledNodes.Size() )
	{
		Stop( m_RootNode );
	}
}

void WBCompRodinBehaviorTree::Finish( uint NodeIndex, RodinBTNode::ETickStatus FinishStatus )
{
	// Make a copy, because if we expand the array, a reference into the array won't be valid anymore.
	SScheduledNode ScheduledNode = m_ScheduledNodes[ NodeIndex ];

	// Remove the node first, because we don't know what side effects
	// OnFinish and OnChildCompleted might have on the scheduled array.
	m_ScheduledNodes.Remove( NodeIndex );

	if( NodeIndex <= m_TickIterateNodeIndex )
	{
		--m_TickIterateNodeIndex;
	}

#if BUILD_DEV
	if( m_Verbose )
	{
		const SimpleString Status = ( FinishStatus == RodinBTNode::ETS_Success ) ? "Success" : "Fail";
		PRINTF( "BT: Finish (%s) %s on %s\n", Status.CStr(), ScheduledNode.m_Node->GetDebugName().CStr(), GetEntity()->GetUniqueName().CStr() );
	}
#endif

	ScheduledNode.m_Node->OnFinish();

	// Just make sure it cleans up after itself properly.
	ASSERT( !ScheduledNode.m_Node->m_IsSleeping );

	if( ScheduledNode.m_ParentNode )
	{
		ScheduledNode.m_ParentNode->OnChildCompleted( ScheduledNode.m_Node, FinishStatus );
	}
}

bool WBCompRodinBehaviorTree::FindScheduledNode( RodinBTNode* pNode, uint& OutIndex )
{
	for( uint NodeIndex = 0; NodeIndex < m_ScheduledNodes.Size(); ++NodeIndex )
	{
		if( m_ScheduledNodes[ NodeIndex ].m_Node == pNode )
		{
			OutIndex = NodeIndex;
			return true;
		}
	}

	return false;
}

RodinBTNode* WBCompRodinBehaviorTree::GetParentNode( RodinBTNode* const pNode ) const
{
	for( uint NodeIndex = 0; NodeIndex < m_ScheduledNodes.Size(); ++NodeIndex )
	{
		const SScheduledNode& ScheduledNode = m_ScheduledNodes[ NodeIndex ];
		if( ScheduledNode.m_Node == pNode )
		{
			return ScheduledNode.m_ParentNode;
		}
	}

	return NULL;
}

#if BUILD_DEV
void WBCompRodinBehaviorTree::Report() const
{
	Super::Report();

#if REPORT_FULL_TREE
	PRINTF( WBPROPERTY_REPORT_PREFIX "Tree:\n" );
	if( m_RootNode )
	{
		m_RootNode->Report( 0 );
	}
#endif

	if( m_ScheduledNodes.Size() > 0 )
	{
		PRINTF( WBPROPERTY_REPORT_PREFIX "Scheduled nodes:\n" );
		for( uint NodeIndex = 0; NodeIndex < m_ScheduledNodes.Size(); ++NodeIndex )
		{
			PRINTF( WBPROPERTY_REPORT_PREFIX WB_REPORT_SPACER "%s\n", m_ScheduledNodes[ NodeIndex ].m_Node->GetDebugName().CStr() );
		}
	}
}
#endif

#define VERSION_EMPTY			0
#define VERSION_PAUSED			1
#define VERSION_NODESTATES		2
#define VERSION_SCHEDULEDNODES	3
#define VERSION_CURRENT			3

uint WBCompRodinBehaviorTree::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_Paused

	return Size;
}

void WBCompRodinBehaviorTree::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.WriteBool( m_Paused );
}

void WBCompRodinBehaviorTree::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_PAUSED )
	{
		m_Paused = Stream.ReadBool();
	}
}
