#include "core.h"
#include "wbcomprosarope.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosamesh.h"
#include "wbcomprosaanchor.h"
#include "ray.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "mathcore.h"

WBCompRosaRope::WBCompRosaRope()
:	m_CollisionFatten( 0.0f )
,	m_MeshFatten( 0.0f )
,	m_EndpointSpacing( 0.0f )
,	m_AnchorDepth( 0.0f )
,	m_HookLength( 0.0f )
,	m_DangleHeight( 0.0f )
,	m_HookEntity( "" )
,	m_Anchor()
,	m_Dropped( false )
{
	STATIC_HASHED_STRING( OnStaticCollisionChanged );
	GetEventManager()->AddObserver( sOnStaticCollisionChanged, this );
}

WBCompRosaRope::~WBCompRosaRope()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnStaticCollisionChanged );
		pEventManager->RemoveObserver( sOnStaticCollisionChanged, this );
	}
}

/*virtual*/ void WBCompRosaRope::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( CollisionFatten );
	m_CollisionFatten = ConfigManager::GetInheritedFloat( sCollisionFatten, 0.0f, sDefinitionName );

	STATICHASH( MeshFatten );
	m_MeshFatten = ConfigManager::GetInheritedFloat( sMeshFatten, 0.0f, sDefinitionName );

	STATICHASH( EndpointSpacing );
	m_EndpointSpacing = ConfigManager::GetInheritedFloat( sEndpointSpacing, 0.0f, sDefinitionName );

	STATICHASH( AnchorDepth );
	m_AnchorDepth = ConfigManager::GetInheritedFloat( sAnchorDepth, 0.0f, sDefinitionName );

	STATICHASH( HookLength );
	m_HookLength = ConfigManager::GetInheritedFloat( sHookLength, 0.0f, sDefinitionName );

	STATICHASH( DangleHeight );
	m_DangleHeight = ConfigManager::GetInheritedFloat( sDangleHeight, 0.0f, sDefinitionName );

	ASSERT( m_DangleHeight < m_HookLength );

	STATICHASH( HookEntity );
	m_HookEntity = ConfigManager::GetInheritedString( sHookEntity, "", sDefinitionName );
	ASSERT( m_HookEntity != "" );
}

/*virtual*/ void WBCompRosaRope::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnStaticCollisionChanged );
	STATIC_HASHED_STRING( OnInitialTransformSet );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnStaticCollisionChanged )
	{
		if( m_Dropped )
		{
			// If anchor is removed, destroy the rope (and drop anyone climbing on it!)

			CollisionInfo Info;
			Info.m_In_CollideWorld		= true;
			Info.m_In_CollideEntities	= true;
			Info.m_In_CollidingEntity	= GetEntity();
			Info.m_In_UserFlags			= EECF_CollideAsEntity | EECF_CollideStaticEntities;

			RosaWorld* const pWorld = GetWorld();
			if( !pWorld->PointCheck( m_Anchor, Info ) )
			{
				GetEntity()->Destroy();
			}
		}
	}
	else if( EventName == sOnInitialTransformSet )
	{
		DropRope();
	}
}

void WBCompRosaRope::DropRope()
{
	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	// HookVector is directed away from the surface (same as hit normal)
	const Vector				HookVector	= Quantize( pTransform->GetOrientation().ToVector() );
	const Vector				DropVector	= Vector( 0.0f, 0.0f, -1.0f );

	ASSERT( HookVector.LengthSquared() == 1.0f );
	ASSERT(	( HookVector.y == 0.0f && HookVector.z == 0.0f ) ||
			( HookVector.x == 0.0f && HookVector.z == 0.0f ) ||
			( HookVector.x == 0.0f && HookVector.y == 0.0f ) );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	WBCompRosaMesh* const		pMesh		= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMesh );

	RosaWorld* const		pWorld		= GetWorld();

	const Vector	HitLocation		= pTransform->GetLocation();
	const Vector	StartLocation	= HitLocation + ( HookVector * m_HookLength );
	const Ray		TraceRay		= Ray( StartLocation, DropVector );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pEntity;
	Info.m_In_UserFlags			= EECF_CollideAsEntity | EECF_CollideStaticEntities;

	if( pWorld->Trace( TraceRay, Info ) )
	{
		m_Dropped = true;

		// Reset orientation now that it is being "baked" into extents.
		pTransform->SetOrientation( Angles() );

		// StartLocation is where the rope attaches to the hook
		// EndLocation is where the rope dangles above the ground
		ASSERT( Info.m_Out_Plane.m_Normal.Equals( Vector::Up, EPSILON ) );
		const Vector EndLocation = Info.m_Out_Intersection + Info.m_Out_Plane.m_Normal * m_DangleHeight;
		pTransform->SetLocation( 0.5f * ( StartLocation + EndLocation ) );

		const Vector HalfExtents = ( 0.5f * ( EndLocation - StartLocation ) ).GetAbs();
		const Vector CollisionFatten = Vector( m_CollisionFatten, m_CollisionFatten, m_CollisionFatten );
		pCollision->SetExtents( HalfExtents + CollisionFatten );

		const Vector MeshFatten = Vector( m_MeshFatten, m_MeshFatten, m_MeshFatten );
		pMesh->SetMeshScale( HalfExtents + MeshFatten );

		m_Anchor = HitLocation - ( HookVector * m_AnchorDepth );

		// Spawn rope hook entity and set up its transform and anchor
		{
			WBEntity* const				pHookEntity		= WBWorld::GetInstance()->CreateEntity( m_HookEntity );
			ASSERT( pHookEntity );

			WBCompRosaTransform* const	pHookTransform	= pHookEntity->GetTransformComponent<WBCompRosaTransform>();
			ASSERT( pHookTransform );

			WBCompRosaAnchor* const		pHookAnchor		= WB_GETCOMP( pHookEntity, RosaAnchor );
			ASSERT( pHookAnchor );

			pHookTransform->SetLocation( StartLocation );
			pHookTransform->SetOrientation( ( -HookVector ).ToAngles() );
		}
	}
	else
	{
		WARNDESC( "Rope could not be launched." );
		pEntity->Destroy();
	}
}

Vector WBCompRosaRope::Quantize( const Vector& V ) const
{
	Vector RetVal;

	for( uint i = 0; i < 3; ++i )
	{
		if( Abs( V.v[i] ) < EPSILON )
		{
			RetVal.v[i] = 0.0f;
		}
		else
		{
			RetVal.v[i] = Sign( V.v[i] );
		}
	}

	return RetVal;
}

#define VERSION_EMPTY	0
#define VERSION_ANCHOR	1
#define VERSION_DROPPED	2
#define VERSION_CURRENT	2

uint WBCompRosaRope::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += sizeof( Vector );	// m_Anchor
	Size += 1;					// m_Dropped

	return Size;
}

void WBCompRosaRope::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.Write( sizeof( Vector ), &m_Anchor );

	Stream.WriteBool( m_Dropped );
}

void WBCompRosaRope::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ANCHOR )
	{
		Stream.Read( sizeof( Vector ), &m_Anchor );
	}

	if( Version >= VERSION_DROPPED )
	{
		m_Dropped = Stream.ReadBool();
	}
}
