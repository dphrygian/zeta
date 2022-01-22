#ifndef ROSAWORLDGEN_H
#define ROSAWORLDGEN_H

#include "simplestring.h"
#include "array.h"
#include "set.h"
#include "rosaworld.h"	// For typedefs
#include "angles.h"
#include "vector.h"
#include "rosaroom.h"
#include "segment.h"
#include "clock.h"

class RosaWorldGen
{
public:
	RosaWorldGen();
	~RosaWorldGen();

	// All configurations can be obtained through the ordered application of these transforms
	// Rotations are CCW
	enum ERoomXform
	{
		ERT_None	= 0x0,
		ERT_Rot90	= 0x1,
		ERT_Rot180	= 0x2,
		ERT_Rot270	= ERT_Rot90 | ERT_Rot180,
		ERT_MAX,
	};

	// DLP 2 May 2019: Added for sector-sector vis stuff, also using for connective rooms graph stuff.
	// Most other portal stuff uses EPortalIndex (from rosaworld.h).
	enum EPortalBits
	{
		EPB_None		= 0x00,
		EPB_East		= 0x01,
		EPB_West		= 0x02,
		EPB_North		= 0x04,
		EPB_South		= 0x08,
		EPB_Up			= 0x10,
		EPB_Down		= 0x20,
		EPB_Horizontal	= EPB_East | EPB_West | EPB_North | EPB_South,
		EPB_Vertical	= EPB_Up | EPB_Down,
		EPB_All			= EPB_East | EPB_West | EPB_North | EPB_South | EPB_Up | EPB_Down,
	};

	// I've repurposed this in a few places as an int vector, maybe just rename it that.
	struct SRoomLoc
	{
		SRoomLoc()
		:	X( 0 )
		,	Y( 0 )
		,	Z( 0 )
		{
		}

		SRoomLoc( int _X, int _Y, int _Z )
		:	X( _X )
		,	Y( _Y )
		,	Z( _Z )
		{
		}

		bool operator==( const SRoomLoc& Other ) const
		{
			return
				X == Other.X &&
				Y == Other.Y &&
				Z == Other.Z;
		}

		bool operator<( const SRoomLoc& Other ) const
		{
			if( X < Other.X )
			{
				return true;
			}

			if( X > Other.X )
			{
				return false;
			}

			if( Y < Other.Y )
			{
				return true;
			}

			if( Y > Other.Y )
			{
				return false;
			}

			if( Z < Other.Z )
			{
				return true;
			}

			return false;
		}

		int X;
		int Y;
		int Z;
	};

	struct SSectorPortal
	{
		SSectorPortal()
		:	m_LocationLo()
		,	m_LocationHi()
		,	m_PortalIndex( 0 )
		,	m_BackSector( 0 )
		{
		}

		SRoomLoc	m_LocationLo;
		SRoomLoc	m_LocationHi;
		uint		m_PortalIndex;
		uint		m_BackSector;
	};

	// For restoring room modules in saved games (and now for world file rooms too)
	struct SModule
	{
		SModule()
		:	m_Filename()
		,	m_Location()
		,	m_Transform( RosaWorldGen::ERT_None )
		,	m_Portals()
		{
		}

		SimpleString			m_Filename;
		SRoomLoc				m_Location;
		uint					m_Transform;
		Array<SSectorPortal>	m_Portals;		// Portals with absolute location, back sectors, etc. We expect these to be coalesced by now.
	};

	void		Initialize( const SimpleString& WorldGenDefinitionName );

	const Array<SModule>&	GetModules() const { return m_Modules; }

	// If SimGeneration is true, the rooms are generated but not pushed to world. Stats will be gathered instead.
	void		Generate( const bool SimGeneration = false );
	void		GatherStats();

	// ROSANOTE: This strictly generates geo/nav/etc., *not* entities.
	// It is for loading a saved game with an existing layout.
	// ZETATODO: This has a lot of overlap with PopulateWorld, might
	// be nice to break that stuff out into a common function?
	void		GenerateFromModules( const Array<SModule>& Modules );

private:
	friend class RosaTools;

	struct SFittingRoom
	{
		SFittingRoom()
		:	m_RoomIndex( 0 )
		,	m_OriginLoc()
		,	m_LowLoc()
		,	m_HighLoc()
		,	m_Transform( 0 )
		,	m_FitWeight( 0.0f )
		,	m_NumLoops( 0 )
		,	m_NumNewOpenPortals( 0 )
		,	m_ForCriticalPath( false )
		,	m_ForDeadEnd( false )
		,	m_CriticalPathAlpha( 0.0f )
		,	m_ExpandPriority( 0 )
		{
		}

		uint		m_RoomIndex;
		SRoomLoc	m_OriginLoc;
		SRoomLoc	m_LowLoc;
		SRoomLoc	m_HighLoc;
		uint		m_Transform;
		float		m_FitWeight;
		uint		m_NumLoops;
		uint		m_NumNewOpenPortals;
		bool		m_ForCriticalPath;
		bool		m_ForDeadEnd;
		float		m_CriticalPathAlpha;
		int			m_ExpandPriority;		// See SPortal/SPortalDef; the priority of this fitting room inherited from its expanding portal.
	};

	struct SGenRoom
	{
		SGenRoom()
		:	m_RoomIndex( INVALID_ARRAY_INDEX )
		,	m_Transform( ERT_None )
		,	m_Location()
		,	m_Portals()
		,	m_ForCriticalPath( false )
		,	m_ForDeadEnd( false )
		,	m_CriticalPathAlpha( 0.0f )
		{
		}

		uint					m_RoomIndex;			// Index into m_Rooms; INVALID_ARRAY_INDEX means the room is invalid (possibly removed after the fact)
		uint					m_Transform;
		SRoomLoc				m_Location;
		Array<SSectorPortal>	m_Portals;				// Portals with absolute transform and back sectors set up, and typically immediately coalesced
		bool					m_ForCriticalPath;
		bool					m_ForDeadEnd;
		float					m_CriticalPathAlpha;	// Valid for non-crit path rooms too; this marks (roughly) the crit path alpha that provides access to this room (doesn't properly consider loops)
	};

	struct SOccupiedTile
	{
		SOccupiedTile()
		:	m_GenRoomIndex( 0 )
		,	m_Portals()
		{
		}

		uint		m_GenRoomIndex;
		SPortals	m_Portals;
	};

	struct SOpenPortal
	{
		SOpenPortal()
		:	m_Room()
		,	m_PortalIndex( 0 )
		{
		}

		SRoomLoc	m_Room;
		uint		m_PortalIndex;
	};

	struct SFeatureRoom
	{
		SFeatureRoom()
		:	m_CriticalPathT( 0.0f )
		,	m_Rooms()
		{
		}

		float		m_CriticalPathT;	// For critical path, how far along the path does feature appear
		Array<uint>	m_Rooms;			// Which rooms satisfy this feature; indexes into RosaWorldGen::m_Rooms
	};

	// Abstraction of a room for generation; might share the same room file with another SRoom,
	// but be used for a different purpose (crit path vs. fill room, etc.).
	struct SRoom
	{
		SRoom()
		:	m_RoomDataKey()
		,	m_DeadEnd( false )
		,	m_Priority( 0 )
		,	m_DefaultWeight( 0.0f )
		,	m_Weight( 0.0f )
		,	m_Scale( 0.0f )
		,	m_Filename()
		,	m_MinTileZ( 0 )
		,	m_MaxTileZ( 0 )
#if BUILD_DEV
		,	m_Qualifier()
		,	m_Break( false )
#endif
		{
		}

		HashedString	m_RoomDataKey;		// Keys into m_RoomDataMap; just the hashed version of m_Filename so I don't rehash it all over the place
		bool			m_DeadEnd;			// If true, this room can be added beyond MaxBranchDistance even if it actually opens new portals
		int				m_Priority;			// Order of expansion, always use highest priority available from fitting rooms
		float			m_DefaultWeight;
		float			m_Weight;
		float			m_Scale;			// Simple hysteresis, multiply weight by this factor every time the room is used
		SimpleString	m_Filename;
		int				m_MinTileZ;			// HACKHACK for Zeta
		int				m_MaxTileZ;			// HACKHACK for Zeta
#if BUILD_DEV
		SimpleString	m_Qualifier;		// To differentiate how a room is used (e.g., a room used for the common set, the crit path, and a feature room would have three SRoom entries)
		bool			m_Break;			// Break when room is added, for testing
#endif
	};

	struct SConnectiveNode
	{
		SConnectiveNode()
		:	m_Location()
		,	m_ParentIndex( INVALID_ARRAY_INDEX )
		,	m_ParentPortalIndex( EPI_MAX )
		,	m_OpenPortalBits( EPB_None )
		,	m_ClosedPortalBits( EPB_None )
		{
		}

		SRoomLoc	m_Location;
		uint		m_ParentIndex;			// Index into (local) ConnectiveNodes array, invalid for root node
		uint		m_ParentPortalIndex;	// (EPortalIndex) The direction the parent followed to reach this node
		uint		m_OpenPortalBits;		// (EPortalBits) Directions we have not yet considered from this node
		uint		m_ClosedPortalBits;		// (EPortalBits) Directions we have considered and are valid (will be pruned)
	};

	struct SConnectiveRoomFit
	{
		SConnectiveRoomFit()
		:	m_Transform( RosaWorldGen::ERT_None )
		,	m_RoomIndex( INVALID_ARRAY_INDEX )
		{
		}

		uint		m_Transform;
		uint		m_RoomIndex;
	};

	struct SWorldFileRoom
	{
		SWorldFileRoom()
		:	m_Location()
		,	m_Transform( RosaWorldGen::ERT_None )
		,	m_RoomIndex( 0 )
		{
		}

		SRoomLoc	m_Location;
		uint		m_Transform;
		uint		m_RoomIndex;	// Index into m_Rooms
	};

	struct SWorldFile
	{
		SWorldFile()
		:	m_Bounds()
		,	m_Rooms()
		,	m_Weight( 0.0f )
		,	m_Filename()
		{
		}

		SRoomLoc				m_Bounds;
		Array<SWorldFileRoom>	m_Rooms;
		float					m_Weight;	// TODO: Apply some hysteresis from campaign?
		SimpleString			m_Filename;
	};

	struct SPreparedSpawn
	{
		SPreparedSpawn()
		:	m_EntityDef()
		,	m_SpawnLocationBase()
		,	m_SpawnLocationOffset()
		,	m_SpawnOrientation()
		,	m_LinkUID( 0 )
		,	m_LinkedSpawns()
		,	m_SpawnPriority( 0 )
		,	m_CriticalPathAlpha( 0.0f )
		,	m_SpawnedEntity( NULL )
#if BUILD_DEV
		,	m_RoomFilename()
#endif
		{
		}

		SimpleString	m_EntityDef;
		Vector			m_SpawnLocationBase;
		Vector			m_SpawnLocationOffset;	// ROSAHACK: Now stored separately so it can be scaled later
		Angles			m_SpawnOrientation;
		uint			m_LinkUID;				// Index of brush in room, plus index of room shifted left, for linking spawners globally
		Array<uint>		m_LinkedSpawns;			// Links to other prepared spawns, from linked brushes and their brush indices
		int				m_SpawnPriority;		// Higher numbers get spawned first
		float			m_CriticalPathAlpha;
		WBEntity*		m_SpawnedEntity;		// The entity spawned here, if any
#if BUILD_DEV
		SimpleString	m_RoomFilename;			// So I can log which room caused errors
#endif
	};

	static bool PrioritySort( const SPreparedSpawn& PreparedSpawnA, const SPreparedSpawn& PreparedSpawnB );
	static bool CriticalPathAlphaSort( const SPreparedSpawn& PreparedSpawnA, const SPreparedSpawn& PreparedSpawnB );

	// Group of potential spawns to be resolved according to some metric
	// (This has evolved from Eldritch to Rosa as needed.)
	struct SSpawnResolveGroup
	{
		SSpawnResolveGroup()
		:	m_Priority( 0 )
		,	m_ResolveLimitLowThreat( 0.0f )
		,	m_ResolveLimitHighThreat( 0.0f )
		,	m_ResolveChanceLowThreat( 0.0f )
		,	m_ResolveChanceHighThreat( 0.0f )
		,	m_MinRadiusSqFromPlayer( 0.0f )
		,	m_CriticalPathAlphaLo( 0.0f )
		,	m_CriticalPathAlphaHi( 0.0f )
		,	m_CriticalPathSampling( false )
		,	m_SpawnNearGroup()
		,	m_MinZ( 0.0f )
		,	m_MaxZ( 0.0f )
		,	m_PreparedSpawns()
		,	m_SpawnedLocations()
		{
		}

		int						m_Priority;					// Order resolve groups are spawned in (low priority spawns first, null group always preempts regardless of priority)
		float					m_ResolveLimitLowThreat;	// How many prepared spawns are actually instantiated?
		float					m_ResolveLimitHighThreat;	// How many prepared spawns are actually instantiated?
		float					m_ResolveChanceLowThreat;	// What fraction of prepared spawns are actually instantiated?
		float					m_ResolveChanceHighThreat;	// What fraction of prepared spawns are actually instantiated?
		float					m_MinRadiusSqFromPlayer;	// How close to the player members of this group may spawn
		float					m_CriticalPathAlphaLo;		// How near to the start of the crit path members of this group may spawn
		float					m_CriticalPathAlphaHi;		// How near to the end of the crit path members of this group may spawn
		bool					m_CriticalPathSampling;		// For spawning with systematic sampling instead of randomly
		HashedString			m_SpawnNearGroup;			// Resolve group name to spawn near
		float					m_MinZ;
		float					m_MaxZ;
		Array<SPreparedSpawn>	m_PreparedSpawns;
		Array<Vector>			m_SpawnedLocations;			// Where we actually spawned this group, for other things to spawn near
	};


	void		AppendResolveGroups( const SimpleString& DefinitionName );
	void		InitializeFeatureRoomDef( const SimpleString& DefinitionName );
	// ConfigPrefix is added to the config keys for properties of these rooms.
	// QualifierPrefix is only used in dev builds, to distinguish between different uses of the same room files.
	void		InitializeRooms( Array<uint>* pRooms, const uint NumRooms, const SimpleString& ConfigPrefix, const SimpleString& QualifierPrefix, const SimpleString& DefinitionName );
	void		InitializeFixedRoom( const SimpleString& Filename );
	void		InitializeWorldFile( SWorldFile& WorldFile, const IDataStream& Stream );
	SRoom&		InitializeRoom( const SimpleString& Filename );

	void			ConditionalLoadRoomData( const SimpleString& RoomFilename );	// Load room data if it isn't already in map
	const RosaRoom&	GetRoomData( const SRoom& Room ) const;							// Get room data, asserting if it's not in the map

	// Entry point for room generation; will be called until it returns true.
	bool			GenerateRooms();

	void			SelectWorldFile( const uint WorldFileIndex );	// Pick explicitly and populate bounds and modules
	void			SelectRandomWorldFile();						// Do random weighted selection

	// Seed functions used by GenerateRooms.
	bool			InsertWorldFileRooms();		// Add bespoke rooms from a world file, without validation or any random element.
	bool			InsertSeedFeatureRoom();	// Add random starting room from m_FeatureRooms (with CriticalPathT = 0.0).

	// Fill functions used by GenerateRooms.
	// ZETATODO: This is where I might try alternative planners.
	// Currently, critical path rooms will be skipped if there is a world file.
	// I may want to change that rule for this game so I can define a world's shape
	// with the world file but its high-level structure with an algorithm.
	// Or I may use world files to define high-level structures, and let the
	// random fill distinguish the final products.
	bool			InsertCriticalPathRooms();	// Add random rooms from m_CritPathRooms or m_FeatureRooms, in linear fashion, with validation, and without turning 180 degrees.
	bool			InsertFillRooms();			// Add random rooms from m_StandardRooms, with validation, until all remaining open portals are closed.
	bool			InsertConnectiveRooms();	// Add random rooms from m_ConnectiveRooms to fit a pruned maze expansion until all remaining open portals are closed.

	bool			CheckRoomConnections();		// Validate that all generated rooms are interconnected

	// Functions for finding and selecting rooms, used by InsertCriticalPathRooms and InsertFillRooms.
	// FindFittingRooms: gather all possible fits for the given room and add to the array of fitting rooms.
	// FilterFittingRooms: cull any fitting rooms that are strictly worse picks than others (based on priority or structure).
	// SelectAndInsertFittingRoom: do a weighted selection from the (usually pre-filtered) fitting rooms array, and add it to the world.
	void			FindFittingRooms(
		const uint RoomIndex,
		const SOpenPortal& OpenPortal,
		const bool ForCriticalPath,
		const uint MinNewOpenPortals,
		const float CriticalPathAlpha,
		Array<SFittingRoom>& OutFittingRooms
		) const;
	void			FilterFittingRooms( Array<SFittingRoom>& InOutFittingRooms ) const;
	void			SelectAndInsertFittingRoom( const Array<SFittingRoom>& FittingRooms );

	// Heavy lifting of room generation happens in here.
	// ValidateRoom: return true if the room is a valid fit at the given placement. Also return info about loops and new open portals.
	// RevalidateFittingRoom: return true if the room still fits; bypasses some checks that are guaranteed to be true, and updates info and loops and portals.
	bool			ValidateRoom(
		const SRoom& Module,
		const SRoomLoc& OriginLoc,
		const SRoomLoc& LowLoc,
		const SRoomLoc& HighLoc,
		const uint Transform,
		const bool Revalidating,
		const bool ForCriticalPath,
		const uint MinNewOpenPortals,
		uint& OutNumLoops,
		uint& OutNumNewOpenPortals
		) const;
	bool			RevalidateFittingRoom( SFittingRoom& FittingRoom, const bool ForCriticalPath, const uint MinNewOpenPortals ) const;
	float			GetDistanceSqToCriticalPath( const Vector& TileLoc ) const;		// Used by ValidateRoom; currently ignored if using a world file, in favor of a cheaper bounds test.
	float			GetFittingRoomWeight( const SFittingRoom& FittingRoom ) const;	// Used by SelectAndInsertFittingRoom for weighted random selection.

	bool			IsLocationWithinWorldBounds( const SRoomLoc& RoomLoc ) const;
	bool			IsLocationWithinWorldBounds( const SRoomLoc& RoomLoc, const SRoomLoc& WorldBounds ) const;
	bool			IsLocationWithinWorldBounds( const int X, const int Y, const int Z, const SRoomLoc& WorldBounds ) const;

	// Add a room to the world (m_GeneratedRooms). Used by many functions above.
	// Has no return values or out parameters that need to be handled; all side effects are done by setting member variables.
	void			InsertRoom( const uint RoomIndex, const SRoomLoc& LowLoc, const uint Transform, const bool ForCriticalPath, const bool ForDeadEnd, const float CriticalPathAlpha );
	void			RemoveGenRoomByIndex( const uint GenRoomIndex );

	void			SetPortals( const SPortals& InPortals, const uint Transform, SPortals& OutPortals ) const;
	uint			OpenPortals( const SRoomLoc& RoomLoc );
	uint			ClosePortals( const SRoomLoc& RoomLoc );
	void			RemoveOpenPortal( const SRoomLoc& RoomLoc, const uint PortalIndex );
	uint			GetComplementaryPortalIndex( const uint PortalIndex ) const;
	uint			GetPortalBitsForPortalIndex( const uint PortalIndex ) const;
	uint			GetPortalBitsFromValidPortals( const SPortals& Portals ) const;
	SRoomLoc		GetRoomLocThroughPortal( const SRoomLoc& RoomLoc, const uint PortalIndex ) const;
	uint			GetPortalTransform( const uint PortalIndex, const uint DesiredPortalIndex ) const;

	SOccupiedTile&			AddOccupiedTile( const SRoomLoc& RoomLoc );
	void					RemoveOccupiedTile( const SRoomLoc& RoomLoc );
	const SOccupiedTile&	GetOccupiedTile( const SRoomLoc& RoomLoc ) const;
	SOccupiedTile&			GetOccupiedTile( const SRoomLoc& RoomLoc );
	const SOccupiedTile*	SafeGetOccupiedTile( const SRoomLoc& RoomLoc ) const;
	bool					IsOccupied( const SRoomLoc& RoomLoc ) const;

	void			GetLowHighLocs( const RosaRoom& Room, const SRoomLoc& Location, const uint Transform, SRoomLoc& OutLowLoc, SRoomLoc& OutHighLoc ) const;
	void			GetOriginFromLowLocForTiles( const RosaRoom& Room, const SRoomLoc& LowLoc, const uint Transform, SRoomLoc& OutOrigin ) const;
	void			GetOriginFromLowLocForWorld( const RosaRoom& Room, const SRoomLoc& LowLoc, const uint Transform, SRoomLoc& OutOrigin ) const;
	void			GetTransformedOffset( const int X, const int Y, const int Z, const uint Transform, int& OutX, int& OutY, int& OutZ ) const;
	Vector			GetLocationForRoomLoc( const SRoomLoc& RoomLoc ) const;
	Angles			GetAnglesForTransform( const uint Transform ) const;

	// Called at the start of GenerateRooms, and again at the end of PopulateWorld.
	// Should put things in a clean state and also get rid of arrays we don't need
	// after generation. Anything which persists between calls to GenerateRooms
	// (i.e., when generation fails and we have to retry) should *not* be cleaned
	// up in here. For example, m_WorldFileIndex is not reset in here, but at the
	// top of Generate instead.
	void			CleanUp();

	// Build an actual game world from the generated rooms (or gather stats about used rooms, if m_SimmingGeneration)
	void			PopulateWorld();

	// Add portal info (including absolute loction) to generated rooms.
	// These are not yet coalesced. (See CoalescePortals; it probably could be done now, tho?)
	// (Or maybe I could wait and do this just before coalescing, to make simming generation cheaper?)
	void			CreateAndCoalesceGenRoomPortals();
	void			CreateGenRoomPortals( SGenRoom& GenRoom );
	void			CoalesceGenRoomPortals( Array<SSectorPortal>& Portals );

	void			StoreModules();		// For storing a record of the world for rebuilding after load
	void			CreateGeoFromRoom( const RosaRoom& Room, const uint Transform, const SRoomLoc& RoomLoc, const SimpleString& RoomFilename );
	void			CreateNavMeshFromRoom( const RosaRoom& Room, const uint Transform, const SRoomLoc& RoomLoc );
	void			LinkNavMesh();
	void			CreateMinimapMeshes();
	bool			AreEdgesCoincident( const SNavEdge& EdgeA, const SNavEdge& EdgeB ) const;
	bool			IsEdgeCoincidentWithNode( const SNavEdge& EdgeA, const SNavNode& NodeB ) const;
	void			CreatePortalsForSector( const uint SectorIndex, const Array<SSectorPortal>& Portals ) const;
	bool			IsSectorVisibleFromSector( const uint SectorIndexA, const uint SectorIndexB, const Array<Array<SSectorPortal> >& PortalsArrays, Array<uint>& OutSectorVisIncidentals ) const;
	void			BuildSectorVisMatrix( const Array<Array<SSectorPortal> >& PortalsArrays ) const;
	void			AddGeoMeshesToSectors() const;
	void			PrepareSpawnEntitiesFromRoom( const RosaRoom& Room, const uint GenRoomIndex, const uint Transform, const SRoomLoc& RoomLoc, const bool ForCriticalPath, const bool ForDeadEnd, const float CriticalPathAlpha );
	void			PrepareSpawnEntity( const RosaRoom& Room, const uint GenRoomIndex, const uint RoomBrushIndex, const SimpleString& SpawnerDef, const HashedString& ResolveGroupName, const Vector& Location, const Angles& Orientation, const Array<uint>& LinkedBrushes, const uint Transform, const SRoomLoc& RoomLoc, const float CriticalPathAlpha );
	SimpleString	GetActualSpawnerDef( const SimpleString& SuperSpawnerDef );
	void			FilterPreparedSpawns();
	void			ResolveEntitySpawns();
	void			ResolveEntitySpawnsInGroup( const HashedString& ResolveGroupName );
	void			LinkEntitySpawns();
	Vector			SpawnPreparedEntity( SPreparedSpawn& PreparedSpawn );	// HACKHACK: Return the location this was spawned at

#if BUILD_DEV
	void			ValidateSpawners();
	void			ValidateSpawnersInRoom( const SRoom& Room );
#endif

	RosaWorld*			m_World;

	// Legacy, for handcrafted levels instead of generated
	bool				m_IsFixedRoom;				// Is this world generated from a singular room file (probably deprecated, subsumed by world files)?

	// World files, for a mix of authored and procgen
	bool				m_IsUsingWorldFile;			// Is this world generated from a world file with some prescribed content?

	// Footprint of the smallest procgen room element; either config,
	// or saved into a world file. Must match the room data tiles.
	uint				m_TileSizeX;
	uint				m_TileSizeY;
	uint				m_TileSizeZ;

	// Procgen algorithm parameters
	float				m_MaxBranchDistanceSq;		// Config; how far non-dead end rooms can be from crit path; ignored if m_IsUsingWorldFile
	float				m_ModuleUseScale;			// Config; hysteresis scalar when a room is used
	bool				m_HasCriticalPathDirection;
	uint				m_CriticalPathDirection;
	uint				m_CriticalPathLength;		// Config; number of rooms (I think? or tiles?) on crit path; ignored if m_IsUsingWorldFile
	Array<Vector>		m_CriticalPathLocations;	// Array of all occupied tiles on the crit path, for checking distance
	uint				m_MinRooms;					// Config
	uint				m_MaxRooms;					// Config
	uint				m_MinTiles;					// Config
	uint				m_MaxTiles;					// Config
	uint				m_MinLoops;					// Config
	uint				m_MaxLoops;					// Config
	uint				m_MinDeadEnds;				// Config
	uint				m_MaxDeadEnds;				// Config

	Array<SWorldFile>			m_WorldFiles;		// All possible world files, from which one will be rolled

	Map<HashedString, RosaRoom>	m_RoomDataMap;		// Room data (brushes, spawners, etc.) shared by SRooms regardless of their usage

	Array<SRoom>				m_Rooms;			// All rooms including world file rooms (one per instance!), feature rooms, connective rooms, etc.
	Array<uint>					m_WorldFileRooms;	// Indexes into m_Rooms; these will be unique instances for *all possible* world files in this worldgen def
	Array<uint>					m_StandardRooms;	// Indexes into m_Rooms; these will be unique instances
	Array<uint>					m_CritPathRooms;	// Indexes into m_Rooms; these will be unique instances even if crit path reuses standard room data
	Array<SFeatureRoom>			m_FeatureRooms;		// Internal array in SFeatureRoom indexes into m_Rooms; these will be unique instances
	Array<uint>					m_ConnectiveRooms;	// Indexes into m_Rooms; these will be unique instances

	// Precomputed map from one-tile portal configurations (EPortalBits) to the rooms/transforms that fit them.
	Map<uint, SConnectiveRoomFit>	m_ConnectiveRoomFits;

	// Sparse array of occupied tiles
	Map<SRoomLoc, SOccupiedTile>	m_OccupiedTiles;
	uint							m_NumOccupiedTiles;	// Separate count, easier than iterating maps

	Array<SOpenPortal>	m_OpenPortals;
	uint				m_NumJustOpenedPortals;		// For critical path expansion, only consider the n most recently opened portals

	uint				m_NumLoops;
	uint				m_NumDeadEnds;

	uint					m_WorldFileIndex;		// Stored so we can retry the same world if generation fails (which ensures that world file weights are respected)

	Array<SGenRoom>		m_GeneratedRooms;			// All the rooms we have actually committed to the world.
	Array<SModule>		m_Modules;					// The version that we store so we can rebuild the world on load. (m_GeneratedRooms is reused for stats and stuff, so can't depend on it)

	Map<HashedString, SSpawnResolveGroup>	m_SpawnResolveGroups;

	Map<uint, WBEntity*>	m_LinkUIDEntities;	// Map from link UIDs (hash of room index and brush index) to spawned entities, for fixing up links

	Vector					m_PlayerStart;

	bool					m_SimmingGeneration;	// Generating rooms but not populating world with them; currently invoked by Alt + Backspace in RosaFramework

	// Stuff used in building sector vis matrix
	mutable Array<uint>				m_ValidDirections;
	mutable Array<Array<uint> >		m_BacktrackSectors;
	mutable Array<uint>				m_DFSStack;
	mutable Array<uint>				m_BacktrackStack;

#if BUILD_DEV
	bool					m_DebugWorldGen;			// Config (RosaWorldGen:DebugWorldGen), get verbose logging
	bool					m_AllowUnfinishedWorlds;	// Config (RosaWorldGen:AllowUnfinishedWorlds)
	bool					m_AllowUnconnectedWorlds;	// Config (RosaWorldGen:AllowUnconnectedWorlds)

	struct SWorldGenFailReasons
	{
		SWorldGenFailReasons()
		:	m_CriticalPath( 0 )
		,	m_CriticalPath_FeatureRoom( 0 )
		,	m_FillRooms( 0 )
		,	m_Connections( 0 )
		,	m_Loops( 0 )
		,	m_DeadEnds( 0 )
		,	m_Size( 0 )
		{
		}

		void Reset()
		{
			m_CriticalPath				= 0;
			m_CriticalPath_FeatureRoom	= 0;
			m_FillRooms					= 0;
			m_Connections				= 0;
			m_Loops						= 0;
			m_DeadEnds					= 0;
			m_Size						= 0;
		}

		SWorldGenFailReasons& operator+=( const SWorldGenFailReasons& FailReasons )
		{
			m_CriticalPath				+= FailReasons.m_CriticalPath;
			m_CriticalPath_FeatureRoom	+= FailReasons.m_CriticalPath_FeatureRoom;
			m_FillRooms					+= FailReasons.m_FillRooms;
			m_Connections				+= FailReasons.m_Connections;
			m_Loops						+= FailReasons.m_Loops;
			m_DeadEnds					+= FailReasons.m_DeadEnds;
			m_Size						+= FailReasons.m_Size;
			return *this;
		}

		// ROSATODO: Make a tree that lets me just push the leaf reason and then sum them?
		uint				m_CriticalPath;
		uint				m_CriticalPath_FeatureRoom;
		uint				m_FillRooms;
		uint				m_Connections;
		uint				m_Loops;
		uint				m_DeadEnds;
		uint				m_Size;
	};
	SWorldGenFailReasons	m_FailReasons;
	Map<SPortal, uint>		m_Failed_OpenPortals;	// Count the type of portals that are failing

	// Stats for simming generations
	typedef Map<HashedString, uint> TStatsMap;
	struct SWorldGenStats
	{
		SWorldGenStats()
		:	m_RoomStats()
		,	m_WorldFileIndex( INVALID_ARRAY_INDEX )
		,	m_NumGenerations( 0 )
		,	m_RoomsSum( 0 )
		,	m_FillRoomsSum( 0 )
		,	m_ConnRoomsSum( 0 )
		,	m_FailsSum( 0 )
		,	m_FailsMax( 0 )
		,	m_StatsClock( 0 )
		,	m_ClockMax( 0 )
		{
		}

		void Reset()
		{
			m_RoomStats.Clear();
			m_WorldFileIndex	= INVALID_ARRAY_INDEX;
			m_NumGenerations	= 0;
			m_RoomsSum			= 0;
			m_FillRoomsSum		= 0;
			m_ConnRoomsSum		= 0;
			m_FailsSum			= 0;
			m_FailsMax			= 0;
			m_StatsClock		= 0;
			m_ClockMax			= 0;
			m_FailReasons.Reset();
		}

		TStatsMap				m_RoomStats;			// Map from qualified room name to count of uses
		uint					m_WorldFileIndex;		// Ignored for global stats
		uint					m_NumGenerations;		// Total number of generations for this stats block
		uint					m_RoomsSum;				// Total number of rooms generated across all simming (world file rooms, fill rooms, and connective rooms)
		uint					m_FillRoomsSum;			// Total number of fill rooms generated across all simming
		uint					m_ConnRoomsSum;			// Total number of connective rooms generated across all simming
		uint					m_FailsSum;				// Total number of failures across all simming
		uint					m_FailsMax;				// Max failures on any one sim
		CLOCK_T					m_StatsClock;			// Total time spent simming
		CLOCK_T					m_ClockMax;				// Max time spent on any one sim
		SWorldGenFailReasons	m_FailReasons;
	};
	SWorldGenStats				m_GlobalStats;
	Array<SWorldGenStats>		m_WorldFileStats;		// Only valid if m_IsUsingWorldFile
	SimpleString				m_WorldGenDef;			// Saved from initialization for output filename

	void		ResetStats();
	void		PrintStats( const IDataStream& Stream );
	void		PrintStats( const IDataStream& Stream, const SWorldGenStats& WorldGenStats );

	// For debugging purposes
	SimpleString			m_CurrentRoomFilename;
#endif
};

#endif // ROSAWORLDGEN_H
