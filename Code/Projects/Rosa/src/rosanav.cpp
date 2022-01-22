#include "core.h"
#include "mathcore.h" // (for Min() in array.h)
#include "rosanav.h"
#include "rosaframework.h"
#include "wbcomponentarrays.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcomprosadoor.h"
#include "aabb.h"
#include "mathcore.h"
#include "segment.h"
#include "collisioninfo.h"
#include "mathfunc.h"
#include "wbeventmanager.h"

RosaNav::RosaNav()
:	m_World( NULL )
,	m_PathData( NULL )
{
	m_World = RosaFramework::GetInstance()->GetWorld();
	DEVASSERT( m_World );
}

RosaNav::~RosaNav()
{
}

/*static*/ RosaNav* RosaNav::GetInstance()
{
	RosaWorld* const pWorld = RosaFramework::GetInstance()->GetWorld();
	DEVASSERT( pWorld );
	DEVASSERT( pWorld->m_Nav );

	return pWorld->m_Nav;
}

RosaNav::EPathStatus RosaNav::FindPath( SPathData& PathData )
{
	m_PathData = &PathData;
	return FindPath();
}

RosaNav::EPathStatus RosaNav::FindPath()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	DEVASSERT( m_World );
	DEVASSERT( m_PathData );
	DEVASSERT( m_PathData->m_Params.m_PathMode != EPM_None );
	DEVASSERT( m_PathData->m_Params.m_MotionType != EMT_None );

	if( !m_World->FindNavNodeUnder( m_PathData->m_Params.m_Start, m_PathData->m_Params.m_StartNodeIndex ) )
	{
		// Couldn't find a node to start from.
		if( m_PathData->m_Params.m_Verbose ) { PRINTF( "No path found: No nav node under start location.\n" ); }
		return EPS_NoPathFound;
	}

	if( IsSearch() || IsFlee() )
	{
		if( !m_World->FindNavNodeUnder( m_PathData->m_Params.m_Destination, m_PathData->m_Params.m_DestNodeIndex ) )
		{
			// Couldn't find a destination/flee node.
			if( m_PathData->m_Params.m_Verbose ) { PRINTF( "No path found: No nav node under destination location.\n" ); }
			return EPS_NoPathFound;
		}

		if( !IsAccessible( m_PathData->m_Params.m_DestNodeIndex ) )
		{
			if( m_PathData->m_Params.m_Verbose ) { PRINTF( "No path found: Destination location was inaccessible.\n" ); }
			return EPS_NoPathFound;
		}
	}

	m_PathData->m_Path.SetDeflate( false );
	m_PathData->m_Path.Clear();

	if( IsSearch() )
	{
		if( m_PathData->m_Params.m_StartNodeIndex == m_PathData->m_Params.m_DestNodeIndex )
		{
			if( m_PathData->m_Params.m_Verbose ) { PRINTF( "Path found: Already at destination.\n" ); }
			m_PathData->m_Path.PushBack( m_PathData->m_Params.m_Start );
			m_PathData->m_Path.PushBack( m_PathData->m_Params.m_Destination );
			return EPS_PathFound;
		}
	}

	m_PathData->m_OpenSet.SetDeflate( false );
	m_PathData->m_OpenSet.Clear();
	m_PathData->m_ClosedSet.SetDeflate( false );
	m_PathData->m_ClosedSet.Clear();
	m_PathData->m_EdgePath.SetDeflate( false );
	m_PathData->m_EdgePath.Clear();

	// Seed the open set with the starting position
	SPathNode& StartNode		= m_PathData->m_OpenSet.PushBack();
	StartNode.m_G				= 0.0f;
	StartNode.m_H				= GetHeuristicCost( m_PathData->m_Params.m_StartNodeIndex, m_PathData->m_Params.m_Start, m_PathData->m_Params.m_Destination );
	StartNode.m_NavNodeIndex	= m_PathData->m_Params.m_StartNodeIndex;
	StartNode.m_Backlink		= NAV_NULL;
	StartNode.m_Step			= m_PathData->m_Params.m_Start;

	return DoAStarSearch();
}

void RosaNav::RecursiveBuildPath( const uint StepNodeIndex, const uint NumSteps )
{
	// Ugh, this is so slow. Change open/closed sets to map from nav node index?
	uint ClosedNodeIndex = INVALID_ARRAY_INDEX;
	const bool FoundNode = FindNavNodeInSet( StepNodeIndex, m_PathData->m_ClosedSet, &ClosedNodeIndex );
	ASSERT( FoundNode );
	Unused( FoundNode );
	const SPathNode& PathNode = m_PathData->m_ClosedSet[ ClosedNodeIndex ];

	if( PathNode.m_Backlink == NAV_NULL )
	{
		// Recursion terminating base case; this should be our start node. Allocate the array once.
		m_PathData->m_Path.Reserve( NumSteps + 1 ); // Add 1 for the end nodes
	}
	else
	{
		RecursiveBuildPath( PathNode.m_Backlink, NumSteps + 1 );
	}

	m_PathData->m_Path.PushBack( PathNode.m_Step );
	m_PathData->m_EdgePath.PushBack( PathNode.m_NavEdgeIndex );
}

void RosaNav::BuildPath( const uint FinalNodeIndex )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	DEVASSERT( m_PathData );
	DEVASSERT( m_PathData->m_Path.Empty() );

	RecursiveBuildPath( FinalNodeIndex, 1 );

	// For a search, add the actual destination vector if the path has reached the destination node
	if( IsSearch() )
	{
		if( FinalNodeIndex == m_PathData->m_Params.m_DestNodeIndex )
		{
			m_PathData->m_Path.PushBack( m_PathData->m_Params.m_Destination );
			m_PathData->m_EdgePath.PushBack( NAV_NULL );
		}
	}

	if( m_PathData->m_Params.m_Verbose )
	{
		PRINTF( "Presmoothed path size: %d\n", m_PathData->m_Path.Size() );
		FOR_EACH_ARRAY( PathIter, m_PathData->m_Path, Vector )
		{
			const uint		PathIndex	= PathIter.GetIndex();
			const Vector&	PathNode	= PathIter.GetValue();
			PRINTF( "Node %d: %s\n", PathIndex, PathNode.GetString().CStr() );
		}
	}

	SmoothPath();

	if( m_PathData->m_Path.Size() < 2 )
	{
		// If we end up with a "partial path" that's just the start position, don't crash
		return;
	}

	if( IsWander() || IsFlee() )
	{
		TrimPathFromStart();
	}
	else
	{
		TrimPathFromDestination();
	}
}

void RosaNav::SmoothPath()
{
	DEBUGASSERT( m_PathData->m_Path.Size() == m_PathData->m_EdgePath.Size() );

	for( uint PathIndex = 1; PathIndex < m_PathData->m_Path.Size() - 1; ++PathIndex )
	{
		const Vector&	StepA		= m_PathData->m_Path[ PathIndex - 1 ];
		Vector			StepB		= m_PathData->m_Params.m_Destination;
		uint			FirstAnchor	= m_PathData->m_Path.Size() - 1;
		for( uint EndIndex = m_PathData->m_EdgePath.Size() - 1; EndIndex >= PathIndex; --EndIndex )
		{
			const int EdgeIndex = m_PathData->m_EdgePath[ EndIndex ];

			if( EdgeIndex == NAV_NULL )
			{
				// Ignore, this is the start or end and can't be smoothed
				continue;
			}

			const SNavEdge& Edge = m_World->GetNavEdge( EdgeIndex );
			DEBUGASSERT( Edge.m_Width >= m_PathData->m_Params.m_AgentRadius * 2.0f );
			const Vector	EdgeDirection	= ( Edge.m_VertB - Edge.m_VertA ).GetNormalized();
			const Vector	RadiusOffset	= EdgeDirection * m_PathData->m_Params.m_AgentRadius;
			const Vector	VertA			= Edge.m_VertA + RadiusOffset;
			const Vector	VertB			= Edge.m_VertB - RadiusOffset;
			const Segment	EdgeSegment		= Segment( VertA, VertB );
			const Segment	EdgeSegment2D	= Segment( VertA.Get2D(), VertB.Get2D() );
			const Segment	Travel2D		= Segment( StepA.Get2D(), StepB.Get2D() );
			float			NearestT		= 0.0f;
			EdgeSegment2D.NearestPointTo( Travel2D, &NearestT );
			StepB							= EdgeSegment.GetPointAt( NearestT );

			m_PathData->m_Path[ PathIndex ] = StepB;

			if( NearestT == 0.0f || NearestT == 1.0f )
			{
				FirstAnchor = PathIndex;
			}
		}

		// Remove anything between current node and the next anchor
		for( uint RemovePathIndex = FirstAnchor - 1; RemovePathIndex >= PathIndex; --RemovePathIndex )
		{
			m_PathData->m_Path.Remove( RemovePathIndex );
		}
	}

	if( m_PathData->m_Params.m_Verbose )
	{
		PRINTF( "Postsmoothed path size: %d\n", m_PathData->m_Path.Size() );
		FOR_EACH_ARRAY( PathIter, m_PathData->m_Path, Vector )
		{
			const uint		PathIndex	= PathIter.GetIndex();
			const Vector&	PathNode	= PathIter.GetValue();
			PRINTF( "Node %d: %s\n", PathIndex, PathNode.GetString().CStr() );
		}
	}
}

// Trim the smoothed path to the desired wander/flee distance if it's too long
void RosaNav::TrimPathFromStart()
{
	DEVASSERT( IsWander() || IsFlee() );
	DEVASSERT( m_PathData->m_Path.Size() >= 2 );

	float Distance = 0.0f;
	for( uint PathIndex = 1; PathIndex < m_PathData->m_Path.Size(); ++PathIndex )
	{
		const Vector&	SegmentStart	= m_PathData->m_Path[ PathIndex - 1 ];
		const Vector&	SegmentEnd		= m_PathData->m_Path[ PathIndex ];
		const Vector	Travel			= SegmentEnd - SegmentStart;
		const float		TravelLength	= Travel.Length();

		Distance += TravelLength;

		if( Distance > m_PathData->m_Params.m_TargetDistance )
		{
			// Trim this segment and drop remaining nodes
			const float		Difference		= Distance - m_PathData->m_Params.m_TargetDistance;
			const float		TrimmedLength	= TravelLength - Difference;
			const Vector	TravelDirection	= Travel.GetNormalized();
			m_PathData->m_Path[ PathIndex ]	= SegmentStart + ( TravelDirection * TrimmedLength );
			m_PathData->m_Path.Resize( PathIndex + 1 );
			break;
		}
	}
}

// Trim the path to the desired approach distance
void RosaNav::TrimPathFromDestination()
{
	DEVASSERT( IsSearch() );
	DEVASSERT( m_PathData->m_Path.Size() >= 2 );

	if( m_PathData->m_Params.m_TargetDistance <= 0.0f )
	{
		// Don't trim the path if we haven't set an approach distance
		return;
	}

	// HACKHACK: Only attempt to trim the last segment of the path;
	// this is not a guarantee of correct behavior (e.g., if pathing
	// around a pit when firing across it might be acceptable), but
	// it avoids the case of stopping the path when the destination
	// is occluded.
	const uint		PathIndex		= m_PathData->m_Path.Size() - 1;
	const Vector&	SegmentStart	= m_PathData->m_Path[ PathIndex - 1 ];
	const Vector&	SegmentEnd		= m_PathData->m_Path[ PathIndex ];
	const Vector	Travel			= SegmentEnd - SegmentStart;
	const float		TravelLength	= Travel.Length();

	if( TravelLength > m_PathData->m_Params.m_TargetDistance )
	{
		// Trim the last segment, no need to drop any nodes here
		const float		Difference		= TravelLength - m_PathData->m_Params.m_TargetDistance;
		const Vector	TravelDirection	= Travel.GetNormalized();
		m_PathData->m_Path[ PathIndex ]	= SegmentStart + ( TravelDirection * Difference );
	}
	else if( TravelLength <= m_PathData->m_Params.m_TargetDistance && m_PathData->m_Path.Size() == 2 )
	{
		// The whole path is within the approach distance, drop the whole thing
		m_PathData->m_Path[ 1 ] = m_PathData->m_Path[ 0 ];
		m_PathData->m_Path.Resize( 2 );
	}
}

RosaNav::EPathStatus RosaNav::DoAStarSearch()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	DEVASSERT( m_PathData->m_OpenSet.Size() );

	uint Step = 0;
	while( m_PathData->m_OpenSet.Size() && Step++ < m_PathData->m_Params.m_MaxSteps )	// ROSATODO: Change MaxSteps to a timeslicing cutoff
	{
		const uint			MinOpenSetIndex	= GetMinOpenSetIndex();
		const SPathNode&	OpenNode		= m_PathData->m_OpenSet[ MinOpenSetIndex ];
		const SPathNode&	ClosedNode		= m_PathData->m_ClosedSet.PushBack( OpenNode );
		m_PathData->m_OpenSet.FastRemove( MinOpenSetIndex );

		if( ( IsSearch() && ClosedNode.m_NavNodeIndex == m_PathData->m_Params.m_DestNodeIndex ) ||
			( IsWander() && ClosedNode.m_G >= m_PathData->m_Params.m_TargetDistance ) ||
			( IsFlee() && ClosedNode.m_H <= 0.0f ) )
		{
			// We've found the destination. Build the path and return.
			BuildPath( ClosedNode.m_NavNodeIndex );
			return EPS_PathFound;
		}
		else
		{
			const SNavNode& NavNode = m_World->GetNavNode( ClosedNode.m_NavNodeIndex );
			TryExpand( ClosedNode, NavNode.m_EdgeA );
			TryExpand( ClosedNode, NavNode.m_EdgeB );
			TryExpand( ClosedNode, NavNode.m_EdgeC );
		}
	}

#if BUILD_DEV
	if( Step >= m_PathData->m_Params.m_MaxSteps )
	{
		PRINTF( "Pathfinding ran out of steps! (Max %d)\n", m_PathData->m_Params.m_MaxSteps );
		PRINTF( "Entity: %s\n", m_PathData->m_Params.m_Entity ? m_PathData->m_Params.m_Entity->GetUniqueName().CStr() : "NULL" );
		PRINTF( "Start:  %s\n", m_PathData->m_Params.m_Start.GetString().CStr() );
		PRINTF( "Dest.:  %s\n", m_PathData->m_Params.m_Destination.GetString().CStr() );

		if( m_PathData->m_Params.m_WarnMaxSteps )
		{
			DEBUGWARNDESC( "Pathfinding ran out of steps!" );
		}
	}
#endif

	// We didn't find a path, or ran out of steps.
	if( m_PathData->m_Params.m_UsePartialPath )
	{
		if( m_PathData->m_Params.m_Verbose ) { PRINTF( "Pathfinding finished with partial path.\n" ); }
		BuildPath( GetBestPartialDestination() );
		return EPS_PartialPathFound;
	}
	else
	{
		if( m_PathData->m_Params.m_Verbose ) { PRINTF( "Pathfinding finished with no path found.\n" ); }
		return EPS_NoPathFound;
	}
}

void RosaNav::TryExpand( const SPathNode& ClosedNode, const uint ExpandEdgeIndex )
{
	const SNavEdge&	ExpandEdge = m_World->GetNavEdge( ExpandEdgeIndex );

	if( ExpandEdge.m_BackNode == NAV_NULL )
	{
		// Edge is a boundary, no expansion possible.
		return;
	}

	if( ExpandEdge.m_Width < m_PathData->m_Params.m_AgentRadius * 2.0f )
	{
		// Agent is too wide to cross this edge
		return;
	}

	if( FindNavNodeInSet( ExpandEdge.m_BackNode, m_PathData->m_ClosedSet, NULL ) )
	{
		// Node is already closed, ignore this edge.
		return;
	}

	if( !IsAccessible( ExpandEdge.m_BackNode ) )
	{
		// Node can't be entered by this agent.
		return;
	}

	Expand( ClosedNode, ExpandEdgeIndex );
}

void RosaNav::Expand( const SPathNode& ClosedNode, const uint ExpandEdgeIndex )
{
	const SNavEdge&	ExpandEdge		= m_World->GetNavEdge( ExpandEdgeIndex );
	const uint		ExpandNodeIndex	= ExpandEdge.m_BackNode;
	DEBUGASSERT( !FindNavNodeInSet( ExpandNodeIndex, m_PathData->m_ClosedSet, NULL ) );

	// Travel cost is estimated as node centroid to node centroid; shouldn't make much difference
	// for search, but could cause odd results for wander and flee.
	const SNavNode&	ExpandNavNode	= m_World->GetNavNode( ExpandNodeIndex );
	const Vector	Step			= ExpandNavNode.m_Centroid;
	const Vector	Travel			= ( Step - ClosedNode.m_Step );
	const float		Cost			= Travel.Length();
	const float		G				= ClosedNode.m_G + Cost;

	uint OpenNodeIndex;
	if( FindNavNodeInSet( ExpandNodeIndex, m_PathData->m_OpenSet, &OpenNodeIndex ) )
	{
		// Node is already open. Update it if we have a faster path.
		SPathNode&	ExpandNode	= m_PathData->m_OpenSet[ OpenNodeIndex ];
		if( G < ExpandNode.m_G )
		{
			ExpandNode.m_G				= G;
			ExpandNode.m_Backlink		= ClosedNode.m_NavNodeIndex;
			ExpandNode.m_NavEdgeIndex	= ExpandEdgeIndex;
		}
	}
	else
	{
		// Node is unopen. Open it.
		SPathNode& ExpandNode		= m_PathData->m_OpenSet.PushBack();
		ExpandNode.m_G				= G;
		ExpandNode.m_H				= GetHeuristicCost( ClosedNode.m_NavNodeIndex, Step, m_PathData->m_Params.m_Destination );
		ExpandNode.m_NavNodeIndex	= ExpandNodeIndex;
		ExpandNode.m_Backlink		= ClosedNode.m_NavNodeIndex;
		ExpandNode.m_Step			= Step;
		ExpandNode.m_NavEdgeIndex	= ExpandEdgeIndex;
	}
}

uint RosaNav::GetMinOpenSetIndex() const
{
	DEVASSERT( m_PathData->m_OpenSet.Size() );

	float	MinF		= FLT_MAX;
	uint	MinIndex	= 0;

	FOR_EACH_ARRAY( NodeIter, m_PathData->m_OpenSet, SPathNode )
	{
		const SPathNode&	PathNode	= NodeIter.GetValue();
		const float			F			= PathNode.F();

		if( F < MinF )
		{
			MinIndex	= NodeIter.GetIndex();
			MinF		= F;
		}
	}

	return MinIndex;
}

bool RosaNav::FindNavNodeInSet( const uint NavNodeIndex, const Array<SPathNode>& NodeSet, uint* pOutNodeIndex ) const
{
	FOR_EACH_ARRAY( NodeIter, NodeSet, SPathNode )
	{
		const SPathNode& PathNode = NodeIter.GetValue();
		if( PathNode.m_NavNodeIndex == NavNodeIndex )
		{
			if( pOutNodeIndex )
			{
				*pOutNodeIndex = NodeIter.GetIndex();
			}
			return true;
		}
	}

	return false;
}

uint RosaNav::GetBestPartialDestination() const
{
	DEVASSERT( m_PathData->m_ClosedSet.Size() );

	// For search, the best partial path is the one that gets us closest to the destination
	// For flee, the best partial path is the one that gets us furthest from it
	// Both are based on minimizing the H value
	// For wander, the best partial path is the one that covers the most distance
	// This is based on the G value

	const bool	MaximizeG	= IsWander();	// If we're wandering, we maximize G; else, we minimize H
	float		Best		= MaximizeG ? -1.0f : FLT_MAX;
	uint		BestIndex	= 0xffffffff;

	FOR_EACH_ARRAY( NodeIter, m_PathData->m_ClosedSet, SPathNode )
	{
		const SPathNode& ClosedNode = NodeIter.GetValue();
		if( MaximizeG )
		{
			// For wander, the best partial path is the one that covers the most distance
			// This is based on the G value
			if( ClosedNode.m_G > Best )
			{
				BestIndex	= ClosedNode.m_NavNodeIndex;
				Best		= ClosedNode.m_G;
			}
		}
		else
		{

			// For search, the best partial path is the one that gets us closest to the destination
			// For flee, the best partial path is the one that gets us furthest from it
			// Both are based on the H value
			if( ClosedNode.m_H < Best )
			{
				BestIndex	= ClosedNode.m_NavNodeIndex;
				Best		= ClosedNode.m_H;
			}
		}
	}

	DEBUGASSERT( BestIndex != 0xffffffff );
	return BestIndex;
}

float RosaNav::GetHeuristicCost( const uint NavNodeIndex, const Vector& NodeLocation, const Vector& Destination ) const
{
	const SNavNode&	NavNode		= m_World->GetNavNode( NavNodeIndex );
	const uint&		NavProps	= NavNode.m_Props;

	float Cost = 0.0f;

	// If we're cautious, add a fixed overhead if the node should be avoided.
	if( m_PathData->m_Params.m_Cautious && ( NavProps & ENP_CautiousShouldAvoid ) )
	{
		// Equivalent to a 100m step
		Cost += 100.0f;
	}

	// If we're tethered, add a fixed overhead if the node is outside the tether sphere.
	if( m_PathData->m_Params.m_UseTether )
	{
		const Vector	ToTether	= NodeLocation - m_PathData->m_Params.m_TetherLocation;
		const float		ToTetherSq	= ToTether.LengthSquared();
		if( ( m_PathData->m_Params.m_TetherDistanceSq > 0.0f && ToTetherSq > m_PathData->m_Params.m_TetherDistanceSq ) ||
			( m_PathData->m_Params.m_TetherDistanceZ > 0.0f && Abs( ToTether.z ) > m_PathData->m_Params.m_TetherDistanceZ ) )
		{
			// Equivalent to a 100m step
			Cost += 100.0f;
		}
	}

	if( IsWander() )
	{
		// For wandering, add a small random weight so we get varied results.
		Cost += Math::Random( 0.0f, 0.1f );
	}
	else if( IsFlee() )
	{
		const Vector&	FleeLocation	= Destination;
		const Vector	OffsetFromFlee	= NodeLocation - FleeLocation;
		const float		Distance		= OffsetFromFlee.Length();
		const float		DistanceToRange	= m_PathData->m_Params.m_TargetDistance - Distance;

		// The magic number here is just to make approaching the target *really bad*.
		const float		ScaledDTR		= 10.0f * DistanceToRange;
		Cost += ScaledDTR;
	}
	else
	{
		Cost += ( Destination - NodeLocation ).Length();
	}

	return Cost;
}

// This means there is a door and nothing else on this spot.
static const uint	skDoorValue			= ENP_Door | 1;
static const uint	skLockedDoorValue	= ENP_Door | ENP_LockedDoor | 1;

bool RosaNav::IsAccessible( const uint NavNodeIndex ) const
{
	const SNavNode&	NavNode		= m_World->GetNavNode( NavNodeIndex );
	const uint&		NavProps	= NavNode.m_Props;

	if( m_PathData->m_Params.m_AgentHeight > NavNode.m_Height )
	{
		// Agent doesn't fit this node
		return false;
	}

	if( NavProps == ENP_None || NavProps == ENP_CautiousShouldAvoid )
	{
		// This is fine, the nav node is empty and accessible
		return true;
	}

	if( m_PathData->m_Params.m_Cautious && ( NavProps & ENP_CautiousMustAvoid ) )
	{
		// This node is inaccessible to cautious searches
		return false;
	}

	if( NavProps == skDoorValue && m_PathData->m_Params.m_CanOpenDoors )
	{
		// This is fine, we can open the door
		return true;
	}

	if( NavProps == skLockedDoorValue && m_PathData->m_Params.m_CanOpenDoors )
	{
		if( m_PathData->m_Params.m_CanUnlockDoors || m_PathData->m_Params.m_CanBreakDoors )
		{
			// This is fine, we can unlock or break down the door
			return true;
		}

		WBEntity* const			pWorldEntity	= m_World->GetNavEntity( NavNodeIndex );
		DEVASSERT( pWorldEntity );

		WBCompRosaDoor* const	pDoor			= WB_GETCOMP( pWorldEntity, RosaDoor );
		DEVASSERT( pDoor );

		// NOTE: This is a bit of a hack; it's using the entity's current location,
		// not the side of the door that the path will actually be on. This can cause
		// the agent to generate a path through a locked door, which will then be
		// invalidated and repathed when they get there, at which point they will also
		// try the handle (below). It's weird, but it does work.
		if( pDoor->IsInsideDoor( m_PathData->m_Params.m_Entity ) )
		{
			// This is fine, we're inside so we can open the door
			return true;
		}

		// HACKHACK: We can't unlock the door but we can try the handle if we're close enough
		if( pDoor->ShouldAITryHandle( m_PathData->m_Params.m_Entity ) )
		{
			WB_MAKE_EVENT( OnPushed, pWorldEntity );
			WB_SET_AUTO( OnPushed, Entity, Pusher, m_PathData->m_Params.m_Entity );
			WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnPushed, pWorldEntity );
		}
	}

	// Nav node is blocked, a door we can't open, etc.
	return false;
}

void RosaNav::UpdateWorldFromEntity( WBCompRosaCollision* const pCollision, const bool Add )
{
	PROFILE_FUNCTION;

	static const uint skEntitiesMask		= ENP_Entities;
	static const uint skEntitiesNotMask		= ~ENP_Entities;
	static const uint skDoorMask			= ENP_Door;
	static const uint skDoorNotMask			= ~ENP_Door;
	static const uint skLockedDoorMask		= ENP_LockedDoor;
	static const uint skLockedDoorNotMask	= ~ENP_LockedDoor;

	DEVASSERT( pCollision );

	Array<uint>		NavNodeIndices;
	if( !pCollision->FindNavNodesUnder( NavNodeIndices ) )
	{
		// Entity is not positioned above the navmesh.
		return;
	}

	WBEntity* const			pEntity			= pCollision->GetEntity();
	WBCompRosaDoor* const	pDoor			= WB_GETCOMP( pEntity, RosaDoor );
	const bool				IsDoor			= ( pDoor != NULL );
	const bool				IsLockedDoor	= ( pDoor && pDoor->IsLocked() );

	FOR_EACH_ARRAY( NavNodeIndexIter, NavNodeIndices, uint )
	{
		const uint& NavNodeIndex	= NavNodeIndexIter.GetValue();
		SNavNode&	NavNode			= m_World->GetNavNode( NavNodeIndex );
		uint&		NavProps		= NavNode.m_Props;
		const uint	CurrentCount	= NavProps & skEntitiesMask;

		if( Add )
		{
			DEVASSERT( CurrentCount < ENP_Entities );
			NavProps &= skEntitiesNotMask;
			NavProps |= ( CurrentCount + 1 );
			if( IsDoor )		{ NavProps |= skDoorMask; }
			if( IsLockedDoor )	{ NavProps |= skLockedDoorMask; }

			if( IsLockedDoor )
			{
				m_World->SetNavEntity( NavNodeIndex, pEntity );
			}
		}
		else
		{
			// Remove
			DEVASSERT( CurrentCount > 0 );
			NavProps &= skEntitiesNotMask;
			NavProps |= ( CurrentCount - 1 );
			if( IsDoor )		{ NavProps &= skDoorNotMask; }
			if( IsLockedDoor )	{ NavProps &= skLockedDoorNotMask; }

			if( IsLockedDoor )
			{
				m_World->ClearNavEntity( NavNodeIndex, pEntity );
			}
		}
	}

	const AABB EntityBounds = pCollision->GetNavBounds();

	WB_MAKE_EVENT( OnNavChanged, NULL );
	WB_SET_AUTO( OnNavChanged, Vector, BoxMin, EntityBounds.m_Min );
	WB_SET_AUTO( OnNavChanged, Vector, BoxMax, EntityBounds.m_Max );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnNavChanged, NULL );
}
