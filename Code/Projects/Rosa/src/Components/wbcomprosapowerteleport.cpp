#include "core.h"
#include "wbcomprosapowerteleport.h"
#include "wbeventmanager.h"
#include "idatastream.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcompowner.h"
#include "rosaworld.h"
#include "collisioninfo.h"

WBCompRosaPowerTeleport::WBCompRosaPowerTeleport()
:	m_Beacon()
{
}

WBCompRosaPowerTeleport::~WBCompRosaPowerTeleport()
{
}

/*virtual*/ void WBCompRosaPowerTeleport::HandleEvent( const WBEvent& Event )
{
	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawnedEntityAction );
	STATIC_HASHED_STRING( TryTeleport );
	STATIC_HASHED_STRING( DestroyBeacon );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawnedEntityAction )
	{
		STATIC_HASHED_STRING( SpawnedEntity );
		WBEntity* const pSpawnedEntity = Event.GetEntity( sSpawnedEntity );

		ASSERT( m_Beacon.Get() == NULL );
		m_Beacon = pSpawnedEntity;
	}
	else if( EventName == sTryTeleport )
	{
		TryTeleport();
	}
	else if( EventName == sDestroyBeacon )
	{
		WBEntity* const pBeacon = m_Beacon.Get();
		if( pBeacon )
		{
			pBeacon->Destroy();
		}
	}
}

void WBCompRosaPowerTeleport::TryTeleport() const
{
	WBEntity* const				pBeacon				= m_Beacon.Get();
	if( !pBeacon )
	{
		WARN;
		return;
	}

	WBEntity* const				pEntity				= GetEntity();

	WBCompRosaTransform* const	pBeaconTransform	= pBeacon->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pBeaconTransform );

	WBEntity* const				pOwner				= WBCompOwner::GetTopmostOwner( pEntity );
	ASSERT( pOwner );

	WBCompRosaTransform* const	pOwnerTransform		= pOwner->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pOwnerTransform );

	WBCompRosaCollision* const	pOwnerCollision		= WB_GETCOMP( pOwner, RosaCollision );
	ASSERT( pOwnerCollision );

	Vector						TeleportLocation	= pBeaconTransform->GetLocation();
	const Vector				OwnerExtents		= pOwnerCollision->GetExtents();

	RosaWorld* const		pWorld				= GetWorld();
	ASSERT( pWorld );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pOwner;
	Info.m_In_UserFlags			= EECF_BlockerCollision;

	if( !pWorld->FindSpot( TeleportLocation, OwnerExtents, Info ) )
	{
		return;
	}

	pOwnerTransform->SetLocation( TeleportLocation );

	// Notify that teleport was successful
	WB_MAKE_EVENT( OnTeleported, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnTeleported, pEntity );
}

#define VERSION_EMPTY	0
#define VERSION_BEACON	1
#define VERSION_CURRENT	1

/*virtual*/ uint WBCompRosaPowerTeleport::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += sizeof( WBEntityRef );	// m_Beacon

	return Size;
}

/*virtual*/ void WBCompRosaPowerTeleport::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.Write( sizeof( WBEntityRef ), &m_Beacon );
}

/*virtual*/ void WBCompRosaPowerTeleport::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_BEACON )
	{
		Stream.Read( sizeof( WBEntityRef ), &m_Beacon );
	}
}
