#include "core.h"
#include "rosaparticles.h"
#include "rosaframework.h"
#include "rosamesh.h"
#include "mathfunc.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "segment.h"
#include "ivertexdeclaration.h"
#include "configmanager.h"
#include "view.h"
#include "irenderer.h"
#include "texturemanager.h"
#include "mathcore.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "shadermanager.h"
#include "wbworld.h"
#include "rosagame.h"
#include "animationstate.h"

/*static*/ RosaParticles::TParamCache RosaParticles::sm_ParamCache;

RosaParticles::RosaParticles()
:	m_Params()
,	m_SystemLocation()
,	m_SystemBeamEndLocation()
,	m_SystemOrientation()
,	m_SpawnAccumulator( 0.0f )
,	m_ExpireTime( 0.0f )
,	m_FinishTime( 0.0f )
,	m_RenderedLastFrame( false )
,	m_FirstTick( false )
,	m_IsBeam( false )
,	m_ShouldUpdateMesh( false )
,	m_NextUpdateMeshTime( 0.0f )
,	m_DynamicMesh( NULL )
,	m_AlbedoMap( NULL )
,	m_NormalMap( NULL )
,	m_SpecMap( NULL )
,	m_ParticlesArray()
,	m_VB_Positions()
,	m_VB_FloatColors()
,	m_VB_UVs()
,	m_VB_Normals()
,	m_VB_Tangents()
,	m_VB_Indices()
{
}

RosaParticles::~RosaParticles()
{
	IRenderer* const pRenderer = RosaFramework::GetInstance()->GetRenderer();

	if( m_DynamicMesh && pRenderer )
	{
		pRenderer->RemoveDynamicVertexBuffer( m_DynamicMesh->m_VertexBuffer );
	}

	SafeDelete( m_DynamicMesh );
}

RosaParticles::SParticle::SParticle()
:	m_SpawnTime( 0.0f )
,	m_ExpireTime( 0.0f )
,	m_TickLifetime( 0 )
,	m_Size( 0.0f )
,	m_SizeVelocity( 0.0f )
,	m_Color()
,	m_Location()
,	m_Velocity()
,	m_Roll( 0.0f )
,	m_RollVelocity( 0.0f )
{
}

void RosaParticles::GetCachedConfig( const SimpleString& DefinitionName )
{
	const HashedString HashedDefinitionName = DefinitionName;
	Map<HashedString, SParticleSystemParams>::Iterator CacheIter = sm_ParamCache.Search( HashedDefinitionName );

	if( CacheIter.IsValid() )
	{
		m_Params = CacheIter.GetValue();
	}
	else
	{
		MAKEHASH( DefinitionName );

		STATICHASH( SystemLifetime );
		m_Params.m_SystemLifetime = ConfigManager::GetInheritedFloat( sSystemLifetime, 0.0f, sDefinitionName );

		STATICHASH( LifetimeMin );
		m_Params.m_LifetimeMin = ConfigManager::GetInheritedFloat( sLifetimeMin, 0.0f, sDefinitionName );

		STATICHASH( LifetimeMax );
		m_Params.m_LifetimeMax = ConfigManager::GetInheritedFloat( sLifetimeMax, m_Params.m_LifetimeMin, sDefinitionName );

		STATICHASH( TickLifetime );
		m_Params.m_TickLifetime = ConfigManager::GetInheritedInt( sTickLifetime, 0, sDefinitionName );

		STATICHASH( FadeInTime );
		m_Params.m_FadeInTime = ConfigManager::GetInheritedFloat( sFadeInTime, 0.0f, sDefinitionName );
		m_Params.m_InvFadeInTime = ( m_Params.m_FadeInTime > 0.0f ) ? ( 1.0f / m_Params.m_FadeInTime ) : 0.0f;

		STATICHASH( FadeOutTime );
		m_Params.m_FadeOutTime = ConfigManager::GetInheritedFloat( sFadeOutTime, 0.0f, sDefinitionName );
		m_Params.m_InvFadeOutTime = ( m_Params.m_FadeOutTime > 0.0f ) ? ( 1.0f / m_Params.m_FadeOutTime ) : 0.0f;

		STATICHASH( IsTracer );
		m_Params.m_IsTracer = ConfigManager::GetInheritedBool( sIsTracer, false, sDefinitionName );

		STATICHASH( ImmediateSpawnMax );
		m_Params.m_ImmediateSpawnMax = ConfigManager::GetInheritedBool( sImmediateSpawnMax, m_Params.m_IsTracer, sDefinitionName );

		STATICHASH( SpawnRate );
		m_Params.m_SpawnRate = ConfigManager::GetInheritedFloat( sSpawnRate, 0.0f, sDefinitionName );
		m_Params.m_InvSpawnRate = ( m_Params.m_SpawnRate > 0.0f ) ? ( 1.0f / m_Params.m_SpawnRate ) : 0.0f;

		STATICHASH( TickSpawnRate );
		m_Params.m_TickSpawnRate = ConfigManager::GetInheritedInt( sTickSpawnRate, 0, sDefinitionName );

		const uint UpperBound = m_Params.m_IsTracer ? 1 : static_cast<uint>( Ceiling( m_Params.m_LifetimeMax * m_Params.m_SpawnRate ) );
		STATICHASH( MaxParticles );
		m_Params.m_MaxParticles = ConfigManager::GetInheritedInt( sMaxParticles, UpperBound, sDefinitionName );
		DEVASSERTDESC( m_Params.m_MaxParticles > 0, "Particle system has 0 MaxParticles." );
		DEVASSERTDESC( m_Params.m_MaxParticles >= UpperBound, "Particle system has MaxParticles less than LifetimeMax * SpawnRate. System may starve." );

		STATICHASH( ParticlesPerMeter );
		m_Params.m_ParticlesPerMeter = ConfigManager::GetInheritedFloat( sParticlesPerMeter, 0.0f, sDefinitionName );

		STATICHASH( AlbedoMap );
		m_Params.m_AlbedoMapName = ConfigManager::GetInheritedString( sAlbedoMap, DEFAULT_ALBEDO, sDefinitionName );

		STATICHASH( NormalMap );
		m_Params.m_NormalMapName = ConfigManager::GetInheritedString( sNormalMap, DEFAULT_NORMAL, sDefinitionName );

		STATICHASH( SpecMap );
		m_Params.m_SpecMapName = ConfigManager::GetInheritedString( sSpecMap, DEFAULT_SPEC, sDefinitionName );

		STATICHASH( AlwaysDraw );
		m_Params.m_AlwaysDraw = ConfigManager::GetInheritedBool( sAlwaysDraw, false, sDefinitionName );

		STATICHASH( ForegroundDraw );
		m_Params.m_ForegroundDraw = ConfigManager::GetInheritedBool( sForegroundDraw, false, sDefinitionName );

		STATICHASH( Collision );
		m_Params.m_Collision = ConfigManager::GetInheritedBool( sCollision, false, sDefinitionName );

		STATICHASH( Material );
		m_Params.m_MaterialOverride = ConfigManager::GetInheritedString( sMaterial, "", sDefinitionName );

		STATICHASH( SpawnOffsetOSX );
		m_Params.m_SpawnOffsetOS.x = ConfigManager::GetInheritedFloat( sSpawnOffsetOSX, 0.0f, sDefinitionName );

		STATICHASH( SpawnOffsetOSY );
		m_Params.m_SpawnOffsetOS.y = ConfigManager::GetInheritedFloat( sSpawnOffsetOSY, 0.0f, sDefinitionName );

		STATICHASH( SpawnOffsetOSZ );
		m_Params.m_SpawnOffsetOS.z = ConfigManager::GetInheritedFloat( sSpawnOffsetOSZ, 0.0f, sDefinitionName );

		STATICHASH( SpawnOffsetWSX );
		m_Params.m_SpawnOffsetWS.x = ConfigManager::GetInheritedFloat( sSpawnOffsetWSX, 0.0f, sDefinitionName );

		STATICHASH( SpawnOffsetWSY );
		m_Params.m_SpawnOffsetWS.y = ConfigManager::GetInheritedFloat( sSpawnOffsetWSY, 0.0f, sDefinitionName );

		STATICHASH( SpawnOffsetWSZ );
		m_Params.m_SpawnOffsetWS.z = ConfigManager::GetInheritedFloat( sSpawnOffsetWSZ, 0.0f, sDefinitionName );

		STATICHASH( SpawnExtentsX );
		m_Params.m_SpawnExtents.x = ConfigManager::GetInheritedFloat( sSpawnExtentsX, 0.0f, sDefinitionName );

		STATICHASH( SpawnExtentsY );
		m_Params.m_SpawnExtents.y = ConfigManager::GetInheritedFloat( sSpawnExtentsY, 0.0f, sDefinitionName );

		STATICHASH( SpawnExtentsZ );
		m_Params.m_SpawnExtents.z = ConfigManager::GetInheritedFloat( sSpawnExtentsZ, 0.0f, sDefinitionName );

		STATICHASH( InitialVelocityOSXY );
		const float InitialVelocityOSXY = ConfigManager::GetInheritedFloat( sInitialVelocityOSXY, 0.0f, sDefinitionName );
		const bool HasInitialVelocityOSXY = ( InitialVelocityOSXY != 0.0f );

		STATICHASH( InitialVelocityOSMinX );
		m_Params.m_InitialVelocityOSMin.x = ConfigManager::GetInheritedFloat( sInitialVelocityOSMinX, -InitialVelocityOSXY, sDefinitionName );

		STATICHASH( InitialVelocityOSMinY );
		m_Params.m_InitialVelocityOSMin.y = ConfigManager::GetInheritedFloat( sInitialVelocityOSMinY, -InitialVelocityOSXY, sDefinitionName );

		STATICHASH( InitialVelocityOSMinZ );
		m_Params.m_InitialVelocityOSMin.z = ConfigManager::GetInheritedFloat( sInitialVelocityOSMinZ, 0.0f, sDefinitionName );

		STATICHASH( InitialVelocityOSMaxX );
		m_Params.m_InitialVelocityOSMax.x = ConfigManager::GetInheritedFloat( sInitialVelocityOSMaxX, HasInitialVelocityOSXY ? InitialVelocityOSXY : m_Params.m_InitialVelocityOSMin.x, sDefinitionName );

		STATICHASH( InitialVelocityOSMaxY );
		m_Params.m_InitialVelocityOSMax.y = ConfigManager::GetInheritedFloat( sInitialVelocityOSMaxY, HasInitialVelocityOSXY ? InitialVelocityOSXY : m_Params.m_InitialVelocityOSMin.y, sDefinitionName );

		STATICHASH( InitialVelocityOSMaxZ );
		m_Params.m_InitialVelocityOSMax.z = ConfigManager::GetInheritedFloat( sInitialVelocityOSMaxZ, m_Params.m_InitialVelocityOSMin.z, sDefinitionName );

		STATICHASH( InitialVelocityWSXY );
		const float InitialVelocityWSXY = ConfigManager::GetInheritedFloat( sInitialVelocityWSXY, 0.0f, sDefinitionName );
		const bool HasInitialVelocityWSXY = ( InitialVelocityWSXY != 0.0f );

		STATICHASH( InitialVelocityWSMinX );
		m_Params.m_InitialVelocityWSMin.x = ConfigManager::GetInheritedFloat( sInitialVelocityWSMinX, -InitialVelocityWSXY, sDefinitionName );

		STATICHASH( InitialVelocityWSMinY );
		m_Params.m_InitialVelocityWSMin.y = ConfigManager::GetInheritedFloat( sInitialVelocityWSMinY, -InitialVelocityWSXY, sDefinitionName );

		STATICHASH( InitialVelocityWSMinZ );
		m_Params.m_InitialVelocityWSMin.z = ConfigManager::GetInheritedFloat( sInitialVelocityWSMinZ, 0.0f, sDefinitionName );

		STATICHASH( InitialVelocityWSMaxX );
		m_Params.m_InitialVelocityWSMax.x = ConfigManager::GetInheritedFloat( sInitialVelocityWSMaxX, HasInitialVelocityWSXY ? InitialVelocityWSXY : m_Params.m_InitialVelocityWSMin.x, sDefinitionName );

		STATICHASH( InitialVelocityWSMaxY );
		m_Params.m_InitialVelocityWSMax.y = ConfigManager::GetInheritedFloat( sInitialVelocityWSMaxY, HasInitialVelocityWSXY ? InitialVelocityWSXY : m_Params.m_InitialVelocityWSMin.y, sDefinitionName );

		STATICHASH( InitialVelocityWSMaxZ );
		m_Params.m_InitialVelocityWSMax.z = ConfigManager::GetInheritedFloat( sInitialVelocityWSMaxZ, m_Params.m_InitialVelocityWSMin.z, sDefinitionName );

		STATICHASH( InitialVelocityWindMin );
		m_Params.m_InitialVelocityWindMin = ConfigManager::GetInheritedFloat( sInitialVelocityWindMin, 0.0f, sDefinitionName );

		STATICHASH( InitialVelocityWindMax );
		m_Params.m_InitialVelocityWindMax = ConfigManager::GetInheritedFloat( sInitialVelocityWindMax, m_Params.m_InitialVelocityWindMin, sDefinitionName );

		STATICHASH( BeamVelocityMin );
		m_Params.m_BeamVelocityMin = ConfigManager::GetInheritedFloat( sBeamVelocityMin, 0.0f, sDefinitionName );

		STATICHASH( BeamVelocityMax );
		m_Params.m_BeamVelocityMax = ConfigManager::GetInheritedFloat( sBeamVelocityMax, m_Params.m_BeamVelocityMin, sDefinitionName );

		STATICHASH( TracerLength );
		m_Params.m_TracerLength = ConfigManager::GetInheritedFloat( sTracerLength, 0.0f, sDefinitionName );

		STATICHASH( Elasticity );
		m_Params.m_Elasticity = ConfigManager::GetInheritedFloat( sElasticity, 0.0f, sDefinitionName );

		STATICHASH( InitialRollMin );
		m_Params.m_InitialRollMin = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sInitialRollMin, 0.0f, sDefinitionName ) );

		STATICHASH( InitialRollMax );
		m_Params.m_InitialRollMax = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sInitialRollMax, m_Params.m_InitialRollMin, sDefinitionName ) );

		STATICHASH( RollVelocityMin );
		m_Params.m_RollVelocityMin = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sRollVelocityMin, 0.0f, sDefinitionName ) );

		STATICHASH( RollVelocityMax );
		m_Params.m_RollVelocityMax = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sRollVelocityMax, m_Params.m_RollVelocityMin, sDefinitionName ) );

		STATICHASH( AccelerationWSX );
		m_Params.m_AccelerationWS.x = ConfigManager::GetInheritedFloat( sAccelerationWSX, 0.0f, sDefinitionName );

		STATICHASH( AccelerationWSY );
		m_Params.m_AccelerationWS.y = ConfigManager::GetInheritedFloat( sAccelerationWSY, 0.0f, sDefinitionName );

		STATICHASH( AccelerationWSZ );
		m_Params.m_AccelerationWS.z = ConfigManager::GetInheritedFloat( sAccelerationWSZ, 0.0f, sDefinitionName );

		// ROSATODO: Automatically generate this from size + size velocity + spawn extents + spawn offset + velocity
		STATICHASH( ViewBoundRadius );
		m_Params.m_ViewBoundRadius = ConfigManager::GetInheritedFloat( sViewBoundRadius, 1.0f, sDefinitionName );

		STATICHASH( InitialSizeMin );
		m_Params.m_InitialSizeMin = ConfigManager::GetInheritedFloat( sInitialSizeMin, 1.0f, sDefinitionName );

		STATICHASH( InitialSizeMax );
		m_Params.m_InitialSizeMax = ConfigManager::GetInheritedFloat( sInitialSizeMax, m_Params.m_InitialSizeMin, sDefinitionName );

		STATICHASH( SizeVelocityMin );
		m_Params.m_SizeVelocityMin = ConfigManager::GetInheritedFloat( sSizeVelocityMin, 0.0f, sDefinitionName );

		STATICHASH( SizeVelocityMax );
		m_Params.m_SizeVelocityMax = ConfigManager::GetInheritedFloat( sSizeVelocityMax, m_Params.m_SizeVelocityMin, sDefinitionName );

		STATICHASH( LinkedRGB );
		m_Params.m_LinkedRGB = ConfigManager::GetInheritedBool( sLinkedRGB, false, sDefinitionName );

		// ROSATODO: Use HSV for particle colors (I mostly just use the texture now, tho)
		STATICHASH( ColorMinR );
		m_Params.m_ColorMin.x = ConfigManager::GetInheritedFloat( sColorMinR, 1.0f, sDefinitionName );

		STATICHASH( ColorMinG );
		m_Params.m_ColorMin.y = ConfigManager::GetInheritedFloat( sColorMinG, 1.0f, sDefinitionName );

		STATICHASH( ColorMinB );
		m_Params.m_ColorMin.z = ConfigManager::GetInheritedFloat( sColorMinB, 1.0f, sDefinitionName );

		STATICHASH( ColorMinA );
		m_Params.m_ColorMin.w = ConfigManager::GetInheritedFloat( sColorMinA, 1.0f, sDefinitionName );

		STATICHASH( ColorMaxR );
		m_Params.m_ColorMax.x = ConfigManager::GetInheritedFloat( sColorMaxR, m_Params.m_ColorMin.x, sDefinitionName );

		STATICHASH( ColorMaxG );
		m_Params.m_ColorMax.y = ConfigManager::GetInheritedFloat( sColorMaxG, m_Params.m_ColorMin.y, sDefinitionName );

		STATICHASH( ColorMaxB );
		m_Params.m_ColorMax.z = ConfigManager::GetInheritedFloat( sColorMaxB, m_Params.m_ColorMin.z, sDefinitionName );

		STATICHASH( ColorMaxA );
		m_Params.m_ColorMax.w = ConfigManager::GetInheritedFloat( sColorMaxA, m_Params.m_ColorMin.w, sDefinitionName );

		STATICHASH( InitialTrace );
		m_Params.m_InitialTrace = ConfigManager::GetInheritedBool( sInitialTrace, false, sDefinitionName );

		STATICHASH( InitialTraceOffsetOSX );
		m_Params.m_InitialTraceOffsetOS.x = ConfigManager::GetInheritedFloat( sInitialTraceOffsetOSX, 0.0f, sDefinitionName );

		STATICHASH( InitialTraceOffsetOSY );
		m_Params.m_InitialTraceOffsetOS.y = ConfigManager::GetInheritedFloat( sInitialTraceOffsetOSY, 0.0f, sDefinitionName );

		STATICHASH( InitialTraceOffsetOSZ );
		m_Params.m_InitialTraceOffsetOS.z = ConfigManager::GetInheritedFloat( sInitialTraceOffsetOSZ, 0.0f, sDefinitionName );

		STATICHASH( UpdateMeshTickRate );
		m_Params.m_UpdateMeshTickRate = ConfigManager::GetInheritedFloat( sUpdateMeshTickRate, 0.0f, sDefinitionName );

		sm_ParamCache.Insert( HashedDefinitionName, m_Params );
	}
}

/*static*/ void RosaParticles::FlushConfigCache()
{
	sm_ParamCache.Clear();
}

void RosaParticles::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	GetCachedConfig( DefinitionName );

	m_FirstTick = true;

	IRenderer* const		pRenderer		= RosaFramework::GetInstance()->GetRenderer();
	TextureManager* const	pTextureManager	= pRenderer->GetTextureManager();
	m_AlbedoMap								= pTextureManager->GetTexture( m_Params.m_AlbedoMapName.CStr() );
	m_NormalMap								= pTextureManager->GetTexture( m_Params.m_NormalMapName.CStr() );
	m_SpecMap								= pTextureManager->GetTexture( m_Params.m_SpecMapName.CStr() );

	// Make the particle system die out after the last particle
	if( SystemIsFinite() )
	{
		m_ExpireTime = GetTime() + m_Params.m_SystemLifetime;
		m_FinishTime = m_ExpireTime + m_Params.m_LifetimeMax;
	}

	m_ParticlesArray.Reserve( m_Params.m_MaxParticles );
	m_ParticlesArray.SetDeflate( false );

	CreateMesh();
}

void RosaParticles::Tick( const float DeltaTime )
{
	const bool ShouldTick = m_FirstTick || m_RenderedLastFrame;

	// DLP 5 Dec 2021: Moved this to before spawning particles so we show particles in their
	// initial position the first time they're rendered instead of one step ahead.
	if( ShouldTick )
	{
		TickParticles( DeltaTime );
	}

	if( ShouldTick )
	{
		SpawnParticles( m_Params.m_TickSpawnRate );
	}

	bool ShouldImmediateSpawnMax = m_Params.m_ImmediateSpawnMax && m_FirstTick;
	if( ShouldImmediateSpawnMax || ( !IsExpired() && ShouldTick ) )
	{
		uint NumParticlesToSpawn	= 0;
		uint MaxParticles			= m_Params.m_MaxParticles;

		if( m_Params.m_IsTracer )
		{
			// Make sure we're not making tracers behave in unexpected ways
			DEVASSERT( m_IsBeam );
			DEVASSERT( ShouldImmediateSpawnMax );
			DEVASSERT( 1 == MaxParticles );
		}
		else if( m_IsBeam )
		{
			// Limit max particles if we have a beam effect
			const Vector	BeamVector			= ( m_SystemBeamEndLocation - m_SystemLocation );
			const float		BeamLength			= BeamVector.Length();
			const uint		DesiredParticles	= static_cast<uint>( Round( m_Params.m_ParticlesPerMeter * BeamLength ) );
			MaxParticles						= Min( DesiredParticles, MaxParticles );
		}

		if( ShouldImmediateSpawnMax )
		{
			NumParticlesToSpawn = MaxParticles;
		}
		else if( m_Params.m_SpawnRate > 0.0f )
		{
			m_SpawnAccumulator += DeltaTime;
			NumParticlesToSpawn = (uint)( m_SpawnAccumulator * m_Params.m_SpawnRate );
			m_SpawnAccumulator = Mod( m_SpawnAccumulator, m_Params.m_InvSpawnRate );
		}

		// This logic means that new particles will not be spawned if the limit
		// is exceeded (rather than new particles killing off older ones).
		NumParticlesToSpawn = Min( NumParticlesToSpawn, MaxParticles - GetNumParticles() );
		SpawnParticles( NumParticlesToSpawn );
	}

	m_RenderedLastFrame	= false;
	m_FirstTick			= false;

	if( !AnimationState::StaticGetStylizedAnim() || m_Params.m_UpdateMeshTickRate <= 0.0f )
	{
		m_ShouldUpdateMesh = true;
	}
	else
	{
		m_NextUpdateMeshTime		-= DeltaTime;
		while( m_NextUpdateMeshTime < 0.0f )
		{
			m_NextUpdateMeshTime	+= m_Params.m_UpdateMeshTickRate;
			m_ShouldUpdateMesh		= true;
		}
	}
}

void RosaParticles::SpawnParticles( uint NumParticlesToSpawn )
{
	XTRACE_FUNCTION;

	const Matrix		RotationMatrix	= m_SystemOrientation.ToMatrix();
	const float			CurrentTime		= GetTime();
	RosaWorld* const	pWorld			= RosaFramework::GetInstance()->GetWorld();
	const Vector&		WindDirection	= pWorld->GetWindDirection();
	const Vector		BeamDirection	= m_IsBeam ? ( m_SystemBeamEndLocation - m_SystemLocation ).GetFastNormalized() : Vector();

	FOR_EACH_INDEX( SpawnParticleIndex, NumParticlesToSpawn )
	{
		// NOTE: For Rosa, since I'm never using sorted transparent elements, push back to array immediately.
		// (In Eldritch and Neon, I would push at the end of this function to either this array or a list.)
		SParticle& Particle			= m_ParticlesArray.PushBack();

		Particle.m_SpawnTime		= CurrentTime;
		Particle.m_ExpireTime		= CurrentTime + Math::Random( m_Params.m_LifetimeMin, m_Params.m_LifetimeMax );
		Particle.m_TickLifetime		= m_Params.m_TickLifetime;
		Particle.m_Size				= Math::Random( m_Params.m_InitialSizeMin, m_Params.m_InitialSizeMax );
		Particle.m_SizeVelocity		= Math::Random( m_Params.m_SizeVelocityMin, m_Params.m_SizeVelocityMax );
		Particle.m_Color			= Math::Random( m_Params.m_ColorMin, m_Params.m_ColorMax );

		if( m_Params.m_LinkedRGB )
		{
			Particle.m_Color.z = Particle.m_Color.y = Particle.m_Color.x;
		}

		Vector			SpawnLocation	= m_SystemLocation;
		if( m_Params.m_IsTracer )
		{
			DEVASSERT( m_IsBeam );
			// Spawn tracers at one half length from system location
			SpawnLocation				= m_SystemLocation + BeamDirection * ( 0.5f * m_Params.m_TracerLength );
		}
		else if( m_IsBeam )
		{
			// HACKHACK: Spawn at regular intervals instead of randomly if this is an immediate PS
			const float	BeamAlpha		= m_Params.m_ImmediateSpawnMax ? ( static_cast<float>( SpawnParticleIndex ) / static_cast<float>( NumParticlesToSpawn ) ) : Math::RandomF();
			SpawnLocation				= Lerp<Vector>( m_SystemLocation, m_SystemBeamEndLocation, BeamAlpha );
		}

		const Vector	InitialOffset	= GetParticleSpawnLocationOffset();
		Particle.m_Location				= SpawnLocation + InitialOffset;

		Particle.m_Velocity			= ( Math::Random( m_Params.m_InitialVelocityOSMin,		m_Params.m_InitialVelocityOSMax ) * RotationMatrix ) +
										Math::Random( m_Params.m_InitialVelocityWSMin,		m_Params.m_InitialVelocityWSMax ) +
									  ( Math::Random( m_Params.m_InitialVelocityWindMin,	m_Params.m_InitialVelocityWindMax ) * WindDirection ) +
									  ( Math::Random( m_Params.m_BeamVelocityMin,			m_Params.m_BeamVelocityMax ) * BeamDirection );
		Particle.m_Roll				= Math::Random( m_Params.m_InitialRollMin, m_Params.m_InitialRollMax );
		Particle.m_RollVelocity		= Math::Random( m_Params.m_RollVelocityMin, m_Params.m_RollVelocityMax );

		if( m_Params.m_InitialTrace )
		{
			const Vector		OffsetLocation	= Particle.m_Location + m_Params.m_InitialTraceOffsetOS * RotationMatrix;
			const Segment		TraceSegment	= Segment( Particle.m_Location, OffsetLocation );

			CollisionInfo		Info;
			Info.m_In_CollideWorld	= true;
			Info.m_In_UserFlags		= EECF_CollideAsEntity;

			if( pWorld->Trace( TraceSegment, Info ) )
			{
				Particle.m_Location = Info.m_Out_Intersection;
			}
			else
			{
				Particle.m_Location = OffsetLocation;
			}
		}
	}
}

Vector RosaParticles::GetParticleSpawnLocationOffset() const
{
	const Matrix SystemOrientation	= m_SystemOrientation.ToMatrix();
	const Vector SpawnOffsetOS		= m_Params.m_SpawnOffsetOS * SystemOrientation;
	const Vector SpawnExtents		= Math::Random( -m_Params.m_SpawnExtents, m_Params.m_SpawnExtents );
	const Vector SpawnExtentsOS		= SpawnExtents * SystemOrientation;
	return m_Params.m_SpawnOffsetWS + SpawnOffsetOS + SpawnExtentsOS;
}

void RosaParticles::TickParticles( const float DeltaTime )
{
	for( uint ParticleIndex = 0; ParticleIndex < m_ParticlesArray.Size(); )
	{
		SParticle& Particle = m_ParticlesArray[ ParticleIndex ];

		if( TickParticle( Particle, DeltaTime ) )
		{
			++ParticleIndex;
		}
		else
		{
			m_ParticlesArray.FastRemove( ParticleIndex );
		}
	}
}

bool RosaParticles::TickParticle( SParticle& Particle, const float DeltaTime )
{
	const float CurrentTime = GetTime();

	if( CurrentTime > Particle.m_ExpireTime )
	{
		return false;
	}

	if( m_Params.m_TickLifetime > 0 )
	{
		if( --Particle.m_TickLifetime < 0 )
		{
			return false;
		}
	}

	Particle.m_Size			= Max( 0.0f, Particle.m_Size + Particle.m_SizeVelocity * DeltaTime );
	Particle.m_Velocity		+= m_Params.m_AccelerationWS * DeltaTime;	// Integrating velocity before position is Euler-Cromer, stabler than Euler
	Particle.m_Roll			+= Particle.m_RollVelocity * DeltaTime;

	Vector Offset			= Particle.m_Velocity * DeltaTime;
	if( m_Params.m_Collision )
	{
		RosaWorld* const	pWorld			= RosaFramework::GetInstance()->GetWorld();
		const Segment		TraceSegment	= Segment( Particle.m_Location, Particle.m_Location + Offset );

		CollisionInfo		Info;
		Info.m_In_CollideWorld	= true;
		Info.m_In_UserFlags		= EECF_CollideAsEntity;

		if( pWorld->Trace( TraceSegment, Info ) )
		{
			Offset = Info.m_Out_Plane.ProjectVector( Offset );

			// Collision response--reflect velocity for bounce
			const Vector	NormalVelocity		= Particle.m_Velocity.ProjectionOnto( Info.m_Out_Plane.m_Normal );
			const Vector	TangentialVelocity	= Info.m_Out_Plane.ProjectVector( Particle.m_Velocity );
			Particle.m_Velocity					= TangentialVelocity + ( m_Params.m_Elasticity * -NormalVelocity );
		}
	}
	Particle.m_Location		+= Offset;

	return true;
}

void RosaParticles::Render()
{
	XTRACE_FUNCTION;

	if( m_ShouldUpdateMesh )
	{
		m_ShouldUpdateMesh = false;
		UpdateMesh();
	}

	if( m_DynamicMesh )
	{
		IRenderer* const pRenderer = RosaFramework::GetInstance()->GetRenderer();
		pRenderer->AddMesh( m_DynamicMesh );
	}

	m_RenderedLastFrame = true;
}

void RosaParticles::CreateMesh()
{
	if( m_Params.m_MaxParticles == 0 )
	{
		m_DynamicMesh = NULL;
	}
	else
	{
		IRenderer* const pRenderer = RosaFramework::GetInstance()->GetRenderer();

		uint NumVertices	= m_Params.m_MaxParticles * 4;
		uint NumIndices		= m_Params.m_MaxParticles * 6;

		m_VB_Positions.ResizeZero(		NumVertices );
		m_VB_FloatColors.ResizeZero(	NumVertices );
		m_VB_UVs.ResizeZero(			NumVertices );
		m_VB_Normals.ResizeZero(		NumVertices );
		m_VB_Tangents.ResizeZero(		NumVertices );
		m_VB_Indices.ResizeZero(		NumIndices );

		// Set up buffers wot won't ever change
		for( uint i = 0; i < m_Params.m_MaxParticles; ++i )
		{
			m_VB_UVs[ i * 4 ]			= Vector2( 0.0f, 0.0f );
			m_VB_UVs[ i * 4 + 1 ]		= Vector2( 1.0f, 0.0f );
			m_VB_UVs[ i * 4 + 2 ]		= Vector2( 0.0f, 1.0f );
			m_VB_UVs[ i * 4 + 3 ]		= Vector2( 1.0f, 1.0f );

			m_VB_Indices[ i * 6 ]		= static_cast<index_t>( i * 4 );
			m_VB_Indices[ i * 6 + 1 ]	= static_cast<index_t>( i * 4 + 2 );
			m_VB_Indices[ i * 6 + 2 ]	= static_cast<index_t>( i * 4 + 1 );
			m_VB_Indices[ i * 6 + 3 ]	= static_cast<index_t>( i * 4 + 2 );
			m_VB_Indices[ i * 6 + 4 ]	= static_cast<index_t>( i * 4 + 3 );
			m_VB_Indices[ i * 6 + 5 ]	= static_cast<index_t>( i * 4 + 1 );
		}

		IVertexBuffer* VertexBuffer = pRenderer->CreateVertexBuffer();
		InitVertexBuffers( VertexBuffer );

		// Register a callback so we can rebuild this mesh when device is reset
		SDeviceResetCallback Callback;
		Callback.m_Callback	= DeviceResetCallback;
		Callback.m_Void		= this;
		VertexBuffer->RegisterDeviceResetCallback( Callback );

		pRenderer->AddDynamicVertexBuffer( VertexBuffer );

		uint VertexSignature = VD_POSITIONS | VD_UVS | VD_FLOATCOLORS | VD_NORMALS | VD_TANGENTS;
		IVertexDeclaration* VertexDeclaration = pRenderer->GetVertexDeclaration( VertexSignature );

		IIndexBuffer* const IndexBuffer = pRenderer->CreateIndexBuffer();
		IndexBuffer->Init( NumIndices, m_VB_Indices.GetData() );
		IndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

		m_DynamicMesh = new RosaMesh;
		ASSERT( m_DynamicMesh );

		m_DynamicMesh->Initialize( VertexBuffer, VertexDeclaration, IndexBuffer, NULL );
		m_DynamicMesh->SetTexture( 0, m_AlbedoMap );
		m_DynamicMesh->SetTexture( 1, m_NormalMap );
		m_DynamicMesh->SetTexture( 2, m_SpecMap );

#if BUILD_DEBUG
		m_DynamicMesh->m_DEBUG_Name = "Particles";
#endif

		if( m_Params.m_MaterialOverride != "" )
		{
			m_DynamicMesh->SetMaterialDefinition( m_Params.m_MaterialOverride, pRenderer );
		}
		else
		{
			m_DynamicMesh->SetMaterialDefinition( "Material_ParticlesAlphaTest", pRenderer );
		}

		// NOTE: Particles should probably never cast shadows, even if they
		// are alpha tested and not transparent, simply because there's no
		// good way to deal with them being flat and casting thin shadows
		// from orthogonal lights.
		uint MaterialFlags = MAT_DYNAMIC;
		MaterialFlags |= m_Params.m_ForegroundDraw	? MAT_FOREGROUND	: MAT_WORLD;
		MaterialFlags |= m_Params.m_AlwaysDraw		? MAT_ALWAYS		: MAT_NONE;
		m_DynamicMesh->SetMaterialFlags( MaterialFlags );
	}
}

inline uint RosaParticles::GetNumParticles() const
{
	return m_ParticlesArray.Size();
}

void RosaParticles::InitVertexBuffers( IVertexBuffer* pVertexBuffer )
{
	ASSERT( pVertexBuffer );
	ASSERT( m_Params.m_MaxParticles > 0 );
	ASSERT( m_VB_Positions.GetData() );
	ASSERT( m_VB_FloatColors.GetData() );
	ASSERT( m_VB_UVs.GetData() );
	ASSERT( m_VB_Normals.GetData() );
	ASSERT( m_VB_Tangents.GetData() );

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= m_Params.m_MaxParticles * 4;
	InitStruct.Positions	= m_VB_Positions.GetData();
	InitStruct.FloatColors	= m_VB_FloatColors.GetData();
	InitStruct.UVs			= m_VB_UVs.GetData();
	InitStruct.Normals		= m_VB_Normals.GetData();
	InitStruct.Tangents		= m_VB_Tangents.GetData();
	InitStruct.Dynamic		= true;
	pVertexBuffer->Init( InitStruct );

	pVertexBuffer->SetNumVertices( GetNumParticles() * 4 );
}

void RosaParticles::UpdateMesh()
{
	XTRACE_FUNCTION;

	if( m_Params.m_MaxParticles == 0 )
	{
		return;
	}

	DEVASSERT( m_DynamicMesh );
	if( !m_DynamicMesh )
	{
		return;
	}

	uint NumParticles = GetNumParticles();
	DEVASSERT( NumParticles <= m_Params.m_MaxParticles );

	uint NumVertices	= NumParticles * 4;
	uint NumIndices		= NumParticles * 6;

	View* const pView	= RosaFramework::GetInstance()->GetMainView();
	DEVASSERT( pView );

	SParticleMeshParams MeshParams;
	MeshParams.m_ViewLocation	= pView->GetLocation();
	if( m_Params.m_IsTracer )
	{
		// For now, I'm assuming tracers travel along beam only
		MeshParams.m_TracerX = ( m_SystemBeamEndLocation - m_SystemLocation ).GetFastNormalized();
		for( uint ParticleIndex = 0; ParticleIndex < NumParticles; ++ParticleIndex )
		{
			const RosaParticles::SParticle& Particle = m_ParticlesArray[ ParticleIndex ];
			AddTracerToMesh( ParticleIndex, Particle, MeshParams );
		}
	}
	else
	{
		// Zero roll so that particles do not roll with player's view--that looks especially silly on foliage.
		Angles ViewAngle			= pView->GetRotation();
		ViewAngle.Roll				= 0.0f;
		MeshParams.m_ViewAxis		= ViewAngle.ToVector();
		MeshParams.m_AngleMatrix	= ViewAngle.ToMatrix();
		for( uint ParticleIndex = 0; ParticleIndex < NumParticles; ++ParticleIndex )
		{
			const RosaParticles::SParticle& Particle = m_ParticlesArray[ ParticleIndex ];
			AddParticleToMesh( ParticleIndex, Particle, MeshParams );
		}
	}

	// Copy updated local vertex elements into vertex buffers;
	// it'd be faster to not do memcpy here, but I do still need to maintain
	// the local copies for rebuilding on device reset, so...
	XTRACE_BEGIN( UpdateBuffers );
		IVertexBuffer* const	pVertexBuffer	= m_DynamicMesh->m_VertexBuffer;
		IIndexBuffer* const		pIndexBuffer	= m_DynamicMesh->m_IndexBuffer;

		DEVASSERT( pVertexBuffer );
		DEVASSERT( pIndexBuffer );

		Vector* const	pPositions	= static_cast<Vector*>(		pVertexBuffer->Lock( IVertexBuffer::EVE_Positions ) );
		DEVASSERT( pPositions );
		Vector4* const pFloatColors	= static_cast<Vector4*>(	pVertexBuffer->Lock( IVertexBuffer::EVE_FloatColors ) );
		DEVASSERT( pFloatColors );
		Vector* const	pNormals	= static_cast<Vector*>(		pVertexBuffer->Lock( IVertexBuffer::EVE_Normals ) );
		DEVASSERT( pNormals );
		Vector4* const	pTangents	= static_cast<Vector4*>(	pVertexBuffer->Lock( IVertexBuffer::EVE_Tangents ) );
		DEVASSERT( pTangents );

		DEVASSERT( m_VB_Positions.GetData() );
		DEVASSERT( m_VB_FloatColors.GetData() );
		DEVASSERT( m_VB_Normals.GetData() );
		DEVASSERT( m_VB_Tangents.GetData() );

		if( pPositions && pFloatColors && pNormals && pTangents )
		{
			memcpy( pPositions,		m_VB_Positions.GetData(),	NumVertices * sizeof( Vector ) );
			memcpy( pFloatColors,	m_VB_FloatColors.GetData(),	NumVertices * sizeof( Vector4 ) );
			memcpy( pNormals,		m_VB_Normals.GetData(),		NumVertices * sizeof( Vector ) );
			memcpy( pTangents,		m_VB_Tangents.GetData(),	NumVertices * sizeof( Vector4 ) );
		}

		pVertexBuffer->Unlock( IVertexBuffer::EVE_Positions );
		pVertexBuffer->Unlock( IVertexBuffer::EVE_FloatColors );
		pVertexBuffer->Unlock( IVertexBuffer::EVE_Normals );
		pVertexBuffer->Unlock( IVertexBuffer::EVE_Tangents );

		pVertexBuffer->SetNumVertices( NumVertices );
		pIndexBuffer->SetNumIndices( NumIndices );
	XTRACE_END;
}

void RosaParticles::SetLocation( const Vector& Location )
{
	m_SystemLocation			= Location;

	if( m_DynamicMesh )
	{
		m_DynamicMesh->m_Location	= Location;

		// Cheap bounding box; it's not worthwhile to compute a real bound every frame
		Vector BoundRadius( m_Params.m_ViewBoundRadius, m_Params.m_ViewBoundRadius, m_Params.m_ViewBoundRadius );
		m_DynamicMesh->SetAABB( AABB( Location - BoundRadius, Location + BoundRadius ) );
	}
}

// Call this *after* SetLocation!
void RosaParticles::SetBeamEndLocation( const Vector& BeamEndLocation )
{
	DEVASSERT( m_IsBeam );

	m_SystemBeamEndLocation = BeamEndLocation;

	// Create a bounding box to encompass the whole beam
	if( m_DynamicMesh )
	{
		Vector BoundMin, BoundMax;
		Vector::MinMax( m_SystemLocation, m_SystemBeamEndLocation, BoundMin, BoundMax );
		Vector BoundRadius( m_Params.m_ViewBoundRadius, m_Params.m_ViewBoundRadius, m_Params.m_ViewBoundRadius );
		m_DynamicMesh->SetAABB( AABB( BoundMin - BoundRadius, BoundMax + BoundRadius ) );
	}

	// HACKHACK: Fit lifetime to beam velocity and tracer length
	// (This works fine if the beam length is less than the tracer length;
	// the system immediately finishes.)
	if( SystemIsFinite() &&
		m_Params.m_IsTracer &&
		m_Params.m_BeamVelocityMin > 0.0f )
	{
		const float	BeamLength			= ( m_SystemBeamEndLocation - m_SystemLocation ).Length() - m_Params.m_TracerLength;
		const float	TravelDurationMax	= BeamLength / m_Params.m_BeamVelocityMin;
		const float	TravelEndTime		= GetTime() + TravelDurationMax;
		m_FinishTime					= Min( m_FinishTime, TravelEndTime );
	}
}

inline void RosaParticles::AddTracerToMesh( const uint Index, const SParticle& Particle, const SParticleMeshParams& MeshParams )
{
	const uint j = Index * 4;
	const uint k = j + 1;
	const uint l = k + 1;
	const uint m = l + 1;

	const Vector TracerZ		= MeshParams.m_TracerX.Cross( Particle.m_Location - MeshParams.m_ViewLocation ).GetFastNormalized();
	const Vector TracerY		= MeshParams.m_TracerX.Cross( TracerZ );
	const Matrix TracerMatrix	= Matrix::CreateCoordinate( MeshParams.m_TracerX, TracerY, TracerZ );
	const Matrix ParticleMatrix	= TracerMatrix * Matrix::CreateTranslation( Particle.m_Location - m_SystemLocation );	// Translate particles to be relative to origin (then set mesh location)

	const float HalfSize = Particle.m_Size * 0.5f;
	const float HalfLength = m_Params.m_TracerLength * 0.5f;
	m_VB_Positions[ j ] = Vector( -HalfLength, 0.0f, HalfSize ) * ParticleMatrix;
	m_VB_Positions[ k ] = Vector( HalfLength, 0.0f, HalfSize ) * ParticleMatrix;
	m_VB_Positions[ l ] = Vector( -HalfLength, 0.0f, -HalfSize ) * ParticleMatrix;
	m_VB_Positions[ m ] = Vector( HalfLength, 0.0f, -HalfSize ) * ParticleMatrix;

	float		Alpha			= 1.0f;
	const float	TimeAlive		= GetTime() - Particle.m_SpawnTime;
	const float	TimeRemaining	= Particle.m_ExpireTime - GetTime();
	if( TimeAlive < m_Params.m_FadeInTime )
	{
		Alpha = TimeAlive * m_Params.m_InvFadeInTime;
	}
	else if( TimeRemaining < m_Params.m_FadeOutTime )
	{
		Alpha = TimeRemaining * m_Params.m_InvFadeOutTime;
	}

	Vector4 FloatColor = Particle.m_Color;
	FloatColor.w *= Alpha;

	m_VB_FloatColors[ j ] = FloatColor;
	m_VB_FloatColors[ k ] = FloatColor;
	m_VB_FloatColors[ l ] = FloatColor;
	m_VB_FloatColors[ m ] = FloatColor;

	// NOTE: No longer angling normals out for Rosa. Plays badly with lighting,
	// and I can use normal maps to recreate the volumetric effect where needed.
	static const Vector kNormal	= Vector( 0.0f, -1.0f, 0.0f );
	const Vector		Normal	= kNormal * TracerMatrix;
	m_VB_Normals[ j ] = Normal;
	m_VB_Normals[ k ] = Normal;
	m_VB_Normals[ l ] = Normal;
	m_VB_Normals[ m ] = Normal;

	static const Vector kTangent	= Vector( 1.0f, 01.0f, 0.0f );
	const Vector		Tangent		= kTangent * TracerMatrix;
	const Vector4		Tangent4	= Vector4( Tangent, -1.0f );
	m_VB_Tangents[ j ] = Tangent4;
	m_VB_Tangents[ k ] = Tangent4;
	m_VB_Tangents[ l ] = Tangent4;
	m_VB_Tangents[ m ] = Tangent4;
}

inline void RosaParticles::AddParticleToMesh( const uint Index, const SParticle& Particle, const SParticleMeshParams& MeshParams )
{
	const uint j = Index * 4;
	const uint k = j + 1;
	const uint l = k + 1;
	const uint m = l + 1;

	const Matrix ParticleRotationMatrix	= MeshParams.m_AngleMatrix * Matrix::CreateRotation( -MeshParams.m_ViewAxis, Particle.m_Roll );
	const Matrix ParticleMatrix			= ParticleRotationMatrix * Matrix::CreateTranslation( Particle.m_Location - m_SystemLocation );	// Translate particles to be relative to origin (then set mesh location)

	const float HalfSize = Particle.m_Size * 0.5f;
	m_VB_Positions[ j ] = Vector( -HalfSize, 0.0f, HalfSize ) * ParticleMatrix;
	m_VB_Positions[ k ] = Vector( HalfSize, 0.0f, HalfSize ) * ParticleMatrix;
	m_VB_Positions[ l ] = Vector( -HalfSize, 0.0f, -HalfSize ) * ParticleMatrix;
	m_VB_Positions[ m ] = Vector( HalfSize, 0.0f, -HalfSize ) * ParticleMatrix;

	float		Alpha			= 1.0f;
	const float	TimeAlive		= GetTime() - Particle.m_SpawnTime;
	const float	TimeRemaining	= Particle.m_ExpireTime - GetTime();
	if( TimeAlive < m_Params.m_FadeInTime )
	{
		Alpha = TimeAlive * m_Params.m_InvFadeInTime;
	}
	else if( TimeRemaining < m_Params.m_FadeOutTime )
	{
		Alpha = TimeRemaining * m_Params.m_InvFadeOutTime;
	}

	Vector4 FloatColor = Particle.m_Color;
	FloatColor.w *= Alpha;

	m_VB_FloatColors[ j ] = FloatColor;
	m_VB_FloatColors[ k ] = FloatColor;
	m_VB_FloatColors[ l ] = FloatColor;
	m_VB_FloatColors[ m ] = FloatColor;

	// NOTE: No longer angling normals out for Rosa. Plays badly with lighting,
	// and I can use normal maps to recreate the volumetric effect where needed.
	static const Vector kNormal	= Vector( 0.0f, -1.0f, 0.0f );
	const Vector		Normal	= kNormal * ParticleRotationMatrix;
	m_VB_Normals[ j ] = Normal;
	m_VB_Normals[ k ] = Normal;
	m_VB_Normals[ l ] = Normal;
	m_VB_Normals[ m ] = Normal;

	static const Vector kTangent	= Vector( 1.0f, 01.0f, 0.0f );
	const Vector		Tangent		= kTangent * ParticleRotationMatrix;
	const Vector4		Tangent4	= Vector4( Tangent, -1.0f );
	m_VB_Tangents[ j ] = Tangent4;
	m_VB_Tangents[ k ] = Tangent4;
	m_VB_Tangents[ l ] = Tangent4;
	m_VB_Tangents[ m ] = Tangent4;
}

/*static*/ void RosaParticles::DeviceResetCallback( void* pVoid, IVertexBuffer* pBuffer )
{
	RosaParticles* const pParticleSystem = static_cast<RosaParticles*>( pVoid );
	pParticleSystem->InitVertexBuffers( pBuffer );
}

float RosaParticles::GetTime() const
{
	return WBWorld::GetInstance()->GetTime();
}

bool RosaParticles::SystemIsFinite() const
{
	return m_Params.m_SystemLifetime > 0.0f || ( m_Params.m_SpawnRate == 0.0f && m_Params.m_LifetimeMax > 0.0f );
}

void RosaParticles::Expire()
{
	m_Params.m_SystemLifetime	= 1.0f;	// HACKHACK: This is arbitrary, it just ensures SystemIsFinite will return true
	m_ExpireTime				= GetTime();
	m_FinishTime				= m_ExpireTime + m_Params.m_LifetimeMax;
}

bool RosaParticles::IsExpired() const
{
	return SystemIsFinite() && GetTime() >= m_ExpireTime;
}

bool RosaParticles::IsFinished() const
{
	return SystemIsFinite() && GetTime() >= m_FinishTime;
}
