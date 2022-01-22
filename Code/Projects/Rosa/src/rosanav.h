#ifndef ROSANAV_H
#define ROSANAV_H

#include "array.h"
#include "vector.h"
#include "rosaworld.h"
#include "set.h"
#include "wbentityref.h"
#include "map.h"

class WBCompRosaCollision;
class Segment;

class RosaNav
{
public:
	RosaNav();
	~RosaNav();

	enum EPathMode
	{
		EPM_None,
		EPM_Search,	// Directed path search from start to destination
		EPM_Wander,	// Undirected path search from start to a target distance
		EPM_Flee,	// Undirected path search from start to a target distance, avoiding "destination"
	};

	enum EMotionType
	{
		EMT_None,
		EMT_Walking,
		EMT_Flying,
	};

	typedef Array<Vector> TPath;

	struct SPathfindingParams
	{
		SPathfindingParams()
		:	m_Entity( NULL )
		,	m_PathMode( EPM_None )
		,	m_Start()
		,	m_Destination()
		,	m_AgentHeight( 0.0f )
		,	m_AgentRadius( 0.0f )
		,	m_MaxSteps( 0 )
		,	m_TargetDistance( 0.0f )
		,	m_UseTether( false )
		,	m_TetherLocation()
		,	m_TetherDistanceSq( 0.0f )
		,	m_TetherDistanceZ( 0.0f )
		,	m_MotionType( EMT_None )
		,	m_CanOpenDoors( false )
		,	m_CanUnlockDoors( false )
		,	m_CanBreakDoors( false )
		,	m_Cautious( false )
		,	m_UsePartialPath( false )
		,	m_Verbose( false )
#if BUILD_DEV
		,	m_WarnMaxSteps( false )
#endif
		,	m_StartNodeIndex( 0 )
		,	m_DestNodeIndex( 0 )
		{
		}

		// For client to set
		WBEntity*	m_Entity;
		EPathMode	m_PathMode;
		Vector		m_Start;
		Vector		m_Destination;
		float		m_AgentHeight;
		float		m_AgentRadius;
		uint		m_MaxSteps;
		float		m_TargetDistance;	// For search, this is how close to approach the destination. For wandering/fleeing, this is the range to move from start.
		bool		m_UseTether;
		Vector		m_TetherLocation;
		float		m_TetherDistanceSq;
		float		m_TetherDistanceZ;
		EMotionType	m_MotionType;
		bool		m_CanOpenDoors;
		bool		m_CanUnlockDoors;
		bool		m_CanBreakDoors;
		bool		m_Cautious;
		bool		m_UsePartialPath;	// If there's no path, or a path isn't found after MaxSteps, return the partial path anyway.
		bool		m_Verbose;

#if BUILD_DEV
		bool		m_WarnMaxSteps;
#endif

		// For internal use
		uint		m_StartNodeIndex;
		uint		m_DestNodeIndex;
	};

	enum EPathStatus
	{
		EPS_None,
		EPS_Working,			// For timesliced searches
		EPS_NoPathFound,
		EPS_PartialPathFound,
		EPS_PathFound,
	};

	struct SPathNode
	{
		SPathNode()
		:	m_G( 0.0f )
		,	m_H( 0.0f )
		,	m_NavNodeIndex( 0 )
		,	m_NavEdgeIndex( NAV_NULL )
		,	m_Backlink( NAV_NULL )
		,	m_Step()
		{
		}

		float F() const
		{
			return m_G + m_H;
		}

		float	m_G;
		float	m_H;
		uint	m_NavNodeIndex;
		int		m_NavEdgeIndex;
		int		m_Backlink;		// Nav node index
		Vector	m_Step;
	};

	// Transient data, held by client for potential timeslicing
	struct SPathData
	{
		SPathData()
		:	m_Status( EPS_None )
		,	m_Params()
		,	m_OpenSet()
		,	m_ClosedSet()
		,	m_EdgePath()
		,	m_Path()
		{
		}

		EPathStatus			m_Status;
		SPathfindingParams	m_Params;
		Array<SPathNode>	m_OpenSet;
		Array<SPathNode>	m_ClosedSet;
		Array<int>			m_EdgePath;		// List of nav edges the path crosses; used for smoothing the path
		TPath				m_Path;
	};

	static RosaNav*	GetInstance();
	EPathStatus		FindPath( SPathData& PathData );

	// ROSATODO: Move to RosaWorld?
	void			UpdateWorldFromEntity( WBCompRosaCollision* const pCollision, const bool Add );

private:
	friend class RosaWorld;

	bool		IsSearch() const	{ DEBUGASSERT( m_PathData ); return m_PathData->m_Params.m_PathMode == EPM_Search; }
	bool		IsWander() const	{ DEBUGASSERT( m_PathData ); return m_PathData->m_Params.m_PathMode == EPM_Wander; }
	bool		IsFlee() const		{ DEBUGASSERT( m_PathData ); return m_PathData->m_Params.m_PathMode == EPM_Flee; }

	bool		IsWalking() const	{ DEBUGASSERT( m_PathData ); return m_PathData->m_Params.m_MotionType == EMT_Walking; }
	bool		IsFlying() const	{ DEBUGASSERT( m_PathData ); return m_PathData->m_Params.m_MotionType == EMT_Flying; }

	EPathStatus	FindPath();
	EPathStatus	DoAStarSearch();
	void		TryExpand( const SPathNode& ClosedNode, const uint ExpandEdgeIndex );
	void		Expand( const SPathNode& ClosedNode, const uint ExpandEdgeIndex );
	void		BuildPath( const uint FinalNodeIndex );
	void		RecursiveBuildPath( const uint StepNodeIndex, const uint NumSteps );
	void		SmoothPath();
	void		TrimPathFromStart();		// For wander/flee
	void		TrimPathFromDestination();	// For search

	uint		GetMinOpenSetIndex() const;
	bool		FindNavNodeInSet( const uint NavNodeIndex, const Array<SPathNode>& NodeSet, uint* pOutNodeIndex ) const;

	bool		IsAccessible( const uint NavNodeIndex ) const;
	float		GetHeuristicCost( const uint NavNodeIndex, const Vector& NodeLocation, const Vector& Destination ) const;
	uint		GetBestPartialDestination() const;	// Returns nav node index

	RosaWorld*	m_World;

	// Pointer to data we're currently operating on. Owned by client.
	SPathData*	m_PathData;
};

#endif // ROSANAV_H
