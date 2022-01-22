#include "core.h"
#include "wbcomprosaspawnmanager.h"
#include "wbeventmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "configmanager.h"
#include "idatastream.h"
#include "rosagame.h"
#include "rosaworld.h"
#include "rosacampaign.h"
#include "collisioninfo.h"
#include "wbworld.h"
#include "mathcore.h"
#include "mathfunc.h"

WBCompRosaSpawnManager::WBCompRosaSpawnManager()
:	m_SpawnEntity()
,	m_BigBadEntity()
,	m_Minibosses()
,	m_ExtentsMax()
,	m_MinSpawnDistSq( 0.0f )
,	m_MaxSpawnDistSq( 0.0f )
,	m_TargetPopulationLowThreat( 0.0f )
,	m_TargetPopulationHighThreat( 0.0f )
,	m_MaxMinibosses( 0 )
,	m_RelevanceCheckRate( 0.0f )
,	m_NextRelevanceCheckTime( 0.0f )
,	m_MaxRelevanceDistSq( 0.0f )
,	m_IsSpawning( false )
,	m_SpawnRateMin( 0.0f )
,	m_SpawnRateMax( 0.0f )
,	m_SpawnPointTimeout( 0.0f )
,	m_NextSpawnTime( 0.0f )
,	m_SpawnPhase( 0 )
,	m_CanSpawnBigBad( false )
,	m_CanSpawnMiniboss( false )
,	m_BigBadScoreThreshold( 0.0f )
,	m_MinibossSpawnPhase( 0 )
,	m_DeferSpawning( false )
,	m_StealthMode( false )
,	m_StealthPopulationPercent( 0.0f )
,	m_SpawnPoints()
,	m_ManagedEntities()
,	m_NumManagedMinibosses( 0 )
{
}

WBCompRosaSpawnManager::~WBCompRosaSpawnManager()
{
}

/*virtual*/ void WBCompRosaSpawnManager::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	RosaCampaign* const pCampaign = RosaCampaign::GetCampaign();

	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( SpawnEntity );
	m_SpawnEntity = ConfigManager::GetInheritedString( sSpawnEntity, "", sDefinitionName );

	STATICHASH( BigBadEntity );
	m_BigBadEntity = ConfigManager::GetInheritedString( sBigBadEntity, "", sDefinitionName );

	STATICHASH( NumMinibosses );
	const uint NumMinibosses = ConfigManager::GetInheritedInt( sNumMinibosses, 0, sDefinitionName );
	FOR_EACH_INDEX( MinibossIndex, NumMinibosses )
	{
		SMiniboss& Miniboss = m_Minibosses.PushBack();

		Miniboss.m_Entity		= ConfigManager::GetInheritedSequenceString(	"Miniboss%d",			MinibossIndex, "",		sDefinitionName );
		Miniboss.m_Weight		= ConfigManager::GetInheritedSequenceFloat(		"Miniboss%dWeight",		MinibossIndex, 1.0f,	sDefinitionName );
		Miniboss.m_MinSeason	= ConfigManager::GetInheritedSequenceInt(		"Miniboss%dMinSeason",	MinibossIndex, 1,		sDefinitionName );
	}

	STATICHASH( MinSpawnDistance );
	m_MinSpawnDistSq = Square( pCampaign->ModifyFloat( sMinSpawnDistance, ConfigManager::GetInheritedFloat( sMinSpawnDistance, 0.0f, sDefinitionName ) ) );

	STATICHASH( MaxSpawnDistance );
	m_MaxSpawnDistSq = Square( pCampaign->ModifyFloat( sMaxSpawnDistance, ConfigManager::GetInheritedFloat( sMaxSpawnDistance, 0.0f, sDefinitionName ) ) );

	DEVASSERT( m_MinSpawnDistSq < m_MaxSpawnDistSq );

	STATIC_HASHED_STRING( TargetPopulation );
	STATICHASH( TargetPopulationLowThreat );
	m_TargetPopulationLowThreat = pCampaign->ModifyFloat( sTargetPopulation, ConfigManager::GetInheritedFloat( sTargetPopulationLowThreat, 0.0f, sDefinitionName ) );

	STATICHASH( TargetPopulationHighThreat );
	m_TargetPopulationHighThreat = pCampaign->ModifyFloat( sTargetPopulation, ConfigManager::GetInheritedFloat( sTargetPopulationHighThreat, 0.0f, sDefinitionName ) );

	STATICHASH( MaxMinibosses );
	m_MaxMinibosses = pCampaign->ModifyInt( sMaxMinibosses, ConfigManager::GetInheritedInt( sMaxMinibosses, 0, sDefinitionName ) );

	STATICHASH( RelevanceCheckRate );
	m_RelevanceCheckRate = ConfigManager::GetInheritedFloat( sRelevanceCheckRate, 0.0f, sDefinitionName );

	STATICHASH( MaxRelevanceDistance );
	m_MaxRelevanceDistSq = Square( ConfigManager::GetInheritedFloat( sMaxRelevanceDistance, 0.0f, sDefinitionName ) );

	STATICHASH( SpawnRate );
	STATICHASH( SpawnRateMin );
	m_SpawnRateMin = pCampaign->ModifyFloat( sSpawnRate, ConfigManager::GetInheritedFloat( sSpawnRateMin, 0.0f, sDefinitionName ) );

	STATICHASH( SpawnRateMax );
	m_SpawnRateMax = pCampaign->ModifyFloat( sSpawnRate, ConfigManager::GetInheritedFloat( sSpawnRateMax, 0.0f, sDefinitionName ) );

	STATICHASH( SpawnPointTimeout );
	m_SpawnPointTimeout = ConfigManager::GetInheritedFloat( sSpawnPointTimeout, 0.0f, sDefinitionName );

	STATICHASH( CanSpawnBigBad );
	m_CanSpawnBigBad = pCampaign->OverrideBool( sCanSpawnBigBad, ConfigManager::GetInheritedBool( sCanSpawnBigBad, false, sDefinitionName ) );

	m_CanSpawnMiniboss = true;

	STATICHASH( BigBadScoreThreshold );
	m_BigBadScoreThreshold = pCampaign->ModifyFloat( sBigBadScoreThreshold, ConfigManager::GetInheritedFloat( sBigBadScoreThreshold, 0.0f, sDefinitionName ) );

	STATICHASH( MinibossSpawnPhase );
	m_MinibossSpawnPhase = pCampaign->OverrideInt( sMinibossSpawnPhase, ConfigManager::GetInheritedInt( sMinibossSpawnPhase, 0, sDefinitionName ) );
	DEVASSERT( m_MinibossSpawnPhase > 0 );

	STATICHASH( DeferSpawning );
	m_DeferSpawning = pCampaign->OverrideBool( sDeferSpawning, ConfigManager::GetInheritedBool( sDeferSpawning, false, sDefinitionName ) );

	STATICHASH( StealthPopulationPercent );
	m_StealthPopulationPercent = pCampaign->ModifyFloat( sStealthPopulationPercent, 1.0f );

	STATICHASH( StealthMode );
	m_StealthMode = ( m_StealthPopulationPercent != 1.0f );

	// Cache the max extents so we don't have to create the default collision every time
	// ROSANOTE: This assumes that we'll only ever scale a character *down* from its base extents!
	WBCompRosaCollision* pDefaultCollision	= WB_CREATECOMP( RosaCollision, m_SpawnEntity );
	m_ExtentsMax							= pDefaultCollision->GetExtents();
	WB_DESTROYCOMP( pDefaultCollision );
}

/*virtual*/ void WBCompRosaSpawnManager::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SpawnManager_RegisterSpawnPoint );
	STATIC_HASHED_STRING( SpawnManager_SpawnInitialEntities );
	STATIC_HASHED_STRING( SpawnManager_RequestSpawn );
	STATIC_HASHED_STRING( SpawnManager_RequestSpawnNear );
	STATIC_HASHED_STRING( SpawnManager_RemoveManagedEntity );
	STATIC_HASHED_STRING( SpawnManager_Toggle );
	STATIC_HASHED_STRING( SpawnManager_Stop );
	STATIC_HASHED_STRING( SpawnManager_OnCombat );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSpawnManager_RegisterSpawnPoint )
	{
		STATIC_HASHED_STRING( Location );
		const Vector Location		= Event.GetVector( sLocation );

		STATIC_HASHED_STRING( Orientation );
		const Angles Orientation	= Event.GetAngles( sOrientation );

		RegisterSpawnPoint( Location, Orientation );
	}
	else if( EventName == sSpawnManager_SpawnInitialEntities )
	{
		SpawnInitialEntities();
	}
	else if( EventName == sSpawnManager_RequestSpawn )
	{
		STATIC_HASHED_STRING( EntityDef );
		const SimpleString	EntityDef			= Event.GetString( sEntityDef );

		STATIC_HASHED_STRING( SpawnLocation );
		const Vector		SpawnLocation		= Event.GetVector( sSpawnLocation );

		STATIC_HASHED_STRING( SpawnOrientation );
		const Angles		SpawnOrientation	= Event.GetAngles( sSpawnOrientation );

		STATIC_HASHED_STRING( ManageEntity );
		const bool			ManageEntity		= Event.GetBool( sManageEntity );

		SpawnEntityAt( SpawnLocation, SpawnOrientation, EntityDef, ManageEntity );
	}
	else if( EventName == sSpawnManager_RequestSpawnNear )
	{
		STATIC_HASHED_STRING( EntityDef );
		const SimpleString	EntityDef			= Event.GetString( sEntityDef );

		STATIC_HASHED_STRING( TargetLocation );
		const Vector		TargetLocation		= Event.GetVector( sTargetLocation );

		STATIC_HASHED_STRING( MinSpawnDist );
		const float			MinSpawnDist		= Event.GetFloat( sMinSpawnDist );

		STATIC_HASHED_STRING( MaxSpawnDist );
		const float			MaxSpawnDist		= Event.GetFloat( sMaxSpawnDist );

		STATIC_HASHED_STRING( ManageEntity );
		const bool			ManageEntity		= Event.GetBool( sManageEntity );

		SpawnNear( EntityDef, TargetLocation, MinSpawnDist, MaxSpawnDist, ManageEntity );
	}
	else if( EventName == sSpawnManager_RemoveManagedEntity )
	{
		STATIC_HASHED_STRING( EventOwner );
		WBEntity* const pEventOwner = Event.GetEntity( sEventOwner );
		DEVASSERT( pEventOwner );

		RemoveManagedEntity( pEventOwner );
	}
	else if( EventName == sSpawnManager_Toggle )
	{
		m_IsSpawning = !m_IsSpawning;
	}
	else if( EventName == sSpawnManager_Stop )
	{
		m_IsSpawning = false;
	}
	else if( EventName == sSpawnManager_OnCombat )
	{
		if( m_DeferSpawning )
		{
			m_DeferSpawning	= false;
			m_IsSpawning	= true;
		}

		if( m_StealthMode )
		{
			m_StealthMode = false;
		}
	}
}

/*virtual*/ void WBCompRosaSpawnManager::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	// Periodically destroy managed entities if they have become irrelevant
	if( GetTime() >= m_NextRelevanceCheckTime )
	{
		DestroyIrrelevantEntities();
		m_NextRelevanceCheckTime = GetTime() + m_RelevanceCheckRate;
	}

	// Periodically spawn new entities up to a target population
	if( m_IsSpawning && GetTime() >= m_NextSpawnTime )
	{
		// Try to spawn Big Bad first, if we're due for them
		if( m_CanSpawnBigBad )
		{
			// OLDVAMP
			const float MissionScore = 0.0f;//RosaCampaign::GetCampaign()->ScoreMission();
			if( MissionScore >= m_BigBadScoreThreshold )
			{
				if( SpawnMiniboss( m_BigBadEntity, false ) )	// Don't manage Big Bads, they should never be despawned or counted as part of the population
				{
					m_CanSpawnBigBad = false;
				}
			}
		}

		// Try to spawn miniboss if we're due for one
		if( !m_StealthMode &&
			m_CanSpawnMiniboss &&
			m_NumManagedMinibosses < m_MaxMinibosses &&
			( m_SpawnPhase % m_MinibossSpawnPhase ) == ( m_MinibossSpawnPhase - 1 ) )
		{
			const SimpleString Miniboss = PickMiniboss();
			if( SpawnMiniboss( Miniboss, true ) )
			{
				m_CanSpawnMiniboss = false;
			}
		}

		// Spawn one entity each tick until we've reached target population
		// Give up for now if we couldn't spawn anything
		const uint TargetPopulation = GetTargetPopulation();
		if( m_ManagedEntities.Size() >= TargetPopulation || 0 == SpawnEntities( 1 ) )
		{
			m_NextSpawnTime = GetTime() + Math::Random( m_SpawnRateMin, m_SpawnRateMax );
			m_SpawnPhase++;
			m_CanSpawnMiniboss = true;
		}
	}
}

SimpleString WBCompRosaSpawnManager::PickMiniboss() const
{
	const RosaCampaign* const	pCampaign		= RosaCampaign::GetCampaign();
	// OLDVAMP
	uint						CurrentSeason	= 0; //pCampaign->GetSeason();

	// Find the active minibosses
	Array<uint> ActiveMinibosses;
	FOR_EACH_ARRAY( MinibossIter, m_Minibosses, SMiniboss )
	{
		const SMiniboss& Miniboss = MinibossIter.GetValue();
		if( Miniboss.m_MinSeason > CurrentSeason )
		{
			continue;
		}

		const SimpleString CampaignTag = SimpleString::PrintF( "CanSpawn_%s", Miniboss.m_Entity.CStr() );
		if( !pCampaign->OverrideBool( CampaignTag, true ) )
		{
			continue;
		}

		ActiveMinibosses.PushBack( MinibossIter.GetIndex() );
	}
	DEVASSERT( ActiveMinibosses.Size() > 0 );

	// Capture "&" so we can use m_Minibosses by reference (could capture "this" as well)
	const uint SelectedMinibossIndex = Math::ArrayWeightedRandom( ActiveMinibosses, [&]( const uint MinibossIndex ) { return m_Minibosses[ MinibossIndex ].m_Weight; } );
	return m_Minibosses[ SelectedMinibossIndex ].m_Entity;
}

void WBCompRosaSpawnManager::RegisterSpawnPoint( const Vector& Location, const Angles& Orientation )
{
	SSpawnPoint& NewSpawnPoint	= m_SpawnPoints.PushBack();
	NewSpawnPoint.m_Location	= Location;
	NewSpawnPoint.m_Orientation	= Orientation;
}

// Remove the entity from the list of managed entities.
// Disregard if it's not an managed entity.
// This does not destroy the entity. Its purpose is to stop tracking
// entities that are removed in the sim (e.g., when the player kills them).
void WBCompRosaSpawnManager::RemoveManagedEntity( WBEntity* const pEntity )
{
	uint ManagedEntityIndex;
	if( !m_ManagedEntities.Find( pEntity, ManagedEntityIndex ) )
	{
		return;
	}

	if( IsMiniboss( pEntity ) )
	{
		DEVASSERT( m_NumManagedMinibosses > 0 );
		--m_NumManagedMinibosses;

		// When we remove a miniboss, reset the spawn phase so we won't
		// get another miniboss for a while. This is a super cheap, hacky
		// way to maintain pacing and prevent players getting overwhelmed.
		m_SpawnPhase = 0;
	}

	m_ManagedEntities.FastRemove( ManagedEntityIndex );
}

uint WBCompRosaSpawnManager::GetTargetPopulation() const
{
	// OLDVAMP
	const float	ThreatAlpha			= 0.0f; //RosaCampaign::GetCampaign()->GetThreatAlpha();
	const float CombatPopulation	= Lerp( m_TargetPopulationLowThreat, m_TargetPopulationHighThreat, ThreatAlpha );
	const float StealthModeScalar	= m_StealthMode ? m_StealthPopulationPercent : 1.0f;
	const uint	TargetPopulation	= static_cast<uint>( Round( StealthModeScalar * CombatPopulation ) );

	return TargetPopulation;
}

void WBCompRosaSpawnManager::SpawnInitialEntities()
{
	if( m_DeferSpawning )
	{
		return;
	}

	DEVASSERT( m_ManagedEntities.Empty() );
	DEVASSERT( 0 == m_NumManagedMinibosses );

	const uint TargetPopulation = GetTargetPopulation();
	SpawnEntities( TargetPopulation );

	m_IsSpawning = true;
}

uint WBCompRosaSpawnManager::SpawnEntities( const uint NumEntities )
{
	PROFILE_FUNCTION;

	const float				Time				= GetTime();

	const Vector			PlayerLocation		= RosaGame::GetPlayerLocation();
	const Vector			PlayerViewLocation	= RosaGame::GetPlayerViewLocation();
	const Vector			ExtentsTop			= Vector( 0.0f, 0.0f, m_ExtentsMax.z );
	const RosaWorld* const	pWorld				= GetWorld();
	uint					EntitiesSpawned		= 0;

	const uint				NumSpawnPoints		= m_SpawnPoints.Size();
	Array<uint>				SpawnPointIndexes;
	SpawnPointIndexes.SetDeflate( false );
	SpawnPointIndexes.Resize( NumSpawnPoints );
	for( uint SpawnPointIndex = 0; SpawnPointIndex < NumSpawnPoints; ++SpawnPointIndex )
	{
		SpawnPointIndexes[ SpawnPointIndex ] = SpawnPointIndex;
	}

	CollisionInfo ClearanceInfo;
	ClearanceInfo.m_In_CollideWorld			= true;
	ClearanceInfo.m_In_CollideEntities		= true;
	ClearanceInfo.m_In_UserFlags			= EECF_BlockerCollision;
	ClearanceInfo.m_In_StopAtAnyCollision	= true;

	CollisionInfo OcclusionInfo;
	OcclusionInfo.m_In_CollideWorld			= true;
	OcclusionInfo.m_In_CollideEntities		= true;
	OcclusionInfo.m_In_UserFlags			= EECF_Occlusion;
	OcclusionInfo.m_In_StopAtAnyCollision	= true;

	while( EntitiesSpawned < NumEntities && SpawnPointIndexes.Size() > 0 )
	{
		// Find a valid spawn point, removing invalid options along the way
		const uint		SpawnPointIndexIndex	= Math::Random( SpawnPointIndexes.Size() );
		SSpawnPoint&	SpawnPoint				= m_SpawnPoints[ SpawnPointIndexes[ SpawnPointIndexIndex ] ];
		SpawnPointIndexes.FastRemove( SpawnPointIndexIndex );

		// Is the spawn point's timeout elapsed?
		if( Time < SpawnPoint.m_SpawnTime )
		{
			continue;
		}

		// Is the spawn point at a reasonable distance from the player?
		const float			DistanceSq		= ( SpawnPoint.m_Location - PlayerLocation ).LengthSquared();
		if( DistanceSq < m_MinSpawnDistSq ||
			DistanceSq > m_MaxSpawnDistSq )
		{
			continue;
		}

		// Is the spawn point clear (e.g., not occupied by another AI)?
		const bool Blocked = pWorld->CheckClearance( SpawnPoint.m_Location, m_ExtentsMax, ClearanceInfo );
		if( Blocked )
		{
			continue;
		}

		// Is the spawn point occluded from the player?
		// ROSATODO: This could be a more thorough test against each corner of the extents.
		// Currently, I just test the center and top (to prevent spawning behind half-height cover)
		const bool Occluded =
			pWorld->LineCheck( PlayerViewLocation, SpawnPoint.m_Location, OcclusionInfo ) &&
			pWorld->LineCheck( PlayerViewLocation, SpawnPoint.m_Location + ExtentsTop, OcclusionInfo );
		if( !Occluded )
		{
			continue;
		}

		// All checks passed, spawn the entity here
		SpawnEntityAt( SpawnPoint, m_SpawnEntity, true );
		EntitiesSpawned++;
	}

	if( EntitiesSpawned < NumEntities )
	{
		PRINTF( "WBCompRosaSpawnManager: Not enough spawn points for %s. (Spawned %d of %d)\n", m_SpawnEntity.CStr(), EntitiesSpawned, NumEntities );
	}
	ASSERTDESC( EntitiesSpawned == NumEntities, "WBCompRosaSpawnManager: Not enough spawn points." );

	return EntitiesSpawned;
}

bool WBCompRosaSpawnManager::SpawnNear( const SimpleString& Entity, const Vector& TargetLocation, const float MinSpawnDist, const float MaxSpawnDist, const bool ManageEntity )
{
	PROFILE_FUNCTION;

	const float				MinSpawnDistSq		= Square( MinSpawnDist );
	const float				MaxSpawnDistSq		= Square( MaxSpawnDist );
	DEVASSERT( MaxSpawnDistSq > MinSpawnDistSq );

	const RosaWorld* const	pWorld				= GetWorld();

	// Instead of spawning at the nearest valid point, build an array of valid points and pick at random.
	Array<uint>				ValidSpawnPoints;

	CollisionInfo ClearanceInfo;
	ClearanceInfo.m_In_CollideWorld			= true;
	ClearanceInfo.m_In_CollideEntities		= true;
	ClearanceInfo.m_In_UserFlags			= EECF_BlockerCollision;
	ClearanceInfo.m_In_StopAtAnyCollision	= true;

	CollisionInfo OcclusionInfo;
	OcclusionInfo.m_In_CollideWorld			= true;
	OcclusionInfo.m_In_CollideEntities		= true;
	OcclusionInfo.m_In_UserFlags			= EECF_Occlusion;
	OcclusionInfo.m_In_StopAtAnyCollision	= true;

	// Find the nearest valid spawn points
	FOR_EACH_ARRAY( SpawnPointIter, m_SpawnPoints, SSpawnPoint )
	{
		const SSpawnPoint&	SpawnPoint	= SpawnPointIter.GetValue();

		// HACKHACK: For this purpose, ignore spawn point timeout

		// Is the spawn point at a reasonable distance from the target?
		const float			DistanceSq		= ( SpawnPoint.m_Location - TargetLocation ).LengthSquared();
		if( DistanceSq < MinSpawnDistSq ||
			DistanceSq > MaxSpawnDistSq )
		{
			continue;
		}

		// Is the spawn point clear (e.g., not occupied by another AI)?
		const bool Blocked = pWorld->CheckClearance( SpawnPoint.m_Location, m_ExtentsMax, ClearanceInfo );
		if( Blocked )
		{
			continue;
		}

		// HACKHACK: For this purpose, ignore player view occlusion check
		ValidSpawnPoints.PushBack( SpawnPointIter.GetIndex() );
	}

	if( ValidSpawnPoints.Empty() )
	{
		PRINTF( "WBCompRosaSpawnManager::SpawnNear: Could not find place to spawn %s.\n", Entity.CStr() );
		WARNDESC( "WBCompRosaSpawnManager::SpawnNear: Could not find place to spawn." );
		return false;
	}
	else
	{
		const uint SpawnPointIndex = Math::ArrayRandom( ValidSpawnPoints );
		SSpawnPoint& SpawnPoint = m_SpawnPoints[ SpawnPointIndex ];
		SpawnEntityAt( SpawnPoint, Entity, ManageEntity );
		return true;
	}
}

bool WBCompRosaSpawnManager::SpawnMiniboss( const SimpleString& Entity, const bool ManageEntity )
{
	PROFILE_FUNCTION;

	const float				Time				= GetTime();

	const Vector			PlayerLocation		= RosaGame::GetPlayerLocation();
	const Vector			PlayerViewLocation	= RosaGame::GetPlayerViewLocation();
	const Vector			ExtentsTop			= Vector( 0.0f, 0.0f, m_ExtentsMax.z );
	const RosaWorld* const	pWorld				= GetWorld();

	uint					NearestValidSpawnPointIndex		= INVALID_ARRAY_INDEX;
	float					NearestValidSpawnPointDistSq	= FLT_MAX;

	CollisionInfo ClearanceInfo;
	ClearanceInfo.m_In_CollideWorld			= true;
	ClearanceInfo.m_In_CollideEntities		= true;
	ClearanceInfo.m_In_UserFlags			= EECF_BlockerCollision;
	ClearanceInfo.m_In_StopAtAnyCollision	= true;

	CollisionInfo OcclusionInfo;
	OcclusionInfo.m_In_CollideWorld			= true;
	OcclusionInfo.m_In_CollideEntities		= true;
	OcclusionInfo.m_In_UserFlags			= EECF_Occlusion;
	OcclusionInfo.m_In_StopAtAnyCollision	= true;

	// Find the nearest valid spawn points
	FOR_EACH_ARRAY( SpawnPointIter, m_SpawnPoints, SSpawnPoint )
	{
		const SSpawnPoint&	SpawnPoint	= SpawnPointIter.GetValue();

		// Is the spawn point's timeout elapsed?
		if( Time < SpawnPoint.m_SpawnTime )
		{
			continue;
		}

		// Is the spawn point at a reasonable distance from the player?
		const float			DistanceSq		= ( SpawnPoint.m_Location - PlayerLocation ).LengthSquared();
		if( DistanceSq < m_MinSpawnDistSq ||
			DistanceSq > m_MaxSpawnDistSq )
		{
			continue;
		}

		// Is the spawn point nearer than our current nearest?
		if( DistanceSq >= NearestValidSpawnPointDistSq )
		{
			continue;
		}

		// Is the spawn point clear (e.g., not occupied by another AI)?
		const bool Blocked = pWorld->CheckClearance( SpawnPoint.m_Location, m_ExtentsMax, ClearanceInfo );
		if( Blocked )
		{
			continue;
		}

		// Is the spawn point occluded from the player?
		// ROSATODO: This could be a more thorough test against each corner of the extents.
		// Currently, I just test the center and top (to prevent spawning behind half-height cover)
		const bool Occluded =
			pWorld->LineCheck( PlayerViewLocation, SpawnPoint.m_Location, OcclusionInfo ) &&
			pWorld->LineCheck( PlayerViewLocation, SpawnPoint.m_Location + ExtentsTop, OcclusionInfo );
		if( !Occluded )
		{
			continue;
		}

		NearestValidSpawnPointIndex		= SpawnPointIter.GetIndex();
		NearestValidSpawnPointDistSq	= DistanceSq;
	}

	if( NearestValidSpawnPointIndex == INVALID_ARRAY_INDEX )
	{
		PRINTF( "WBCompRosaSpawnManager: Could not find place to spawn miniboss %s.\n", Entity.CStr() );
		WARNDESC( "WBCompRosaSpawnManager: Could not find place to spawn miniboss." );
		return false;
	}
	else
	{
		SSpawnPoint& SpawnPoint = m_SpawnPoints[ NearestValidSpawnPointIndex ];
		SpawnEntityAt( SpawnPoint, Entity, ManageEntity );
		return true;
	}
}

void WBCompRosaSpawnManager::SpawnEntityAt( SSpawnPoint& SpawnPoint, const SimpleString& EntityDef, const bool ManageEntity )
{
	SpawnEntityAt( SpawnPoint.m_Location, SpawnPoint.m_Orientation, EntityDef, ManageEntity );

	SpawnPoint.m_SpawnTime = GetTime() + m_SpawnPointTimeout;
}

void WBCompRosaSpawnManager::SpawnEntityAt( const Vector& SpawnLocation, const Angles& SpawnOrientation, const SimpleString& EntityDef, const bool ManageEntity )
{
	WBEntity* const				pSpawnedEntity		= WBWorld::GetInstance()->CreateEntity( EntityDef );
	DEVASSERT( pSpawnedEntity );

	WBCompRosaTransform* const	pSpawnedTransform	= pSpawnedEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pSpawnedTransform );

	WBCompRosaCollision* const	pSpawnedCollision	= WB_GETCOMP( pSpawnedEntity, RosaCollision );
	DEVASSERT( pSpawnedCollision );

	const float					ExtentsZ			= pSpawnedCollision->GetExtents().z;
	Vector						ActualSpawnLocation	= SpawnLocation + Vector( 0.0f, 0.0f, ExtentsZ - m_ExtentsMax.z );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_UserFlags			= EECF_BlockerCollision;

	RosaWorld* const pWorld		= GetWorld();
	pWorld->FindSpot( ActualSpawnLocation, pSpawnedCollision->GetExtents(), Info );

	pSpawnedTransform->SetInitialTransform( ActualSpawnLocation, SpawnOrientation );

	if( ManageEntity )
	{
		if( IsMiniboss( pSpawnedEntity ) )
		{
			++m_NumManagedMinibosses;
		}

		m_ManagedEntities.PushBack( pSpawnedEntity );
	}
}

void WBCompRosaSpawnManager::DestroyIrrelevantEntities()
{
	const Vector			PlayerLocation		= RosaGame::GetPlayerLocation();
	const Vector			PlayerViewLocation	= RosaGame::GetPlayerViewLocation();
	const RosaWorld* const	pWorld				= GetWorld();

	CollisionInfo Info;
	Info.m_In_CollideWorld			= true;
	Info.m_In_CollideEntities		= true;
	Info.m_In_UserFlags				= EECF_Occlusion;
	Info.m_In_StopAtAnyCollision	= true;

	FOR_EACH_ARRAY_REVERSE( ManagedEntityIter, m_ManagedEntities, WBEntityRef )
	{
		WBEntity* const				pManagedEntity	= ManagedEntityIter.GetValue().Get();
		DEVASSERT( pManagedEntity );	// If a thing gets destroyed another way, it should unregister from the manager

		WBCompRosaTransform* const	pTransform		= pManagedEntity->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		const Vector				EntityLocation	= pTransform->GetLocation();

		// Is the entity within relevant distance from the player?
		const float					DistanceSq		= ( EntityLocation - PlayerLocation ).LengthSquared();
		if( DistanceSq < m_MaxRelevanceDistSq )
		{
			continue;
		}

		// Is the entity occluded from the player?
		const bool					Occluded		= pWorld->LineCheck( PlayerViewLocation, EntityLocation, Info );
		if( !Occluded )
		{
			continue;
		}

		// All checks passed, entity is irrelevant and occluded. Unmanage and destroy it.
		RemoveManagedEntity( pManagedEntity );
		pManagedEntity->Destroy();
	}
}

// ROSANOTE: This is a bit hacky, it just compares the entity to the miniboss list.
bool WBCompRosaSpawnManager::IsMiniboss( const WBEntity* const pEntity ) const
{
	DEVASSERT( pEntity );

	FOR_EACH_ARRAY( MinibossIter, m_Minibosses, SMiniboss )
	{
		const SMiniboss& Miniboss = MinibossIter.GetValue();
		if( pEntity->GetName() == Miniboss.m_Entity )
		{
			return true;
		}
	}

	return false;
}

#define VERSION_EMPTY					0
#define VERSION_MANAGEDENTITIES			1
#define VERSION_NEXTRELEVANCECHECKTIME	2
#define VERSION_NEXTSPAWNTIME			3
#define VERSION_ISSPAWNING				4
#define VERSION_CANSPAWNBIGBAD			5
#define VERSION_DEFERSPAWNING			6
#define VERSION_SPAWNPHASE				7
#define VERSION_CANSPAWNMINIBOSS		8
#define VERSION_NUMMANAGEDMINIBOSSES	9
#define VERSION_STEALTHMODE				10
#define VERSION_CURRENT					10

uint WBCompRosaSpawnManager::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;													// Version

	Size += 4;													// m_ManagedEntities.Size()
	Size += sizeof( WBEntityRef ) * m_ManagedEntities.Size();	// m_ManagedEntities
	Size += 4;													// m_NumManagedMinibosses

	Size += 4;													// m_NextRelevanceCheckTime
	Size += 4;													// m_NextSpawnTime

	Size += 1;													// m_IsSpawning

	Size += 1;													// m_CanSpawnBigBad
	Size += 1;													// m_CanSpawnMiniboss
	Size += 1;													// m_DeferSpawning
	Size += 1;													// m_StealthMode
	Size += 4;													// m_SpawnPhase

	return Size;
}

void WBCompRosaSpawnManager::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_ManagedEntities.Size() );
	FOR_EACH_ARRAY( ManagedEntityIter, m_ManagedEntities, WBEntityRef )
	{
		const WBEntityRef& ManagedEntity = ManagedEntityIter.GetValue();
		Stream.Write<WBEntityRef>( ManagedEntity );
	}
	Stream.WriteUInt32( m_NumManagedMinibosses );

	Stream.WriteFloat( m_NextRelevanceCheckTime - GetTime() );
	Stream.WriteFloat( m_NextSpawnTime - GetTime() );

	Stream.WriteBool( m_IsSpawning );

	Stream.WriteBool( m_CanSpawnBigBad );
	Stream.WriteBool( m_CanSpawnMiniboss );
	Stream.WriteBool( m_DeferSpawning );
	Stream.WriteBool( m_StealthMode );
	Stream.WriteUInt32( m_SpawnPhase );
}

void WBCompRosaSpawnManager::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_MANAGEDENTITIES )
	{
		DEVASSERT( m_ManagedEntities.Empty() );
		const uint NumManagedEntities = Stream.ReadUInt32();
		m_ManagedEntities.Resize( NumManagedEntities );

		FOR_EACH_ARRAY( ManagedEntityIter, m_ManagedEntities, WBEntityRef )
		{
			WBEntityRef& ManagedEntity = ManagedEntityIter.GetValue();
			Stream.Read<WBEntityRef>( ManagedEntity );
		}
	}

	if( Version >= VERSION_NUMMANAGEDMINIBOSSES )
	{
		m_NumManagedMinibosses = Stream.ReadUInt32();
	}

	if( Version >= VERSION_NEXTRELEVANCECHECKTIME )
	{
		m_NextRelevanceCheckTime = GetTime() + Stream.ReadFloat();
	}

	if( Version >= VERSION_NEXTSPAWNTIME )
	{
		m_NextSpawnTime = GetTime() + Stream.ReadFloat();
	}

	if( Version >= VERSION_ISSPAWNING )
	{
		m_IsSpawning = Stream.ReadBool();
	}

	if( Version >= VERSION_CANSPAWNBIGBAD )
	{
		m_CanSpawnBigBad = Stream.ReadBool();
	}

	if( Version >= VERSION_CANSPAWNMINIBOSS )
	{
		m_CanSpawnMiniboss = Stream.ReadBool();
	}

	if( Version >= VERSION_DEFERSPAWNING )
	{
		m_DeferSpawning = Stream.ReadBool();
	}

	if( Version >= VERSION_STEALTHMODE )
	{
		m_StealthMode = Stream.ReadBool();
	}

	if( Version >= VERSION_SPAWNPHASE )
	{
		m_SpawnPhase = Stream.ReadUInt32();
	}
}
