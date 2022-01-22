#include "core.h"
#include "rosaworldgen.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "rosaworld.h"
#include "rosaroom.h"
#include "packstream.h"
#include "mathfunc.h"
#include "reversehash.h"
#include "filestream.h"
#include "wbworld.h"
#include "wbentity.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcomprosapatrolpath.h"
#include "mathcore.h"
#include "matrix.h"
#include "wbeventmanager.h"
#include "collisioninfo.h"
#include "fileutil.h"
#include "rosagame.h"
#include "wbparamevaluator.h"
#include "mesh.h"
#include "rosacampaign.h"
#include "irenderer.h"
#include "ivertexdeclaration.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "multimap.h"
#include "timedate.h"

// ROSANOTE: The DEBUG_WORLDGEN definition has been moved to a RosaWorldGen:DebugWorldGen config var
#define VERBOSE_WORLDGEN	0

RosaWorldGen::RosaWorldGen()
:	m_World( NULL )
,	m_IsFixedRoom( false )
,	m_IsUsingWorldFile( false )
,	m_TileSizeX( 0 )
,	m_TileSizeY( 0 )
,	m_TileSizeZ( 0 )
,	m_MaxBranchDistanceSq( 0.0f )
,	m_ModuleUseScale( 0.0f )
,	m_HasCriticalPathDirection( false )
,	m_CriticalPathDirection( 0 )
,	m_CriticalPathLength( 0 )
,	m_CriticalPathLocations()
,	m_MinRooms( 0 )
,	m_MaxRooms( 0 )
,	m_MinTiles( 0 )
,	m_MaxTiles( 0 )
,	m_MinLoops( 0 )
,	m_MaxLoops( 0 )
,	m_MinDeadEnds( 0 )
,	m_MaxDeadEnds( 0 )
,	m_WorldFiles()
,	m_RoomDataMap()
,	m_Rooms()
,	m_WorldFileRooms()
,	m_StandardRooms()
,	m_CritPathRooms()
,	m_FeatureRooms()
,	m_ConnectiveRooms()
,	m_ConnectiveRoomFits()
,	m_OccupiedTiles()
,	m_NumOccupiedTiles( 0 )
,	m_OpenPortals()
,	m_NumJustOpenedPortals( 0 )
,	m_NumLoops( 0 )
,	m_NumDeadEnds( 0 )
,	m_WorldFileIndex( 0 )
,	m_GeneratedRooms()
,	m_Modules()
,	m_SpawnResolveGroups()
,	m_LinkUIDEntities()
,	m_PlayerStart()
,	m_SimmingGeneration( false )
,	m_ValidDirections()
,	m_BacktrackSectors()
,	m_DFSStack()
,	m_BacktrackStack()
#if BUILD_DEV
,	m_DebugWorldGen( false )
,	m_AllowUnfinishedWorlds( false )
,	m_AllowUnconnectedWorlds( false )
,	m_FailReasons()
,	m_Failed_OpenPortals()
,	m_GlobalStats()
,	m_WorldFileStats()
,	m_WorldGenDef()
,	m_CurrentRoomFilename()
#endif
{
	m_ValidDirections.SetDeflate( false );
	m_BacktrackSectors.SetDeflate( false );
	m_DFSStack.SetDeflate( false );
	m_BacktrackStack.SetDeflate( false );
}

RosaWorldGen::~RosaWorldGen()
{
}

void RosaWorldGen::Initialize( const SimpleString& WorldGenDefinitionName )
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaCampaign* const		pCampaign	= RosaCampaign::GetCampaign();
	ASSERT( pCampaign );

	m_World = pFramework->GetWorld();
	ASSERT( m_World );

#if BUILD_DEV
	m_WorldGenDef = WorldGenDefinitionName;

	STATICHASH( RosaWorldGen );
	STATICHASH( DebugWorldGen );
	m_DebugWorldGen = ConfigManager::GetBool( sDebugWorldGen, false, sRosaWorldGen );

	STATICHASH( AllowUnfinishedWorlds );
	m_AllowUnfinishedWorlds = ConfigManager::GetBool( sAllowUnfinishedWorlds, false, sRosaWorldGen );

	STATICHASH( AllowUnconnectedWorlds );
	m_AllowUnconnectedWorlds = ConfigManager::GetBool( sAllowUnconnectedWorlds, false, sRosaWorldGen );
#endif

	MAKEHASH( WorldGenDefinitionName );

	STATICHASH( FixedRoom );
	const SimpleString FixedRoomFilename = ConfigManager::GetInheritedString( sFixedRoom, "", sWorldGenDefinitionName );
	m_IsFixedRoom = ( FixedRoomFilename != "" );

	if( m_IsFixedRoom )
	{
		// WORLDTODO: Deprecate fixed room in favor of world files
		InitializeFixedRoom( FixedRoomFilename );
	}
	else
	{
		STATICHASH( TileSizeX );
		m_TileSizeX = ConfigManager::GetInheritedInt( sTileSizeX, 0, sWorldGenDefinitionName );

		STATICHASH( TileSizeY );
		m_TileSizeY = ConfigManager::GetInheritedInt( sTileSizeY, 0, sWorldGenDefinitionName );

		STATICHASH( TileSizeZ );
		m_TileSizeZ = ConfigManager::GetInheritedInt( sTileSizeZ, 0, sWorldGenDefinitionName );

		STATICHASH( NumWorldFiles );
		const uint NumWorldFiles = ConfigManager::GetInheritedInt( sNumWorldFiles, 0, sWorldGenDefinitionName );
		FOR_EACH_INDEX( WorldFileIndex, NumWorldFiles )
		{
			SWorldFile&	NewWorldFile	= m_WorldFiles.PushBack();
			NewWorldFile.m_Filename		= ConfigManager::GetInheritedSequenceString(	"WorldFile%d",			WorldFileIndex, "",		sWorldGenDefinitionName );
			NewWorldFile.m_Weight		= ConfigManager::GetInheritedSequenceFloat(		"WorldFile%dWeight",	WorldFileIndex, 1.0f,	sWorldGenDefinitionName );

			InitializeWorldFile( NewWorldFile, PackStream( NewWorldFile.m_Filename.CStr() ) );
		}
		m_IsUsingWorldFile = ( NumWorldFiles > 0 );

		DEVASSERT( m_TileSizeX * m_TileSizeY * m_TileSizeZ );

		STATICHASH( CriticalPathLength );
		m_CriticalPathLength = pCampaign->ModifyInt( sCriticalPathLength, ConfigManager::GetInheritedInt( sCriticalPathLength, 0, sWorldGenDefinitionName ) );
		DEVASSERT( m_IsUsingWorldFile || m_CriticalPathLength > 0 );

		STATICHASH( MinRooms );
		m_MinRooms	= pCampaign->ModifyInt( sMinRooms, ConfigManager::GetInheritedInt( sMinRooms, 0, sWorldGenDefinitionName ) );
		DEVASSERT( m_MinRooms == 0 || m_MinRooms >= m_CriticalPathLength );

		STATICHASH( MaxRooms );
		m_MaxRooms	= pCampaign->ModifyInt( sMaxRooms, ConfigManager::GetInheritedInt( sMaxRooms, 0, sWorldGenDefinitionName ) );

		STATICHASH( MinTiles );
		m_MinTiles	= pCampaign->ModifyInt( sMinTiles, ConfigManager::GetInheritedInt( sMinTiles, 0, sWorldGenDefinitionName ) );

		STATICHASH( MaxTiles );
		m_MaxTiles	= pCampaign->ModifyInt( sMaxTiles, ConfigManager::GetInheritedInt( sMaxTiles, 0, sWorldGenDefinitionName ) );

		STATICHASH( MinLoops );
		m_MinLoops	= pCampaign->ModifyInt( sMinLoops, ConfigManager::GetInheritedInt( sMinLoops, 0, sWorldGenDefinitionName ) );

		STATICHASH( MaxLoops );
		m_MaxLoops	= pCampaign->ModifyInt( sMaxLoops, ConfigManager::GetInheritedInt( sMaxLoops, 0, sWorldGenDefinitionName ) );

		STATICHASH( MinDeadEnds );
		m_MinDeadEnds	= pCampaign->ModifyInt( sMinDeadEnds, ConfigManager::GetInheritedInt( sMinDeadEnds, 0, sWorldGenDefinitionName ) );

		STATICHASH( MaxDeadEnds );
		m_MaxDeadEnds	= pCampaign->ModifyInt( sMaxDeadEnds, ConfigManager::GetInheritedInt( sMaxDeadEnds, 0, sWorldGenDefinitionName ) );

		STATICHASH( MaxBranchDistance );
		m_MaxBranchDistanceSq = Square( pCampaign->ModifyFloat( sMaxBranchDistance, ConfigManager::GetInheritedFloat( sMaxBranchDistance, 0.0f, sWorldGenDefinitionName ) ) );

		STATICHASH( ModuleUseScale );
		m_ModuleUseScale = pCampaign->ModifyFloat( sModuleUseScale, ConfigManager::GetInheritedFloat( sModuleUseScale, 1.0f, sWorldGenDefinitionName ) );

		// To simplify stats tracking and stuff, just skip crit path and feature rooms for now when using world files.
		// ZETATODO: I may revisit the notion of feature rooms later, but the current incarnation is from Vamp, where
		// they were only used to do start/end points, finale rooms, and the underground transition in Old Town. Not
		// nearly as useful and robust as Eldritch's, which is more what I'd want for Zeta.
		if( !m_IsUsingWorldFile )
		{
			STATICHASH( NumFeatureRoomDefs );
			const uint NumFeatureRoomDefs = ConfigManager::GetInheritedInt( sNumFeatureRoomDefs, 0, sWorldGenDefinitionName );
			for( uint FeatureRoomDefIndex = 0; FeatureRoomDefIndex < NumFeatureRoomDefs; ++FeatureRoomDefIndex )
			{
				const SimpleString FeatureRoomDefName = ConfigManager::GetInheritedSequenceString( "FeatureRoomDef%d", FeatureRoomDefIndex, "", sWorldGenDefinitionName );
				InitializeFeatureRoomDef( FeatureRoomDefName );
			}
		}

		STATICHASH( NumRooms );
		const uint NumRooms = ConfigManager::GetInheritedInt( sNumRooms, 0, sWorldGenDefinitionName );
		InitializeRooms( &m_StandardRooms, NumRooms, "", "", WorldGenDefinitionName );

		// Only use connective tissue with world files, only because I don't define a world bound anywhere else.
		if( m_IsUsingWorldFile )
		{
			STATICHASH( NumConnectiveRooms );
			const uint NumConnectiveRooms = ConfigManager::GetInheritedInt( sNumConnectiveRooms, 0, sWorldGenDefinitionName );
			// NOTE: Crit path rooms pass a Prefix here because they're in the same context as standard rooms.
			// This also means anything used as a crit path room can have different weighting/priority
			// than the same room in the standard set.
			InitializeRooms( &m_ConnectiveRooms, NumConnectiveRooms, "Connective", "Connective", WorldGenDefinitionName );
			DEBUGASSERT( m_ConnectiveRooms.Empty() || 5 == m_ConnectiveRooms.Size() );	// This is all the algorithm supports for now (5 horizontal permutations)

			// Precompute connective room fits
			FOR_EACH_ARRAY( ConnectiveRoomIter, m_ConnectiveRooms, uint )
			{
				const uint		ConnectiveRoomIndex	= ConnectiveRoomIter.GetValue();
				const SRoom&	ConnectiveRoom		= m_Rooms[ ConnectiveRoomIndex ];
				const RosaRoom&	ConnectiveRoomData	= GetRoomData( ConnectiveRoom );
				DEBUGASSERT( 1 == ConnectiveRoomData.m_TilesX );
				DEBUGASSERT( 1 == ConnectiveRoomData.m_TilesY );
				DEBUGASSERT( 1 == ConnectiveRoomData.m_TilesZ );
				DEBUGASSERT( 1 == ConnectiveRoomData.m_Portals.Size() );

				for( uint Transform = ERT_None; Transform <= ERT_MAX; ++Transform )
				{
					const SPortals&	RoomPortals		= ConnectiveRoomData.m_Portals[0];
					SPortals		TransformedPortals;
					SetPortals( RoomPortals, Transform, TransformedPortals );
					const uint		RoomPortalBits	= GetPortalBitsFromValidPortals( TransformedPortals );

					if( m_ConnectiveRoomFits.Contains( RoomPortalBits ) )
					{
						// NS and NSEW rooms can be oriented in 2 or 4 ways and serve the same fit.
						// Unlike in Eldritch, I don't need all variations here.
						continue;
					}

					SConnectiveRoomFit& RoomFit = m_ConnectiveRoomFits.Insert( RoomPortalBits );
					RoomFit.m_RoomIndex	= ConnectiveRoomIndex;
					RoomFit.m_Transform	= Transform;
				}
			}
		}

		// To simplify stats tracking and stuff, just skip crit path and feature rooms for now when using world files.
		if( !m_IsUsingWorldFile )
		{
			STATICHASH( NumCritPathRooms );
			const uint NumCritPathRooms = ConfigManager::GetInheritedInt( sNumCritPathRooms, 0, sWorldGenDefinitionName );
			// NOTE: Crit path rooms pass a Prefix here because they're in the same context as standard rooms.
			// This also means anything used as a crit path room can have different weighting/priority
			// than the same room in the standard set.
			InitializeRooms( &m_CritPathRooms, NumCritPathRooms, "CritPath", "CritPath", WorldGenDefinitionName );

			// Use the standard set for crit path rooms if we don't have a separate set
			if( m_CritPathRooms.Empty() )
			{
				// DLP 24 May 2020: Initialize a second set so they have their own data, don't just copy!
				// Use the normal config stuff (no ConfigPrefix) but qualify as CritPath for stats.
				InitializeRooms( &m_CritPathRooms, NumRooms, "", "CritPath", WorldGenDefinitionName );
			}

			// Remove anything from the crit path set that doesn't belong
			// (obviously, only useful if we're using the standard set)
			STATICHASH( NumProscribedCritPathRooms );
			const uint NumProscribedCritPathRooms = ConfigManager::GetInheritedInt( sNumProscribedCritPathRooms, 0, sWorldGenDefinitionName );
			for( uint ProscribedRoomIndex = 0; ProscribedRoomIndex < NumProscribedCritPathRooms; ++ProscribedRoomIndex )
			{
				uint RemovedCount = 0;
				const SimpleString RoomFilename = ConfigManager::GetInheritedSequenceString( "ProscribedCritPathRoom%d", ProscribedRoomIndex, "", sWorldGenDefinitionName );
				FOR_EACH_ARRAY_REVERSE( RoomIter, m_CritPathRooms, uint )
				{
					const uint		IterRoomIndex	= RoomIter.GetValue();
					const SRoom&	IterRoom		= m_Rooms[ IterRoomIndex ];
					if( IterRoom.m_Filename == RoomFilename )
					{
						m_CritPathRooms.Remove( RoomIter );
						++RemovedCount;
					}
				}

				if( 0 == RemovedCount )
				{
					PRINTF( "Proscribed room %s was not found in default room set for %s.\n", RoomFilename.CStr(), WorldGenDefinitionName.CStr() );
					WARNDESC( "Proscribed room was not found in default room set, see log for details" );
				}
			}
		}
	}

	// Add the null resolve group with a 100% chance
	SSpawnResolveGroup&	NullResolveGroup		= m_SpawnResolveGroups[ HashedString::NullString ];
	NullResolveGroup.m_ResolveChanceLowThreat	= 1.0f;
	NullResolveGroup.m_ResolveChanceHighThreat	= 1.0f;

	AppendResolveGroups( WorldGenDefinitionName );
	for( uint CampaignElementIndex = 0; CampaignElementIndex < pCampaign->GetElementCount(); ++CampaignElementIndex )
	{
		AppendResolveGroups( pCampaign->GetElementName( CampaignElementIndex ) );
	}
}

void RosaWorldGen::AppendResolveGroups( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	// ROSANOTE: Unlike Eldritch, any resolve groups not defined will default to spawning no entities.
	STATICHASH( NumResolveGroups );
	const uint NumResolveGroups = ConfigManager::GetInheritedInt( sNumResolveGroups, 0, sDefinitionName );
	for( uint ResolveGroupIndex = 0; ResolveGroupIndex < NumResolveGroups; ++ResolveGroupIndex )
	{
		const HashedString	GroupName				= ConfigManager::GetInheritedSequenceHash( "ResolveGroup%dName",	ResolveGroupIndex, HashedString::NullString,	sDefinitionName );

		SSpawnResolveGroup&	ResolveGroup			= m_SpawnResolveGroups[ GroupName ];

		float				DefaultResolveLimit		= ConfigManager::GetInheritedSequenceFloat( "ResolveGroup%dLimit",	ResolveGroupIndex, 0.0f,						sDefinitionName );
		float				DefaultResolveChance	= ConfigManager::GetInheritedSequenceFloat( "ResolveGroup%dChance",	ResolveGroupIndex, 0.0f,						sDefinitionName );

		WBParamEvaluator LimitPE;
		LimitPE.InitializeFromDefinition( ConfigManager::GetInheritedSequenceString( "ResolveGroup%dLimitPE", ResolveGroupIndex, "", sDefinitionName ) );
		if( LimitPE.IsInitialized() )
		{
			LimitPE.Evaluate( WBParamEvaluator::SPEContext() );
			DefaultResolveLimit = LimitPE.GetFloat();
		}

		WBParamEvaluator ChancePE;
		ChancePE.InitializeFromDefinition( ConfigManager::GetInheritedSequenceString( "ResolveGroup%dChancePE", ResolveGroupIndex, "", sDefinitionName ) );
		if( ChancePE.IsInitialized() )
		{
			ChancePE.Evaluate( WBParamEvaluator::SPEContext() );
			DefaultResolveChance = ChancePE.GetFloat();
		}

		ResolveGroup.m_Priority					= ConfigManager::GetInheritedSequenceInt(	"ResolveGroup%dPriority",			ResolveGroupIndex, 0,							sDefinitionName );

		ResolveGroup.m_ResolveLimitLowThreat	= ConfigManager::GetInheritedSequenceFloat( "ResolveGroup%dLimitLowThreat",		ResolveGroupIndex, DefaultResolveLimit,			sDefinitionName );
		ResolveGroup.m_ResolveLimitHighThreat	= ConfigManager::GetInheritedSequenceFloat( "ResolveGroup%dLimitHighThreat",	ResolveGroupIndex, DefaultResolveLimit,			sDefinitionName );
		ResolveGroup.m_ResolveChanceLowThreat	= ConfigManager::GetInheritedSequenceFloat( "ResolveGroup%dChanceLowThreat",	ResolveGroupIndex, DefaultResolveChance,		sDefinitionName );
		ResolveGroup.m_ResolveChanceHighThreat	= ConfigManager::GetInheritedSequenceFloat( "ResolveGroup%dChanceHighThreat",	ResolveGroupIndex, DefaultResolveChance,		sDefinitionName );

		ResolveGroup.m_MinRadiusSqFromPlayer	= Square( ConfigManager::GetInheritedSequenceFloat(	"ResolveGroup%dMinRadiusFromPlayer",	ResolveGroupIndex, 0.0f,	sDefinitionName ) );
		ResolveGroup.m_CriticalPathAlphaLo		= ConfigManager::GetInheritedSequenceFloat(			"ResolveGroup%dCriticalPathAlphaLo",	ResolveGroupIndex, 0.0f,	sDefinitionName );
		ResolveGroup.m_CriticalPathAlphaHi		= ConfigManager::GetInheritedSequenceFloat(			"ResolveGroup%dCriticalPathAlphaHi",	ResolveGroupIndex, 1.0f,	sDefinitionName );
		ResolveGroup.m_CriticalPathSampling		= ConfigManager::GetInheritedSequenceBool(			"ResolveGroup%dCriticalPathSampling",	ResolveGroupIndex, false,	sDefinitionName );

		ResolveGroup.m_SpawnNearGroup			= ConfigManager::GetInheritedSequenceHash(	"ResolveGroup%dSpawnNearGroup",		ResolveGroupIndex, HashedString::NullString,	sDefinitionName );

		ResolveGroup.m_MinZ						= ConfigManager::GetInheritedSequenceFloat(	"ResolveGroup%dMinZ",				ResolveGroupIndex,	0.0f,						sDefinitionName );
		ResolveGroup.m_MaxZ						= ConfigManager::GetInheritedSequenceFloat(	"ResolveGroup%dMaxZ",				ResolveGroupIndex,	0.0f,						sDefinitionName );
	}
}

void RosaWorldGen::InitializeFeatureRoomDef( const SimpleString& FeatureRoomDefName )
{
	MAKEHASH( FeatureRoomDefName );

	SFeatureRoom& FeatureRoom = m_FeatureRooms.PushBack();

	STATICHASH( CriticalPathN );
	const int	CriticalPathN	= ConfigManager::GetInheritedInt( sCriticalPathN, 0, sFeatureRoomDefName );
	const uint	AdjCritPathN	= ( CriticalPathN >= 0 ) ? CriticalPathN : ( m_CriticalPathLength + CriticalPathN );	// This means an index of -1 corresponds to a T of 1.0.
	const float	CriticalPathT	= static_cast<float>( AdjCritPathN ) - 0.5f;	// Subtract half so this falls in the middle of the T steps, to avoid rounding errors
	const float	RcpLength		= ( m_CriticalPathLength > 1 ) ? ( 1.0f / static_cast<float>( m_CriticalPathLength - 1 ) ) : 0.0f;
	FeatureRoom.m_CriticalPathT = CriticalPathT * RcpLength;

	STATICHASH( CriticalPathT );
	FeatureRoom.m_CriticalPathT = ConfigManager::GetInheritedFloat( sCriticalPathT, FeatureRoom.m_CriticalPathT, sFeatureRoomDefName );

	STATICHASH( NumRooms );
	const uint NumRooms = ConfigManager::GetInheritedInt( sNumRooms, 0, sFeatureRoomDefName );
	ASSERT( NumRooms );
	// NOTE: Feature rooms don't pass a Prefix here because they're in their own context already.
	// This also means anything used as a feature room can have different weighting/priority
	// than the same room in the standard/critpath sets.
	InitializeRooms( &FeatureRoom.m_Rooms, NumRooms, "", "", FeatureRoomDefName );
}

// pRooms is an out array of indices into the main m_Rooms array.
void RosaWorldGen::InitializeRooms( Array<uint>* pRooms, const uint NumRooms, const SimpleString& ConfigPrefix, const SimpleString& QualifierPrefix, const SimpleString& DefinitionName )
{
	Unused( QualifierPrefix );

	DEVASSERT( m_ModuleUseScale > 0.0f );

	MAKEHASH( DefinitionName );

	const SimpleString RoomKey			= ConfigPrefix + "Room%d";
	const SimpleString RoomWeightKey	= ConfigPrefix + "Room%dWeight";
	const SimpleString RoomUseScaleKey	= ConfigPrefix + "Room%dUseScale";
	const SimpleString RoomDeadEndKey	= ConfigPrefix + "Room%dDeadEnd";
	const SimpleString RoomPriorityKey	= ConfigPrefix + "Room%dPriority";
	const SimpleString RoomMinTileZKey	= ConfigPrefix + "Room%dMinTileZ";
	const SimpleString RoomMaxTileZKey	= ConfigPrefix + "Room%dMaxTileZ";
#if BUILD_DEV
	const SimpleString RoomBreakKey		= ConfigPrefix + "Room%dBreak";
#endif

	m_Rooms.Reserve( m_Rooms.Size() + NumRooms );

	for( uint RoomIndex = 0; RoomIndex < NumRooms; ++RoomIndex )
	{
		const SimpleString	Filename	= ConfigManager::GetInheritedSequenceString(	RoomKey,			RoomIndex, "",		sDefinitionName );
		SRoom&				Room		= InitializeRoom( Filename );
		Room.m_DefaultWeight			= ConfigManager::GetInheritedSequenceFloat(		RoomWeightKey,		RoomIndex, 1.0f,	sDefinitionName );
		Room.m_Scale					= ConfigManager::GetInheritedSequenceFloat(		RoomUseScaleKey,	RoomIndex, 1.0f,	sDefinitionName ) * m_ModuleUseScale;
		Room.m_DeadEnd					= ConfigManager::GetInheritedSequenceBool(		RoomDeadEndKey,		RoomIndex, false,	sDefinitionName );
		Room.m_Priority					= ConfigManager::GetInheritedSequenceInt(		RoomPriorityKey,	RoomIndex, 0,		sDefinitionName );
		Room.m_MinTileZ					= ConfigManager::GetInheritedSequenceInt(		RoomMinTileZKey,	RoomIndex, 0,		sDefinitionName );
		Room.m_MaxTileZ					= ConfigManager::GetInheritedSequenceInt(		RoomMaxTileZKey,	RoomIndex, 0,		sDefinitionName );
#if BUILD_DEV
		Room.m_Qualifier				= ( QualifierPrefix != "" ) ? SimpleString::PrintF( "%s/%s", DefinitionName.CStr(), QualifierPrefix.CStr() ) : DefinitionName;
		Room.m_Break					= ConfigManager::GetInheritedSequenceBool(		RoomBreakKey,		RoomIndex, false,	sDefinitionName );
#endif

		if( pRooms )
		{
			( *pRooms ).PushBack( m_Rooms.Size() - 1 );
		}
	}
}

void RosaWorldGen::InitializeFixedRoom( const SimpleString& Filename )
{
	SRoom& FixedRoom = InitializeRoom( Filename );

	FixedRoom.m_DefaultWeight	= 1.0;
	FixedRoom.m_Weight			= 1.0;
	FixedRoom.m_Scale			= 1.0;
}

void RosaWorldGen::InitializeWorldFile( SWorldFile& WorldFile, const IDataStream& Stream )
{
	const uint	TileSizeX	= Stream.ReadUInt32();
	const uint	TileSizeY	= Stream.ReadUInt32();
	const uint	TileSizeZ	= Stream.ReadUInt32();

	// HACKHACK: Just initialize these from the first world file,
	// they should all be the same (verified by the assert below).
	if( 0 == m_TileSizeX ||
		0 == m_TileSizeY ||
		0 == m_TileSizeZ )
	{
		m_TileSizeX	= TileSizeX;
		m_TileSizeY	= TileSizeY;
		m_TileSizeZ	= TileSizeZ;
	}

	DEVASSERT( m_TileSizeX == TileSizeX );
	DEVASSERT( m_TileSizeY == TileSizeY );
	DEVASSERT( m_TileSizeZ == TileSizeZ );

	WorldFile.m_Bounds.X = static_cast<int>( Stream.ReadUInt32() );
	WorldFile.m_Bounds.Y = static_cast<int>( Stream.ReadUInt32() );
	WorldFile.m_Bounds.Z = static_cast<int>( Stream.ReadUInt32() );

	const uint NumRooms = Stream.ReadUInt32();
	for( uint RoomIndex = 0; RoomIndex < NumRooms; ++RoomIndex )
	{
		SWorldFileRoom&		WorldFileRoom	= WorldFile.m_Rooms.PushBack();
		const SimpleString	RoomFilename	= Stream.ReadString();
		WorldFileRoom.m_Location			= Stream.Read<SRoomLoc>();
		WorldFileRoom.m_Transform			= Stream.ReadUInt32();
		WorldFileRoom.m_RoomIndex			= m_Rooms.Size();

		SRoom&				Room			= InitializeRoom( RoomFilename );
		const RosaRoom&		RoomData		= GetRoomData( Room );

		// Fix module loc using loaded room properties
		SRoomLoc LowLoc, HighLoc;
		GetLowHighLocs( RoomData, WorldFileRoom.m_Location, WorldFileRoom.m_Transform, LowLoc, HighLoc );
		WorldFileRoom.m_Location		= LowLoc;

		// This stuff doesn't really matter but why not
		Room.m_DefaultWeight			= 1.0f;
		Room.m_Weight					= 1.0f;
		Room.m_Scale					= 1.0f;
#if BUILD_DEV
		Room.m_Qualifier				= "WORLD FILE";
#endif

		m_WorldFileRooms.PushBack( m_Rooms.Size() - 1 );
	}
}

void RosaWorldGen::ConditionalLoadRoomData( const SimpleString& RoomFilename )
{
	const HashedString	RoomDataKey	= RoomFilename;
	if( m_RoomDataMap.Contains( RoomDataKey ) )
	{
		return;
	}

	RosaRoom&			NewRoomData			= m_RoomDataMap.Insert( RoomDataKey );
	NewRoomData.Load( PackStream( RoomFilename.CStr() ) );

	if( m_IsFixedRoom )
	{
		// No need to validate properties that aren't used since we're not generating anything
	}
	else
	{
		ASSERT( m_TileSizeX == NewRoomData.m_MetersX / NewRoomData.m_TilesX );
		ASSERT( m_TileSizeY == NewRoomData.m_MetersY / NewRoomData.m_TilesY );
		ASSERT( m_TileSizeZ == NewRoomData.m_MetersZ / NewRoomData.m_TilesZ );
	}
}

const RosaRoom& RosaWorldGen::GetRoomData( const SRoom& Room ) const
{
	DEVASSERT( Room.m_RoomDataKey != HashedString::NullString );
	DEVASSERT( m_RoomDataMap.Contains( Room.m_RoomDataKey ) );
	return m_RoomDataMap[ Room.m_RoomDataKey ];
}

RosaWorldGen::SRoom& RosaWorldGen::InitializeRoom( const SimpleString& Filename )
{
	SRoom& Room = m_Rooms.PushBack();

	Room.m_RoomDataKey	= Filename;
	Room.m_Filename		= Filename;

	ConditionalLoadRoomData( Filename );

	// ROSATODO: Cache the room by portal info or whatever.

	return Room;
}

void RosaWorldGen::Generate( const bool SimGeneration /*= false*/ )
{
	PROFILE_FUNCTION;

	m_SimmingGeneration	= SimGeneration;
	m_WorldFileIndex	= INVALID_ARRAY_INDEX;

#if BUILD_DEV
	uint FailedAttempts	= 0;
	m_FailReasons.Reset();
	m_Failed_OpenPortals.Clear();

	if( !m_SimmingGeneration )
	{
		PRINTF( "Generating world...\n" );
	}
#endif

	DEV_DECLARE_AND_START_CLOCK( GenerateClock );
	DEV_DECLARE_AND_START_CLOCK( GenerateRoomsClock );

	if( m_IsFixedRoom )
	{
		ASSERT( m_Rooms.Size() == 1 );

		static const uint		skRoomIndex			= 0;
		static const SRoomLoc	skLowLoc			= SRoomLoc( 0, 0, 0 );
		static const uint		skTransform			= ERT_None;
		static const bool		skForCriticalPath	= true;
		static const bool		skForDeadEnd		= true;
		static const float		skCriticalPathAlpha	= 0.0f;
		InsertRoom( skRoomIndex, skLowLoc, skTransform, skForCriticalPath, skForDeadEnd, skCriticalPathAlpha );
	}
	else
	{
		while( !GenerateRooms() )
		{
#if BUILD_DEV
			FailedAttempts++;

			if( !m_SimmingGeneration &&
				FailedAttempts > 0 &&
				( FailedAttempts % 100 ) == 0 )
			{
					PRINTF( "Failed to generate world %d times...\n", FailedAttempts );
					PRINTF( "  %d failed in critical path.\n",		m_FailReasons.m_CriticalPath );
					PRINTF( "    %d failed in feature rooms.\n",	m_FailReasons.m_CriticalPath_FeatureRoom );
					PRINTF( "  %d failed in fill rooms.\n",			m_FailReasons.m_FillRooms );
					PRINTF( "  %d failed in connections.\n",		m_FailReasons.m_Connections );
					PRINTF( "  %d failed in loops.\n",				m_FailReasons.m_Loops );
					PRINTF( "  %d failed in dead ends.\n",			m_FailReasons.m_DeadEnds );
					PRINTF( "  %d failed in size.\n",				m_FailReasons.m_Size );

					if( m_IsUsingWorldFile )
					{
						PRINTF( "  ...with world file %s\n", m_WorldFiles[ m_WorldFileIndex ].m_Filename.CStr() );
					}
			}
#endif
		}
	}

#if BUILD_DEV
	if( m_SimmingGeneration )
	{
		m_GlobalStats.m_FailsSum			+= FailedAttempts;
		m_GlobalStats.m_FailsMax			= Max( m_GlobalStats.m_FailsMax, FailedAttempts );
		m_GlobalStats.m_FailReasons			+= m_FailReasons;
		if( m_IsUsingWorldFile )
		{
			SWorldGenStats&	WorldFileStats	= m_WorldFileStats[ m_WorldFileIndex ];
			WorldFileStats.m_FailsSum		+= FailedAttempts;
			WorldFileStats.m_FailsMax		= Max( WorldFileStats.m_FailsMax, FailedAttempts );
			WorldFileStats.m_FailReasons	+= m_FailReasons;
		}
	}
#endif

	if( !m_SimmingGeneration )
	{
		DEV_STOP_AND_REPORT_CLOCK( GenerateRoomsClock, "  " );
	}

	// ZETANOTE: CreateGenRoomPortals used to happen here, but there should be no need until later.

#if BUILD_DEV
	// Save off some values before PopulateWorld cleans them up.
	const uint NumGeneratedRooms	= m_GeneratedRooms.Size();
	const uint NumOccupiedTiles		= m_NumOccupiedTiles;
	const uint NumLoops				= m_NumLoops;
	const uint NumDeadEnds			= m_NumDeadEnds;
#endif

	PopulateWorld();

#if BUILD_DEV
	STOP_CLOCK( GenerateClock );

	if( m_SimmingGeneration )
	{
		m_GlobalStats.m_StatsClock		+= GenerateClock;
		if( GenerateClock > m_GlobalStats.m_ClockMax )
		{
			m_GlobalStats.m_ClockMax	= GenerateClock;
		}
		if( m_IsUsingWorldFile )
		{
			SWorldGenStats&	WorldFileStats	= m_WorldFileStats[ m_WorldFileIndex ];
			WorldFileStats.m_StatsClock		+= GenerateClock;
			if( GenerateClock > WorldFileStats.m_ClockMax )
			{
				WorldFileStats.m_ClockMax	= GenerateClock;
			}
		}
	}

	if( !m_SimmingGeneration )
	{
		PRINTF( "Generated world in %.3fs after %d failed attempts.\n", GET_CLOCK( GenerateClock ), FailedAttempts );
		PRINTF( "  %d modules.\n",		NumGeneratedRooms );
		PRINTF( "  %d tiles.\n",		NumOccupiedTiles );
		PRINTF( "  %d loops.\n",		NumLoops );
		PRINTF( "  %d dead ends.\n",	NumDeadEnds );

		if( FailedAttempts > 0 )
		{
			PRINTF( "  %d failed in critical path.\n",		m_FailReasons.m_CriticalPath );
			PRINTF( "    %d failed in feature rooms.\n",	m_FailReasons.m_CriticalPath_FeatureRoom );
			PRINTF( "  %d failed in fill rooms.\n",			m_FailReasons.m_FillRooms );
			PRINTF( "  %d failed in connections.\n",		m_FailReasons.m_Connections );
			PRINTF( "  %d failed in loops.\n",				m_FailReasons.m_Loops );
			PRINTF( "  %d failed in dead ends.\n",			m_FailReasons.m_DeadEnds );
			PRINTF( "  %d failed in size.\n",				m_FailReasons.m_Size );
		}

		if( !m_Failed_OpenPortals.Empty() )
		{
			// Sort failed open portals by count by making a reverse multimap
			Multimap<uint, SPortal> ReverseFailedPortals;
			FOR_EACH_MAP( PortalIter, m_Failed_OpenPortals, SPortal, uint )
			{
				const SPortal&	Portal		= PortalIter.GetKey();
				const uint		FailCount	= PortalIter.GetValue();
				ReverseFailedPortals.Insert( FailCount, Portal );
			}

			// Then print in reverse sorted order (largest to smallest)
			PRINTF( "Total open portal counts at fail time:\n" );
			FOR_EACH_MULTIMAP_REVERSE( PortalIter, ReverseFailedPortals, uint, SPortal )
			{
				const uint			FailCount	= PortalIter.GetKey();
				const SPortal&		Portal		= PortalIter.GetValue();
				const SimpleString	FrontTag	= ReverseHash::ReversedHash( Portal.m_FrontTag );
				PRINTF( "  %s / (", FrontTag.CStr() );
				FOR_EACH_ARRAY( BackTagIter, Portal.m_BackTags, HashedString )
				{
					const SimpleString	BackTag	= ReverseHash::ReversedHash( BackTagIter.GetValue() );
					PRINTF( "%s", BackTag.CStr() );
					if( BackTagIter.GetIndex() < Portal.m_BackTags.Size() - 1 )
					{
						PRINTF( ", " );
					}
				}
				PRINTF( "): %d\n", FailCount );
			}
		}
	}
#endif

	if( !m_SimmingGeneration )
	{
		WB_MAKE_EVENT( PostWorldGen, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), PostWorldGen, NULL );
	}
}

bool RosaWorldGen::InsertWorldFileRooms()
{
#if BUILD_DEV
	if( m_DebugWorldGen && !m_SimmingGeneration )
	{
		DEVPRINTF( "Inserting world file rooms...\n" );
	}
#endif

	DEVASSERT( m_WorldFiles.IsValidIndex( m_WorldFileIndex ) );
	const SWorldFile&	WorldFile	= m_WorldFiles[ m_WorldFileIndex ];

	FOR_EACH_ARRAY( WorldFileRoomIter, WorldFile.m_Rooms, SWorldFileRoom )
	{
		const SWorldFileRoom&	WorldFileRoom		= WorldFileRoomIter.GetValue();
		const SRoomLoc			kLowLoc				= WorldFileRoom.m_Location;
		const uint				kTransform			= WorldFileRoom.m_Transform;
		const uint				kRoomIndex			= WorldFileRoom.m_RoomIndex;
		static const bool		skForCriticalPath	= true;
		static const bool		skForDeadEnd		= false;	// WORLDTODO: Revisit this if needed
		static const float		skCriticalPathAlpha	= 0.0f;
		InsertRoom( kRoomIndex, kLowLoc, kTransform, skForCriticalPath, skForDeadEnd, skCriticalPathAlpha );
	}

	return WorldFile.m_Rooms.Size() > 0;
}

bool RosaWorldGen::InsertSeedFeatureRoom()
{
#if BUILD_DEV
	if( m_DebugWorldGen && !m_SimmingGeneration )
	{
		DEVPRINTF( "Inserting seed feature room...\n" );
	}
#endif

	FOR_EACH_ARRAY( FeatureRoomIter, m_FeatureRooms, SFeatureRoom )
	{
		const SFeatureRoom& FeatureRoom = FeatureRoomIter.GetValue();
		if( FeatureRoom.m_CriticalPathT > 0.0f )
		{
			continue;
		}

		// ROSANOTE: No need to validate this at the moment;
		// any room should be as good as any other, so pick randomly.
		const uint				kRoomIndex			= Math::ArrayRandom( FeatureRoom.m_Rooms );
		static const SRoomLoc	skLocation			= SRoomLoc( 0, 0, 0 );
		static const uint		skTransform			= ERT_None;
		static const bool		skForCriticalPath	= true;
		static const bool		skForDeadEnd		= false;	// The seed *could* be a dead end, but generally shouldn't be regarded as such for spawning purposes.
		static const float		skCriticalPathAlpha	= 0.0f;
		InsertRoom( kRoomIndex, skLocation, skTransform, skForCriticalPath, skForDeadEnd, skCriticalPathAlpha );

		//m_CriticalPathLocations.PushBack( Vector() );	// Redundant, but here just in case I ever change the seed location (skLocation).

		return true;
	}

	return false;
}

bool RosaWorldGen::InsertCriticalPathRooms()
{
#if BUILD_DEV
	if( m_DebugWorldGen && !m_SimmingGeneration )
	{
		DEVPRINTF( "Inserting critical path rooms...\n" );
	}
#endif

	ASSERT( !m_IsUsingWorldFile );
	ASSERT( m_GeneratedRooms.Size() == 1 );	// We should be building a linear path from a singular starting room.

	// 1-length critical paths are valid; everything will branch from a seed.
	const float RcpLength = ( m_CriticalPathLength > 1 ) ? ( 1.0f / static_cast<float>( m_CriticalPathLength - 1 ) ) : 0.0f;

	// CriticalPathLength includes starting room, so start iteration from 1.
	for( uint CriticalPathIndex = 1; CriticalPathIndex < m_CriticalPathLength; ++CriticalPathIndex )
	{
		Array<SFittingRoom> FittingRooms;
		const bool			IsEndOfCriticalPath	= CriticalPathIndex == ( m_CriticalPathLength - 1 );

		const uint			PrevIndex			= CriticalPathIndex - 1;
		const float			PrevT				= static_cast<float>( PrevIndex ) * RcpLength;
		const float			CurrT				= static_cast<float>( CriticalPathIndex ) * RcpLength;

		// Check if there's a feature room that falls at this distance along the crit path.
		// We expect there to be one at most.
		bool	UsingFeatureRoom	= false;
		uint	FeatureRoomIndex	= 0;
		FOR_EACH_ARRAY( FeatureRoomIter, m_FeatureRooms, SFeatureRoom )
		{
			const SFeatureRoom& FeatureRoom = FeatureRoomIter.GetValue();
			if( PrevT >= FeatureRoom.m_CriticalPathT || CurrT < FeatureRoom.m_CriticalPathT )
			{
				continue;
			}

			// If this fails, it means there are two feature room defs that hit this critical path T; that's a problem!
			ASSERT( !UsingFeatureRoom );

			UsingFeatureRoom	= true;
			FeatureRoomIndex	= FeatureRoomIter.GetIndex();
		}

		Array<uint>& CritPathRooms = UsingFeatureRoom ? m_FeatureRooms[ FeatureRoomIndex ].m_Rooms : m_CritPathRooms;

		// Get all possible fits, making sure the crit path never turns 180 degrees (m_CriticalPathDirection)
		DEVASSERT( m_NumJustOpenedPortals > 0 );
		DEVASSERT( m_OpenPortals.Size() >= m_NumJustOpenedPortals );
		for( uint OpenPortalIndex = m_OpenPortals.Size() - m_NumJustOpenedPortals; OpenPortalIndex < m_OpenPortals.Size(); ++OpenPortalIndex )
		{
			const SOpenPortal&	OpenPortal		= m_OpenPortals[ OpenPortalIndex ];

			// HACKHACK: Prevent crit path expansion in the opposite of the initial direction.
			// This will cause more crit path failures (because I'm not doing anything to make sure
			// opened portals face in good directions), but those are cheap and the results are good.
			if( m_HasCriticalPathDirection && OpenPortal.m_PortalIndex == GetComplementaryPortalIndex( m_CriticalPathDirection ) )
			{
				continue;
			}

			// Add all possible fits from the feature room set or crit path set
			FOR_EACH_ARRAY( RoomIter, CritPathRooms, uint )
			{
				const uint			RoomIndex			= RoomIter.GetValue();
				static const bool	skForCriticalPath	= true;
				const uint			MinNewOpenPortals	= IsEndOfCriticalPath ? 0 : 1;	// If this is the end of the critical path, we don't need more portals.
				const float			CriticalPathAlpha	= CurrT;
				FindFittingRooms( RoomIndex, OpenPortal, skForCriticalPath, MinNewOpenPortals, CriticalPathAlpha, FittingRooms );
			}
		}

		FilterFittingRooms( FittingRooms );

		if( FittingRooms.Empty() )
		{
			// No possible expansion on critical path, bail and retry generation
#if BUILD_DEV
			m_FailReasons.m_CriticalPath++;

			if( UsingFeatureRoom )
			{
				m_FailReasons.m_CriticalPath_FeatureRoom++;
			}
#endif
			return false;
		}

		SelectAndInsertFittingRoom( FittingRooms );
	}

	return true;
}

bool RosaWorldGen::InsertFillRooms()
{
#if BUILD_DEV
	if( m_DebugWorldGen && !m_SimmingGeneration )
	{
		DEVPRINTF( "Inserting fill rooms...\n" );
	}
#endif

	Array<SFittingRoom> FittingRooms;
	FittingRooms.SetDeflate( false );

	// Continue until the level is closed (no open portals remain) or we can't fit anything.
	while( m_OpenPortals.Size() > 0 )
	{
		if( FittingRooms.Empty() )
		{
			// The first time through, gather fitting rooms for all open portals.
			for( uint OpenPortalIndex = 0; OpenPortalIndex < m_OpenPortals.Size(); ++OpenPortalIndex )
			{
				const SOpenPortal&		OpenPortal		= m_OpenPortals[ OpenPortalIndex ];
				const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( OpenPortal.m_Room );
				const SGenRoom&			GenRoom			= m_GeneratedRooms[ OccupiedTile.m_GenRoomIndex ];

				// Add all possible fits from the standard set.
				FOR_EACH_ARRAY( RoomIter, m_StandardRooms, uint )
				{
					const uint			RoomIndex			= RoomIter.GetValue();
					static const bool	skForCriticalPath	= false;
					static const uint	skMinNewOpenPortals	= 0;
					const float			CriticalPathAlpha	= GenRoom.m_CriticalPathAlpha;	// Inherit the crit path alpha from whatever room we're expanding from (ROSANOTE: This means alphas could be discontinuous where loops meet)
					FindFittingRooms( RoomIndex, OpenPortal, skForCriticalPath, skMinNewOpenPortals, CriticalPathAlpha, FittingRooms );
				}
			}
		}
		else
		{
			// On subsequent iterations, revalidate fitting rooms and cull if they no longer fit.
			FOR_EACH_ARRAY_REVERSE( FittingRoomIter, FittingRooms, SFittingRoom )
			{
				SFittingRoom&	FittingRoom			= FittingRoomIter.GetValue();
				const bool		ForCriticalPath		= false;
				const uint		MinNewOpenPortals	= 0;
				if( !RevalidateFittingRoom( FittingRoom, ForCriticalPath, MinNewOpenPortals ) )
				{
					FittingRooms.FastRemove( FittingRoomIter );
				}
			}

			// Gather new fitting rooms for just-opened portals (if any).
			DEVASSERT( m_OpenPortals.Size() >= m_NumJustOpenedPortals );
			for( uint OpenPortalIndex = m_OpenPortals.Size() - m_NumJustOpenedPortals; OpenPortalIndex < m_OpenPortals.Size(); ++OpenPortalIndex )
			{
				const SOpenPortal&		OpenPortal		= m_OpenPortals[ OpenPortalIndex ];
				const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( OpenPortal.m_Room );
				const SGenRoom&			GenRoom			= m_GeneratedRooms[ OccupiedTile.m_GenRoomIndex ];

				// Add all possible fits from the standard set
				FOR_EACH_ARRAY( RoomIter, m_StandardRooms, uint )
				{
					const uint			RoomIndex			= RoomIter.GetValue();
					static const bool	skForCriticalPath	= false;
					static const uint	skMinNewOpenPortals	= 0;
					const float			CriticalPathAlpha	= GenRoom.m_CriticalPathAlpha;	// Inherit the crit path alpha from whatever room we're expanding from (ROSANOTE: This means alphas could be discontinuous where loops meet)
					FindFittingRooms( RoomIndex, OpenPortal, skForCriticalPath, skMinNewOpenPortals, CriticalPathAlpha, FittingRooms );
				}
			}
		}

		// Make a copy to modify
		Array<SFittingRoom> FilteredFittingRooms = FittingRooms;
		FilterFittingRooms( FilteredFittingRooms );

		if( FilteredFittingRooms.Empty() )
		{
			if( !m_ConnectiveRooms.Empty() )
			{
				// HACKHACK: Remove any spacers that we couldn't close.
				FOR_EACH_ARRAY_REVERSE( OpenPortalIter, m_OpenPortals, SOpenPortal )
				{
					const SOpenPortal&		OpenPortal		= OpenPortalIter.GetValue();
					const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( OpenPortal.m_Room );
					const SPortal&			OccupiedPortal	= OccupiedTile.m_Portals.m_Portals[ OpenPortal.m_PortalIndex ];
					if( OccupiedPortal.m_MustClose )
					{
						const SRoom&	Room		= m_Rooms[ m_GeneratedRooms[ OccupiedTile.m_GenRoomIndex ].m_RoomIndex ];
						const RosaRoom&	RoomData	= GetRoomData( Room );
						ASSERTDESC( 1 == ( RoomData.m_TilesX * RoomData.m_TilesY * RoomData.m_TilesZ ), "InsertFillRooms: Trying to roll back a non-single-tile room, this is not supported." );

#if BUILD_DEV
						if( m_DebugWorldGen && !m_SimmingGeneration )
						{
							DEVPRINTF( "  Removing room %s at %d,%d,%d\n", Room.m_Filename.CStr(), OpenPortal.m_Room.X, OpenPortal.m_Room.Y, OpenPortal.m_Room.Z );
						}
#endif

						// Get back to the room that led here and reopen its portal in this direction
						{
							const uint				BackPortalIndex		= GetComplementaryPortalIndex( OpenPortal.m_PortalIndex );
							const SRoomLoc			BackLocation		= GetRoomLocThroughPortal( OpenPortal.m_Room, BackPortalIndex );
							SOpenPortal& BackOpenPortal					= m_OpenPortals.PushBack();
							BackOpenPortal.m_Room						= BackLocation;
							BackOpenPortal.m_PortalIndex				= OpenPortal.m_PortalIndex;
						}

						// Remove the genroom
						RemoveGenRoomByIndex( OccupiedTile.m_GenRoomIndex );

						// Remove the occupied tile
						RemoveOccupiedTile( OpenPortal.m_Room );

						// Remove the open portal
						m_OpenPortals.FastRemove( OpenPortalIter );
					}
				}

				// This is not a failure case because we will close all remaining portals with connective rooms.
				return true;
			}

			// No possible expansion on branches, bail and retry generation
#if BUILD_DEV
			m_FailReasons.m_FillRooms++;

			if( m_DebugWorldGen && !m_SimmingGeneration )
			{
				PRINTF( "%d failed open portals:\n", m_OpenPortals.Size() );
			}

			FOR_EACH_ARRAY( OpenPortalIter, m_OpenPortals, SOpenPortal )
			{
				const SOpenPortal&		OpenPortal		= OpenPortalIter.GetValue();
				const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( OpenPortal.m_Room );
				const SPortal&			OccupiedPortal	= OccupiedTile.m_Portals.m_Portals[ OpenPortal.m_PortalIndex ];

				m_Failed_OpenPortals[ OccupiedPortal ]++;

				if( m_DebugWorldGen && !m_SimmingGeneration )
				{
					const SimpleString		FrontTag		= ReverseHash::ReversedHash( OccupiedPortal.m_FrontTag );
					PRINTF( "  At %d,%d,%d / facing %d / front: %s / backs: (",
						OpenPortal.m_Room.X, OpenPortal.m_Room.Y, OpenPortal.m_Room.Z,
						OpenPortal.m_PortalIndex,
						FrontTag.CStr() );
					FOR_EACH_ARRAY( BackTagIter, OccupiedPortal.m_BackTags, HashedString )
					{
						const SimpleString	BackTag	= ReverseHash::ReversedHash( BackTagIter.GetValue() );
						PRINTF( "%s", BackTag.CStr() );
						if( BackTagIter.GetIndex() < OccupiedPortal.m_BackTags.Size() - 1 )
						{
							PRINTF( ", " );
						}
					}
					PRINTF( ")\n" );
				}
			}

			if( m_AllowUnfinishedWorlds )
			{
				if( !m_SimmingGeneration )
				{
					PRINTF( "!!!!! BUILDING UNFINISHED WORLD !!!!!\n" );
				}
				return true;	// Go ahead and build the world with missing regions
			}
			else
#endif
			{
				return false;
			}
		}

		// ROSANOTE: I don't have to explicitly remove this room from FittingRooms
		// after inserting it into the world; it will be removed when revalidating.
		SelectAndInsertFittingRoom( FilteredFittingRooms );
	}

	return true;
}

// If there are any open portals, do a random expansion from them until
// space is filled, then prune any dead-end paths from the expansion.
bool RosaWorldGen::InsertConnectiveRooms()
{
#if BUILD_DEV
	if( m_DebugWorldGen && !m_SimmingGeneration )
	{
		DEVPRINTF( "Inserting connective rooms...\n" );
	}
#endif

	DEVASSERT( m_IsUsingWorldFile );
	DEVASSERT( m_WorldFiles.IsValidIndex( m_WorldFileIndex ) );
	const SWorldFile&	WorldFile		= m_WorldFiles[ m_WorldFileIndex ];
	const SRoomLoc&		WorldFileBounds	= WorldFile.m_Bounds;

	Array<SConnectiveNode>	ConnectiveNodes;
	ConnectiveNodes.SetDeflate( false );

	Array<uint>				OpenSet;	// Indexes into ConnectiveNodes
	OpenSet.SetDeflate( false );

	Array<uint>				ClosedSet;	// Indexes into ConnectiveNodes
	ClosedSet.SetDeflate( false );

	Set<SRoomLoc>			OccupiedConnectionNodeLocations;

	Array<uint>				Exits;		// (EPortalIndex)
	Exits.Reserve( 4 );
	Exits.SetDeflate( false );

	// While there are any open portals in the actual generated rooms...
	while( m_OpenPortals.Size() > 0 )
	{
		ConnectiveNodes.Clear();
		OpenSet.Clear();
		ClosedSet.Clear();
		OccupiedConnectionNodeLocations.Clear();

		// ...pick any one of those open portals...
		const SOpenPortal&		OpenPortal		= Math::ArrayRandom( m_OpenPortals );
		const SRoomLoc			SeedRoomLoc		= GetRoomLocThroughPortal( OpenPortal.m_Room, OpenPortal.m_PortalIndex );
		//const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( OpenPortal.m_Room );
		//const SPortal&			OccupiedPortal	= OccupiedTile.m_Portals.m_Portals[ OpenPortal.m_PortalIndex ];

		OpenSet.PushBack( 0 );
		const uint				SeedPortalBits	= GetPortalBitsForPortalIndex( GetComplementaryPortalIndex( OpenPortal.m_PortalIndex ) );
		SConnectiveNode&		SeedNode		= ConnectiveNodes.PushBack();
		SeedNode.m_Location						= SeedRoomLoc;
		SeedNode.m_OpenPortalBits				= EPB_Horizontal & ~SeedPortalBits;
		SeedNode.m_ClosedPortalBits				= SeedPortalBits;

		OccupiedConnectionNodeLocations.Insert( SeedRoomLoc );

		// ...and do a random expansion from there until space is filled.
		// We don't use any heuristic to guide this expansion toward other portals,
		// so this does not guarantee shortest paths between portals. That's a design
		// choice because little twisty paths is fine in this game, I think.
		while( OpenSet.Size() > 0 )
		{
			const uint			OpenNodeIndexIndex	= Math::Random( OpenSet.Size() );
			const uint			OpenNodeIndex		= OpenSet[ OpenNodeIndexIndex ];
			SConnectiveNode&	OpenNode			= ConnectiveNodes[ OpenNodeIndex ];

			if( EPB_None == OpenNode.m_OpenPortalBits )
			{
				// Nowhere left to expand this, close it.
				ClosedSet.PushBack( OpenNodeIndex );
				OpenSet.FastRemove( OpenNodeIndexIndex );
				continue;
			}

			Exits.Clear();
			if( 0 != ( EPB_East & OpenNode.m_OpenPortalBits ) )		{ Exits.PushBack( EPI_East ); }
			if( 0 != ( EPB_West & OpenNode.m_OpenPortalBits ) )		{ Exits.PushBack( EPI_West ); }
			if( 0 != ( EPB_North & OpenNode.m_OpenPortalBits ) )	{ Exits.PushBack( EPI_North ); }
			if( 0 != ( EPB_South & OpenNode.m_OpenPortalBits ) )	{ Exits.PushBack( EPI_South ); }
			DEBUGASSERT( Exits.Size() > 0 );

			const uint	ExitPortalIndex	= Math::ArrayRandom( Exits );
			const uint	ExitPortalBits	= GetPortalBitsForPortalIndex( ExitPortalIndex );
			OpenNode.m_OpenPortalBits	&= ~ExitPortalBits;

			const SRoomLoc				FarLocation			= GetRoomLocThroughPortal( OpenNode.m_Location, ExitPortalIndex );
			if( OccupiedConnectionNodeLocations.Contains( FarLocation ) )
			{
				// We've already traversed here from another node
				continue;
			}

			const SOccupiedTile* const	pFarOccupiedTile	= SafeGetOccupiedTile( FarLocation );
			if( NULL == pFarOccupiedTile )
			{
				if( !IsLocationWithinWorldBounds( FarLocation, WorldFileBounds ) )
				{
					// Far location is not valid because it is outside world bounds
					continue;
				}
			}
			else
			{
				const uint		BackPortalIndex	= GetComplementaryPortalIndex( ExitPortalIndex );
				const SPortal&	BackPortal		= pFarOccupiedTile->m_Portals.m_Portals[ BackPortalIndex ];
				if( !BackPortal.m_FrontTag )
				{
					// Far location is not valid because it is occupied and does not have a portal to here.
					// TODO: I should also be checking portal tags; it's possible that not everything will
					// be valid to join to connective rooms, but that's a bigger problem. I may not end up
					// using tags nearly as much as I did in Vamp.
					// The use case for this would be, like, if I wanted connective rooms to be either halls
					// or air vents in the same level; I'd want to compare the root portal to whatever
					// it joined to. But this introduces bigger problems (needing every combination of those
					// two or more room types to fit, for one!) so just ignore it for now.
					continue;
				}
				//else
				//{
				//	// Catch the case described above
				//	ASSERT( OccupiedPortal.m_BackTags.Contains( BackPortal.m_FrontTag ) );
				//}
			}

			// Checks passed, we can expand in this direction.
			OpenNode.m_ClosedPortalBits			|= ExitPortalBits;

			// But only create a new node in valid space.
			if( NULL == pFarOccupiedTile )
			{
				OpenSet.PushBack( ConnectiveNodes.Size() );
				const uint			NextPortalBits	= GetPortalBitsForPortalIndex( GetComplementaryPortalIndex( ExitPortalIndex ) );
				SConnectiveNode&	NextNode		= ConnectiveNodes.PushBack();
				NextNode.m_Location					= FarLocation;
				NextNode.m_ParentIndex				= OpenNodeIndex;
				NextNode.m_ParentPortalIndex		= ExitPortalIndex;
				NextNode.m_OpenPortalBits			= EPB_Horizontal & ~NextPortalBits;
				NextNode.m_ClosedPortalBits			= NextPortalBits;

				OccupiedConnectionNodeLocations.Insert( FarLocation );
			}
		}

		// Having filled space from the chosen open portal, we now have a graph to be pruned.
		// This removes any expanded paths that terminated in a dead end, so all connective
		// rooms actually connect somewhere. If there was nowhere for an expansion to join,
		// it will be pruned down to a dead-end stub just to close the portal.
		// NOTE: We need the outer loop because there's no guarantee of the order
		// in which nodes were closed, since the expansion is random instead of e.g. BFS.
		// A child may be closed before its parent and iterating in reverse only once
		// would miss that.
		bool DidPruneAnything = false;
		do
		{
			DidPruneAnything = false;
			FOR_EACH_ARRAY_REVERSE( ClosedSetIter, ClosedSet, uint )
			{
				const uint			ClosedSetIndex		= ClosedSetIter.GetValue();
				SConnectiveNode&	ConnectiveNode		= ConnectiveNodes[ ClosedSetIndex ];
				const uint			NumClosedPortals	= CountBits( ConnectiveNode.m_ClosedPortalBits );
				DEBUGASSERT( NumClosedPortals > 0 );

				if( NumClosedPortals > 1 )
				{
					// This is not a dead end, it's fine.
					continue;
				}

				if( INVALID_ARRAY_INDEX == ConnectiveNode.m_ParentIndex )
				{
					// This is a dead end, but it's the root node, it's fine.
					continue;
				}

				// We've got a dead end, prune it.
				DidPruneAnything					= true;
				SConnectiveNode&	ParentNode		= ConnectiveNodes[ ConnectiveNode.m_ParentIndex ];
				const uint			PrunePortalBits	= GetPortalBitsForPortalIndex( ConnectiveNode.m_ParentPortalIndex );
				ParentNode.m_ClosedPortalBits		&= ~PrunePortalBits;

				ClosedSet.FastRemove( ClosedSetIter );
			}
		}
		while (DidPruneAnything);

		// Finally, insert the actual rooms, which closes the portals.
		// Then we continue this process with any remaining open portals.
		FOR_EACH_ARRAY( ClosedSetIter, ClosedSet, uint )
		{
			const uint					ClosedSetIndex		= ClosedSetIter.GetValue();
			const SConnectiveNode&		ConnectiveNode		= ConnectiveNodes[ ClosedSetIndex ];
			DEBUGASSERT( m_ConnectiveRoomFits.Contains( ConnectiveNode.m_ClosedPortalBits ) );
			const SConnectiveRoomFit&	RoomFit				= m_ConnectiveRoomFits[ ConnectiveNode.m_ClosedPortalBits ];

			static const bool		skForCriticalPath	= false;
			static const bool		skForDeadEnd		= false;	// HACKHACK: Even if this is a dead end, don't treat it as one for the purposes of spawners
			static const float		skCriticalPathAlpha	= 0.0f;		// HACKHACK: I'm not caring about critpath alpha in this game.
			InsertRoom( RoomFit.m_RoomIndex, ConnectiveNode.m_Location, RoomFit.m_Transform, skForCriticalPath, skForDeadEnd, skCriticalPathAlpha );
		}
	}

	return true;
}

// Breadth-first search to check connectivity
bool RosaWorldGen::CheckRoomConnections()
{
	DEVASSERT( m_GeneratedRooms.Size() > 0 );

	// Special case for worlds that only consist of a single rooms.
	if( 1 == m_GeneratedRooms.Size() )
	{
		return true;
	}

	// Don't count rooms with no portals (these would only ever be
	// placed in a world file to prevent generation in that space).
	uint NumEmptyRooms = 0;
	FOR_EACH_ARRAY( GeneratedRoomIter, m_GeneratedRooms, SGenRoom )
	{
		const SGenRoom& GeneratedRoom = GeneratedRoomIter.GetValue();
		if( 0 == GeneratedRoom.m_Portals.Size() )
		{
			++NumEmptyRooms;
		}
	}

	const uint NumVisitableRooms = m_GeneratedRooms.Size() - NumEmptyRooms;

	Array<int>	VisitedRoomIdxs;
	Array<int>	BFSQueue;

	VisitedRoomIdxs.Reserve( NumVisitableRooms );
	VisitedRoomIdxs.PushBack( 0 );

	BFSQueue.SetDeflate( false );
	BFSQueue.PushBack( 0 );

	while( !BFSQueue.Empty() )
	{
		const int		NextRoomIdx	= BFSQueue[0];
		BFSQueue.PopFront();

		const SGenRoom&	NextRoom	= m_GeneratedRooms[ NextRoomIdx ];
		FOR_EACH_ARRAY( PortalIter, NextRoom.m_Portals, SSectorPortal )
		{
			const SSectorPortal&	Portal	= PortalIter.GetValue();
			if( VisitedRoomIdxs.PushBackUnique( Portal.m_BackSector ) )
			{
				BFSQueue.PushBackUnique( Portal.m_BackSector );
			}
		}
	}

	return VisitedRoomIdxs.Size() == NumVisitableRooms;
}

void RosaWorldGen::SelectWorldFile( const uint WorldFileIndex )
{
	DEVASSERT( m_WorldFiles.IsValidIndex( WorldFileIndex ) );

	// Save this so we can retry the same world if we have to bail
	m_WorldFileIndex				= WorldFileIndex;
}

void RosaWorldGen::SelectRandomWorldFile()
{
	DEVASSERT( m_IsUsingWorldFile );
	DEVASSERT( !m_WorldFiles.Empty() );

	const uint SelectedWorldFileIndex = Math::ArrayWeightedRandom( m_WorldFiles, []( const SWorldFile& WorldFile ) { return WorldFile.m_Weight; } );
	SelectWorldFile( SelectedWorldFileIndex );

	if( !m_SimmingGeneration )
	{
		PRINTF( "Selected world file %s\n", m_WorldFiles[ m_WorldFileIndex ].m_Filename.CStr() );
	}
}

bool RosaWorldGen::GenerateRooms()
{
	PROFILE_FUNCTION;

	CleanUp();

	bool WorldSeeded = false;

	// First roll our world file, if we have any.
	if( m_IsUsingWorldFile )
	{
		// If this is the first time generating rooms, pick randomly.
		// Else, reuse the same world file we tried last time. This
		// ensures that the world file's weight is respected instead
		// of biasing toward world files which fail generation less
		// frequently.
		if( m_WorldFileIndex == INVALID_ARRAY_INDEX )
		{
			SelectRandomWorldFile();
		}
		else
		{
			SelectWorldFile( m_WorldFileIndex );
		}

		// Try to seed with world file first (assuming it contains a start)...
		WorldSeeded = InsertWorldFileRooms();
	}

	// ...then fall back to seeding from a CriticalPathT = 0.0 feature room.
	if( !WorldSeeded )
	{
		WorldSeeded = InsertSeedFeatureRoom();
	}

	if( !WorldSeeded )
	{
		WARNDESC( "No seed for RosaWorldGen::GenerateRooms." );
		return true;	// Returning false would retry generation and cause an endless loop here
	}

	// Generate remainder of crit path, only if we're not using a world file.
	if( !m_IsUsingWorldFile )
	{
		const bool CriticalPathBuilt = InsertCriticalPathRooms();
		if( !CriticalPathBuilt )
		{
			// Bail and retry generation
			// (Fail reason is counted in InsertCriticalPathRooms.)
			return false;
		}
	}

	const uint NumWorldRooms = m_GeneratedRooms.Size();

	// Add "fill" rooms until the level is complete.
	const bool FillRoomsAdded = InsertFillRooms();
	if( !FillRoomsAdded )
	{
		// Bail and retry generation
		// (Fail reason is counted in InsertFillRooms.)
		return false;
	}
	const uint NumFillRooms = m_GeneratedRooms.Size() - NumWorldRooms;

	const bool ConnectiveRoomsAdded = m_ConnectiveRooms.Empty() || InsertConnectiveRooms();
	if( !ConnectiveRoomsAdded )
	{
		// Bail and retry generation
		// (This shouldn't happen unless portal tags are wonky.)
		return false;
	}
	const uint NumConnRooms = m_GeneratedRooms.Size() - ( NumWorldRooms + NumFillRooms );
	Unused( NumConnRooms );

	// NOTE: Prior to Rosa, on Vamp, coalescing happened inside CreatePortalsForSector (and so portals
	// were coalesced when loading a map, which had the uncoalesced portals saved in it). This should
	// save a bit of time and map file size, and should function the same, but it's something to watch
	// for if I see new portal strangeness.
	// ZETANOTE: I had briefly moved this into PopulateWorld(), but now I need it to determine connectivity.
	CreateAndCoalesceGenRoomPortals();

	// Validate that all parts of the level are connected.
	const bool IsConnected = CheckRoomConnections();
	if( !IsConnected )
	{
#if BUILD_DEV
		m_FailReasons.m_Connections++;

		if( m_AllowUnconnectedWorlds )
		{
			if( !m_SimmingGeneration )
			{
				PRINTF( "!!!!! BUILDING UNCONNECTED WORLD !!!!!\n" );
			}
			// Go ahead and build the world with missing connections
		}
		else
#endif
		{
			return false;
		}
	}

	// TODO: Revisit this; I'm not using it anymore, and it's not taking connective rooms into account.
	if( ( m_MinLoops > 0 && m_NumLoops	< m_MinLoops ) ||
		( m_MaxLoops > 0 && m_NumLoops	> m_MaxLoops ) )
	{
		// World had too few or too many loops. Lots of reasons this could happen.
		// Adjust module ratios, extend branch distance, create new modules, etc.
#if BUILD_DEV
		m_FailReasons.m_Loops++;
#endif
		return false;
	}

	// TODO: Revisit this; I'm not using it anymore, and it's not taking connective room dead ends into account.
	if( ( m_MinDeadEnds > 0 && m_NumDeadEnds < m_MinDeadEnds ) ||
		( m_MaxDeadEnds > 0 && m_NumDeadEnds > m_MaxDeadEnds ) )
	{
#if BUILD_DEV
		m_FailReasons.m_DeadEnds++;
#endif
		return false;
	}

	// TODO: Revisit this; I'm not using it anymore, and not sure if it's accurate.
	if( ( m_MinRooms > 0 && m_GeneratedRooms.Size()	< m_MinRooms ) ||
		( m_MaxRooms > 0 && m_GeneratedRooms.Size()	> m_MaxRooms ) ||
		( m_MinTiles > 0 && m_NumOccupiedTiles		< m_MinTiles ) ||
		( m_MaxTiles > 0 && m_NumOccupiedTiles		> m_MaxTiles ) )
	{
		// World was too big or too small. If we stall here,
		// MaxBranchDistance probably needs to be tuned.
#if BUILD_DEV
		m_FailReasons.m_Size++;
#endif
		return false;
	}

#if BUILD_DEV
	// Stats for simming generation
	if( m_SimmingGeneration )
	{
		m_GlobalStats.m_NumGenerations	+= 1;
		m_GlobalStats.m_RoomsSum		+= m_GeneratedRooms.Size();
		m_GlobalStats.m_FillRoomsSum	+= NumFillRooms;
		m_GlobalStats.m_ConnRoomsSum	+= NumConnRooms;
		if( m_IsUsingWorldFile )
		{
			SWorldGenStats& WorldFileStats = m_WorldFileStats[ m_WorldFileIndex ];
			WorldFileStats.m_NumGenerations	+= 1;
			WorldFileStats.m_RoomsSum		+= m_GeneratedRooms.Size();
			WorldFileStats.m_FillRoomsSum	+= NumFillRooms;
			WorldFileStats.m_ConnRoomsSum	+= NumConnRooms;
		}
	}
#endif

	return true;
}

float RosaWorldGen::GetFittingRoomWeight( const SFittingRoom& FittingRoom ) const
{
	const SRoom&	Room	= m_Rooms[ FittingRoom.m_RoomIndex ];
	DEVASSERT( Room.m_Weight > 0.0f );
	const float		Weight	= FittingRoom.m_FitWeight * Room.m_Weight;
	DEVASSERT( Weight > 0.0f );

	return Weight;
}

void RosaWorldGen::SelectAndInsertFittingRoom( const Array<SFittingRoom>& FittingRooms )
{
	ASSERT( FittingRooms.Size() > 0 );

	// Capture "this" because GetFittingRoomWeight is not a static function, it needs m_Rooms.
	// (Capturing "&" also works fine, I just want another example in my codebase.)
	const uint			SelectedFittingRoomIndex	= Math::ArrayWeightedRandom( FittingRooms,
		[this]( const SFittingRoom& FittingRoom )
		{
			return GetFittingRoomWeight( FittingRoom );
		}
	);
	const SFittingRoom&	RandomFit					= FittingRooms[ SelectedFittingRoomIndex ];
	DEVASSERT( GetFittingRoomWeight( RandomFit ) > 0.0f );
	InsertRoom( RandomFit.m_RoomIndex, RandomFit.m_LowLoc, RandomFit.m_Transform, RandomFit.m_ForCriticalPath, RandomFit.m_ForDeadEnd, RandomFit.m_CriticalPathAlpha );

	DEVASSERT( m_NumJustOpenedPortals == RandomFit.m_NumNewOpenPortals );
	m_NumLoops += RandomFit.m_NumLoops;

	if( RandomFit.m_ForDeadEnd )
	{
		m_NumDeadEnds++;
	}
}

void RosaWorldGen::FindFittingRooms(
	const uint RoomIndex,
	const SOpenPortal& OpenPortal,
	const bool ForCriticalPath,
	const uint MinNewOpenPortals,
	const float CriticalPathAlpha,
	Array<SFittingRoom>& OutFittingRooms
	) const
{
	//PROFILE_FUNCTION;

	const SRoom&			Module			= m_Rooms[ RoomIndex ];	// Ugh, naming

	if( Module.m_Weight == 0.0f )
	{
		// Modules shouldn't be used at all if they've been reduced to 0,
		// and the weighting algorithm won't properly handle 0 weights.
		return;
	}

	const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( OpenPortal.m_Room );
	const SPortal&			OccupiedPortal	= OccupiedTile.m_Portals.m_Portals[ OpenPortal.m_PortalIndex ];

	DEVASSERT( !OccupiedPortal.m_MustJoin );	// If this portal were must-join, it should never be open.
	if( OccupiedPortal.m_NoExpand )
	{
		// Can't expand through this portal
		return;
	}

	const RosaRoom&			Room			= GetRoomData( Module );
	const SRoomLoc			PortalRoomLoc	= GetRoomLocThroughPortal( OpenPortal.m_Room, OpenPortal.m_PortalIndex );

	// Fitting algorithm test:
	// For each portal in the room
	// If the portal tags match the expansion portal tags
	// If there is unoccupied space to align the module (location and orientation) to meet the portal
	// If all other tests pass (crit path validation, etc.)
	// Add the room with that location and orientation as a fit

	// ROSATODO: Move some of this stuff to RosaWorldGen::InitializeRoom?
	// Cache things by which portals they fit, and the requisite transform?

	uint NumFound = 0;
	FOR_EACH_ARRAY( PortalsIter, Room.m_Portals, SPortals )
	{
		const SPortals& Portals = PortalsIter.GetValue();
		for( uint PortalIndex = 0; PortalIndex < EPI_MAX; ++PortalIndex )
		{
			const SPortal&	Portal	= Portals.m_Portals[ PortalIndex ];

			// ROSANOTE: New behavior: back tags can be left unspecified and any front tag will match it.
			if( !Portal.Fits( OccupiedPortal ) )
			{
				continue;
			}

			// Portal tags match. Determine the appropriate orientation to fit.
			const uint	DesiredPortalIndex	= GetComplementaryPortalIndex( OpenPortal.m_PortalIndex );
			const uint	Transform			= GetPortalTransform( PortalIndex, DesiredPortalIndex );

			// Given that orientation, position this room to align to the expansion portal.
			// ROSATODO: Wrap this up if I reuse it.
			const uint	RoomTileIndex		= PortalsIter.GetIndex();
			const uint	RoomTileX			= RoomTileIndex % Room.m_TilesX;
			const uint	RoomTileY			= ( RoomTileIndex / Room.m_TilesX ) % Room.m_TilesY;
			const uint	RoomTileZ			= ( ( RoomTileIndex / Room.m_TilesX ) / Room.m_TilesY ) % Room.m_TilesZ;
			int TransformX, TransformY, TransformZ;
			GetTransformedOffset( RoomTileX, RoomTileY, RoomTileZ, Transform, TransformX, TransformY, TransformZ );

			// Position the whole module relative to that transform *and* to PortalRoomLoc (the tile on the far side of the expansion portal)
			// I need the south-west-bottom corner (i.e., "base location") for testing overlap with occupied tiles.
			// Subtract the transformed portal tile loc from the "tile on far side of expansion portal" loc to get the
			// world space position of the origin of the module. Then do the same clamped subtraction as I did before
			// to get the south west bottom corner from there. (Wrap that up in a function.)
			const SRoomLoc OriginLoc = SRoomLoc( PortalRoomLoc.X - TransformX, PortalRoomLoc.Y - TransformY, PortalRoomLoc.Z - TransformZ );
			SRoomLoc LowLoc, HighLoc;
			GetLowHighLocs( Room, OriginLoc, Transform, LowLoc, HighLoc );

			// Test that this placement doesn't overlap anything or violate any neighboring portals!!
			uint NumLoops			= 0;
			uint NumNewOpenPortals	= 0;
			static const bool skRevalidating = false;
			if( !ValidateRoom(
				Module,
				OriginLoc,
				LowLoc,
				HighLoc,
				Transform,
				skRevalidating,
				ForCriticalPath,
				MinNewOpenPortals,
				NumLoops,
				NumNewOpenPortals ) )
			{
				continue;
			}

#if BUILD_DEV
			if( m_DebugWorldGen && !m_SimmingGeneration )
			{
				//DEVPRINTF( "  Fitting room %s at %d,%d,%d with transform %d\n", m_Rooms[ RoomIndex ].m_Filename.CStr(), LowLoc.X, LowLoc.Y, LowLoc.Z, Transform );
			}
#endif

			NumFound++;
			SFittingRoom& NewFittingRoom		= OutFittingRooms.PushBack();
			NewFittingRoom.m_RoomIndex			= RoomIndex;
			NewFittingRoom.m_OriginLoc			= OriginLoc;
			NewFittingRoom.m_LowLoc				= LowLoc;
			NewFittingRoom.m_HighLoc			= HighLoc;
			NewFittingRoom.m_Transform			= Transform;
			NewFittingRoom.m_NumLoops			= NumLoops;
			NewFittingRoom.m_NumNewOpenPortals	= NumNewOpenPortals;
			NewFittingRoom.m_ForCriticalPath	= ForCriticalPath;
			NewFittingRoom.m_ForDeadEnd			= Module.m_DeadEnd;
			NewFittingRoom.m_CriticalPathAlpha	= CriticalPathAlpha;
			NewFittingRoom.m_ExpandPriority		= OccupiedPortal.m_ExpandPriority;
		}
	}

	// Apply a weight based on the number of times room is added, so e.g., a NSEW configuration
	// isn't more likely to be used just because it attaches in a lot of ways.
	// ROSANOTE: I don't try to correct weight when revalidating, because how would I?
	if( NumFound > 0 )
	{
		const float FitWeight = 1.0f / static_cast<float>( NumFound );
		const uint NumFittingRooms = OutFittingRooms.Size();
		for( uint FittingRoomIndex = NumFittingRooms - NumFound; FittingRoomIndex < NumFittingRooms; ++FittingRoomIndex )
		{
			SFittingRoom&	FittingRoom	= OutFittingRooms[ FittingRoomIndex ];
			FittingRoom.m_FitWeight		= FitWeight;
		}
	}
}

bool RosaWorldGen::RevalidateFittingRoom( SFittingRoom& FittingRoom, const bool ForCriticalPath, const uint MinNewOpenPortals ) const
{
	//PROFILE_FUNCTION;

	const SRoom&		Module				= m_Rooms[ FittingRoom.m_RoomIndex ];

	if( Module.m_Weight == 0.0f )
	{
		// Modules shouldn't be used at all if they've been reduced to 0,
		// and the weighting algorithm won't properly handle 0 weights.
		return false;
	}

	uint				NumLoops			= 0;
	uint				NumNewOpenPortals	= 0;
	static const bool	skRevalidating		= true;
	if( !ValidateRoom(
		Module,
		FittingRoom.m_OriginLoc,
		FittingRoom.m_LowLoc,
		FittingRoom.m_HighLoc,
		FittingRoom.m_Transform,
		skRevalidating,
		ForCriticalPath,
		MinNewOpenPortals,
		NumLoops,
		NumNewOpenPortals ) )
	{
		return false;
	}

	// Update fitting room with new info
	// ROSANOTE: I don't try to correct m_FitWeight when revalidating
	// (except for the 0 check above), because how would I without
	// context of the other remaining rooms found in the same pass?
	FittingRoom.m_NumLoops			= NumLoops;
	FittingRoom.m_NumNewOpenPortals	= NumNewOpenPortals;

	return true;
}

void RosaWorldGen::FilterFittingRooms( Array<SFittingRoom>& InOutFittingRooms ) const
{
	//PROFILE_FUNCTION;

	if( InOutFittingRooms.Empty() )
	{
		return;
	}

	// Filter anything below our higher portal expand priority
	{
		int MaxExpandPriority = INT_MIN;
		FOR_EACH_ARRAY( FittingRoomIter, InOutFittingRooms, SFittingRoom )
		{
			const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
			if( FittingRoom.m_ExpandPriority > MaxExpandPriority )
			{
				MaxExpandPriority = FittingRoom.m_ExpandPriority;
			}
		}

		FOR_EACH_ARRAY_REVERSE( FittingRoomIter, InOutFittingRooms, SFittingRoom )
		{
			const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
			if( FittingRoom.m_ExpandPriority < MaxExpandPriority )
			{
				InOutFittingRooms.FastRemove( FittingRoomIter );
			}
		}
	}

	// Filter anything below our highest priority
	{
		int MaxPriority = INT_MIN;
		FOR_EACH_ARRAY( FittingRoomIter, InOutFittingRooms, SFittingRoom )
		{
			const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
			const SRoom&		Module		= m_Rooms[ FittingRoom.m_RoomIndex ];
			if( Module.m_Priority > MaxPriority )
			{
				MaxPriority = Module.m_Priority;
			}
		}

		FOR_EACH_ARRAY_REVERSE( FittingRoomIter, InOutFittingRooms, SFittingRoom )
		{
			const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
			const SRoom&		Module		= m_Rooms[ FittingRoom.m_RoomIndex ];
			if( Module.m_Priority < MaxPriority )
			{
				InOutFittingRooms.FastRemove( FittingRoomIter );
			}
		}
	}

	// Disabling this for Zeta, I'd rather control this explicitly by putting dead end
	// rooms in a separate priority bin if I want this behavior. I'm currently trying
	// to get *some* dead ends into the mix in a dungeon, and this was preventing it.
	// ZETATODO: Maaaybe this should be controlled by worldgendef config params.
#if 0
	// Then, if we have any non-dead end fitting rooms at that priority, use only those.
	// Or, if we have any "true" dead ends (no new open portals), use those. (As opposed
	// to rooms tagged as dead ends, but which may have open portals because they're used
	// to clad the level with an outer wall of rooms. This was used in Vamp for outdoor
	// levels; ideally, I don't do that sort of thing again because it's a mess. Like,
	// I can't remember *why* true dead ends take precedence over false ones here.)
	{
		bool HaveOpenNonDeadEnds	= false;	// Non-dead ends with new open portals
		bool HaveClosedNonDeadEnds	= false;	// Non-dead ends with no new open portals
		bool HaveTrueDeadEnds		= false;	// Dead ends with no new open portals
		FOR_EACH_ARRAY( FittingRoomIter, InOutFittingRooms, SFittingRoom )
		{
			const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
			const SRoom&		Module		= m_Rooms[ FittingRoom.m_RoomIndex ];

			if( !Module.m_DeadEnd )
			{
				if( FittingRoom.m_NumNewOpenPortals > 0 )
				{
					HaveOpenNonDeadEnds = true;
					break;	// We can break here because this takes utmost priority below.
				}
				else
				{
					HaveClosedNonDeadEnds = true;
				}
			}
			else
			{
				if( FittingRoom.m_NumNewOpenPortals == 0 )
				{
					HaveTrueDeadEnds = true;
				}
			}
		}

		if( HaveOpenNonDeadEnds )
		{
			FOR_EACH_ARRAY_REVERSE( FittingRoomIter, InOutFittingRooms, SFittingRoom )
			{
				const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
				const SRoom&		Module		= m_Rooms[ FittingRoom.m_RoomIndex ];
				if( Module.m_DeadEnd || FittingRoom.m_NumNewOpenPortals == 0 )
				{
					InOutFittingRooms.FastRemove( FittingRoomIter );
				}
			}
		}
		else if( HaveClosedNonDeadEnds )
		{
			FOR_EACH_ARRAY_REVERSE( FittingRoomIter, InOutFittingRooms, SFittingRoom )
			{
				const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
				const SRoom&		Module		= m_Rooms[ FittingRoom.m_RoomIndex ];
				if( Module.m_DeadEnd )
				{
					InOutFittingRooms.FastRemove( FittingRoomIter );
				}
			}
		}
		else if( HaveTrueDeadEnds )
		{
			FOR_EACH_ARRAY_REVERSE( FittingRoomIter, InOutFittingRooms, SFittingRoom )
			{
				const SFittingRoom&	FittingRoom	= FittingRoomIter.GetValue();
				if( FittingRoom.m_NumNewOpenPortals > 0 )	// We can assume Module.m_DeadEnd would be true, since HaveNonDeadEnds was false.
				{
					InOutFittingRooms.FastRemove( FittingRoomIter );
				}
			}
		}
	}
#endif

	// Filtering should never result in an empty set!
	// It is only meant to remove less ideal rooms from the possible set.
	DEVASSERT( !InOutFittingRooms.Empty() );
}

float RosaWorldGen::GetDistanceSqToCriticalPath( const Vector& TileLoc ) const
{
	DEVASSERT( !m_CriticalPathLocations.Empty() );

	float MinDistSq = FLT_MAX;

	FOR_EACH_ARRAY( CritPathIter, m_CriticalPathLocations, Vector )
	{
		const Vector&	CritPathLoc	= CritPathIter.GetValue();
		const float		DistSq		= ( TileLoc - CritPathLoc ).LengthSquared();
		if( DistSq < MinDistSq )
		{
			MinDistSq = DistSq;
		}
	}

	return MinDistSq;
}

bool RosaWorldGen::IsLocationWithinWorldBounds( const SRoomLoc& RoomLoc ) const
{
	DEVASSERT( m_IsUsingWorldFile );
	DEVASSERT( m_WorldFiles.IsValidIndex( m_WorldFileIndex ) );

	const SWorldFile&	WorldFile		= m_WorldFiles[ m_WorldFileIndex ];
	const SRoomLoc&		WorldFileBounds	= WorldFile.m_Bounds;

	return IsLocationWithinWorldBounds( RoomLoc, WorldFileBounds );
}

bool RosaWorldGen::IsLocationWithinWorldBounds( const SRoomLoc& RoomLoc, const SRoomLoc& WorldBounds ) const
{
	return IsLocationWithinWorldBounds( RoomLoc.X, RoomLoc.Y, RoomLoc.Z, WorldBounds );
}

bool RosaWorldGen::IsLocationWithinWorldBounds( const int X, const int Y, const int Z, const SRoomLoc& WorldBounds ) const
{
	if( X < 0 || X >= WorldBounds.X ||
		Y < 0 || Y >= WorldBounds.Y ||
		Z < 0 || Z >= WorldBounds.Z )
	{
		return false;
	}

	return true;
}

// Test that this placement doesn't overlap anything or violate any neighboring portals!!
// If we're on the critical path, any fitting room must not form a loop
// (close portals to 2+ different gen rooms) and must have 1+ just-opened portal.
bool RosaWorldGen::ValidateRoom(
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
	) const
{
	//PROFILE_FUNCTION;

	const RosaRoom& Room = GetRoomData( Module );

	OutNumLoops				= 0;
	OutNumNewOpenPortals	= 0;

	// HACKHACK for Zeta
	if( ( Module.m_MinTileZ > 0 && HighLoc.Z < Module.m_MinTileZ ) ||
		( Module.m_MaxTileZ > 0 && LowLoc.Z > Module.m_MaxTileZ ) )
	{
		// This room is for the wrong story of the building
		return false;
	}

	// Look for overlap with existing rooms
	for( int X = LowLoc.X; X <= HighLoc.X; ++X )
	{
		for( int Y = LowLoc.Y; Y <= HighLoc.Y; ++Y )
		{
			for( int Z = LowLoc.Z; Z <= HighLoc.Z; ++Z )
			{
				const SRoomLoc TestTile = SRoomLoc( X, Y, Z );
				if( IsOccupied( TestTile ) )
				{
					// This room would overlap an existing room.
					return false;
				}
			}
		}
	}

	Array<uint>	UniqueGenRooms;
	SPortals	TempPortals;
	int			TileX, TileY, TileZ;
	for( uint X = 0; X < Room.m_TilesX; ++X )
	{
		for( uint Y = 0; Y < Room.m_TilesY; ++Y )
		{
			for( uint Z = 0; Z < Room.m_TilesZ; ++Z )
			{
				GetTransformedOffset( X, Y, Z, Transform, TileX, TileY, TileZ );
				const SRoomLoc TileLoc = SRoomLoc( OriginLoc.X + TileX, OriginLoc.Y + TileY, OriginLoc.Z + TileZ );

				const uint		RoomTileIndex	= X + Y * Room.m_TilesX + Z * Room.m_TilesX * Room.m_TilesY;
				const SPortals&	RoomPortals		= Room.m_Portals[ RoomTileIndex ];
				SetPortals( RoomPortals, Transform, TempPortals );

				for( uint PortalIndex = 0; PortalIndex < EPI_MAX; ++PortalIndex )
				{
					const SPortal&				Portal			= TempPortals.m_Portals[ PortalIndex ];
					const bool					IsWall			= !Portal.m_FrontTag;
					const SRoomLoc				FarLocation		= GetRoomLocThroughPortal( TileLoc, PortalIndex );
					const SOccupiedTile* const	pOccupiedTile	= SafeGetOccupiedTile( FarLocation );

					if( pOccupiedTile )
					{
						if( Portal.m_NoJoin )
						{
							// We're not allowed to join with this portal, therefore this room doesn't fit
							return false;
						}

						const uint		BackPortalIndex	= GetComplementaryPortalIndex( PortalIndex );
						const SPortal&	BackPortal		= pOccupiedTile->m_Portals.m_Portals[ BackPortalIndex ];

						// ROSANOTE: New behavior: back tags can be left unspecified and any front tag will match it.
						if( !Portal.Fits( BackPortal ) )
						{
							// This room doesn't fit with neighboring portals/walls
							return false;
						}
					}
					else
					{
						if( Portal.m_MustJoin )
						{
							// We're required to join to an existing room through this portal, therefore this room doesn't fit
							return false;
						}

						if( !IsWall )
						{
							if( m_IsUsingWorldFile )
							{
								DEVASSERT( m_WorldFiles.IsValidIndex( m_WorldFileIndex ) );

								const SWorldFile&	WorldFile		= m_WorldFiles[ m_WorldFileIndex ];
								const SRoomLoc&		WorldFileBounds	= WorldFile.m_Bounds;

								if( !IsLocationWithinWorldBounds( FarLocation, WorldFileBounds ) )
								{
									// This room placement is invalid because it would open a portal beyond world bounds.
									// (This is allowed if we're connecting to an occupied tile, in case the world file
									// has rooms intentionally placed outside world bounds.)
									return false;
								}
							}
						}
					}

					if( IsWall )
					{
						// This portal fits and we don't need to open anything.
						continue;
					}

					if( pOccupiedTile )
					{
						// Count how many unique genrooms we touch so we don't create loops on crit path
						UniqueGenRooms.PushBackUnique( pOccupiedTile->m_GenRoomIndex );
					}
					else
					{
						// This is a portal with nothing on the other side yet
						OutNumNewOpenPortals++;
					}
				}
			}
		}
	}

	// Make sure crit path always has enough new open portals
	if( ForCriticalPath && OutNumNewOpenPortals < MinNewOpenPortals )
	{
		return false;
	}

	// Don't create loops in the crit path
	if( ForCriticalPath && UniqueGenRooms.Size() >= 2 )
	{
		return false;
	}

	// Check if we're too far from the crit path (NOTE: or outside the world bounds!)
	// (moved down here because it's shockingly expensive).
	// Don't keep branching beyond a certain distance from the crit path,
	// unless this is tagged as a dead end, in which case it's allowed.
	if( !Revalidating && !ForCriticalPath && !Module.m_DeadEnd && OutNumNewOpenPortals > 0 )
	{
		if( m_IsUsingWorldFile )
		{
			DEVASSERT( m_WorldFiles.IsValidIndex( m_WorldFileIndex ) );
			const SWorldFile&	WorldFile		= m_WorldFiles[ m_WorldFileIndex ];
			const SRoomLoc&		WorldFileBounds	= WorldFile.m_Bounds;

			for( int X = LowLoc.X; X <= HighLoc.X; ++X )
			{
				for( int Y = LowLoc.Y; Y <= HighLoc.Y; ++Y )
				{
					for( int Z = LowLoc.Z; Z <= HighLoc.Z; ++Z )
					{
						// Instead of checking distance to crit path, check containment in world bounds
						if( !IsLocationWithinWorldBounds( X, Y, Z, WorldFileBounds ) )
						{
							return false;
						}
					}
				}
			}
		}
		else
		{
			for( int X = LowLoc.X; X <= HighLoc.X; ++X )
			{
				for( int Y = LowLoc.Y; Y <= HighLoc.Y; ++Y )
				{
					for( int Z = LowLoc.Z; Z <= HighLoc.Z; ++Z )
					{
						Vector TileLoc;
						TileLoc.x = static_cast<float>( X );
						TileLoc.y = static_cast<float>( Y );
						TileLoc.z = static_cast<float>( Z );
						const float DistanceSqToCritPath = GetDistanceSqToCriticalPath( TileLoc );
						if( DistanceSqToCritPath >= m_MaxBranchDistanceSq )
						{
							// Too far from crit path
							return false;
						}
					}
				}
			}
		}
	}

	// Since we don't need to validate the seed/starting room, this should always connect to at least one existing room.
	DEVASSERT( UniqueGenRooms.Size() > 0 );

	// Modules tagged as dead ends should never count for loops. This way,
	// I can use them for things like building facades that just clad a street
	// without actually expanding the footprint of the level in a meaningful way.
	if( !Module.m_DeadEnd )
	{
		OutNumLoops = UniqueGenRooms.Size() - 1;
	}

	return true;
}

// Get the west/south/bottom corner of the room with given orientation and origin at the given location
void RosaWorldGen::GetLowHighLocs( const RosaRoom& Room, const SRoomLoc& Location, const uint Transform, SRoomLoc& OutLowLoc, SRoomLoc& OutHighLoc ) const
{
	int OffsetX, OffsetY, OffsetZ;
	GetTransformedOffset( Room.m_TilesX - 1, Room.m_TilesY - 1, Room.m_TilesZ - 1, Transform, OffsetX, OffsetY, OffsetZ );
	OutLowLoc.X		= Location.X + Min( OffsetX, 0 );
	OutLowLoc.Y		= Location.Y + Min( OffsetY, 0 );
	OutLowLoc.Z		= Location.Z + Min( OffsetZ, 0 );
	OutHighLoc.X	= OutLowLoc.X + Abs( OffsetX );
	OutHighLoc.Y	= OutLowLoc.Y + Abs( OffsetY );
	OutHighLoc.Z	= OutLowLoc.Z + Abs( OffsetZ );
}

// HACKHACK, correct for LowLoc later, on tile map instead of world space
void RosaWorldGen::GetOriginFromLowLocForTiles( const RosaRoom& Room, const SRoomLoc& LowLoc, const uint Transform, SRoomLoc& OutOrigin ) const
{
	int OffsetX, OffsetY, OffsetZ;
	GetTransformedOffset( Room.m_TilesX - 1, Room.m_TilesY - 1, Room.m_TilesZ - 1, Transform, OffsetX, OffsetY, OffsetZ );
	OutOrigin.X	= LowLoc.X + Max( -OffsetX, 0 );
	OutOrigin.Y	= LowLoc.Y + Max( -OffsetY, 0 );
	OutOrigin.Z	= LowLoc.Z + Max( -OffsetZ, 0 );
}

// HACKHACK, correct for LowLoc later
void RosaWorldGen::GetOriginFromLowLocForWorld( const RosaRoom& Room, const SRoomLoc& LowLoc, const uint Transform, SRoomLoc& OutOrigin ) const
{
	int OffsetX, OffsetY, OffsetZ;
	GetTransformedOffset( Room.m_TilesX, Room.m_TilesY, Room.m_TilesZ, Transform, OffsetX, OffsetY, OffsetZ );
	OutOrigin.X	= LowLoc.X + Max( -OffsetX, 0 );
	OutOrigin.Y	= LowLoc.Y + Max( -OffsetY, 0 );
	OutOrigin.Z	= LowLoc.Z + Max( -OffsetZ, 0 );
}

uint RosaWorldGen::GetPortalTransform( const uint PortalIndex, const uint DesiredPortalIndex ) const
{
	static const uint Transforms[ EPI_MAX * EPI_MAX ] =
	{
		ERT_None,	ERT_Rot180,	ERT_Rot270,	ERT_Rot90,	ERT_None,	ERT_None,
		ERT_Rot180,	ERT_None,	ERT_Rot90,	ERT_Rot270,	ERT_None,	ERT_None,
		ERT_Rot90,	ERT_Rot270,	ERT_None,	ERT_Rot180,	ERT_None,	ERT_None,
		ERT_Rot270,	ERT_Rot90,	ERT_Rot180,	ERT_None,	ERT_None,	ERT_None,
		ERT_None,	ERT_None,	ERT_None,	ERT_None,	ERT_None,	ERT_None,
		ERT_None,	ERT_None,	ERT_None,	ERT_None,	ERT_None,	ERT_None,
	};

	return Transforms[ EPI_MAX * DesiredPortalIndex + PortalIndex ];
}

void RosaWorldGen::InsertRoom( const uint RoomIndex, const SRoomLoc& LowLoc, const uint Transform, const bool ForCriticalPath, const bool ForDeadEnd, const float CriticalPathAlpha )
{
#if BUILD_DEV
	if( m_DebugWorldGen && !m_SimmingGeneration )
	{
		DEVPRINTF( "  Adding room %s at %d,%d,%d with transform %d\n", m_Rooms[ RoomIndex ].m_Filename.CStr(), LowLoc.X, LowLoc.Y, LowLoc.Z, Transform );
		if( !m_IsUsingWorldFile )
		{
			DEVPRINTF( "    Critical path alpha: %f\n", CriticalPathAlpha );
		}
	}
#endif

	m_NumJustOpenedPortals = 0;

	const uint		GenRoomIndex	= m_GeneratedRooms.Size();
	SGenRoom&		GenRoom			= m_GeneratedRooms.PushBack();
	GenRoom.m_RoomIndex				= RoomIndex;
	GenRoom.m_Location				= LowLoc;
	GenRoom.m_Transform				= Transform;
	GenRoom.m_ForCriticalPath		= ForCriticalPath;
	GenRoom.m_ForDeadEnd			= ForDeadEnd;
	GenRoom.m_CriticalPathAlpha		= CriticalPathAlpha;

	SRoom&			RoomDef			= m_Rooms[ RoomIndex ];
	DEVASSERT( RoomDef.m_Weight > 0.0f );

	RoomDef.m_Weight				*= RoomDef.m_Scale;		// Simple hysteresis

	const RosaRoom&	Room			= GetRoomData( RoomDef );

	// Mark room as occupied, and insert its portals
	for( uint X = 0; X < Room.m_TilesX; ++X )
	{
		for( uint Y = 0; Y < Room.m_TilesY; ++Y )
		{
			for( uint Z = 0; Z < Room.m_TilesZ; ++Z )
			{
				int LevelX, LevelY, LevelZ;
				GetTransformedOffset( X, Y, Z, Transform, LevelX, LevelY, LevelZ );

				SRoomLoc OriginLoc;
				GetOriginFromLowLocForTiles( Room, LowLoc, Transform, OriginLoc );

				LevelX += OriginLoc.X;
				LevelY += OriginLoc.Y;
				LevelZ += OriginLoc.Z;
				const SRoomLoc	LevelLoc		= SRoomLoc( LevelX, LevelY, LevelZ );
				SOccupiedTile&	OccupiedTile	= AddOccupiedTile( LevelLoc );	// ROSANOTE: This is the only place we should be creating an occupied tile
				OccupiedTile.m_GenRoomIndex		= GenRoomIndex;

				if( ForCriticalPath )
				{
					Vector& CriticalPathLocation	= m_CriticalPathLocations.PushBack();
					CriticalPathLocation.x			= static_cast<float>( LevelX );
					CriticalPathLocation.y			= static_cast<float>( LevelY );
					CriticalPathLocation.z			= static_cast<float>( LevelZ );
				}

				const uint	RoomTileIndex	= X + Y * Room.m_TilesX + Z * Room.m_TilesX * Room.m_TilesY;
				SetPortals( Room.m_Portals[ RoomTileIndex ], Transform, OccupiedTile.m_Portals );

				const uint	NumOpenedPortals	= OpenPortals( LevelLoc );
				const uint	NumClosedPortals	= ClosePortals( LevelLoc );

				DEVASSERT( NumOpenedPortals >= NumClosedPortals );
				m_NumJustOpenedPortals += ( NumOpenedPortals - NumClosedPortals );
			}
		}
	}
}

void RosaWorldGen::RemoveGenRoomByIndex( const uint GenRoomIndex )
{
	if( !m_GeneratedRooms.IsValidIndex( GenRoomIndex ) )
	{
		return;
	}

	// Fix up whatever occupied tiles will be swapped to this index from FastRemove
	const uint LastGenRoomIndex = m_GeneratedRooms.Size() - 1;
	FOR_EACH_MAP( OccupiedTileIter, m_OccupiedTiles, SRoomLoc, SOccupiedTile )
	{
		SOccupiedTile& OccupiedTile = OccupiedTileIter.GetValue();
		if( OccupiedTile.m_GenRoomIndex == LastGenRoomIndex )
		{
			OccupiedTile.m_GenRoomIndex = GenRoomIndex;
		}
	}

	m_GeneratedRooms.FastRemove( GenRoomIndex );
}

void RosaWorldGen::GetTransformedOffset( const int X, const int Y, const int Z, const uint Transform, int& OutX, int& OutY, int& OutZ ) const
{
	switch( Transform )
	{
	case ERT_None:		OutX =  X; OutY =  Y; OutZ = Z; return;
	case ERT_Rot90:		OutX = -Y; OutY =  X; OutZ = Z; return;
	case ERT_Rot180:	OutX = -X; OutY = -Y; OutZ = Z; return;
	case ERT_Rot270:	OutX =  Y; OutY = -X; OutZ = Z; return;
	}

	WARN;
}

Vector RosaWorldGen::GetLocationForRoomLoc( const SRoomLoc& RoomLoc ) const
{
	Vector Location;
	Location.x = static_cast<float>( RoomLoc.X * static_cast<int>( m_TileSizeX ) );
	Location.y = static_cast<float>( RoomLoc.Y * static_cast<int>( m_TileSizeY ) );
	Location.z = static_cast<float>( RoomLoc.Z * static_cast<int>( m_TileSizeZ ) );
	return Location;
}

Angles RosaWorldGen::GetAnglesForTransform( const uint Transform ) const
{
	switch( Transform )
	{
	case ERT_None:		return Angles( 0.0f, 0.0f, 0.0f );
	case ERT_Rot90:		return Angles( 0.0f, 0.0f, 0.5f * PI );
	case ERT_Rot180:	return Angles( 0.0f, 0.0f, PI );
	case ERT_Rot270:	return Angles( 0.0f, 0.0f, 1.5f * PI );
	}

	WARN;
	return Angles();
}

uint RosaWorldGen::OpenPortals( const SRoomLoc& RoomLoc )
{
	uint NumOpenedPortals = 0;
	const SOccupiedTile& OccupiedTile = GetOccupiedTile( RoomLoc );

	for( uint PortalIndex = 0; PortalIndex < EPI_MAX; ++PortalIndex )
	{
		const SPortal& Portal = OccupiedTile.m_Portals.m_Portals[ PortalIndex ];
		if( !Portal.m_FrontTag )
		{
			continue;
		}

		SOpenPortal& OpenPortal		= m_OpenPortals.PushBack();
		OpenPortal.m_Room			= RoomLoc;
		OpenPortal.m_PortalIndex	= PortalIndex;

		NumOpenedPortals++;
	}

	return NumOpenedPortals;
}

uint RosaWorldGen::ClosePortals( const SRoomLoc& RoomLoc )
{
	uint NumClosedPortals = 0;
	const SOccupiedTile& OccupiedTile = GetOccupiedTile( RoomLoc );

	for( uint PortalIndex = 0; PortalIndex < EPI_MAX; ++PortalIndex )
	{
		const SPortal& Portal = OccupiedTile.m_Portals.m_Portals[ PortalIndex ];
		if( !Portal.m_FrontTag )
		{
			continue;
		}

		// Look at neighboring tile
		const SRoomLoc				NeighborLoc		= GetRoomLocThroughPortal( RoomLoc, PortalIndex );;
		const SOccupiedTile* const	pNeighborTile	= SafeGetOccupiedTile( NeighborLoc );
		if( !pNeighborTile )
		{
			// No neighbor, so this portal must still be open
			continue;
		}

		const uint		BackPortalIndex	= GetComplementaryPortalIndex( PortalIndex );
#if BUILD_DEV
		const SPortal&	BackPortal		= pNeighborTile->m_Portals.m_Portals[ BackPortalIndex ];
		ASSERT( Portal.Fits( BackPortal ) );
#endif

		// Remove both portals
		RemoveOpenPortal( RoomLoc, PortalIndex );
		RemoveOpenPortal( NeighborLoc, BackPortalIndex );

		NumClosedPortals++;

		// If we're on the crit path, we can't be closing multiple portal
		// directions simultaneously. So this is always safe and correct.
		if( !m_HasCriticalPathDirection )
		{
			m_HasCriticalPathDirection	= true;
			m_CriticalPathDirection		= BackPortalIndex;
		}
	}

	return NumClosedPortals;
}

RosaWorldGen::SOccupiedTile& RosaWorldGen::AddOccupiedTile( const SRoomLoc& RoomLoc )
{
	DEBUGASSERT( !IsOccupied( RoomLoc ) );
	m_NumOccupiedTiles++;
	return m_OccupiedTiles[ RoomLoc ];
}

void RosaWorldGen::RemoveOccupiedTile( const SRoomLoc& RoomLoc )
{
	DEBUGASSERT( IsOccupied( RoomLoc ) );
	m_NumOccupiedTiles--;
	m_OccupiedTiles.Remove( RoomLoc );
}

const RosaWorldGen::SOccupiedTile& RosaWorldGen::GetOccupiedTile( const SRoomLoc& RoomLoc ) const
{
	DEBUGASSERT( IsOccupied( RoomLoc ) );
	return m_OccupiedTiles[ RoomLoc ];
}

RosaWorldGen::SOccupiedTile& RosaWorldGen::GetOccupiedTile( const SRoomLoc& RoomLoc )
{
	DEBUGASSERT( IsOccupied( RoomLoc ) );
	return m_OccupiedTiles[ RoomLoc ];
}

const RosaWorldGen::SOccupiedTile* RosaWorldGen::SafeGetOccupiedTile( const SRoomLoc& RoomLoc ) const
{
	Map<SRoomLoc, SOccupiedTile>::Iterator RoomIter = m_OccupiedTiles.Search( RoomLoc );
	if( RoomIter.IsNull() )
	{
		return NULL;
	}

	const SOccupiedTile& Tile = RoomIter.GetValue();
	return &Tile;
}

bool RosaWorldGen::IsOccupied( const SRoomLoc& RoomLoc ) const
{
	Map<SRoomLoc, SOccupiedTile>::Iterator RoomIter = m_OccupiedTiles.Search( RoomLoc );
	return RoomIter.IsValid();
}

RosaWorldGen::SRoomLoc RosaWorldGen::GetRoomLocThroughPortal( const SRoomLoc& RoomLoc, const uint PortalIndex ) const
{
	switch( PortalIndex )
	{
	case EPI_East:	return SRoomLoc( RoomLoc.X + 1,	RoomLoc.Y,		RoomLoc.Z );
	case EPI_West:	return SRoomLoc( RoomLoc.X - 1,	RoomLoc.Y,		RoomLoc.Z );
	case EPI_North:	return SRoomLoc( RoomLoc.X,		RoomLoc.Y + 1,	RoomLoc.Z );
	case EPI_South:	return SRoomLoc( RoomLoc.X,		RoomLoc.Y - 1,	RoomLoc.Z );
	case EPI_Up:	return SRoomLoc( RoomLoc.X,		RoomLoc.Y,		RoomLoc.Z + 1 );
	case EPI_Down:	return SRoomLoc( RoomLoc.X,		RoomLoc.Y,		RoomLoc.Z - 1 );
	}

	WARN;
	return SRoomLoc();
}

void RosaWorldGen::RemoveOpenPortal( const SRoomLoc& RoomLoc, const uint PortalIndex )
{
	FOR_EACH_ARRAY( OpenPortalIter, m_OpenPortals, SOpenPortal )
	{
		const SOpenPortal& OpenPortal = OpenPortalIter.GetValue();
		if( OpenPortal.m_Room == RoomLoc && OpenPortal.m_PortalIndex == PortalIndex )
		{
			m_OpenPortals.Remove( OpenPortalIter );
			return;
		}
	}

	// Trying to remove a portal that wasn't open!
	WARN;
}

uint RosaWorldGen::GetComplementaryPortalIndex( const uint PortalIndex ) const
{
	switch( PortalIndex )
	{
	case EPI_East:	return EPI_West;
	case EPI_West:	return EPI_East;
	case EPI_North:	return EPI_South;
	case EPI_South:	return EPI_North;
	case EPI_Up:	return EPI_Down;
	case EPI_Down:	return EPI_Up;
	}

	WARN;
	return 0;
}

uint RosaWorldGen::GetPortalBitsForPortalIndex( const uint PortalIndex ) const
{
	switch( PortalIndex )
	{
	case EPI_East:	return EPB_East;
	case EPI_West:	return EPB_West;
	case EPI_North:	return EPB_North;
	case EPI_South:	return EPB_South;
	case EPI_Up:	return EPB_Up;
	case EPI_Down:	return EPB_Down;
	}

	WARN;
	return EPB_None;
}

uint RosaWorldGen::GetPortalBitsFromValidPortals( const SPortals& Portals ) const
{
	uint PortalBits = EPB_None;

	if( Portals.m_Portals[ EPI_East ].m_FrontTag )	{ PortalBits |= EPB_East; }
	if( Portals.m_Portals[ EPI_West ].m_FrontTag )	{ PortalBits |= EPB_West; }
	if( Portals.m_Portals[ EPI_North ].m_FrontTag )	{ PortalBits |= EPB_North; }
	if( Portals.m_Portals[ EPI_South ].m_FrontTag )	{ PortalBits |= EPB_South; }
	if( Portals.m_Portals[ EPI_Up ].m_FrontTag )	{ PortalBits |= EPB_Up; }
	if( Portals.m_Portals[ EPI_Down ].m_FrontTag )	{ PortalBits |= EPB_Down; }

	return PortalBits;
}

// Gross. I could simplify this but also who cares.
void RosaWorldGen::SetPortals( const SPortals& InPortals, const uint Transform, SPortals& OutPortals ) const
{
	if( Transform == ERT_None )
	{
		OutPortals.m_Portals[ EPI_East ]	= InPortals.m_Portals[ EPI_East ];	// Right
		OutPortals.m_Portals[ EPI_West ]	= InPortals.m_Portals[ EPI_West ];	// Left
		OutPortals.m_Portals[ EPI_North ]	= InPortals.m_Portals[ EPI_North ];	// Front
		OutPortals.m_Portals[ EPI_South ]	= InPortals.m_Portals[ EPI_South ];	// Back
	}
	else if( Transform == ERT_Rot90 )
	{
		OutPortals.m_Portals[ EPI_North ]	= InPortals.m_Portals[ EPI_East ];	// Front <- Right
		OutPortals.m_Portals[ EPI_South ]	= InPortals.m_Portals[ EPI_West ];	// Back <- Left
		OutPortals.m_Portals[ EPI_West ]	= InPortals.m_Portals[ EPI_North ];	// Left <- Front
		OutPortals.m_Portals[ EPI_East ]	= InPortals.m_Portals[ EPI_South ];	// Right <- Back
	}
	else if( Transform == ERT_Rot180 )
	{
		OutPortals.m_Portals[ EPI_West ]	= InPortals.m_Portals[ EPI_East ];	// Left <- Right
		OutPortals.m_Portals[ EPI_East ]	= InPortals.m_Portals[ EPI_West ];	// Right <- Left
		OutPortals.m_Portals[ EPI_South ]	= InPortals.m_Portals[ EPI_North ];	// Back <- Front
		OutPortals.m_Portals[ EPI_North ]	= InPortals.m_Portals[ EPI_South ];	// Front <- Back
	}
	else if( Transform == ERT_Rot270 )
	{
		OutPortals.m_Portals[ EPI_South ]	= InPortals.m_Portals[ EPI_East ];	// Back <- Right
		OutPortals.m_Portals[ EPI_North ]	= InPortals.m_Portals[ EPI_West ];	// Front <- Left
		OutPortals.m_Portals[ EPI_East ]	= InPortals.m_Portals[ EPI_North ];	// Right <- Front
		OutPortals.m_Portals[ EPI_West ]	= InPortals.m_Portals[ EPI_South ];	// Left <- Back
	}

	OutPortals.m_Portals[ EPI_Up ]		= InPortals.m_Portals[ EPI_Up ];	// Up
	OutPortals.m_Portals[ EPI_Down ]	= InPortals.m_Portals[ EPI_Down ];	// Down
}

void RosaWorldGen::StoreModules()
{
	DEV_DECLARE_AND_START_CLOCK( StoreModulesClock );

	m_Modules.Clear();
	m_Modules.Reserve( m_GeneratedRooms.Size() );

	FOR_EACH_ARRAY( RoomIter, m_GeneratedRooms, SGenRoom )
	{
		const SGenRoom&	Room	= RoomIter.GetValue();
		SModule&		Module	= m_Modules.PushBack();

		Module.m_Filename		= m_Rooms[ Room.m_RoomIndex ].m_Filename;
		Module.m_Location		= Room.m_Location;
		Module.m_Transform		= Room.m_Transform;
		Module.m_Portals		= Room.m_Portals;
	}

	DEV_STOP_AND_REPORT_CLOCK( StoreModulesClock, "    " );
}

void RosaWorldGen::CleanUp()
{
	m_GeneratedRooms.Clear();
	m_OccupiedTiles.Clear();
	m_HasCriticalPathDirection = false;
	m_CriticalPathDirection = 0;
	m_CriticalPathLocations.Clear();
	m_NumOccupiedTiles = 0;
	m_OpenPortals.Clear();
	m_NumLoops = 0;
	m_NumDeadEnds = 0;

	// Reset hysteresis
	FOR_EACH_ARRAY( RoomIter, m_Rooms, SRoom )
	{
		SRoom&	Room	= RoomIter.GetValue();
		Room.m_Weight	= Room.m_DefaultWeight;
	}
}

void RosaWorldGen::PopulateWorld()
{
	PROFILE_FUNCTION;

	if( !m_SimmingGeneration )
	{
		DEVPRINTF( "  Populating world...\n" );
	}

	DEV_DECLARE_AND_START_CLOCK( PopulateWorldClock );
	DEV_DECLARE_CLOCK( CreateGeoFromRoomClock );
	DEV_DECLARE_CLOCK( CreateNavMeshFromRoomClock );
	DEV_DECLARE_CLOCK( PrepareSpawnEntitiesFromRoomClock );

	FOR_EACH_ARRAY( GenRoomIter, m_GeneratedRooms, SGenRoom )
	{
		const SGenRoom& GenRoom			= GenRoomIter.GetValue();
		const uint		GenRoomIndex	= GenRoomIter.GetIndex();	// Global index, not index into m_Rooms
		const SRoom&	Room			= m_Rooms[ GenRoom.m_RoomIndex ];

#if BUILD_DEV
		if( !m_SimmingGeneration )
		{
#if VERBOSE_WORLDGEN
			DEVPRINTF( "Populating room %s at %d,%d,%d with transform %d\n", Room.m_Filename.CStr(), GenRoom.m_Location.X, GenRoom.m_Location.Y, GenRoom.m_Location.Z, GenRoom.m_Transform );
#endif
			if( Room.m_Break )
			{
				WARNDESC( "Added Break room" );
			}
		}
#endif

#if BUILD_DEV
		m_CurrentRoomFilename = Room.m_Filename;
#endif

		if( m_SimmingGeneration )
		{
#if BUILD_DEV
			if( Room.m_Qualifier == "WORLD FILE" )
			{
				// HACKHACK: Skip gathering stats for rooms from world files.
				// At least for now, I don't care since there are lots of them and they are constant per world file.
			}
			else
			{
				// Gather stats instead of populating from room
				// NOTE: This format should match the one in PrintStats!
				const SimpleString	QualifiedFilename	= SimpleString::PrintF( "%s (%s)", Room.m_Filename.CStr(), Room.m_Qualifier.CStr() );
				m_GlobalStats.m_RoomStats[ QualifiedFilename ]++;
				if( m_IsUsingWorldFile )
				{
					SWorldGenStats&	WorldFileStats		= m_WorldFileStats[ m_WorldFileIndex ];
					WorldFileStats.m_RoomStats[ QualifiedFilename ]++;
				}
			}
#endif
			continue;
		}

		const RosaRoom&	RoomData	= GetRoomData( Room );

		DEV_START_CLOCK( CreateGeoFromRoomClock );
		CreateGeoFromRoom( RoomData, GenRoom.m_Transform, GenRoom.m_Location, Room.m_Filename );
		DEV_STOP_CLOCK( CreateGeoFromRoomClock );

		DEV_START_CLOCK( CreateNavMeshFromRoomClock );
		CreateNavMeshFromRoom( RoomData, GenRoom.m_Transform, GenRoom.m_Location );
		DEV_STOP_CLOCK( CreateNavMeshFromRoomClock );

		DEV_START_CLOCK( PrepareSpawnEntitiesFromRoomClock );
		PrepareSpawnEntitiesFromRoom( RoomData, GenRoomIndex, GenRoom.m_Transform, GenRoom.m_Location, GenRoom.m_ForCriticalPath, GenRoom.m_ForDeadEnd, GenRoom.m_CriticalPathAlpha );
		DEV_STOP_CLOCK( PrepareSpawnEntitiesFromRoomClock );
	}

	if( m_SimmingGeneration )
	{
		CleanUp();
		return;
	}

	DEV_REPORT_CLOCK( CreateGeoFromRoomClock, "    " );
	DEV_REPORT_CLOCK( CreateNavMeshFromRoomClock, "    " );
	DEV_REPORT_CLOCK( PrepareSpawnEntitiesFromRoomClock, "    " );

	// Along the same lines as CopyMazeToWorld, store a manifest for the world with the
	// names of modules and their transforms (location in the "maze" and rotation).
	StoreModules();

	DEV_DECLARE_AND_START_CLOCK( CreatePortalsForSectorClock );
	Array<Array<SSectorPortal> > PortalsArrays;
	FOR_EACH_ARRAY( GenRoomIter, m_GeneratedRooms, SGenRoom )
	{
		const SGenRoom&	GenRoom		= GenRoomIter.GetValue();
		const uint		SectorIndex	= GenRoomIter.GetIndex();
		CreatePortalsForSector( SectorIndex, GenRoom.m_Portals );
		PortalsArrays.PushBack( GenRoom.m_Portals );
	}
	DEV_STOP_AND_REPORT_CLOCK( CreatePortalsForSectorClock, "    " );

	BuildSectorVisMatrix( PortalsArrays );
	AddGeoMeshesToSectors();
	LinkNavMesh();
	CreateMinimapMeshes();

	FilterPreparedSpawns();
	ResolveEntitySpawns();

	CleanUp();

	DEV_STOP_AND_REPORT_CLOCK( PopulateWorldClock, "  " );

	DEVPRINTF( "  Finished populating world.\n" );
}

void RosaWorldGen::GenerateFromModules( const Array<SModule>& Modules )
{
	PROFILE_FUNCTION;

	DEVPRINTF( "  Generating world from modules...\n" );

	DEV_DECLARE_AND_START_CLOCK( GenerateFromModulesClock );
	DEV_DECLARE_CLOCK( CreateGeoFromRoomClock );
	DEV_DECLARE_CLOCK( CreateNavMeshFromRoomClock );

	// Copy modules so we'll preserve them for future serialization
	m_Modules.Clear();
	m_Modules.Reserve( Modules.Size() );

	// Then create the geo from those modules
	FOR_EACH_ARRAY( ModuleIter, Modules, SModule )
	{
		const SModule& Module = ModuleIter.GetValue();
		m_Modules.PushBack( Module );	// Deep copy modules because of strings and inner arrays

		RosaRoom Room;
		Room.Load( PackStream( Module.m_Filename.CStr() ) );

		DEV_START_CLOCK( CreateGeoFromRoomClock );
		CreateGeoFromRoom( Room, Module.m_Transform, Module.m_Location, Module.m_Filename );
		DEV_STOP_CLOCK( CreateGeoFromRoomClock );

		DEV_START_CLOCK( CreateNavMeshFromRoomClock );
		CreateNavMeshFromRoom( Room, Module.m_Transform, Module.m_Location );
		DEV_STOP_CLOCK( CreateNavMeshFromRoomClock );
	}

	DEV_REPORT_CLOCK( CreateGeoFromRoomClock, "    " );
	DEV_REPORT_CLOCK( CreateNavMeshFromRoomClock, "    " );

	DEV_DECLARE_AND_START_CLOCK( CreatePortalsForSectorClock );
	Array<Array<SSectorPortal> > PortalsArrays;
	FOR_EACH_ARRAY( ModuleIter, Modules, SModule )
	{
		const SModule&	Module		= ModuleIter.GetValue();
		const uint		SectorIndex	= ModuleIter.GetIndex();
		CreatePortalsForSector( SectorIndex, Module.m_Portals );
		PortalsArrays.PushBack( Module.m_Portals );
	}
	DEV_STOP_AND_REPORT_CLOCK( CreatePortalsForSectorClock, "    " );

	BuildSectorVisMatrix( PortalsArrays );
	AddGeoMeshesToSectors();
	LinkNavMesh();
	CreateMinimapMeshes();

	DEV_STOP_AND_REPORT_CLOCK( GenerateFromModulesClock, "  " );

	DEVPRINTF( "  Finished generating world from modules.\n" );
}

void RosaWorldGen::FilterPreparedSpawns()
{
	PROFILE_FUNCTION;

	DEV_DECLARE_AND_START_CLOCK( FilterPreparedSpawnsClock );

	FOR_EACH_MAP( GroupIter, m_SpawnResolveGroups, HashedString, SSpawnResolveGroup )
	{
		SSpawnResolveGroup& ResolveGroup = GroupIter.GetValue();

		FOR_EACH_ARRAY_REVERSE( PreparedSpawnIter, ResolveGroup.m_PreparedSpawns, SPreparedSpawn )
		{
			const SPreparedSpawn&	PreparedSpawn		= PreparedSpawnIter.GetValue();

			// Filter by distance to player
			if( ResolveGroup.m_MinRadiusSqFromPlayer > 0.0f )
			{
				const Vector			ToPlayer			= PreparedSpawn.m_SpawnLocationBase - m_PlayerStart;
				const float				DistanceSqToPlayer	= ToPlayer.LengthSquared();
				if( DistanceSqToPlayer < ResolveGroup.m_MinRadiusSqFromPlayer )
				{
					ResolveGroup.m_PreparedSpawns.FastRemove( PreparedSpawnIter );
					continue;
				}
			}

			// ZETATODO: Revisit this since I don't have crit path alpha anymore
			// Filter by minimum critical path alpha
			if( PreparedSpawn.m_CriticalPathAlpha < ResolveGroup.m_CriticalPathAlphaLo ||
				PreparedSpawn.m_CriticalPathAlpha > ResolveGroup.m_CriticalPathAlphaHi )
			{
				ResolveGroup.m_PreparedSpawns.FastRemove( PreparedSpawnIter );
				continue;
			}

			if( ( ResolveGroup.m_MinZ > 0.0f && PreparedSpawn.m_SpawnLocationBase.z < ResolveGroup.m_MinZ ) ||
				( ResolveGroup.m_MaxZ > 0.0f && PreparedSpawn.m_SpawnLocationBase.z > ResolveGroup.m_MaxZ ) )
			{
				ResolveGroup.m_PreparedSpawns.FastRemove( PreparedSpawnIter );
				continue;
			}
		}
	}

	DEV_STOP_AND_REPORT_CLOCK( FilterPreparedSpawnsClock, "    " );
}

void RosaWorldGen::ResolveEntitySpawnsInGroup( const HashedString& ResolveGroupName )
{
	DEVASSERT( m_SpawnResolveGroups.Search( ResolveGroupName ).IsValid() );
	SSpawnResolveGroup&	ResolveGroup	= m_SpawnResolveGroups[ ResolveGroupName ];

	RosaCampaign* const	pCampaign	= RosaCampaign::GetCampaign();
	DEVASSERT( pCampaign );

	// OLDVAMP
	const float	ThreatAlpha			= 0.0f; //pCampaign->GetThreatAlpha();
	const uint	ResolveLimit		= static_cast<uint>( Lerp( ResolveGroup.m_ResolveLimitLowThreat, ResolveGroup.m_ResolveLimitHighThreat, ThreatAlpha ) );
	const float	ResolveChance		= Lerp( ResolveGroup.m_ResolveChanceLowThreat, ResolveGroup.m_ResolveChanceHighThreat, ThreatAlpha );

	const uint	NumPreparedSpawns	= ResolveGroup.m_PreparedSpawns.Size();
	const uint	NumDesiredSpawnsPre	= Max( ResolveLimit, static_cast<uint>( ResolveChance * NumPreparedSpawns ) );
	const uint	NumDesiredSpawns	= pCampaign->ModifyInt( ResolveGroupName, NumDesiredSpawnsPre );	// HACKHACK: I don't have a better modifier key than the resolve group name!
	const uint	NumSpawns			= Min( NumPreparedSpawns, NumDesiredSpawns );

#if BUILD_DEV
	if( NumSpawns < NumDesiredSpawns )
	{
		const SimpleString	ResolveGroupString	= ReverseHash::ReversedHash( ResolveGroupName );
		PRINTF( "Not enough spawners in world to satisfy resolve group %s (%d of %d).\n", ResolveGroupString.CStr(), NumSpawns, NumDesiredSpawns );
	}
#endif
	ASSERTDESC( NumSpawns == NumDesiredSpawns, "Not enough spawners in world to satisfy resolve group limit." );

	if( NumSpawns == 0 )
	{
		// Do nothing
	}
	else if( NumSpawns == NumPreparedSpawns )
	{
		// Spawn in priority order (Neon-style, to spawn player first)

		// Sort prepared spawns by priority
		// Use insertion sort, because it has no heap allocation overhead and the array is already mostly sorted (assuming most things are priority 0)
		ResolveGroup.m_PreparedSpawns.InsertionSort(
			[]( const RosaWorldGen::SPreparedSpawn& PreparedSpawnA, const RosaWorldGen::SPreparedSpawn& PreparedSpawnB )
			{
				return PreparedSpawnA.m_SpawnPriority > PreparedSpawnB.m_SpawnPriority;
			}
		);

		FOR_EACH_ARRAY( PreparedSpawnIter, ResolveGroup.m_PreparedSpawns, SPreparedSpawn )
		{
			SPreparedSpawn&	PreparedSpawn	= PreparedSpawnIter.GetValue();
			const Vector	SpawnLocation	= SpawnPreparedEntity( PreparedSpawn );
			ResolveGroup.m_SpawnedLocations.PushBack( SpawnLocation );
		}
	}
	else if( ResolveGroup.m_CriticalPathSampling )
	{
		// This should be guaranteed by the conditions to get here, but just be sure.
		DEVASSERT( NumSpawns < NumPreparedSpawns );
		DEVASSERT( NumSpawns > 0 );

		// Sort prepared spawns by critical path alpha and spawn evenly across range (systematic sampling)
		// Use insertion sort, because it has no heap allocation overhead and the array is assumed to be small
		// ZETATODO: Revisit this since I don't have crit path alpha anymore
		ResolveGroup.m_PreparedSpawns.InsertionSort(
			[]( const RosaWorldGen::SPreparedSpawn& PreparedSpawnA, const RosaWorldGen::SPreparedSpawn& PreparedSpawnB )
			{
				return PreparedSpawnA.m_CriticalPathAlpha < PreparedSpawnB.m_CriticalPathAlpha;
			}
		);

		uint		StepIndex	= NumPreparedSpawns / 2;	// Start at the central point of the sample step (this is arbitrary, could be anything [0, NumPreparedSpawns])
#if BUILD_DEV
		uint		SpawnCount	= 0;
#endif
		for( uint PreparedSpawnIndex = 0; PreparedSpawnIndex < NumPreparedSpawns; ++PreparedSpawnIndex, StepIndex = ( StepIndex + NumSpawns ) % NumPreparedSpawns )
		{
			if( StepIndex >= NumSpawns )
			{
				continue;
			}

			SPreparedSpawn&	PreparedSpawn	= ResolveGroup.m_PreparedSpawns[ PreparedSpawnIndex ];
			const Vector	SpawnLocation	= SpawnPreparedEntity( PreparedSpawn );
			ResolveGroup.m_SpawnedLocations.PushBack( SpawnLocation );
#if BUILD_DEV
			SpawnCount++;
#endif
		}

		DEVASSERT( SpawnCount == NumSpawns );
	}
	else if( ResolveGroup.m_SpawnNearGroup )
	{
		DEVASSERT( m_SpawnResolveGroups.Search( ResolveGroup.m_SpawnNearGroup ).IsValid() );
		const SSpawnResolveGroup& NearGroup = m_SpawnResolveGroups[ ResolveGroup.m_SpawnNearGroup ];

		// Sort spawns by distance from near locations using a multimap
		Multimap<float, uint> DistanceMap;
		FOR_EACH_ARRAY( SpawnIter, ResolveGroup.m_PreparedSpawns, SPreparedSpawn )
		{
			const SPreparedSpawn&	PreparedSpawn	= SpawnIter.GetValue();
			float					NearestDistSq	= FLT_MAX;
			FOR_EACH_ARRAY( NearIter, NearGroup.m_SpawnedLocations, Vector )
			{
				const Vector&	NearLocation	= NearIter.GetValue();
				const float		DistSq			= ( NearLocation - PreparedSpawn.m_SpawnLocationBase ).LengthSquared();
				NearestDistSq					= Min( NearestDistSq, DistSq );
			}
			DistanceMap.Insert( NearestDistSq, SpawnIter.GetIndex() );
		}

		// Spawn the first N items in distance priority order
		uint NumSpawned = 0;
		FOR_EACH_MULTIMAP( DistanceIter, DistanceMap, float, uint )
		{
			const uint		SpawnIndex		= DistanceIter.GetValue();
			SPreparedSpawn&	PreparedSpawn	= ResolveGroup.m_PreparedSpawns[ SpawnIndex ];
			const Vector	SpawnLocation	= SpawnPreparedEntity( PreparedSpawn );
			ResolveGroup.m_SpawnedLocations.PushBack( SpawnLocation );

			if( ++NumSpawned >= NumSpawns )
			{
				// Stop when we've spawned enough
				break;
			}
		}
	}
	else
	{
		// Spawn in random order (Eldritch-style, to use random spawn locations)
		for( uint SpawnCount = 0; SpawnCount < NumSpawns; ++SpawnCount )
		{
			const uint		PreparedSpawnIndex	= Math::Random( ResolveGroup.m_PreparedSpawns.Size() );
			SPreparedSpawn&	PreparedSpawn		= ResolveGroup.m_PreparedSpawns[ PreparedSpawnIndex ];
			const Vector	SpawnLocation		= SpawnPreparedEntity( PreparedSpawn );
			ResolveGroup.m_SpawnedLocations.PushBack( SpawnLocation );
			ResolveGroup.m_PreparedSpawns.FastRemove( PreparedSpawnIndex );
		}
	}
}

void RosaWorldGen::ResolveEntitySpawns()
{
	PROFILE_FUNCTION;

	DEV_DECLARE_AND_START_CLOCK( ResolveEntitySpawnsClock );

	// Sort resolve groups by priority using a multimap (priority -> group name)
	Multimap<int, HashedString> PriorityMap;
	FOR_EACH_MAP( GroupIter, m_SpawnResolveGroups, HashedString, SSpawnResolveGroup )
	{
		const HashedString&	ResolveGroupName	= GroupIter.GetKey();
		SSpawnResolveGroup&	ResolveGroup		= GroupIter.GetValue();

		if( ResolveGroupName == HashedString::NullString )
		{
			// Skip the null group, we'll always spawn it first
			continue;
		}

		PriorityMap.Insert( ResolveGroup.m_Priority, ResolveGroupName );
	}

	// Spawn null resolve group first regardless of priority
	ResolveEntitySpawnsInGroup( HashedString::NullString );

	FOR_EACH_MULTIMAP( PriorityIter, PriorityMap, int, HashedString )
	{
		const HashedString&	ResolveGroupName	= PriorityIter.GetValue();
		ResolveEntitySpawnsInGroup( ResolveGroupName );
	}

	// Clean up all the spawn locations now that we don't need them
	FOR_EACH_MAP( GroupIter, m_SpawnResolveGroups, HashedString, SSpawnResolveGroup )
	{
		SSpawnResolveGroup&	ResolveGroup	= GroupIter.GetValue();
		ResolveGroup.m_SpawnedLocations.Clear();
	}

	LinkEntitySpawns();

	DEV_STOP_AND_REPORT_CLOCK( ResolveEntitySpawnsClock, "    " );
}

void RosaWorldGen::LinkEntitySpawns()
{
	// Order of spawn resolve groups doesn't matter here, we just need the prepared spawns
	FOR_EACH_MAP( GroupIter, m_SpawnResolveGroups, HashedString, SSpawnResolveGroup )
	{
		SSpawnResolveGroup& ResolveGroup = GroupIter.GetValue();
		FOR_EACH_ARRAY( SpawnIter, ResolveGroup.m_PreparedSpawns, SPreparedSpawn )
		{
			const SPreparedSpawn&	PreparedSpawn	= SpawnIter.GetValue();
			if( NULL == PreparedSpawn.m_SpawnedEntity )
			{
				// We didn't use this spawner, nothing to link
				continue;
			}

			if( PreparedSpawn.m_LinkedSpawns.Empty() )
			{
				// Nothing was linked to this, don't send the event
				continue;
			}

			// Convert to entity refs now so we can pass the array around more easily
			Array<WBEntityRef> LinkedEntities;
			FOR_EACH_ARRAY( LinkIter, PreparedSpawn.m_LinkedSpawns, uint )
			{
				const uint						LinkedUID			= LinkIter.GetValue();
				Map<uint, WBEntity*>::Iterator	LinkUIDEntityIter	= m_LinkUIDEntities.Search( LinkedUID );
				WBEntity* const					pLinkedEntity		= LinkUIDEntityIter.IsValid() ? LinkUIDEntityIter.GetValue() : NULL;
				DEVASSERT( pLinkedEntity );
				LinkedEntities.PushBack( pLinkedEntity );
			}

			DEVASSERT( LinkedEntities.Size() );
			WB_MAKE_EVENT( SetLinkedEntities, NULL );
			WB_SET_AUTO( SetLinkedEntities, Pointer, LinkedEntities, &LinkedEntities );
			WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetLinkedEntities, PreparedSpawn.m_SpawnedEntity );
		}

		// NOW we can clear the prepared spawns
		ResolveGroup.m_PreparedSpawns.Clear();
	}
}

Vector RosaWorldGen::SpawnPreparedEntity( SPreparedSpawn& PreparedSpawn )
{
	WBEntity* const				pEntity		= WBWorld::GetInstance()->CreateEntity( PreparedSpawn.m_EntityDef );
	DEVASSERT( pEntity );

	if( !pEntity )
	{
		return Vector();
	}

	// Track what we spawned here, for linking
	DEVASSERT( PreparedSpawn.m_SpawnedEntity == NULL );
	PreparedSpawn.m_SpawnedEntity = pEntity;

	// Add this to the link map so other spawned entities can reference it
	DEVASSERT( m_LinkUIDEntities.Search( PreparedSpawn.m_LinkUID ).IsNull() );
	m_LinkUIDEntities[ PreparedSpawn.m_LinkUID ] = pEntity;

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	if( !pTransform )
	{
		return Vector();
	}

	const Vector SpawnLocation = PreparedSpawn.m_SpawnLocationBase + ( PreparedSpawn.m_SpawnLocationOffset * pTransform->GetScale() );
	pTransform->SetInitialTransform( SpawnLocation, PreparedSpawn.m_SpawnOrientation );

#if BUILD_DEV
	STATICHASH( ShouldDebugCheckSpawnClearance );
	MAKEHASHFROM( DefinitionName, pEntity->GetName() );
	const bool ShouldDebugCheckSpawnClearance = ConfigManager::GetInheritedBool( sShouldDebugCheckSpawnClearance, true, sDefinitionName );
	if( ShouldDebugCheckSpawnClearance )
	{
		WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
		const Vector				TestExtents	= pCollision ? pCollision->GetExtents() : Vector();
		CollisionInfo				Info;
		Info.m_In_CollideWorld		= true;
		Info.m_In_CollideEntities	= true;
		Info.m_In_CollidingEntity	= pEntity;
		Info.m_In_UserFlags			= EECF_EntityCollision;
		if( m_World->CheckClearance( SpawnLocation, TestExtents, Info ) )
		{
			PRINTF( "Spawned entity %s in room %s does not have clearance at %s!\n", pEntity->GetUniqueName().CStr(), PreparedSpawn.m_RoomFilename.CStr(), SpawnLocation.GetString().CStr() );
			WARNDESC( "Spawned entity does not have clearance!" );
		}
	}
#endif

	return PreparedSpawn.m_SpawnLocationBase;
}

#if BUILD_DEV
void RosaWorldGen::ValidateSpawners()
{
	FOR_EACH_ARRAY( RoomIter, m_Rooms, SRoom )
	{
		const SRoom& Room = RoomIter.GetValue();
		ValidateSpawnersInRoom( Room );
	}
}

void RosaWorldGen::ValidateSpawnersInRoom( const SRoom& Room )
{
	const RosaRoom&					RoomData	= GetRoomData( Room );
	const Array<RosaRoom::SBrush>&	RoomBrushes	= RoomData.GetBrushes();

	FOR_EACH_ARRAY( BrushIter, RoomBrushes, RosaRoom::SBrush )
	{
		const RosaRoom::SBrush& RoomBrush = BrushIter.GetValue();
		if( RoomBrush.m_Type == EBT_Spawner )
		{
			const SimpleString&	SpawnerDef	= RoomBrush.m_DefName;

			MAKEHASH( SpawnerDef );
			STATICHASH( Deprecated );
			const bool Deprecated = ConfigManager::GetInheritedBool( sDeprecated, false, sSpawnerDef );
			if( Deprecated )
			{
				PRINTF( "Using deprecated spawner def %s in room %s!\n", SpawnerDef.CStr(), Room.m_Filename.CStr() );
				WARNDESC( "Deprecated spawner" );
			}
		}
	}
}
#endif

void RosaWorldGen::PrepareSpawnEntitiesFromRoom( const RosaRoom& Room, const uint GenRoomIndex, const uint Transform, const SRoomLoc& RoomLoc, const bool ForCriticalPath, const bool ForDeadEnd, const float CriticalPathAlpha )
{
	PROFILE_FUNCTION;

	const Array<RosaRoom::SBrush>&	RoomBrushes	= Room.GetBrushes();
	FOR_EACH_ARRAY( BrushIter, RoomBrushes, RosaRoom::SBrush )
	{
		const RosaRoom::SBrush& RoomBrush		= BrushIter.GetValue();
		const uint				RoomBrushIndex	= BrushIter.GetIndex();
		if( RoomBrush.m_Type != EBT_Spawner )
		{
			continue;
		}

		const SimpleString&	RoomSpawnerDef	= RoomBrush.m_DefName;
		const SimpleString	SpawnerDef		= m_World->GetSpawnerOverride( RoomSpawnerDef );

		MAKEHASH( SpawnerDef );

#if BUILD_FINAL
		STATICHASH( DevOnly );
		const bool DevOnly = ConfigManager::GetInheritedBool( sDevOnly, false, sSpawnerDef );
		if( DevOnly )
		{
			continue;
		}
#endif

#if BUILD_DEV
		// NOTE: To fix up deprecated spawners:
		// Deprecated	= true
		// RemappedDef	= "..."
		// (See also RemappedDef in rosatools.cpp)
		STATICHASH( Deprecated );
		const bool Deprecated = ConfigManager::GetInheritedBool( sDeprecated, false, sSpawnerDef );
		if( Deprecated )
		{
			PRINTF( "Using deprecated spawner def %s in room %s!\n", SpawnerDef.CStr(), m_CurrentRoomFilename.CStr() );
			WARNDESC( "Deprecated spawner" );
		}
#endif

		STATICHASH( CritPathOnly );
		const bool CritPathOnly	= ConfigManager::GetInheritedBool( sCritPathOnly, false, sSpawnerDef );
		if( CritPathOnly && !ForCriticalPath )
		{
			continue;
		}

		// ROSANOTE: I can also control this per resolve group, but sometimes it is useful per-spawner
		// ZETATODO: Revisit this since I don't have crit path alpha anymore
		STATICHASH( CritPathAlphaLo );
		const float CritPathAlphaLo	= ConfigManager::GetInheritedFloat( sCritPathAlphaLo, 0.0f, sSpawnerDef );
		STATICHASH( CritPathAlphaHi );
		const float CritPathAlphaHi	= ConfigManager::GetInheritedFloat( sCritPathAlphaHi, 1.0f, sSpawnerDef );
		if( CriticalPathAlpha < CritPathAlphaLo ||
			CriticalPathAlpha > CritPathAlphaHi )
		{
			continue;
		}

		STATICHASH( BranchesOnly );
		const bool BranchesOnly	= ConfigManager::GetInheritedBool( sBranchesOnly, false, sSpawnerDef );
		if( BranchesOnly && ForCriticalPath )
		{
			continue;
		}

		STATICHASH( DeadEndsOnly );
		const bool DeadEndsOnly	= ConfigManager::GetInheritedBool( sDeadEndsOnly, false, sSpawnerDef );
		if( DeadEndsOnly && !ForDeadEnd )
		{
			continue;
		}

		STATICHASH( Chance );
		const float Chance		= ConfigManager::GetInheritedFloat( sChance, 1.0f, sSpawnerDef );
		if( !Math::RandomF( Chance ) )
		{
			continue;
		}

		const SimpleString ActualSpawnerDef = GetActualSpawnerDef( SpawnerDef );
		if( ActualSpawnerDef == "" )
		{
			continue;
		}

		STATICHASH( ResolveGroup );
		const HashedString ResolveGroupName = ConfigManager::GetInheritedHash( sResolveGroup, HashedString::NullString, sSpawnerDef );

		PrepareSpawnEntity( Room, GenRoomIndex, RoomBrushIndex, ActualSpawnerDef, ResolveGroupName, RoomBrush.m_Location, RoomBrush.m_Orientation, RoomBrush.m_LinkedBrushes, Transform, RoomLoc, CriticalPathAlpha );
	}
}

SimpleString RosaWorldGen::GetActualSpawnerDef( const SimpleString& SuperSpawnerDef )
{
	MAKEHASH( SuperSpawnerDef );

	STATICHASH( NumSubSpawners );
	const uint NumSubSpawners = ConfigManager::GetInheritedInt( sNumSubSpawners, 0, sSuperSpawnerDef );

	// ROSATODO: Revisit this without cumulative weight array? Maybe not worth it since
	// I'm doing config manager gets within this function for the weights. And that's
	// not great but whatever, this isn't a perf hotspot.
	if( NumSubSpawners > 0 )
	{
		Array<float> CumulativeWeightArray;
		CumulativeWeightArray.ResizeZero( NumSubSpawners );

		float WeightSum = 0.0f;
		for( uint SubSpawnerIndex = 0; SubSpawnerIndex < NumSubSpawners; ++SubSpawnerIndex )
		{
			WeightSum += ConfigManager::GetInheritedSequenceFloat( "SubSpawner%dWeight", SubSpawnerIndex, 1.0f, sSuperSpawnerDef );
			CumulativeWeightArray[ SubSpawnerIndex ] = WeightSum;
		}

		const float RolledWeight = Math::Random( 0.0f, WeightSum );
		for( uint SubSpawnerIndex = 0; SubSpawnerIndex < NumSubSpawners; ++SubSpawnerIndex )
		{
			if( RolledWeight <= CumulativeWeightArray[ SubSpawnerIndex ] )
			{
				return ConfigManager::GetInheritedSequenceString( "SubSpawner%d", SubSpawnerIndex, "", sSuperSpawnerDef );
			}
		}

		// We shouldn't ever get here!
		WARN;
	}

	return SuperSpawnerDef;
}

void RosaWorldGen::PrepareSpawnEntity( const RosaRoom& Room, const uint GenRoomIndex, const uint RoomBrushIndex, const SimpleString& SpawnerDef, const HashedString& ResolveGroupName, const Vector& Location, const Angles& Orientation, const Array<uint>& LinkedBrushes, const uint Transform, const SRoomLoc& RoomLoc, const float CriticalPathAlpha )
{
	MAKEHASH( SpawnerDef );

	STATICHASH( Condition );
	const SimpleString Condition = ConfigManager::GetInheritedString( sCondition, "", sSpawnerDef );
	WBParamEvaluator ConditionPE;
	ConditionPE.InitializeFromDefinition( Condition );
	if( ConditionPE.IsInitialized() )
	{
		WBParamEvaluator::SPEContext PEContext;
		ConditionPE.Evaluate( PEContext );
		if( !ConditionPE.GetBool() )
		{
			// Condition failed, don't spawn
			return;
		}
	}

	// ROSAHACK: Let the campaign enable/disable spawners by name
	RosaCampaign* const	pCampaign	= RosaCampaign::GetCampaign();
	DEVASSERT( pCampaign );

	STATICHASH( Enabled );
	const bool Enabled = pCampaign->OverrideBool( SpawnerDef, ConfigManager::GetInheritedBool( sEnabled, true, sSpawnerDef ), true /*ForceModify*/ );

	if( !Enabled )
	{
		return;
	}

	STATICHASH( OffsetX );
	const float OffsetX = ConfigManager::GetInheritedFloat( sOffsetX, 0.0f, sSpawnerDef );

	STATICHASH( OffsetY );
	const float OffsetY = ConfigManager::GetInheritedFloat( sOffsetY, 0.0f, sSpawnerDef );

	STATICHASH( OffsetZ );
	const float OffsetZ = ConfigManager::GetInheritedFloat( sOffsetZ, 0.0f, sSpawnerDef );

	STATICHASH( Priority );
	const int SpawnPriority	 = ConfigManager::GetInheritedInt( sPriority, 0, sSpawnerDef );

	STATICHASH( Entity );
	const SimpleString EntityDef = ConfigManager::GetInheritedString( sEntity, "", sSpawnerDef );
	ASSERT( EntityDef != "" );

	SRoomLoc OriginLoc;
	GetOriginFromLowLocForWorld( Room, RoomLoc, Transform, OriginLoc );

	const Vector	RoomLocation		= GetLocationForRoomLoc( OriginLoc );
	const Matrix	RoomLocationMat		= Matrix::CreateTranslation( RoomLocation );
	const Angles	RoomOrientation		= GetAnglesForTransform( Transform );
	const Matrix	RoomOrientationMat	= RoomOrientation.ToMatrix();
	const Matrix	RoomMat				= RoomOrientationMat * RoomLocationMat;

	const Matrix	ObjOrientationMat	= Orientation.ToMatrix();
	const Angles	SpawnOrientation	= ( ObjOrientationMat * RoomOrientationMat ).ToAngles();
	const Vector	SpawnOffset			= Vector( OffsetX, OffsetY, OffsetZ );
	const Vector	OrientedSpawnOffset	= SpawnOffset * ObjOrientationMat * RoomOrientationMat;
	const Vector	FixedOffsetZ		= Vector( 0.0f, 0.0f, 0.5f );
	const Vector	SpawnLocationBase	= ( Location * RoomMat ) - FixedOffsetZ;	// HACKHACK: Move the common Z offset from here...
	const Vector	SpawnLocationOffset	= OrientedSpawnOffset + FixedOffsetZ;		// HACKHACK: ...to here, so it will scale with entity.

	SSpawnResolveGroup&	ResolveGroup	= m_SpawnResolveGroups[ ResolveGroupName ];
	SPreparedSpawn&		PreparedSpawn	= ResolveGroup.m_PreparedSpawns.PushBack();
	PreparedSpawn.m_EntityDef			= EntityDef;
	PreparedSpawn.m_SpawnLocationBase	= SpawnLocationBase;
	PreparedSpawn.m_SpawnLocationOffset	= SpawnLocationOffset;
	PreparedSpawn.m_SpawnOrientation	= SpawnOrientation;
	DEVASSERT( RoomBrushIndex < 0xffff );
	DEVASSERT( GenRoomIndex < 0xffff );
	PreparedSpawn.m_LinkUID				= RoomBrushIndex | ( GenRoomIndex << 16 );
	PreparedSpawn.m_SpawnPriority		= SpawnPriority;
	PreparedSpawn.m_CriticalPathAlpha	= CriticalPathAlpha;
#if BUILD_DEV
	PreparedSpawn.m_RoomFilename		= m_CurrentRoomFilename;
#endif

	// Set up links between entities, to be resolved after they've all been spawned
	// (replacement for the Neon-era "loop metadata" stuff)
	PreparedSpawn.m_LinkedSpawns.Reserve( LinkedBrushes.Size() );
	FOR_EACH_ARRAY( LinkedBrushIter, LinkedBrushes, uint )
	{
		const uint	LinkedBrushIndex	= LinkedBrushIter.GetValue();
		uint&		LinkedSpawn			= PreparedSpawn.m_LinkedSpawns.PushBack();

		DEVASSERT( LinkedBrushIndex < 0xffff );
		DEVASSERT( GenRoomIndex < 0xffff );
		LinkedSpawn						= LinkedBrushIndex | ( GenRoomIndex << 16 );
	}

	// HACKHACK: Identify where the player is spawning by seeing if this entity will have a player component.
	STATICHASH( RosaPlayer );
	MAKEHASH( EntityDef );
	if( ConfigManager::GetInheritedString( sRosaPlayer, NULL, sEntityDef ) )
	{
		m_PlayerStart = SpawnLocationBase;
	}
}

void RosaWorldGen::GatherStats()
{
#if BUILD_DEV
	ResetStats();

	STATICHASH( RosaWorldGen );
	STATICHASH( NumStatsIterations );
	const uint NumStatsIterations = ConfigManager::GetInt( sNumStatsIterations, 0, sRosaWorldGen );

	STATICHASH( PrintStatsCycles );
	const uint PrintStatsCycles = ConfigManager::GetInt( sPrintStatsCycles, 10, sRosaWorldGen );

	for( uint GenerateIndex = 0; GenerateIndex < NumStatsIterations; ++GenerateIndex )
	{
		const uint DisplayIndex = GenerateIndex + 1;
		if( DisplayIndex % PrintStatsCycles == 0 ||
			DisplayIndex == NumStatsIterations )
		{
			PRINTF( "Stats cycle: %d/%d (%d fails)\n", DisplayIndex, NumStatsIterations, m_GlobalStats.m_FailsSum );
		}

		Generate( true );
	}

	//const SimpleString	TimeDateString	= TimeDate::GetTimeDateString();
	//const SimpleString	OutputFile		= SimpleString::PrintF( "worldgen-stats-%s-%s.csv", m_WorldGenDef.CStr(), TimeDateString.CStr() );
	const SimpleString	OutputFile		= SimpleString::PrintF( "worldgen-stats-%s.csv", m_WorldGenDef.CStr() );
	PrintStats( FileStream( OutputFile.CStr(), FileStream::EFM_Write ) );

	// This would open the file...
	//FileUtil::TryShellExecute( OutputFile.CStr(), "open" );
	// ...but what I really want is to open the containing folder.
	// This works on Windows, at least. Using "./" did not. Weird!
	FileUtil::TryShellExecute( "", "open" );
	// This doesn't work because "explore" is only for folders.
	// The way to open an explorer window on the file would be with "open" and specifying "explorer.exe /select,..."
	//FileUtil::TryShellExecute( OutputFile.CStr(), "explore" );

	ValidateSpawners();
#endif
}

#if BUILD_DEV
void RosaWorldGen::ResetStats()
{
	m_GlobalStats.Reset();
	m_WorldFileStats.Clear();
	FOR_EACH_INDEX( WorldFileIndex, m_WorldFiles.Size() )
	{
		SWorldGenStats& WorldFileStats = m_WorldFileStats.PushBack();
		WorldFileStats.m_WorldFileIndex = WorldFileIndex;
	}
}

void RosaWorldGen::PrintStats( const IDataStream& Stream )
{
	Stream.PrintF( "Global Stats:\n" );
	PrintStats( Stream, m_GlobalStats );

	if( m_IsUsingWorldFile )
	{
		const float fIterations		= static_cast<float>( m_GlobalStats.m_NumGenerations );
		// QuickSort does memcpy instead of deep copying each element, so use InsertionSort instead
		m_WorldFileStats.InsertionSort( [&]( const SWorldGenStats& WorldFileStatsA, const SWorldGenStats& WorldFileStatsB ) { return WorldFileStatsA.m_NumGenerations > WorldFileStatsB.m_NumGenerations; } );

		Stream.PrintF( "World Filename,Num Uses,Frequency\n" );
		FOR_EACH_ARRAY( WorldFileStatsIter, m_WorldFileStats, SWorldGenStats )
		{
			SWorldGenStats&		WorldFileStats	= WorldFileStatsIter.GetValue();
			const float			Frequency		= static_cast<float>( WorldFileStats.m_NumGenerations ) / fIterations;
			Stream.PrintF( "%s,%d,%f\n", m_WorldFiles[ WorldFileStats.m_WorldFileIndex ].m_Filename.CStr(), WorldFileStats.m_NumGenerations, Frequency );
		}

		FOR_EACH_ARRAY( WorldFileStatsIter, m_WorldFileStats, SWorldGenStats )
		{
			SWorldGenStats& WorldFileStats = WorldFileStatsIter.GetValue();
			Stream.PrintF( "\n%s Stats:\n", m_WorldFiles[ WorldFileStats.m_WorldFileIndex ].m_Filename.CStr() );
			PrintStats( Stream, WorldFileStats );
		}
	}
}

void RosaWorldGen::PrintStats( const IDataStream& Stream, const RosaWorldGen::SWorldGenStats& WorldGenStats )
{
	const float fIterations			= static_cast<float>( WorldGenStats.m_NumGenerations );
	const float AverageRooms		= static_cast<float>( WorldGenStats.m_RoomsSum )		/ fIterations;
	const float AverageFillRooms	= static_cast<float>( WorldGenStats.m_FillRoomsSum )	/ fIterations;
	const float AverageConnRooms	= static_cast<float>( WorldGenStats.m_ConnRoomsSum )	/ fIterations;
	const float AverageTime			= GET_CLOCK( WorldGenStats.m_StatsClock )				/ fIterations;
	const float AverageFails		= static_cast<float>( WorldGenStats.m_FailsSum )		/ fIterations;

	// HACKHACK: Add up the number of rooms that are actually relevant.
	uint		NumProcGenRooms	= 0;
	if( m_IsUsingWorldFile )
	{
		// If we're using world files, we don't use crit path or feature rooms.
		// (Even if they're defined, they're simply ignored!)
		NumProcGenRooms = m_StandardRooms.Size() + m_ConnectiveRooms.Size();
	}
	else
	{
		NumProcGenRooms = m_StandardRooms.Size() + m_CritPathRooms.Size();
		FOR_EACH_ARRAY( FeatureRoomIter, m_FeatureRooms, SFeatureRoom )
		{
			const SFeatureRoom&	FeatureRoom = FeatureRoomIter.GetValue();
			NumProcGenRooms += FeatureRoom.m_Rooms.Size();
		}
	}

	struct SRoomStats
	{
		SRoomStats()
		:	m_QualifiedFilename()
		,	m_NumUses( 0 )
		{
		}

		SimpleString	m_QualifiedFilename;
		uint			m_NumUses;
	};

	// DLP 31 Jul 2021: Mapping by qualifier so I can print connective rooms separately, or any similar set in the future.
	// The key here is not user-facing, but I'm sticking with a string instead of a hash so it stays alphabetically sorted.
	Map<SimpleString, Array<SRoomStats>>	SortedRoomStatsByQualifier;
	FOR_EACH_ARRAY( RoomIter, m_Rooms, SRoom )
	{
		const SRoom&					Room					= RoomIter.GetValue();

		// HACKHACK: Skip stats from world files; at least for now, I don't care about their frequency.
		if( Room.m_Qualifier == "WORLD FILE" )
		{
			continue;
		}

		// The key here is not user-facing, but I'm sticking with a string instead of a hash so it stays alphabetically sorted.
		Array<SRoomStats>&			SortedRoomStats		= SortedRoomStatsByQualifier[ Room.m_Qualifier ];
		SRoomStats&					RoomStats			= SortedRoomStats.PushBack();
		// NOTE: This format should match the one in PopulateWorld!
		RoomStats.m_QualifiedFilename					= SimpleString::PrintF( "%s (%s)", Room.m_Filename.CStr(), Room.m_Qualifier.CStr() );
		const TStatsMap::Iterator	RoomStatsIter		= WorldGenStats.m_RoomStats.Search( RoomStats.m_QualifiedFilename );
		RoomStats.m_NumUses								= RoomStatsIter.IsValid() ? RoomStatsIter.GetValue() : 0;
	}

	FOR_EACH_MAP( QualifierIter, SortedRoomStatsByQualifier, SimpleString, Array<SRoomStats> )
	{
		Array<SRoomStats>&	SortedRoomStats	= QualifierIter.GetValue();
		// QuickSort does memcpy instead of deep copying each element, so use InsertionSort instead
		SortedRoomStats.InsertionSort( [&]( const SRoomStats& RoomStatsA, const SRoomStats& RoomStatsB ) { return RoomStatsA.m_NumUses > RoomStatsB.m_NumUses; } );
	}

	// This all has to go on one line if I want Excel to properly sort the output (which I don't care about anymore because I'm sorting here)
	Stream.PrintF( "%d generations in %.2fs - %.3fs max/%.3fs avg - %.2f avg modules (%.2f fill/%.2f conn) - %d max/%.2f avg failed attempts - %d of %d modules used\n",
		WorldGenStats.m_NumGenerations, GET_CLOCK( WorldGenStats.m_StatsClock ),
		GET_CLOCK( WorldGenStats.m_ClockMax ), AverageTime,
		AverageRooms, AverageFillRooms, AverageConnRooms,
		WorldGenStats.m_FailsMax, AverageFails,
		WorldGenStats.m_RoomStats.Size(), NumProcGenRooms );

	if( WorldGenStats.m_FailsSum > 0 )
	{
		Stream.PrintF( "Fail Reason,Num Fails,Frequency\n" );
#define PRINTFAILREASON( reason ) \
			if( WorldGenStats.m_FailReasons.m_##reason > 0 ) \
			{ \
				Stream.PrintF( #reason ",%d,%f\n", WorldGenStats.m_FailReasons.m_##reason, static_cast<float>( WorldGenStats.m_FailReasons.m_##reason ) / fIterations ); \
			}
		PRINTFAILREASON( CriticalPath );
		PRINTFAILREASON( CriticalPath_FeatureRoom );
		PRINTFAILREASON( FillRooms );
		PRINTFAILREASON( Connections );
		PRINTFAILREASON( Loops );
		PRINTFAILREASON( DeadEnds );
		PRINTFAILREASON( Size );
#undef PRINTFAILREASON
	}

	Stream.PrintF( "Room Filename,Num Uses,Frequency\n" );
	FOR_EACH_MAP( QualifierIter, SortedRoomStatsByQualifier, SimpleString, Array<SRoomStats> )
	{
		Array<SRoomStats>&	SortedRoomStats	= QualifierIter.GetValue();
		FOR_EACH_ARRAY( RoomStatsIter, SortedRoomStats, SRoomStats )
		{
			const SRoomStats&	RoomStats		= RoomStatsIter.GetValue();
			const float			Frequency		= static_cast<float>( RoomStats.m_NumUses ) / fIterations;
			Stream.PrintF( "%s,%d,%f\n", RoomStats.m_QualifiedFilename.CStr(), RoomStats.m_NumUses, Frequency );
		}
	}
}
#endif // BUILD_DEV

// Fix up generated rooms with portal info now that all rooms exist.
void RosaWorldGen::CreateAndCoalesceGenRoomPortals()
{
	DEV_DECLARE_AND_START_CLOCK( CreateAndCoalesceGenRoomPortalsClock );
	FOR_EACH_ARRAY( GenRoomIter, m_GeneratedRooms, SGenRoom )
	{
		SGenRoom&		GenRoom	= GenRoomIter.GetValue();

		CreateGenRoomPortals( GenRoom );

		// ZETANOTE: It should be safe to coalesce portals before other gen rooms have their portals,
		// but if I run into any problems, just do a second iteration through m_GeneratedRooms to coalesce.
		CoalesceGenRoomPortals( GenRoom.m_Portals );
	}

	if( !m_SimmingGeneration )
	{
		DEV_STOP_AND_REPORT_CLOCK( CreateAndCoalesceGenRoomPortalsClock, "  " );
	}
}

void RosaWorldGen::CreateGenRoomPortals( SGenRoom& GenRoom )
{
	const SRoom&	Room		= m_Rooms[ GenRoom.m_RoomIndex ];
	const RosaRoom&	RoomData	= GetRoomData( Room );

	// Mark room as occupied, and insert its portals
	for( uint X = 0; X < RoomData.m_TilesX; ++X )
	{
		for( uint Y = 0; Y < RoomData.m_TilesY; ++Y )
		{
			for( uint Z = 0; Z < RoomData.m_TilesZ; ++Z )
			{
				int LevelX, LevelY, LevelZ;
				GetTransformedOffset( X, Y, Z, GenRoom.m_Transform, LevelX, LevelY, LevelZ );

				SRoomLoc OriginLoc;
				GetOriginFromLowLocForTiles( RoomData, GenRoom.m_Location, GenRoom.m_Transform, OriginLoc );

				LevelX += OriginLoc.X;
				LevelY += OriginLoc.Y;
				LevelZ += OriginLoc.Z;
				const SRoomLoc			LevelLoc		= SRoomLoc( LevelX, LevelY, LevelZ );
				const SOccupiedTile&	OccupiedTile	= GetOccupiedTile( LevelLoc );

				for( uint PortalIndex = 0; PortalIndex < EPI_MAX; ++PortalIndex )
				{
					const SPortal& Portal = OccupiedTile.m_Portals.m_Portals[ PortalIndex ];
					if( !Portal.m_FrontTag )
					{
						continue;
					}

#if BUILD_DEV
					if( m_DebugWorldGen )
					{
						// Do the same thing as below but with the safety check
						const SRoomLoc				NeighborLoc		= GetRoomLocThroughPortal( LevelLoc, PortalIndex );
						const SOccupiedTile* const	pNeighborTile	= SafeGetOccupiedTile( NeighborLoc );

						// Add the portal info
						if( pNeighborTile )
						{
							SSectorPortal& SectorPortal = GenRoom.m_Portals.PushBack();
							SectorPortal.m_LocationLo	= LevelLoc;
							SectorPortal.m_LocationHi	= LevelLoc;
							SectorPortal.m_PortalIndex	= PortalIndex;
							SectorPortal.m_BackSector	= pNeighborTile->m_GenRoomIndex;	// Sectors should always be ordered the same as genrooms
						}
					}
					else
#endif
					{
						const SRoomLoc			NeighborLoc		= GetRoomLocThroughPortal( LevelLoc, PortalIndex );
						const SOccupiedTile&	NeighborTile	= GetOccupiedTile( NeighborLoc );

						// Add the portal info
						SSectorPortal& SectorPortal = GenRoom.m_Portals.PushBack();
						SectorPortal.m_LocationLo	= LevelLoc;
						SectorPortal.m_LocationHi	= LevelLoc;
						SectorPortal.m_PortalIndex	= PortalIndex;
						SectorPortal.m_BackSector	= NeighborTile.m_GenRoomIndex;	// Sectors should always be ordered the same as genrooms
					}
				}
			}
		}
	}
}

void RosaWorldGen::CoalesceGenRoomPortals( Array<SSectorPortal>& Portals )
{
	// Coalesce portals for runtime perf.
	// Order of portals is not guaranteed, so I have to take that into account when merging.
	// Portals are adjacent if they have the same portal index and their location differs by 1 in only one orthogonal axis.
	// Portals are mergeable if they are adjacent and share a common back sector.
	Array<SSectorPortal> CoalescedPortals;
	CoalescedPortals.Reserve( Portals.Size() );

	FOR_EACH_ARRAY( PortalIter, Portals, SSectorPortal )
	{
		// Add the portal first
		CoalescedPortals.PushBack( PortalIter.GetValue() );

		// Then try to coalesce
		bool DidCoalesce;
		do
		{
			DidCoalesce = false;
			FOR_EACH_ARRAY_REVERSE( PortalBIter, CoalescedPortals, SSectorPortal )
			{
				// Skip the last item in the list (the portal we've just added or coalesced into)
				if( PortalBIter.GetIndex() == CoalescedPortals.Size() - 1 )
				{
					continue;
				}

				const uint				PortalBIndex	= PortalBIter.GetIndex();
				SSectorPortal&			PortalA			= CoalescedPortals.Last();
				const SSectorPortal&	PortalB			= PortalBIter.GetValue();

				if( PortalB.m_PortalIndex != PortalA.m_PortalIndex )
				{
					// Facing different directions, can't coalesce
					continue;
				}

				if( PortalB.m_BackSector != PortalA.m_BackSector )
				{
					// Different back sectors, can't coalesce
					continue;
				}

				const bool	AlignedX	=
					PortalA.m_LocationLo.X == PortalB.m_LocationLo.X &&
					PortalA.m_LocationHi.X == PortalB.m_LocationHi.X;
				const bool	AlignedY	=
					PortalA.m_LocationLo.Y == PortalB.m_LocationLo.Y &&
					PortalA.m_LocationHi.Y == PortalB.m_LocationHi.Y;
				const bool	AlignedZ	=
					PortalA.m_LocationLo.Z == PortalB.m_LocationLo.Z &&
					PortalA.m_LocationHi.Z == PortalB.m_LocationHi.Z;
				const int	DistanceX	= Max( PortalB.m_LocationLo.X - PortalA.m_LocationHi.X, PortalA.m_LocationLo.X - PortalB.m_LocationHi.X );
				const int	DistanceY	= Max( PortalB.m_LocationLo.Y - PortalA.m_LocationHi.Y, PortalA.m_LocationLo.Y - PortalB.m_LocationHi.Y );
				const int	DistanceZ	= Max( PortalB.m_LocationLo.Z - PortalA.m_LocationHi.Z, PortalA.m_LocationLo.Z - PortalB.m_LocationHi.Z );
				if( ( DistanceX == 1 && AlignedY && AlignedZ ) ||
					( DistanceY == 1 && AlignedX && AlignedZ ) ||
					( DistanceZ == 1 && AlignedX && AlignedY ) )
				{
					// All good
				}
				else
				{
					// Not adjacent, or dimensions aren't mergeable (would form a non-convex portal)
					continue;
				}

				// Portals are adjacent and share a common back sector, coalesce them (into PortalA)
				PortalA.m_LocationLo.X	= Min( PortalA.m_LocationLo.X, PortalB.m_LocationLo.X );
				PortalA.m_LocationLo.Y	= Min( PortalA.m_LocationLo.Y, PortalB.m_LocationLo.Y );
				PortalA.m_LocationLo.Z	= Min( PortalA.m_LocationLo.Z, PortalB.m_LocationLo.Z );
				PortalA.m_LocationHi.X	= Max( PortalA.m_LocationHi.X, PortalB.m_LocationHi.X );
				PortalA.m_LocationHi.Y	= Max( PortalA.m_LocationHi.Y, PortalB.m_LocationHi.Y );
				PortalA.m_LocationHi.Z	= Max( PortalA.m_LocationHi.Z, PortalB.m_LocationHi.Z );

				// Remove the redundant portal B
				CoalescedPortals.Remove( PortalBIndex );

				// We coalesced at least once so we'll restart from the top
				DidCoalesce = true;
			}
		}
		while( DidCoalesce );
	}

	Portals = CoalescedPortals;
}

void RosaWorldGen::CreatePortalsForSector( const uint SectorIndex, const Array<SSectorPortal>& Portals ) const
{
	RosaWorld::SSector& Sector = m_World->m_Sectors[ SectorIndex ];
	FOR_EACH_ARRAY( PortalIter, Portals, SSectorPortal )
	{
		const SSectorPortal&		GenPortal	= PortalIter.GetValue();
		RosaWorld::SSectorPortal&	WorldPortal	= Sector.m_Portals.PushBack();

		const Vector	TileOriginLo	= GetLocationForRoomLoc( GenPortal.m_LocationLo );
		const Vector	TileOriginHi	= GetLocationForRoomLoc( GenPortal.m_LocationHi );
		const Vector	TileSize		= Vector( static_cast<float>( m_TileSizeX ), static_cast<float>( m_TileSizeY ), static_cast<float>( m_TileSizeZ ) );
		const AABB		TileBound		= AABB( TileOriginLo, TileOriginHi + TileSize );

		switch( GenPortal.m_PortalIndex )
		{
		case EPI_East:
			WorldPortal.m_Corners[0]			= Vector( TileBound.m_Max.x, TileBound.m_Max.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[1]			= Vector( TileBound.m_Max.x, TileBound.m_Min.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[2]			= Vector( TileBound.m_Max.x, TileBound.m_Max.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[3]			= Vector( TileBound.m_Max.x, TileBound.m_Min.y, TileBound.m_Max.z );
			WorldPortal.m_FrontPlane.m_Normal	= Vector( -1.0f,  0.0f,  0.0f );
			WorldPortal.m_FrontPlane.m_Distance	= TileBound.m_Max.x;
			break;
		case EPI_West:
			WorldPortal.m_Corners[0]			= Vector( TileBound.m_Min.x, TileBound.m_Min.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[1]			= Vector( TileBound.m_Min.x, TileBound.m_Max.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[2]			= Vector( TileBound.m_Min.x, TileBound.m_Min.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[3]			= Vector( TileBound.m_Min.x, TileBound.m_Max.y, TileBound.m_Max.z );
			WorldPortal.m_FrontPlane.m_Normal	= Vector(  1.0f,  0.0f,  0.0f );
			WorldPortal.m_FrontPlane.m_Distance	= -TileBound.m_Min.x;
			break;
		case EPI_North:
			WorldPortal.m_Corners[0]			= Vector( TileBound.m_Min.x, TileBound.m_Max.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[1]			= Vector( TileBound.m_Max.x, TileBound.m_Max.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[2]			= Vector( TileBound.m_Min.x, TileBound.m_Max.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[3]			= Vector( TileBound.m_Max.x, TileBound.m_Max.y, TileBound.m_Max.z );
			WorldPortal.m_FrontPlane.m_Normal	= Vector(  0.0f, -1.0f,  0.0f );
			WorldPortal.m_FrontPlane.m_Distance	= TileBound.m_Max.y;
			break;
		case EPI_South:
			WorldPortal.m_Corners[0]			= Vector( TileBound.m_Max.x, TileBound.m_Min.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[1]			= Vector( TileBound.m_Min.x, TileBound.m_Min.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[2]			= Vector( TileBound.m_Max.x, TileBound.m_Min.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[3]			= Vector( TileBound.m_Min.x, TileBound.m_Min.y, TileBound.m_Max.z );
			WorldPortal.m_FrontPlane.m_Normal	= Vector(  0.0f,  1.0f,  0.0f );
			WorldPortal.m_FrontPlane.m_Distance	= -TileBound.m_Min.y;
			break;
		case EPI_Up:
			WorldPortal.m_Corners[0]			= Vector( TileBound.m_Min.x, TileBound.m_Max.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[1]			= Vector( TileBound.m_Max.x, TileBound.m_Max.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[2]			= Vector( TileBound.m_Min.x, TileBound.m_Min.y, TileBound.m_Max.z );
			WorldPortal.m_Corners[3]			= Vector( TileBound.m_Max.x, TileBound.m_Min.y, TileBound.m_Max.z );
			WorldPortal.m_FrontPlane.m_Normal	= Vector(  0.0f,  0.0f, -1.0f );
			WorldPortal.m_FrontPlane.m_Distance	= TileBound.m_Max.z;
			break;
		case EPI_Down:
			WorldPortal.m_Corners[0]			= Vector( TileBound.m_Min.x, TileBound.m_Min.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[1]			= Vector( TileBound.m_Max.x, TileBound.m_Min.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[2]			= Vector( TileBound.m_Min.x, TileBound.m_Max.y, TileBound.m_Min.z );
			WorldPortal.m_Corners[3]			= Vector( TileBound.m_Max.x, TileBound.m_Max.y, TileBound.m_Min.z );
			WorldPortal.m_FrontPlane.m_Normal	= Vector(  0.0f,  0.0f,  1.0f );
			WorldPortal.m_FrontPlane.m_Distance	= -TileBound.m_Min.z;
			break;
		}

		WorldPortal.m_BackSector	= GenPortal.m_BackSector;
	}
}

// Depth-first search through portals, marking each visited sector with directions not worth considering.
// This is an extremely conservative algorithm and marks most things visible because it does not consider portal size or position.
bool RosaWorldGen::IsSectorVisibleFromSector( const uint SectorIndexA, const uint SectorIndexB, const Array<Array<SSectorPortal> >& PortalsArrays, Array<uint>& OutSectorVisIncidentals ) const
{
	if( SectorIndexA == SectorIndexB )
	{
		return true;
	}

	m_ValidDirections.Clear();
	m_BacktrackSectors.Clear();
	m_DFSStack.Clear();
	m_BacktrackStack.Clear();

	m_ValidDirections.ResizeZero( m_World->m_Sectors.Size() );
	m_ValidDirections[ SectorIndexA ] = EPB_All;

	m_BacktrackSectors.ResizeZero( m_World->m_Sectors.Size() );

	m_DFSStack.Reserve( m_World->m_Sectors.Size() );
	m_DFSStack.PushBack( SectorIndexA );

	bool FoundSectorB = false;

	while( !m_DFSStack.Empty() )
	{
		const uint					SectorIndex		= m_DFSStack.Last();
		m_DFSStack.PopBack();

		if( SectorIndex == SectorIndexB )
		{
			// We've found the goal, but continue searching to make sure we have all paths.
			FoundSectorB = true;
		}

		// Since world sectors are created from genrooms/modules, indices should match, right?
		const Array<SSectorPortal>&	Portals			= PortalsArrays[ SectorIndex ];
		const uint					ValidDirs		= m_ValidDirections[ SectorIndex ];
		// Exit when we've run out of directions
		if( EPB_None == ValidDirs )
		{
			continue;
		}

		FOR_EACH_ARRAY( PortalIter, Portals, SSectorPortal )
		{
			const SSectorPortal&	Portal			= PortalIter.GetValue();
			bool					CanTraverse		= false;
			uint					NextValidDirs	= ValidDirs;

			switch( Portal.m_PortalIndex )
			{
			case EPI_East:	CanTraverse = ( 0 != ( ValidDirs & EPB_East ) );	NextValidDirs &= ~EPB_West;		break;
			case EPI_West:	CanTraverse = ( 0 != ( ValidDirs & EPB_West ) );	NextValidDirs &= ~EPB_East;		break;
			case EPI_North:	CanTraverse = ( 0 != ( ValidDirs & EPB_North ) );	NextValidDirs &= ~EPB_South;	break;
			case EPI_South:	CanTraverse = ( 0 != ( ValidDirs & EPB_South ) );	NextValidDirs &= ~EPB_North;	break;
			case EPI_Up:	CanTraverse = ( 0 != ( ValidDirs & EPB_Up ) );		NextValidDirs &= ~EPB_Down;		break;
			case EPI_Down:	CanTraverse = ( 0 != ( ValidDirs & EPB_Down ) );	NextValidDirs &= ~EPB_Up;		break;
			}

			if( !CanTraverse )
			{
				continue;
			}

			m_BacktrackSectors[ Portal.m_BackSector ].PushBack( SectorIndex );

			const uint ExistingValidDirs	= m_ValidDirections[ Portal.m_BackSector ];
			const uint MixedValidDirs		= NextValidDirs & ExistingValidDirs;
			if( NextValidDirs == MixedValidDirs )
			{
				// We've already visited this node with all the same directions.
				// Skip it (but we've still added this node for backtracking!).
				continue;
			}

			m_DFSStack.PushBackUnique( Portal.m_BackSector );
			m_ValidDirections[ Portal.m_BackSector ] |= NextValidDirs;
		}
	}

	if( FoundSectorB )
	{
		// We found the goal by at least one path, it's visible!
		// Build the array of incidentals (in-between sectors).
		DEBUGASSERT( OutSectorVisIncidentals.Empty() );

		m_BacktrackStack.Append( m_BacktrackSectors[ SectorIndexB ] );
		while( !m_BacktrackStack.Empty() )
		{
			const uint BacktrackSector = m_BacktrackStack.Last();
			m_BacktrackStack.PopBack();
			if( BacktrackSector == SectorIndexA )
			{
				continue;
			}

			// DLP 4 May 2019: I think something may be going wrong in the DFS above, but checking uniqueness should fix it for now.
			// (This warrants further investigation at some point... I don't think it's a real problem, but there shouldn't be loops here.)
			// (I've done further investigation, and there *can* be loops, and that's okay. For example:)
			// AAA
			// | |
			// B-C
			// In this example, B and C are both reachable from A by going south;
			// C is then reachable from B by going east, but B is *also* reachable
			// from C by going west; this creates a loop between B and C.
			if( OutSectorVisIncidentals.PushBackUnique( BacktrackSector ) )
			{
				m_BacktrackStack.Append( m_BacktrackSectors[ BacktrackSector ] );
			}
		}

		return true;
	}

	// We searched through every valid portal and found nothing, must not be visible
	return false;
}

void RosaWorldGen::BuildSectorVisMatrix( const Array<Array<SSectorPortal> >& PortalsArrays ) const
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	DEV_DECLARE_AND_START_CLOCK( BuildSectorVisMatrixClock );

	const uint SectorStride = m_World->m_Sectors.Size();

	DEVASSERT( m_World->m_SectorVisMatrix.Empty() );
	DEVASSERT( m_World->m_SectorVisIncidentals.Empty() );
	m_World->m_SectorVisMatrix.ResizeZero( Square( SectorStride ) );
	m_World->m_SectorVisIncidentals.ResizeZero( SectorStride );

	Array<uint> SectorVisIncidentals;
	SectorVisIncidentals.SetDeflate( false );
	SectorVisIncidentals.Reserve( SectorStride );

	FOR_EACH_INDEX( SectorIndexA, SectorStride )
	{
		FOR_EACH_INDEX_FROM( SectorIndexB, SectorIndexA, SectorStride )
		{
			SectorVisIncidentals.Clear();
			const bool IsVisible = IsSectorVisibleFromSector( SectorIndexA, SectorIndexB, PortalsArrays, SectorVisIncidentals );

			// Visibility is commutative (which is why we're iterating only the triangle)
			m_World->m_SectorVisMatrix[ SectorIndexA + SectorIndexB * SectorStride ] = IsVisible;
			m_World->m_SectorVisMatrix[ SectorIndexB + SectorIndexA * SectorStride ] = IsVisible;

			if( IsVisible && !SectorVisIncidentals.Empty() )
			{
				m_World->m_SectorVisIncidentals[ SectorIndexA ][ SectorIndexB ] = SectorVisIncidentals;
			}
		}
	}

	DEV_STOP_AND_REPORT_CLOCK( BuildSectorVisMatrixClock, "    " );
}

void RosaWorldGen::AddGeoMeshesToSectors() const
{
	PROFILE_FUNCTION;

	DEV_DECLARE_AND_START_CLOCK( AddGeoMeshesToSectorsClock );

	FOR_EACH_ARRAY( MeshIter, m_World->m_GeoMeshes, RosaWorld::SGeoMesh )
	{
		const RosaWorld::SGeoMesh&	GeoMesh		= MeshIter.GetValue();
		const uint					MeshIndex	= MeshIter.GetIndex();
		const AABB&					MeshBound	= GeoMesh.m_Mesh->m_AABB;

		FOR_EACH_ARRAY( SectorIter, m_World->m_Sectors, RosaWorld::SSector )
		{
			RosaWorld::SSector& Sector = SectorIter.GetValue();
			// HACKHACK: Use the slightly contracted bound here so we don't
			// grab meshes from adjacent sectors that are flush on the boundary.
			if( MeshBound.Intersects( Sector.m_GeoMeshBound ) )
			{
				Sector.m_GeoMeshes.PushBack( MeshIndex );
			}
		}
	}

	// Combine geomeshes with identical materials now that they're assigned to sectors
	m_World->FinalizeGeoMeshes();

	DEV_STOP_AND_REPORT_CLOCK( AddGeoMeshesToSectorsClock, "    " );
}

void RosaWorldGen::CreateGeoFromRoom( const RosaRoom& Room, const uint Transform, const SRoomLoc& RoomLoc, const SimpleString& RoomFilename )
{
	PROFILE_FUNCTION;

	SRoomLoc OriginLoc;
	GetOriginFromLowLocForWorld( Room, RoomLoc, Transform, OriginLoc );

	const Vector	RoomLocation		= GetLocationForRoomLoc( OriginLoc );
	const Matrix	RoomLocationMat		= Matrix::CreateTranslation( RoomLocation );
	const Angles	RoomOrientation		= GetAnglesForTransform( Transform );
	const Matrix	RoomOrientationMat	= RoomOrientation.ToMatrix();
	const Matrix	RoomMat				= RoomOrientationMat * RoomLocationMat;

	// Side effect: also create a new sector!
	RosaWorld::SSector&	RoomSector			= m_World->m_Sectors.PushBack();
	RoomSector.m_RenderBound				= Room.m_RenderBound.GetTransformedBound( RoomMat );
	RoomSector.m_GeoMeshBound				= RoomSector.m_RenderBound;
	// HACKHACK: Contract the geomesh bound just slightly
	static const Vector skMillimeterBounds	= Vector( -0.001f, -0.001f, -0.001f );
	RoomSector.m_GeoMeshBound.ExpandBy( skMillimeterBounds );
	RoomSector.m_CollisionBound				= Room.m_CollisionBound.GetTransformedBound( RoomMat );
	RoomSector.m_IsSingleTile				= ( 1 == ( Room.m_TilesX * Room.m_TilesY * Room.m_TilesZ ) );

	// Side effect: also create bounding ambient light and fog mesh if needed!
	// HACKHACK: Creating the fog mesh first, so the fog mesh def will be cached
	// by the time the ambient light mesh requests a cubemap using its fog values.
	m_World->ConditionalCreateBoundingFogMesh( RoomSector, RoomFilename );
	m_World->ConditionalCreateBoundingAmbientLight( RoomSector, RoomFilename );

	const Array<RosaRoom::SBrush>&	RoomBrushes	= Room.GetBrushes();
	FOR_EACH_ARRAY( BrushIter, RoomBrushes, RosaRoom::SBrush )
	{
		const RosaRoom::SBrush& RoomBrush = BrushIter.GetValue();
		if( RoomBrush.m_Type != EBT_Geo )
		{
			continue;
		}

		const Vector BrushLocation		= RoomBrush.m_Location * RoomMat;
		const Angles BrushOrientation	= ( RoomBrush.m_Orientation.ToMatrix() * RoomOrientationMat ).ToAngles();

		FOR_EACH_ARRAY( MeshIter, RoomBrush.m_Meshes, RosaRoom::SMesh )
		{
			const RosaRoom::SMesh& Mesh = MeshIter.GetValue();
			m_World->CreateGeoMesh( Mesh.m_MeshName, Mesh.m_MaterialName, Mesh.m_CastsShadows, RoomBrush.m_Mat, BrushLocation, BrushOrientation, RoomBrush.m_Scale );
		}

		FOR_EACH_ARRAY( HullIter, RoomBrush.m_Hulls, SConvexHull )
		{
			// Make a copy I can modify for local transformation
			SConvexHull Hull = HullIter.GetValue();

			Hull.m_Bounds.TransformBound( RoomMat );
			Hull.m_Hull.MoveBy( RoomLocation, RoomOrientation );

			// HACKHACK for Cyme. Fix floating point errors that cause the player to stick on walls.
			Hull.m_Bounds.m_Min.x	= QuantizeWithin( Hull.m_Bounds.m_Min.x );
			Hull.m_Bounds.m_Min.y	= QuantizeWithin( Hull.m_Bounds.m_Min.y );
			Hull.m_Bounds.m_Min.z	= QuantizeWithin( Hull.m_Bounds.m_Min.z );
			Hull.m_Bounds.m_Max.x	= QuantizeWithin( Hull.m_Bounds.m_Max.x );
			Hull.m_Bounds.m_Max.y	= QuantizeWithin( Hull.m_Bounds.m_Max.y );
			Hull.m_Bounds.m_Max.z	= QuantizeWithin( Hull.m_Bounds.m_Max.z );

			FOR_EACH_ARRAY( HullPlaneIter, Hull.m_Hull.GetPlanes(), Plane )
			{
				Plane& HullPlane		= HullPlaneIter.GetValue();
				HullPlane.m_Normal.x	= QuantizeWithin( HullPlane.m_Normal.x );
				HullPlane.m_Normal.y	= QuantizeWithin( HullPlane.m_Normal.y );
				HullPlane.m_Normal.z	= QuantizeWithin( HullPlane.m_Normal.z );
				HullPlane.m_Distance	= QuantizeWithin( HullPlane.m_Distance );
			}

			RoomSector.m_Hulls.PushBack( Hull );
		}

		FOR_EACH_ARRAY( AmbientLightIter, RoomBrush.m_AmbientLights, RosaRoom::SAmbientLight )
		{
			const RosaRoom::SAmbientLight& AmbientLight = AmbientLightIter.GetValue();

			// Make a copy of the hull I can modify for local transformation
			SConvexHull Hull = AmbientLight.m_Hull;
			Hull.m_Bounds.TransformBound( RoomMat );
			Hull.m_Hull.MoveBy( RoomLocation, RoomOrientation );

			m_World->CreateAmbientLight( RoomSector, AmbientLight.m_Mesh, AmbientLight.m_Cubemap, Hull, BrushLocation, BrushOrientation, RoomBrush.m_Scale );
		}

		FOR_EACH_ARRAY( FogMeshIter, RoomBrush.m_FogMeshes, RosaRoom::SFogMesh )
		{
			const RosaRoom::SFogMesh& FogMesh = FogMeshIter.GetValue();

			m_World->CreateFogMesh( RoomSector, FogMesh.m_Mesh, FogMesh.m_FogMeshDef, BrushLocation, BrushOrientation, RoomBrush.m_Scale );
		}
	}
}

void RosaWorldGen::CreateNavMeshFromRoom( const RosaRoom& Room, const uint Transform, const SRoomLoc& RoomLoc )
{
	PROFILE_FUNCTION;

	SRoomLoc OriginLoc;
	GetOriginFromLowLocForWorld( Room, RoomLoc, Transform, OriginLoc );

	const Vector	RoomLocation		= GetLocationForRoomLoc( OriginLoc );
	const Matrix	RoomLocationMat		= Matrix::CreateTranslation( RoomLocation );
	const Angles	RoomOrientation		= GetAnglesForTransform( Transform );
	const Matrix	RoomOrientationMat	= RoomOrientation.ToMatrix();
	const Matrix	RoomMat				= RoomOrientationMat * RoomLocationMat;

	const uint		BaseNodeIndex		= m_World->m_NavNodes.Size();
	const uint		BaseEdgeIndex		= m_World->m_NavEdges.Size();
	m_World->m_NavNodes.Reserve( m_World->m_NavNodes.Size() + Room.m_NavNodes.Size() );
	m_World->m_NavEdges.Reserve( m_World->m_NavEdges.Size() + Room.m_NavEdges.Size() );
	FOR_EACH_ARRAY( NavNodeIter, Room.m_NavNodes, SNavNode )
	{
		// Make a copy to modify
		const SNavNode&	RoomNavNode		= NavNodeIter.GetValue();
		const SNavEdge&	RoomNavEdgeA	= Room.m_NavEdges[ RoomNavNode.m_EdgeA ];
		const SNavEdge&	RoomNavEdgeB	= Room.m_NavEdges[ RoomNavNode.m_EdgeB ];
		const SNavEdge&	RoomNavEdgeC	= Room.m_NavEdges[ RoomNavNode.m_EdgeC ];

		SNavNode&		WorldNavNode	= m_World->m_NavNodes.PushBack();
		SNavEdge&		WorldNavEdgeA	= m_World->m_NavEdges.PushBack();
		SNavEdge&		WorldNavEdgeB	= m_World->m_NavEdges.PushBack();
		SNavEdge&		WorldNavEdgeC	= m_World->m_NavEdges.PushBack();

		WorldNavNode	= RoomNavNode;
		WorldNavEdgeA	= RoomNavEdgeA;
		WorldNavEdgeB	= RoomNavEdgeB;
		WorldNavEdgeC	= RoomNavEdgeC;

		WorldNavEdgeA.m_VertA	*= RoomMat;
		WorldNavEdgeA.m_VertB	*= RoomMat;
		WorldNavEdgeB.m_VertA	*= RoomMat;
		WorldNavEdgeB.m_VertB	*= RoomMat;
		WorldNavEdgeC.m_VertA	*= RoomMat;
		WorldNavEdgeC.m_VertB	*= RoomMat;

		WorldNavNode.m_Tri.m_Vec1	*= RoomMat;
		WorldNavNode.m_Tri.m_Vec2	*= RoomMat;
		WorldNavNode.m_Tri.m_Vec3	*= RoomMat;
		WorldNavNode.m_Centroid		*= RoomMat;
		WorldNavNode.m_Bounds.TransformBound( RoomMat );

		if( WorldNavEdgeA.m_BackNode != NAV_NULL ) { WorldNavEdgeA.m_BackNode += BaseNodeIndex; }
		if( WorldNavEdgeB.m_BackNode != NAV_NULL ) { WorldNavEdgeB.m_BackNode += BaseNodeIndex; }
		if( WorldNavEdgeC.m_BackNode != NAV_NULL ) { WorldNavEdgeC.m_BackNode += BaseNodeIndex; }

		WorldNavNode.m_EdgeA += BaseEdgeIndex;
		WorldNavNode.m_EdgeB += BaseEdgeIndex;
		WorldNavNode.m_EdgeC += BaseEdgeIndex;

		// HACKHACK: I'm assuming this is always done in sync with CreateGeoFromRoom
		DEVASSERT( !m_World->m_Sectors.Empty() );
		WorldNavNode.m_Sector		= m_World->m_Sectors.Size() - 1;
	}
}

bool RosaWorldGen::AreEdgesCoincident( const SNavEdge& EdgeA, const SNavEdge& EdgeB ) const
{
	// HACKHACK: Because I sticky all verts before linking, these should be actually equal now

	if( EdgeA.m_VertA == EdgeB.m_VertA &&
		EdgeA.m_VertB == EdgeB.m_VertB )
	{
		return true;
	}

	if( EdgeA.m_VertA == EdgeB.m_VertB &&
		EdgeA.m_VertB == EdgeB.m_VertA )
	{
		return true;
	}

	return false;
}

bool RosaWorldGen::IsEdgeCoincidentWithNode( const SNavEdge& EdgeA, const SNavNode& NodeB ) const
{
	if( AreEdgesCoincident( EdgeA, m_World->m_NavEdges[ NodeB.m_EdgeA ] ) ||
		AreEdgesCoincident( EdgeA, m_World->m_NavEdges[ NodeB.m_EdgeB ] ) ||
		AreEdgesCoincident( EdgeA, m_World->m_NavEdges[ NodeB.m_EdgeC ] ) )
	{
		return true;
	}

	return false;
}

// Connect mesh edges at sector bounds
void RosaWorldGen::LinkNavMesh()
{
	PROFILE_FUNCTION;

	DEV_DECLARE_AND_START_CLOCK( LinkNavMeshClock );

	// HACKHACK: First, stick all near verts to exactly the same position
	{
		PROFILE_SCOPE( LinkNavMesh_StickyVerts );

		// EPSILON was too small in some cases, using a min just for this
		static const float skMinVertDist = 0.05f;

		FOR_EACH_ARRAY( NavNodeAIter, m_World->m_NavNodes, SNavNode )
		{
			const SNavNode&	NavNodeA	= NavNodeAIter.GetValue();
			const SNavEdge&	NavEdgeAA	= m_World->m_NavEdges[ NavNodeA.m_EdgeA ];
			const SNavEdge&	NavEdgeAB	= m_World->m_NavEdges[ NavNodeA.m_EdgeB ];
			const SNavEdge&	NavEdgeAC	= m_World->m_NavEdges[ NavNodeA.m_EdgeC ];

			// Ignore nodes that don't have at least one boundary edge; nothing else
			// should need to be stickied because they'll be the same by definition.
			if( NavEdgeAA.m_BackNode != NAV_NULL &&
				NavEdgeAB.m_BackNode != NAV_NULL &&
				NavEdgeAC.m_BackNode != NAV_NULL )
			{
				continue;
			}

			FOR_EACH_ARRAY( NavEdgeBIter, m_World->m_NavEdges, SNavEdge )
			{
				SNavEdge&	NavEdgeB	= NavEdgeBIter.GetValue();

#define STICKY_VERTS( a, b ) if( ( a ).Equals( ( b ), skMinVertDist ) ) { ( b ) = ( a ); }
				if( NavEdgeAA.m_BackNode == NAV_NULL )
				{
					STICKY_VERTS( NavEdgeAA.m_VertA, NavEdgeB.m_VertA );
					STICKY_VERTS( NavEdgeAA.m_VertA, NavEdgeB.m_VertB );
					STICKY_VERTS( NavEdgeAA.m_VertB, NavEdgeB.m_VertA );
					STICKY_VERTS( NavEdgeAA.m_VertB, NavEdgeB.m_VertB );
				}

				if( NavEdgeAB.m_BackNode == NAV_NULL )
				{
					STICKY_VERTS( NavEdgeAB.m_VertA, NavEdgeB.m_VertA );
					STICKY_VERTS( NavEdgeAB.m_VertA, NavEdgeB.m_VertB );
					STICKY_VERTS( NavEdgeAB.m_VertB, NavEdgeB.m_VertA );
					STICKY_VERTS( NavEdgeAB.m_VertB, NavEdgeB.m_VertB );
				}

				if( NavEdgeAC.m_BackNode == NAV_NULL )
				{
					STICKY_VERTS( NavEdgeAC.m_VertA, NavEdgeB.m_VertA );
					STICKY_VERTS( NavEdgeAC.m_VertA, NavEdgeB.m_VertB );
					STICKY_VERTS( NavEdgeAC.m_VertB, NavEdgeB.m_VertA );
					STICKY_VERTS( NavEdgeAC.m_VertB, NavEdgeB.m_VertB );
				}
#undef STICKY_VERTS
			}
		}
	}

	{
		PROFILE_SCOPE( LinkNavMesh_LinkBackNodes );
		FOR_EACH_ARRAY( NavNodeAIter, m_World->m_NavNodes, SNavNode )
		{
			SNavNode&	NavNodeA	= NavNodeAIter.GetValue();
			SNavEdge&	NavEdgeAA	= m_World->m_NavEdges[ NavNodeA.m_EdgeA ];
			SNavEdge&	NavEdgeAB	= m_World->m_NavEdges[ NavNodeA.m_EdgeB ];
			SNavEdge&	NavEdgeAC	= m_World->m_NavEdges[ NavNodeA.m_EdgeC ];

			// Ignore nodes that don't have at least one boundary edge
			if( NavEdgeAA.m_BackNode != NAV_NULL &&
				NavEdgeAB.m_BackNode != NAV_NULL &&
				NavEdgeAC.m_BackNode != NAV_NULL )
			{
				continue;
			}

			FOR_EACH_ARRAY( NavNodeBIter, m_World->m_NavNodes, SNavNode )
			{
				if( NavNodeAIter.GetIndex() == NavNodeBIter.GetIndex() )
				{
					continue;
				}

				const SNavNode&	NavNodeB	= NavNodeBIter.GetValue();

				if( NavEdgeAA.m_BackNode == NAV_NULL &&
					IsEdgeCoincidentWithNode( NavEdgeAA, NavNodeB ) )
				{
					NavEdgeAA.m_BackNode = NavNodeBIter.GetIndex();
				}

				if( NavEdgeAB.m_BackNode == NAV_NULL &&
					IsEdgeCoincidentWithNode( NavEdgeAB, NavNodeB ) )
				{
					NavEdgeAB.m_BackNode = NavNodeBIter.GetIndex();
				}

				if( NavEdgeAC.m_BackNode == NAV_NULL &&
					IsEdgeCoincidentWithNode( NavEdgeAC, NavNodeB ) )
				{
					NavEdgeAC.m_BackNode = NavNodeBIter.GetIndex();
				}
			}
		}
	}

	DEV_STOP_AND_REPORT_CLOCK( LinkNavMeshClock, "    " );
}

void RosaWorldGen::CreateMinimapMeshes()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	DEV_DECLARE_AND_START_CLOCK( CreateMinimapMeshes );

	FOR_EACH_MAP( MinimapMeshIter, m_World->m_MinimapMeshes, uint, Mesh* )
	{
		Mesh* pMinimapMesh = MinimapMeshIter.GetValue();
		SafeDelete( pMinimapMesh );
	}
	m_World->m_MinimapMeshes.Clear();

	// Build a temporary map from sector to nav node indices
	Multimap<uint, uint> SectorNavNodeMap;
	FOR_EACH_ARRAY( NavNodeIter, m_World->m_NavNodes, SNavNode )
	{
		const uint		NavNodeIndex	= NavNodeIter.GetIndex();
		const SNavNode&	NavNode			= NavNodeIter.GetValue();
		SectorNavNodeMap.Insert( NavNode.m_Sector, NavNodeIndex );
	}

	for( uint SectorIndex = 0; SectorIndex < m_World->m_Sectors.Size(); ++SectorIndex )
	{
		const uint	NumNodes	= SectorNavNodeMap.Count( SectorIndex );
		if( 0 == NumNodes )
		{
			continue;
		}

		// HACKHACK: Since I don't have a master list of nav verts, I'm not trying to index this mesh
		const uint	NumVertices	= 3 * NumNodes;
		const uint	NumIndices	= NumVertices;
		ASSERT( NumVertices < 65536 );

		Array<Vector> TempPositions;
		TempPositions.Reserve( 3 );
		TempPositions.SetDeflate( false );

		Array<Vector> Positions;
		Positions.Reserve( NumVertices );

		Array<index_t> Indices;
		Indices.Resize( NumIndices );

		FOR_EACH_MULTIMAP_SEARCH( NavNodeIndexIter, SectorNavNodeMap, uint, uint, SectorIndex )
		{
			const uint		NavNodeIndex	= NavNodeIndexIter.GetValue();
			const SNavNode&	NavNode			= m_World->m_NavNodes[ NavNodeIndex ];
			const SNavEdge&	NavEdgeA		= m_World->m_NavEdges[ NavNode.m_EdgeA ];
			const SNavEdge&	NavEdgeB		= m_World->m_NavEdges[ NavNode.m_EdgeB ];
			const SNavEdge&	NavEdgeC		= m_World->m_NavEdges[ NavNode.m_EdgeC ];

			// HACKHACK: I know nothing about the order of verts in edges.
			TempPositions.Clear();
			TempPositions.PushBackUnique( NavEdgeA.m_VertA );
			TempPositions.PushBackUnique( NavEdgeA.m_VertB );
			TempPositions.PushBackUnique( NavEdgeB.m_VertA );
			TempPositions.PushBackUnique( NavEdgeB.m_VertB );
			TempPositions.PushBackUnique( NavEdgeC.m_VertA );
			TempPositions.PushBackUnique( NavEdgeC.m_VertB );
			DEVASSERT( TempPositions.Size() == 3 );

			const Vector	AB		= TempPositions[1] - TempPositions[0];
			const Vector	AC		= TempPositions[2] - TempPositions[0];
			const Vector	Cross	= AB.Cross( AC ); // No need to normalize, we're only using this for sign of dot
			const Vector	Up		= Vector( 0.0f, 0.0f, 1.0f );
			const bool		CCW		= Cross.Dot( Up ) > 0.0f;

			Positions.PushBack( TempPositions[0] );
			Positions.PushBack( TempPositions[ CCW ? 1 : 2 ] );
			Positions.PushBack( TempPositions[ CCW ? 2 : 1 ] );
		}

		for( index_t Index = 0; Index < NumIndices; ++Index )
		{
			Indices[ Index ] = Index;
		}

		IRenderer* const	pRenderer			= RosaFramework::GetInstance()->GetRenderer();

		IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
		IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS );
		IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
		IVertexBuffer::SInit InitStruct;
		InitStruct.NumVertices	= NumVertices;
		InitStruct.Positions	= Positions.GetData();
		pVertexBuffer->Init( InitStruct );
		pIndexBuffer->Init( NumIndices, Indices.GetData() );
		pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

		Mesh* const pMinimapMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

#if BUILD_DEBUG
		pMinimapMesh->m_DEBUG_Name = "Minimap";
#endif

		pMinimapMesh->SetMaterialFlags( MAT_MINIMAP );

		pMinimapMesh->SetMaterialDefinition( "Material_MinimapA", pRenderer );

		STATIC_HASHED_STRING( MinimapAParams );
		static const Vector4	skRenderableMinimapAParams		= Vector4( 1.0f, 0.0f, 0.0f, 0.0f );	// Only the x/red is used here
		static const Vector4	skNonRenderableMinimapAParams	= Vector4( 0.0f, 0.0f, 0.0f, 0.0f );
		const bool				Renderable						= m_World->GetCurrentWorldDef().m_MinimapRenderAll || m_World->m_VisitedSectors.Find( SectorIndex );
		const Vector4&			DefaultMinimapAParams			= Renderable ? skRenderableMinimapAParams : skNonRenderableMinimapAParams;
		pMinimapMesh->SetShaderConstant( sMinimapAParams, DefaultMinimapAParams );

		m_World->m_MinimapMeshes.Insert( SectorIndex, pMinimapMesh );
	}

	DEV_STOP_AND_REPORT_CLOCK( CreateMinimapMeshes, "    " );
}
