#include "core.h"
#include "wbcomprosarespawner.h"
#include "wbeventmanager.h"
#include "hashedstring.h"
#include "wbcomprosatransform.h"
#include "configmanager.h"
#include "rosaworld.h"
#include "rosagame.h"
#include "collisioninfo.h"
#include "mathcore.h"
#include "idatastream.h"

WBCompRosaRespawner::WBCompRosaRespawner()
:	m_OriginSet( false )
,	m_OriginLocation()
,	m_OriginOrientation()
,	m_RetryRespawnTime( 0.0f )
,	m_RespawnMinPlayerDistanceSq( 0.0f )
{
}

WBCompRosaRespawner::~WBCompRosaRespawner()
{
}

/*virtual*/ void WBCompRosaRespawner::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( RetryRespawnTime );
	m_RetryRespawnTime = ConfigManager::GetInheritedFloat( sRetryRespawnTime, 0.0f, sDefinitionName );

	STATICHASH( RespawnMinPlayerDistance );
	m_RespawnMinPlayerDistanceSq = Square( ConfigManager::GetInheritedFloat( sRespawnMinPlayerDistance, 0.0f, sDefinitionName ) );
}


/*virtual*/ void WBCompRosaRespawner::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawnedQueued );
	STATIC_HASHED_STRING( Respawn );
	STATIC_HASHED_STRING( ForceRespawn );
	STATIC_HASHED_STRING( GoToOrigin );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawnedQueued )
	{
		ASSERT( !m_OriginSet );	// This isn't really meaningful anymore since I use OnSpawnedQueued instead of OnMoved
		m_OriginSet = true;

		WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		m_OriginLocation	= pTransform->GetLocation();
		m_OriginOrientation	= pTransform->GetOrientation();
	}
	else if( EventName == sRespawn )
	{
		TryRespawn();
	}
	else if( EventName == sForceRespawn )
	{
		Respawn();
	}
	else if( EventName == sGoToOrigin )
	{
		ASSERT( m_OriginSet );

		WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		pTransform->SetLocation( m_OriginLocation );
		pTransform->SetOrientation( m_OriginOrientation );
	}
}

void WBCompRosaRespawner::TryRespawn()
{
	if( CanRespawn() )
	{
		Respawn();
	}
	else
	{
		// Requeue event
		WB_MAKE_EVENT( Respawn, GetEntity() );
		WB_QUEUE_EVENT_DELAY( GetEventManager(), Respawn, GetEntity(), m_RetryRespawnTime );
	}
}

bool WBCompRosaRespawner::CanRespawn()
{
	// NOTE: I don't care about player facing;
	// player can turn so fast that it's basically irrelevant.
	// We want to spawn when player is distant and occluded.

	if( IsOriginNearPlayer() )
	{
		return false;
	}

	if( CanOriginBeSeenByPlayer() )
	{
		return false;
	}

	return true;
}

bool WBCompRosaRespawner::IsOriginNearPlayer()
{
	const Vector PlayerLocation = RosaGame::GetPlayerLocation();
	const float DistanceSq = ( PlayerLocation - m_OriginLocation ).LengthSquared();
	const bool IsNear = DistanceSq < m_RespawnMinPlayerDistanceSq;

	return IsNear;
}

bool WBCompRosaRespawner::CanOriginBeSeenByPlayer()
{
	// Check player occlusion
	CollisionInfo Info;
	Info.m_In_CollideWorld			= true;
	Info.m_In_CollideEntities		= true;
	Info.m_In_UserFlags				= EECF_Occlusion;
	Info.m_In_StopAtAnyCollision	= true;
	const bool Occluded				= GetWorld()->LineCheck( RosaGame::GetPlayerViewLocation(), m_OriginLocation, Info );

	return !Occluded;
}

void WBCompRosaRespawner::Respawn()
{
	ASSERT( m_OriginSet );

	// Destroy self
	GetEntity()->Destroy();

	WBEntity* const				pSpawnedEntity		= WBWorld::GetInstance()->CreateEntity( GetEntity()->GetName() );

	WBCompRosaTransform* const	pSpawnedTransform	= pSpawnedEntity->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pSpawnedTransform );

	pSpawnedTransform->SetInitialTransform( m_OriginLocation, m_OriginOrientation );
}

#define VERSION_EMPTY	0
#define VERSION_ORIGIN	1
#define VERSION_CURRENT	1

uint WBCompRosaRespawner::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += 1;					// m_OriginSet
	Size += sizeof( Vector );	// m_OriginLocation
	Size += sizeof( Angles );	// m_OriginOrientation

	return Size;
}

void WBCompRosaRespawner::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_OriginSet );
	Stream.Write( sizeof( Vector ), &m_OriginLocation );
	Stream.Write( sizeof( Angles ), &m_OriginOrientation );
}

void WBCompRosaRespawner::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ORIGIN )
	{
		m_OriginSet = Stream.ReadBool();
		Stream.Read( sizeof( Vector ), &m_OriginLocation );
		Stream.Read( sizeof( Angles ), &m_OriginOrientation );
	}
}
