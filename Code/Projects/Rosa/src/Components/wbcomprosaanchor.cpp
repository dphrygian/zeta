#include "core.h"
#include "wbcomprosaanchor.h"
#include "wbcomprosatransform.h"
#include "ray.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "matrix.h"

WBCompRosaAnchor::WBCompRosaAnchor()
:	m_IsAnchored( false )
,	m_AnchorPoint()
,	m_AnchorDirection()
{
	STATIC_HASHED_STRING( OnStaticCollisionChanged );
	GetEventManager()->AddObserver( sOnStaticCollisionChanged, this );
}

WBCompRosaAnchor::~WBCompRosaAnchor()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnStaticCollisionChanged );
		pEventManager->RemoveObserver( sOnStaticCollisionChanged, this );
	}
}

/*virtual*/ void WBCompRosaAnchor::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AnchorDirectionX );
	m_AnchorDirection.x = ConfigManager::GetInheritedFloat( sAnchorDirectionX, 0.0f, sDefinitionName );

	STATICHASH( AnchorDirectionY );
	m_AnchorDirection.y = ConfigManager::GetInheritedFloat( sAnchorDirectionY, 0.0f, sDefinitionName );

	STATICHASH( AnchorDirectionZ );
	m_AnchorDirection.z = ConfigManager::GetInheritedFloat( sAnchorDirectionZ, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaAnchor::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawnedQueued );
	STATIC_HASHED_STRING( OnStaticCollisionChanged );
	STATIC_HASHED_STRING( Unanchor );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawnedQueued )
	{
		SetAnchor();
	}
	else if( EventName == sOnStaticCollisionChanged )
	{
		if( m_IsAnchored )
		{
			CheckAnchor();
		}
	}
	else if( EventName == sUnanchor )
	{
		if( m_IsAnchored )
		{
			// Forcibly unanchor the entity
			Unanchor();
		}
	}
}

/*virtual*/ void WBCompRosaAnchor::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsAnchored, m_IsAnchored );
}

// Check anchoring collision. If it is gone, send the OnUnanchored event.
void WBCompRosaAnchor::CheckAnchor()
{
	ASSERT( m_IsAnchored );

	RosaWorld* const pWorld = GetWorld();

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= EECF_CollideAsEntity | EECF_CollideStaticEntities;

	if( !pWorld->PointCheck( m_AnchorPoint, Info ) )
	{
		Unanchor();
	}
}

void WBCompRosaAnchor::Unanchor()
{
	ASSERT( m_IsAnchored );

	m_IsAnchored = false;

	WB_MAKE_EVENT( OnUnanchored, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnUnanchored, GetEntity() );
}

void WBCompRosaAnchor::SetAnchor()
{
	RosaWorld* const		pWorld			= GetWorld();
	ASSERT( pWorld );

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Angles				Orientation		= pTransform->GetOrientation();
	const Matrix				RotationMatrix	= Orientation.ToMatrix();
	const Vector				AnchorVector	= m_AnchorDirection * RotationMatrix;
	const Vector				StartLocation	= pTransform->GetLocation();
	const Ray					TraceRay		= Ray( StartLocation, AnchorVector );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pEntity;
	Info.m_In_UserFlags			= EECF_CollideAsEntity | EECF_CollideStaticEntities;

	if( pWorld->Trace( TraceRay, Info ) )
	{
		m_AnchorPoint	= Info.m_Out_Intersection + AnchorVector * 0.1f;
		ASSERT( pWorld->PointCheck( m_AnchorPoint, Info ) );	// Since hard-coded 0.1 might break for thin surfaces.
		m_IsAnchored	= true;
	}
}

#define VERSION_EMPTY	0
#define VERSION_ANCHOR	1
#define VERSION_CURRENT	1

uint WBCompRosaAnchor::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += sizeof( Vector );	// m_AnchorPoint
	Size += 1;					// m_IsAnchored

	return Size;
}

void WBCompRosaAnchor::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.Write( sizeof( Vector ), &m_AnchorPoint );
	Stream.WriteBool( m_IsAnchored );
}

void WBCompRosaAnchor::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ANCHOR )
	{
		Stream.Read( sizeof( Vector ), &m_AnchorPoint );
		m_IsAnchored = Stream.ReadBool();
	}
}
