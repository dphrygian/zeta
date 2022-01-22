#ifndef ROSAWORLD_H
#define ROSAWORLD_H

#include "rosairradiance.h"
#include "iindexbuffer.h"
#include "vector.h"
#include "vector2.h"
#include "vector4.h"
#include "array.h"
#include "set.h"
#include "map.h"
#include "hashedstring.h"
#include "simplestring.h"
#include "reversehash.h"
#include "convexhull.h"
#include "aabb.h"
#include "triangle.h"
#include "wbentityref.h"
#include "iwbeventobserver.h"
#include "configmanager.h"

class WBEntity;
class RosaFramework;
class RosaNav;
class IRenderer;
class Mesh;
class Segment;
class Ray;
class AABB;
class CollisionInfo;
class IDataStream;
class RosaWorldGen;
class RosaMesh;
class ITexture;
class RosaWorldCubemap;
class RosaWorldTexture;
class View;
struct SReadMeshBuffers;

enum ERosaCollisionFlags
{
	EECF_None						= 0x0000,

	// Primary flags, compared with the associative flags below on each potential collision entity
	EECF_CollideAsWorld				= 0x0001,
	EECF_CollideAsEntity			= 0x0002,
	EECF_CollideAsBlocker			= 0x0004,	// "Blockers" are dynamic entities that mutually block each other (and nothing else)
	EECF_CollideAsNav				= 0x0008,	// ROSANOTE: For Rosa, unlike Eldritch/Neon, I have to explicitly mark things as blocking nav
	EECF_CollideAsOcclusion			= 0x0010,
	EECF_CollideAsAudio				= 0x0020,
	EECF_CollideAsTrace				= 0x0040,	// "Trace" is ambiguous, but it generally means things that could receive a scanline weapon test
	EECF_CollideAsRagdoll			= 0x0080,	// Only used for ragdoll masses
	EECF_CollideStaticEntities		= 0x0100,
	EECF_CollideDynamicEntities		= 0x0200,
	// Special case, overrides blocking flag optimization and tests all entities (which match the given EECF_Mask_EntityTypes type).
	// Should be used sparingly. I'm adding this as a hack for movable static entities (LizardWatson, specifically).
	// ROSATODO: Do I still need this hack flag, now that I've got proper collidable dynamic entities? Probably not.
	EECF_CollideAllEntities			= 0x0400,
	// Special case, enables tests against animated hitboxes; only valid for zero-extent checks
	EECF_CollideBones				= 0x0800,

	// Associative meanings, compared with the primary flags to filter which entities should be considered for collision
	EECF_BlocksWorld			= EECF_CollideAsWorld,
	EECF_BlocksEntities			= EECF_CollideAsEntity,
	EECF_BlocksBlockers			= EECF_CollideAsBlocker,
	EECF_BlocksNav				= EECF_CollideAsNav,
	EECF_BlocksOcclusion		= EECF_CollideAsOcclusion,
	EECF_BlocksAudio			= EECF_CollideAsAudio,
	EECF_BlocksTrace			= EECF_CollideAsTrace,
	EECF_BlocksRagdolls			= EECF_CollideAsRagdoll,
	EECF_IsStatic				= EECF_CollideStaticEntities,
	EECF_IsDynamic				= EECF_CollideDynamicEntities,

	// Usage masks, only used in SweepEntities to partition the flags
	EECF_Mask_CollideAs			= EECF_CollideAsWorld
								| EECF_CollideAsEntity
								| EECF_CollideAsBlocker
								| EECF_CollideAsNav
								| EECF_CollideAsOcclusion
								| EECF_CollideAsAudio
								| EECF_CollideAsTrace
								| EECF_CollideAsRagdoll,
	EECF_Mask_EntityTypes		= EECF_CollideStaticEntities
								| EECF_CollideDynamicEntities,

	// Common combinations, typically used when initializing the CollisionInfo struct for a sweep or trace
	EECF_Occlusion				= EECF_CollideAsOcclusion	| EECF_CollideStaticEntities,
	EECF_Audio					= EECF_CollideAsAudio		| EECF_CollideStaticEntities,
	EECF_EntityCollision		= EECF_CollideAsEntity		| EECF_CollideStaticEntities | EECF_CollideDynamicEntities,
	EECF_BlockerCollision		= EECF_CollideAsBlocker		| EECF_CollideStaticEntities | EECF_CollideDynamicEntities,
	EECF_Nav					= EECF_CollideAsNav			| EECF_CollideStaticEntities,
	EECF_Trace					= EECF_CollideAsTrace		| EECF_CollideStaticEntities | EECF_CollideDynamicEntities,
	EECF_RagdollCollision		= EECF_CollideAsRagdoll		| EECF_CollideStaticEntities | EECF_CollideDynamicEntities,
};

// ROSANOTE: New for Rosa collision
// Moved out of RosaWorld because it's so ubiquitous now
struct SConvexHull
{
	SConvexHull()
	:	m_Hull()
	,	m_Bounds()
	,	m_Surface()
	,	m_CollisionFlags( EECF_None )
	{
	}

	ConvexHull		m_Hull;
	AABB			m_Bounds;
	HashedString	m_Surface;			// ROSANOTE: If needed, split this out into surfaces for each plane of the hull.
	uint			m_CollisionFlags;
};

enum EPortalIndex
{
	EPI_East,
	EPI_West,
	EPI_North,
	EPI_South,
	EPI_Up,
	EPI_Down,
	EPI_MAX
};

// ROSAHACK: Shared between room and worldgen stuff, easiest to define here.
struct SPortal
{
	SPortal()
	:	m_DefNameHash()
	,	m_FrontTag()
	,	m_BackTags()
	,	m_NoExpand( false )
	,	m_NoJoin( false )
	,	m_MustClose( false )
	,	m_MustJoin( false )
	,	m_ExpandPriority( 0 )
	{
	}

	// Defined here so it can be shared with the RoomBaker project without making Rosa a library.
	void InitializeFromDefinition( const HashedString& DefinitionNameHash )
	{
		STATICHASH( Front );
		m_FrontTag = ConfigManager::GetInheritedHash( sFront, HashedString::NullString, DefinitionNameHash );

		STATICHASH( NumBacks );
		const uint NumBackTags = ConfigManager::GetInheritedInt( sNumBacks, 0, DefinitionNameHash );
		FOR_EACH_INDEX( BackTagIndex, NumBackTags )
		{
			const HashedString BackTag = ConfigManager::GetInheritedSequenceHash( "Back%d", BackTagIndex, HashedString::NullString, DefinitionNameHash );
			DEVASSERT( !m_BackTags.Contains( BackTag ) );
			m_BackTags.PushBack( BackTag );
		}

		STATICHASH( NoExpand );
		m_NoExpand = ConfigManager::GetInheritedBool( sNoExpand, false, DefinitionNameHash );

		STATICHASH( NoJoin );
		m_NoJoin = ConfigManager::GetInheritedBool( sNoJoin, false, DefinitionNameHash );

		STATICHASH( MustClose );
		m_MustClose = ConfigManager::GetInheritedBool( sMustClose, false, DefinitionNameHash );

		STATICHASH( MustJoin );
		m_MustJoin = ConfigManager::GetInheritedBool( sMustJoin, false, DefinitionNameHash );

		STATICHASH( ExpandPriority );
		m_ExpandPriority = ConfigManager::GetInheritedInt( sExpandPriority, 0, DefinitionNameHash );
	}

	// For sorting portals for debug output (RosaWorldGen::m_Failed_OpenPortals uses a map from portals to their failure counts)
	bool operator==( const SPortal& Other ) const
	{
		return
			m_FrontTag			== Other.m_FrontTag &&
			m_BackTags			== Other.m_BackTags &&		// Array compare by size and element, this should work
			m_NoExpand			== Other.m_NoExpand &&
			m_NoJoin			== Other.m_NoJoin &&
			m_MustClose			== Other.m_MustClose &&
			m_MustJoin			== Other.m_MustJoin &&
			m_ExpandPriority	== Other.m_ExpandPriority;
	}

	bool operator<( const SPortal& Other ) const
	{
		if( m_FrontTag < Other.m_FrontTag )
		{
			return true;
		}

		if( m_FrontTag > Other.m_FrontTag )
		{
			return false;
		}

		// Array compare by size and element, this should work
		if( m_BackTags < Other.m_BackTags )
		{
			return true;
		}

		if( Other.m_BackTags < m_BackTags )
		{
			return false;
		}

		if( !m_NoExpand && Other.m_NoExpand )
		{
			return true;
		}

		if( m_NoExpand && !Other.m_NoExpand )
		{
			return false;
		}

		if( !m_NoJoin && Other.m_NoJoin )
		{
			return true;
		}

		if( m_NoJoin && !Other.m_NoJoin )
		{
			return false;
		}

		if( !m_MustClose && Other.m_MustClose )
		{
			return true;
		}

		if( m_MustClose && !Other.m_MustClose )
		{
			return false;
		}

		if( !m_MustJoin && Other.m_MustJoin )
		{
			return true;
		}

		if( m_MustJoin && !Other.m_MustJoin )
		{
			return false;
		}

		return m_ExpandPriority < Other.m_ExpandPriority;
	}

	bool Fits( const SPortal& Other ) const
	{
		if( !m_FrontTag && !Other.m_FrontTag )
		{
			// These are both walls, they fit
			return true;
		}
		else if( !m_FrontTag || !Other.m_FrontTag )
		{
			// One of these is a wall and the other is not, they don't fit
			return false;
		}

		// Neither of these is a wall; do their tags match?
		if( ( m_BackTags.Empty() || m_BackTags.Contains( Other.m_FrontTag ) ) &&
			( Other.m_BackTags.Empty() || Other.m_BackTags.Contains( m_FrontTag ) ) )
		{
			// These are not walls and their tags do match
			return true;
		}

		// These are not walls and their tags don't match
		return false;
	}

	HashedString		m_DefNameHash;		// DLP 28 Nov 2021: So we can init portal from config at runtime instead of bake time
	HashedString		m_FrontTag;			// Defines what *is* on this side of the portal; null string means a wall, not a portal
	Array<HashedString>	m_BackTags;			// Defines what *can be* on that side of the portal; empty means anything can match
	bool				m_NoExpand;			// We can only join to (existing *or* future) modules with this portal, not expand through it.
	bool				m_NoJoin;			// We can only expand to new modules with this portal, not join existing ones.
	bool				m_MustClose;		// We must close this portal, by expansion or joining; it's a failure if we can't (if this is false, then connective tissue will fill it in)
	bool				m_MustJoin;			// We must join to existing modules with this portal; implies and subsumes NoExpand.
	int					m_ExpandPriority;	// Higher value priority takes precedence; used to expand through certain portals before or after others.
};

struct SPortals
{
	SPortals()
	:	m_Portals()
	{
		memset( m_Portals, 0, sizeof( m_Portals ) );
	}

	// Ordered X+ X- Y+ Y- Z+ Z-
	SPortal	m_Portals[6];
};

#define NAV_NULL -1

// ROSANOTE: A nav edge is one-directional.
// ROSATODO: Add vertical clearance, width, etc.?
struct SNavEdge
{
	SNavEdge()
	:	m_VertA()
	,	m_VertB()
	,	m_Width( 0.0f )
	,	m_BackNode( NAV_NULL )
	{
	}

	Vector	m_VertA;
	Vector	m_VertB;
	float	m_Width;
	int		m_BackNode;	// NAV_NULL (-1) means no connection, or mesh boundary
};

enum ENavProp : uint
{
	ENP_None				= 0x00000000,	// Node is open

	ENP_Door				= 0x80000000,	// Node is blocked by a door
	ENP_LockedDoor			= 0x40000000,	// Node is blocked by a locked door

	ENP_CautiousMustAvoid	= 0x20000000,	// Node is inaccessible for cautious path finds (tagged in editor, should also later be added in RosaNav::UpdateWorldFromEntity for traps)
	ENP_CautiousShouldAvoid	= 0x10000000,	// Node is high cost for cautious path finds (tagged in editor)

	ENP_AllProps			= ENP_Door | ENP_LockedDoor | ENP_CautiousMustAvoid | ENP_CautiousShouldAvoid,

	ENP_Entities			= ~ENP_AllProps	// Remaining bits are used to count collidable entities
};

// ROSATODO: Add vertical clearance for approximate flight nav?
struct SNavNode
{
	SNavNode()
	:	m_EdgeA( 0 )
	,	m_EdgeB( 0 )
	,	m_EdgeC( 0 )
	,	m_Centroid()
	,	m_Bounds()
	,	m_Tri()
	,	m_Height( 0.0f )
	,	m_Props( ENP_None )
	,	m_Sector( 0 )
	{
	}

	uint		m_EdgeA;
	uint		m_EdgeB;
	uint		m_EdgeC;
	Vector		m_Centroid;
	AABB		m_Bounds;		// Bounds of the triangle, does not include height
	Triangle	m_Tri;
	float		m_Height;		// Approximate height of node from trace up from centroid

	uint		m_Props;		// Baked/transient, used for marking a node as a door node, etc.

	// HACKHACK: Transient properties, despite that this struct is used by room files/RoomBaker
	// (which means adding properties here bloats room files and requires rebuilding/rebaking)
	uint		m_Sector;		// Transient, used for associating minimap primitives
};

// ROSAHACK: This started as part of RosaTools, but it is spreading everywhere.
enum EBrushType
{
	EBT_None,
	EBT_Geo,
	EBT_Spawner,
	EBT_Prefab,		// Only used for creating brushes in tools; immediately creates sub-brushes of other types
	EBT_Room,
};

class RosaWorld : public IWBEventObserver
{
public:
	RosaWorld();
	~RosaWorld();

	virtual void		HandleEvent( const WBEvent& Event );

	void				Initialize();
	void				Create();
	void				GatherStats();

	void				SetCurrentWorld( const SimpleString& WorldDef );
	SimpleString		GetCurrentWorld() const { return m_CurrentWorldDef; }

	void				Tick( const float DeltaTime );
	void				Render();
#if BUILD_DEV
	void				DebugRender() const;
#endif

	RosaFramework*		GetFramework() const { return m_Framework; }

	// Primary function for all collision tests: can be used for point, line, and sweep checks against voxels and entities.
	bool				Sweep( const Segment& SweepSegment, const Vector& HalfExtents, CollisionInfo& Info ) const;

	// Helper functions implemented with Sweep
	bool				Trace( const Ray& TraceRay, CollisionInfo& Info ) const;
	bool				Sweep( const Ray& TraceRay, const Vector& HalfExtents, CollisionInfo& Info ) const;
	bool				Trace( const Segment& TraceSegment, CollisionInfo& Info ) const;
	bool				CheckClearance( const Vector& Location, const Vector& HalfExtents, CollisionInfo& Info ) const;
	bool				PointCheck( const Vector& Location, CollisionInfo& Info ) const;
	bool				LineCheck( const Vector& Start, const Vector& End, CollisionInfo& Info ) const;
	bool				SweepCheck( const Vector& Start, const Vector& End, const Vector& HalfExtents, CollisionInfo& Info ) const;
	bool				FindSpot( Vector& InOutSpot, const Vector& Extents, CollisionInfo& Info ) const;

	void				Save( const IDataStream& Stream );
	void				Load( const IDataStream& Stream );

	HashedString		GetCollisionSurface( const CollisionInfo& Info ) const;
	HashedString		GetSurfaceBelowPoint( const Vector& Location, WBEntity* const pSourceEntity ) const;
	HashedString		GetSurfaceBelow( const Vector& Location, const Vector& Extents, WBEntity* const pSourceEntity ) const;

	// ROSATODO: Refactor the source of this so it supports Rosa levels, or just use a Very Large Number.
	float				GetRayTraceLength() const { return GetCurrentWorldDef().m_RayTraceLength; }

	float				GetSkylineViewScalar() const { return GetCurrentWorldDef().m_SkylineViewScalar; }

	uint				GetNumVisibleSectors() const { return m_VisibleSectors.Size(); }
	uint				CountVisibleGeoMeshes();
	const AABB&			GetVisibleSectorRenderBound( const uint Index ) const { return m_Sectors[ m_VisibleSectors[ Index ] ].m_RenderBound; }

	void				AddFogMeshDef( const HashedString& FogMeshDefName );
	void				GetFogMeshValues( const HashedString& FogMeshDefName, Vector4& OutFogMeshColor, Vector4& OutFogMeshParams );
	bool				GetFogMeshValuesForCubemap( const SimpleString& CubemapName, Vector4& OutFogMeshColor, Vector4& OutFogMeshParams );

	// Uses potential vis set, not actual portal bounds
	bool				IsLocationVisibleFromAnyVisibleSector( const Vector& Location ) const;

	void				RevealMinimapSector( const uint SectorIndex );
	void				RevealMinimap();

	bool				IsNavNodeUnder( const SNavNode& NavNode, const Vector& Location, float& OutDistance ) const;
	bool				FindNavNodeUnder( const Vector& Location, uint& OutNavNodeIndex ) const;
	bool				IsNavMeshUnder( const Vector& Location ) const;
	const SNavNode&		GetNavNode( const uint NavNodeIndex ) const { return m_NavNodes[ NavNodeIndex ]; }
	SNavNode&			GetNavNode( const uint NavNodeIndex ) { return m_NavNodes[ NavNodeIndex ]; }
	const SNavEdge&		GetNavEdge( const uint NavEdgeIndex ) const { return m_NavEdges[ NavEdgeIndex ]; }
	SNavEdge&			GetNavEdge( const uint NavEdgeIndex ) { return m_NavEdges[ NavEdgeIndex ]; }
	WBEntity*			GetNavEntity( const uint NavNodeIndex ) const;
	void				SetNavEntity( const uint NavNodeIndex, WBEntity* const pEntity );
	void				ClearNavEntity( const uint NavNodeIndex, WBEntity* const pEntity );

	const Vector&		GetWindDirection() const { return GetCurrentWorldDef().m_WindSwayDirection; }

	RosaWorldGen*		GetWorldGen() const { return GetCurrentWorldDef().m_WorldGen; }

private:
	friend class RosaTools;
	friend class RosaWorldGen;
	friend class RosaNav;

	struct SSectorPortal
	{
		SSectorPortal()
		:	m_Corners()
		,	m_FrontPlane()
		,	m_BackSector( 0 )
		{
		}

		Vector	m_Corners[4];
		Plane	m_FrontPlane;
		uint	m_BackSector;
	};

	struct SAmbientLight
	{
		SAmbientLight()
		:	m_Mesh( NULL )
		,	m_Hull()
		,	m_Exposure( 0.0f )
		,	m_RegionFogScalar()
		,	m_MinimapScalar( 0.0f )
		,	m_EdgeColorHSV()
		,	m_Ambience()
		,	m_Reverb()
		{
		}

		Mesh*			m_Mesh;
		SConvexHull		m_Hull;
		float			m_Exposure;
		Vector4			m_RegionFogScalar;
		float			m_MinimapScalar;
		Vector			m_EdgeColorHSV;
		SimpleString	m_Ambience;
		SimpleString	m_Reverb;
	};

	struct SFogMeshDef
	{
		SFogMeshDef()
		:	m_FogMeshColor()
		,	m_FogMeshParams()
		{
		}

		Vector4	m_FogMeshColor;
		Vector4	m_FogMeshParams;
	};

	struct SSector
	{
		SSector()
		:	m_Hulls()
		,	m_GeoMeshes()
		,	m_AmbientLights()
		,	m_FogMeshes()
		,	m_Portals()
		,	m_RenderBound()
		,	m_CollisionBound()
		,	m_IsSingleTile( false )
		{
		}

		Array<SConvexHull>		m_Hulls;
		Array<uint>				m_GeoMeshes;		// Indexes into world's m_GeoMeshes array
		Array<SAmbientLight>	m_AmbientLights;	// If I ever want to support ambient lights that overlap sector bounds, move to a m_GeoMeshes-style indexing model
		Array<RosaMesh*>		m_FogMeshes;		// If I ever want to support fog meshes that overlap sector bounds, move to a m_GeoMeshes-style indexing model (or use an entity's fog mesh)
		Array<SSectorPortal>	m_Portals;
		AABB					m_RenderBound;		// Tight, tile-fitted bound, used in conjunction with sectors and portals
		AABB					m_GeoMeshBound;		// Tight, slightly contracted bound, used for determining which geomeshes belong to sector
		AABB					m_CollisionBound;	// Expanded bound to contain collision hulls that overlap module edges
		bool					m_IsSingleTile;		// HACKHACK for merging small sector shadow meshes
	};

	struct SWorldDef
	{
		SWorldDef()
		:	m_RayTraceLength( 0.0f )
		,	m_MinimapTones()
		,	m_MinimapFloor()
		,	m_MinimapSolid()
		,	m_CubemapName()
		,	m_IrradianceDef()
		,	m_ColorGrading()
		,	m_Noise()
		,	m_NoiseScaleLo( 0.0f )
		,	m_NoiseScaleHi( 0.0f )
		,	m_NoiseRange( 0.0f )
		,	m_Lens()
		,	m_Displace()
		,	m_Blot()
		,	m_Canvas()
		//,	m_EdgeBackColor()
		//,	m_EdgeColor()
		,	m_EdgeColorHSV()
		,	m_EdgeLuminanceLo()
		,	m_EdgeLuminanceHi()
		,	m_WatercolorParams()
		,	m_DisplacePct()
		,	m_EmissiveMax( 0.0f )
		,	m_Exposure( 0.0f )
		,	m_MinimapScalar( 0.0f )
		,	m_MinimapRenderAll( false )
		,	m_BloomKernel()
		,	m_BloomRadius( 0.0f )
		,	m_BloomAspectRatio( 0.0f )
		,	m_BloomThreshold()
		,	m_BloomScalar( 0.0f )
		,	m_WorldGen( NULL )
		,	m_FogNear( 0.0f )
		,	m_FogFar( 0.0f )
		,	m_FogColorNearHi()
		,	m_FogColorFarHi()
		,	m_FogColorNearLo()
		,	m_FogColorFarLo()
		,	m_FogNearFarCurve()
		,	m_FogLoHiCurve()
		,	m_HeightFogLo( 0.0f )
		,	m_HeightFogHi( 0.0f )
		,	m_HeightFogExp( 0.0f )
		,	m_HeightFogLoExp( 0.0f )
		,	m_HeightFogHiExp( 0.0f )
		,	m_FogLightDensity( 0.0f )
		,	m_SunVector()
		,	m_SkyColorHi()
		,	m_SkyColorLo()
		,	m_SkylineViewScalar( 0.0f )
		,	m_WindSwayDirection()
		,	m_WindSwayIntensity( 0.0f )
		,	m_WindSwayNoiseScalar( 0.0f )
		,	m_WindSwayNoiseOctaves( 0 )
		,	m_WindSwayNoiseOffset( 0.0f )
		,	m_WindLiftIntensity( 0.0f )
		,	m_WindLiftNoiseScalar( 0.0f )
		,	m_WindLiftNoiseOctaves( 0 )
		,	m_WindLiftNoiseOffset( 0.0f )
		,	m_WindFlapIntensity( 0.0f )
		,	m_WindFlapNoiseScalar( 0.0f )
		,	m_WindFlapNoiseOctaves( 0 )
		,	m_WindFlapNoiseOffset( 0.0f )
		,	m_WindPhaseTime()
		,	m_WindPhaseSpace()
		,	m_Music()
		,	m_Ambience()
		,	m_Reverb()
		,	m_SpawnerOverrides()
		{
		}

		// ROSATODO: Refactor as mentioned above.
		// Max ray length for this world size
		float					m_RayTraceLength;

		SimpleString			m_MinimapTones;
		SimpleString			m_MinimapFloor;
		SimpleString			m_MinimapSolid;
		SimpleString			m_CubemapName;
		SimpleString			m_IrradianceDef;
		SimpleString			m_ColorGrading;
		SimpleString			m_Noise;
		float					m_NoiseScaleLo;
		float					m_NoiseScaleHi;
		float					m_NoiseRange;
		SimpleString			m_Lens;
		SimpleString			m_Displace;
		SimpleString			m_Blot;
		SimpleString			m_Canvas;
		//Vector4					m_EdgeBackColor;
		//Vector4					m_EdgeColor;
		Vector					m_EdgeColorHSV;
		float					m_EdgeLuminanceLo;
		float					m_EdgeLuminanceHi;
		Vector4					m_WatercolorParams;
		float					m_DisplacePct;
		float					m_EmissiveMax;
		float					m_Exposure;
		float					m_MinimapScalar;
		bool					m_MinimapRenderAll;
		SimpleString			m_BloomKernel;
		float					m_BloomRadius;
		float					m_BloomAspectRatio;
		Vector					m_BloomThreshold;
		float					m_BloomScalar;

		RosaWorldGen*			m_WorldGen;

		float					m_FogNear;
		float					m_FogFar;
		Vector4					m_FogColorNearHi;	// These are linear RGBA colors, inited from HSV
		Vector4					m_FogColorFarHi;	// Lo and Hi here refer to sun direction lo/hi, not height fog!
		Vector4					m_FogColorNearLo;
		Vector4					m_FogColorFarLo;
		// clamp ends of curve, e.g. near point = 0.2 -> f(0.4) = lerp(n, f, saturate(invlerp(0.4, 0.2, 1.0))) = lerp(n, f, 0.25)
		Vector4					m_FogNearFarCurve;	// x = near point, y = far point, z = 1/(far-near), w = exp
		Vector4					m_FogLoHiCurve;		// x = lo point, y = hi point, z = 1/(hi-lo), w = exp

		float					m_HeightFogLo;
		float					m_HeightFogHi;
		float					m_HeightFogExp;
		float					m_HeightFogLoExp;
		float					m_HeightFogHiExp;

		float					m_FogLightDensity;

		Vector					m_SunVector;
		Vector4					m_SkyColorHi;
		Vector4					m_SkyColorLo;
		float					m_SkylineViewScalar;

		Vector					m_WindSwayDirection;	// Config; direction of sway (XY only, normalized)
		float					m_WindSwayIntensity;	// Config; max sway offset
		float					m_WindSwayNoiseScalar;	// Config; time scalar for noise
		uint					m_WindSwayNoiseOctaves;	// Config; number of octaves in noise
		float					m_WindSwayNoiseOffset;	// Random time offset rolled at config time

		float					m_WindLiftIntensity;	// Config; max lift offset
		float					m_WindLiftNoiseScalar;	// Config; time scalar for noise
		uint					m_WindLiftNoiseOctaves;	// Config; number of octaves in noise
		float					m_WindLiftNoiseOffset;	// Random time offset rolled at config time

		float					m_WindFlapIntensity;	// Config; max flap offset
		float					m_WindFlapNoiseScalar;	// Config; time scalar for noise
		uint					m_WindFlapNoiseOctaves;	// Config; number of octaves in noise
		float					m_WindFlapNoiseOffset;	// Random time offset rolled at config time

		Vector4					m_WindPhaseTime;		// Vertex shader phase time scalar; x = sway, y = lift, z = flap, w = unused
		Vector4					m_WindPhaseSpace;		// Vertex shader phase space scalar; x = sway, y = lift, z = flap, w = unused

		SimpleString			m_Music;
		SimpleString			m_Ambience;
		SimpleString			m_Reverb;

		// ROSATODO: Possibly remove spawner overrides in favor of new campaign-based overrides (which would function the same)
		// Override room spawner defs
		typedef Map<HashedString, SimpleString> TSpawnerMap;
		TSpawnerMap				m_SpawnerOverrides;
	};

	// More specific tests used by Sweep
	bool		SweepHulls( const Segment& SweepSegment, const Vector& HalfExtents, const AABB& SegmentBox, CollisionInfo& Info ) const;
	bool		SweepEntities( const Segment& SweepSegment, const Vector& HalfExtents, const AABB& SegmentBox, CollisionInfo& Info ) const;

	void		InitializeConfig();
	void		InitializeWorldDefConfig( const SimpleString& WorldDefinitionName );
	void		InitializeFogConfig( const SimpleString& FogDefinitionName, SWorldDef& WorldDef );
	void		InitializePostConfig( const SimpleString& PostDefinitionName, SWorldDef& WorldDef );
	void		InitializeBloomConfig( const SimpleString& BloomDefinitionName, SWorldDef& WorldDef );
	void		InitializeSkyConfig( const SimpleString& SkyDefinitionName, SWorldDef& WorldDef, const bool CreateSkyMeshes );
	void		InitializeWindConfig( const SimpleString& WindDefinitionName, SWorldDef& WorldDef );
	void		AppendSpawnerOverrides( const SimpleString& DefinitionName, SWorldDef& WorldDef );

	void		PublishWorldProperties();
	void		DeleteWorldGeo();

	void		DeleteCubemaps();
	void		LoadEnvironmentCubemaps( RosaWorldCubemap& OutAlbedoCubemap, RosaWorldCubemap& OutEmissiveCubemap, const SimpleString& CubemapName );
	void		CreateSkyCubemap( const SimpleString& CubemapName, RosaWorldCubemap& SkyCubemap, const RosaWorldCubemap& ReferenceCubemap );
	void		BlendSkyIntoEnvironmentCubemap( const RosaWorldCubemap& SkyCubemap, RosaWorldCubemap& InOutEnvironmentCubemap );
	void		GenerateIrradianceFromCubemap( const RosaWorldCubemap& SourceCubemap, SVoxelIrradiance& AmbientIrradiance );
	void		RasterizeSkyLighting( const RosaWorldCubemap& EnvironmentCubemap, const RosaWorldCubemap& SkyCubemap, RosaWorldCubemap& SkyLightingCubemap );
	void		AddIrradianceFromCubemap( const RosaWorldCubemap& LightCubemap, SVoxelIrradiance& AmbientIrradiance, const float Scalar );
	void		AddIrradianceFromIrradianceCube( const SimpleString& CubemapName, const SimpleString& WorldIrradianceDef, SVoxelIrradiance& AmbientIrradiance );
	void		LightEnvironmentCubemap( const SVoxelIrradiance& AmbientIrradiance, RosaWorldCubemap& EnvironmentCubemap );
	Vector4		GetRegionFogScalar( const SimpleString& CubemapName );
	Vector4		GetIrradiance( const SVoxelIrradiance& AmbientIrradiance, const Vector& Direction );
	void		CopyCubemap( const RosaWorldCubemap& SourceCubemap, RosaWorldCubemap& DestCubemap );
	void		AddEmissiveCubemap( const RosaWorldCubemap& EmissiveCubemap, RosaWorldCubemap& EnvironmentCubemap );
	Vector4		SampleFog( const float NearFar, const float LoHi );
	Vector4		SampleFogMeshValues( const float Distance, const Vector4& FogMeshColor, const Vector4& FogMeshParams );
	void		BlendFogIntoCubemap( const SimpleString& CubemapName, RosaWorldCubemap& EnvironmentCubemap );

	RosaWorldCubemap*	GetCubemap( const SimpleString& CubemapName, const SimpleString& WorldIrradianceDef );

	bool				HasValidCurrentWorldDef() { return m_CurrentWorldDef != ""; }
	const SWorldDef&	GetCurrentWorldDef() const;

	SimpleString		GetSpawnerOverride( const SimpleString& OldSpawner ) const;

	void				CreateGeoMesh( const SimpleString& MeshName, const SimpleString& MaterialName, const bool CastsShadows, const HashedString& Mat, const Vector& Location, const Angles& Orientation, const float Scale );
	void				FinalizeGeoMeshes();
	void				MergeGeoMeshes();
	void				SortGeoMeshes();
	void				DeleteGeoMeshes();
	static void			ReadMeshCallback_SaveBuffers( void* pVoid, const SReadMeshBuffers& Buffers );
	void				SaveBuffers( const SReadMeshBuffers& Buffers );

	void				ConditionalCreateBoundingFogMesh( SSector& Sector, const SimpleString& RoomFilename );
	void				CreateFogMesh( SSector& Sector, const SimpleString& MeshName, const HashedString& FogMeshDefName, const Vector& Location, const Angles& Orientation, const float Scale );
	void				AddFogMesh( SSector& Sector, RosaMesh* const pFogMesh, const HashedString& FogMeshDefName );
	void				InitializeFogMesh( Mesh* const pFogMesh );
	void				UpdateFogMeshes();
	void				DeleteFogMeshes();

	void				ConditionalCreateBoundingAmbientLight( SSector& Sector, const SimpleString& RoomFilename );
	void				CreateAmbientLight( SSector& Sector, const SimpleString& MeshName, const SimpleString& CubemapName, const SConvexHull& Hull, const Vector& Location, const Angles& Orientation, const float Scale );
	void				AddAmbientLight( SSector& Sector, Mesh* const pAmbientLightMesh, const SConvexHull& Hull, const SimpleString& CubemapName );
	void				InitializeAmbientLightMesh( Mesh* const pAmbientLightMesh, const SimpleString& CubemapName );
	void				UpdateAmbientLightMeshes();
	void				DeleteAmbientLightMeshes();

	void				GatherVisibleSectors( const View& Camera, const Plane& ViewPlane, const uint SectorIndex, const SRect& ViewBound, Array<uint>& OutVisibleSectors, const uint Depth ) const;
	bool				ClipPortal( const SSectorPortal& Portal, const View& Camera, const Plane& ViewPlane, const Matrix& ViewProjMat, const SRect& ViewBound, SRect& ClippedBound ) const;

	void				AddShadowVisibleSectors();
	bool				IsLocationInVisibleSector( const Vector& Location ) const;
	bool				DoesAABBIntersectVisibleSector( const AABB& Bounds ) const;
	bool				GetContainingSectorIndex( const Vector& Location, uint& OutSectorIndex ) const;
	bool				IsSectorVisibleFromVisibleSector( const uint SectorIndex ) const;
	const Array<uint>*	GetSectorVisIncidentals( const uint SectorIndexA, const uint SectorIndexB ) const;
	bool				GetIncidentalSectorsFromVisibleSectors( const uint SectorIndex, Array<uint>& OutSectorVisIncidentals ) const;	// Get any sectors in between visible sectors and the given sector; returns false if given sector is not visible from visible sectors

	void				TickWind();

	RosaFramework*		m_Framework;
	RosaNav*			m_Nav;

	Mesh*				m_SkyMesh;
	Mesh*				m_SkylineMesh;
	Mesh*				m_SkyLightMesh;

	SWorldDef			m_WorldDef;
	SimpleString		m_CurrentWorldDef;

	// Map from cubemap name to cubemap; the sky cubemap is always stored as "SKY" for subsequent blending
	Map<HashedString, RosaWorldCubemap*>	m_Cubemaps;

	// Map from fog mesh def name to cached properties
	Map<HashedString, SFogMeshDef>			m_FogMeshDefs;

	Array<SSector>							m_Sectors;
	Array<bool>								m_SectorVisMatrix;				// 2D array (sectors*sectors) indicating potential visibility. It seems worth the storage here to save this being a map lookup.
	Array<Map<uint, Array<uint> > >			m_SectorVisIncidentals;			// Array of sectors' sparse maps of sectors to sectors in between the two. Always index the lower sector first! Used for shadow casting.
	Array<uint>								m_VisibleSectors;				// Transient, gathered each render tick
	Array<uint>								m_SubVisibleSectors;			// Transient, sub-visible sectors gathered in a second pass to prevent iteratively adding each other
	Array<uint>								m_SubVisibleSectorIncidentals;	// Transient, used while gathering sub-visible sectors

	struct SGeoMesh
	{
		SGeoMesh()
		:	m_Mesh( NULL )
		,	m_Rendered( false )
		{
		}

		Mesh*	m_Mesh;
		bool	m_Rendered;	// Was rendered this frame?
	};
	Array<SGeoMesh>	m_GeoMeshes;

	typedef Map<uint, Mesh*> TMinimapMeshMap;
	TMinimapMeshMap	m_MinimapMeshes;
	Array<uint>		m_VisitedSectors;

	Array<SNavNode>	m_NavNodes;
	Array<SNavEdge>	m_NavEdges;

	// Sparse array of *relevant* entities only (e.g. doors matching kLockedDoorValue in nav props)
	// Maps from nav node index to the entity ref.
	Map<uint, WBEntityRef>	m_NavEntities;

	struct SSavedGeoMeshBuffers
	{
		SSavedGeoMeshBuffers()
		:	m_Positions()
		,	m_Colors()
		,	m_UVs()
		,	m_Normals()
		,	m_NormalsB()
		,	m_Tangents()
		,	m_Indices()
		{
		}

		Array<Vector>	m_Positions;
		Array<Vector4>	m_Colors;	// For foliage
		Array<Vector2>	m_UVs;
		Array<Vector>	m_Normals;
		Array<Vector>	m_NormalsB;	// For foliage
		Array<Vector4>	m_Tangents;
		Array<index_t>	m_Indices;
	};

	// ROSANOTE: For the time being, I only care about textures and material flags.
	// Any geo meshes with these in common *should* also have the same shaders, the same SDP, etc.
	struct SMaterialMatcher
	{
		SMaterialMatcher()
		:	m_Flags( 0 )
		,	m_VertexSignature( 0 )
		,	m_Albedo( NULL )
		,	m_Normal( NULL )
		,	m_Spec( NULL )
		,	m_Overlay( NULL )
		{
		}

		bool operator==( const SMaterialMatcher& Other ) const
		{
			return
				m_Flags == Other.m_Flags &&
				m_VertexSignature == Other.m_VertexSignature &&
				m_Albedo == Other.m_Albedo &&
				m_Normal == Other.m_Normal &&
				m_Spec == Other.m_Spec &&
				m_Overlay == Other.m_Overlay;
		}

		bool operator<( const SMaterialMatcher& Other ) const
		{
			if( m_Flags < Other.m_Flags )
			{
				return true;
			}

			if( m_Flags > Other.m_Flags )
			{
				return false;
			}

			if( m_VertexSignature < Other.m_VertexSignature )
			{
				return true;
			}

			if( m_VertexSignature > Other.m_VertexSignature )
			{
				return false;
			}

			if( m_Albedo < Other.m_Albedo )
			{
				return true;
			}

			if( m_Albedo > Other.m_Albedo )
			{
				return false;
			}

			if( m_Normal < Other.m_Normal )
			{
				return true;
			}

			if( m_Normal > Other.m_Normal )
			{
				return false;
			}

			if( m_Spec < Other.m_Spec )
			{
				return true;
			}

			if( m_Spec > Other.m_Spec )
			{
				return false;
			}

			if( m_Overlay < Other.m_Overlay )
			{
				return true;
			}

			return false;
		}

		uint	m_Flags;
		uint	m_VertexSignature;
		void*	m_Albedo;
		void*	m_Normal;
		void*	m_Spec;
		void*	m_Overlay;
	};

	struct SGeoMeshMergeBucket
	{
		SGeoMeshMergeBucket()
		:	m_NumVerts( 0 )
		,	m_Meshes()
		{
		}

		uint		m_NumVerts;
		Array<uint>	m_Meshes;
	};

	// Map from mesh name to buffers; this is kept permanently buffered to mirror what the dynamic mesh manager does!
	Map<SimpleString, SSavedGeoMeshBuffers>	m_SavedGeoMeshBuffers;

	// Parallel array to m_GeoMeshes, for lookup in m_SavedGeoMeshBuffers; redundant, but lets me dump it after merge
	Array<SimpleString>						m_GeoMeshNames;
	// Parallel array to m_GeoMeshes, for assigning material to new mesh
	Array<SimpleString>						m_GeoMeshMaterials;
};

#endif // ROSAWORLD_H
