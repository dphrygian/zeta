#include "core.h"
#include "wbcomprosacollision.h"
#include "collisioninfo.h"
#include "segment.h"
#include "configmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosamesh.h"
#include "rosamesh.h"
#include "Components/wbcompstatmod.h"
#include "wbentity.h"
#include "clock.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "array.h"
#include "aabb.h"
#include "wbeventmanager.h"
#include "idatastream.h"
#include "mathcore.h"
#include "rosanav.h"
#include "triangle.h"

/*static*/ WBCompRosaCollision::TCollisionMap	WBCompRosaCollision::sm_CollisionMap;
/*static*/ WBCompRosaCollision::TCollisionArray	WBCompRosaCollision::sm_TouchingArray;

WBCompRosaCollision::WBCompRosaCollision()
:	m_HalfExtents()
,	m_Bounds()
,	m_UseMeshExtents()
,	m_ExtentsFatten( 0.0f )
,	m_NavExtentsFatten( 0.0f )
,	m_NavExtents()
,	m_NavBounds()
,	m_Elasticity( 0.0f )
,	m_FrictionTargetTime( 0.0f )
,	m_FrictionTargetPercentVelocity( 0.0f )
,	m_UseAirFriction( false )
,	m_AirFrictionTargetTime( 0.0f )
,	m_AirFrictionTargetPercentVelocity( 0.0f )
,	m_StandOnSlopes( false )
,	m_MaxStepHeight( 0.0f )
,	m_CanStepUp( false )
,	m_Surface()
,	m_IsNavBlocking( false )
,	m_Landed( false )
,	m_UnlandedTime( 0.0f )
,	m_Touching()
,	m_CanTouch( false )
,	m_CollisionFlags( 0 )
,	m_DefaultCollisionFlags( 0 )
{
	// DLP 2 Dec 2021: HACKHACK: Register this name so we don't get stat mod debug warnings if we load up in air.
	STATIC_HASHED_STRING( InAir );
}

WBCompRosaCollision::~WBCompRosaCollision()
{
	RemoveFromCollisionMap();
	RemoveFromTouchingArray();
}

/*virtual*/ void WBCompRosaCollision::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( RosaCollision );

	// "Static" is a poor name, but these entities will be included in collision (not touching)
	// tests, occlusion tests, and nav tests. See ERosaCollisionFlags for exact definition.
	// Static does *not* imply that the entity cannot move, although it generally should not move.
	STATICHASH( IsStatic );
	const bool IsStatic = ConfigManager::GetInheritedBool( sIsStatic, false, sDefinitionName );
	SetCollisionFlags( IsStatic ? EECF_IsStatic : EECF_IsDynamic, EECF_Mask_EntityTypes, false, false );

	STATICHASH( CanTouch );
	m_CanTouch = ConfigManager::GetInheritedBool( sCanTouch, !IsStatic, sDefinitionName );

	STATICHASH( UseMeshExtents );
	m_UseMeshExtents = ConfigManager::GetInheritedBool( sUseMeshExtents, false, sDefinitionName );

	STATICHASH( ExtentsFatten );
	m_ExtentsFatten = ConfigManager::GetInheritedFloat( sExtentsFatten, 0.0f, sDefinitionName );

	STATICHASH( NavExtentsFatten );
	m_NavExtentsFatten = ConfigManager::GetInheritedFloat( sNavExtentsFatten, m_ExtentsFatten, sDefinitionName );

	STATICHASH( HalfExtentsXY );
	const float HalfExtentsXY = ConfigManager::GetInheritedFloat( sHalfExtentsXY, 0.0f, sDefinitionName );

	STATICHASH( HalfExtentsX );
	const float HalfExtentsX = ConfigManager::GetInheritedFloat( sHalfExtentsX, HalfExtentsXY, sDefinitionName );

	STATICHASH( HalfExtentsY );
	const float HalfExtentsY = ConfigManager::GetInheritedFloat( sHalfExtentsY, HalfExtentsXY, sDefinitionName );

	STATICHASH( HalfExtentsZ );
	const float HalfExtentsZ = ConfigManager::GetInheritedFloat( sHalfExtentsZ, 0.0f, sDefinitionName );

	m_HalfExtents = Vector( HalfExtentsX, HalfExtentsY, HalfExtentsZ );

	STATICHASH( NavExtentsXY );
	const float NavExtentsXY = ConfigManager::GetInheritedFloat( sNavExtentsXY, HalfExtentsXY, sDefinitionName );

	STATICHASH( NavExtentsX );
	const float NavExtentsX = ConfigManager::GetInheritedFloat( sNavExtentsX, Pick( HalfExtentsX, NavExtentsXY ), sDefinitionName );

	STATICHASH( NavExtentsY );
	const float NavExtentsY = ConfigManager::GetInheritedFloat( sNavExtentsY, Pick( HalfExtentsY, NavExtentsXY ), sDefinitionName );

	STATICHASH( NavExtentsZ );
	const float NavExtentsZ = ConfigManager::GetInheritedFloat( sNavExtentsZ, HalfExtentsZ, sDefinitionName );

	m_NavExtents = Vector( NavExtentsX, NavExtentsY, NavExtentsZ );

	// Entity can be NULL e.g. if we're constructing this collision to do a CheckClearance
	if( GetEntity() )
	{
		WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		m_HalfExtents	*= pTransform->GetScale();
		m_NavExtents	*= pTransform->GetScale();
	}

	STATICHASH( Elasticity );
	m_Elasticity = ConfigManager::GetInheritedFloat( sElasticity, 0.0f, sDefinitionName );

	STATICHASH( FrictionTargetTime );
	const float DefaultFrictionTargetTime = ConfigManager::GetFloat( sFrictionTargetTime, 0.0f, sRosaCollision );
	m_FrictionTargetTime = ConfigManager::GetInheritedFloat( sFrictionTargetTime, DefaultFrictionTargetTime, sDefinitionName );
	ASSERT( m_FrictionTargetTime > 0.0f );

	STATICHASH( FrictionTargetPercentVelocity );
	const float DefaultFrictionTargetPercentVelocity = ConfigManager::GetFloat( sFrictionTargetPercentVelocity, 0.0f, sRosaCollision );
	m_FrictionTargetPercentVelocity = ConfigManager::GetInheritedFloat( sFrictionTargetPercentVelocity, DefaultFrictionTargetPercentVelocity, sDefinitionName );

	STATICHASH( UseAirFriction );
	m_UseAirFriction = ConfigManager::GetInheritedBool( sUseAirFriction, false, sDefinitionName );

	STATICHASH( AirFrictionTargetTime );
	const float DefaultAirFrictionTargetTime = ConfigManager::GetFloat( sAirFrictionTargetTime, 0.0f, sRosaCollision );
	m_AirFrictionTargetTime = ConfigManager::GetInheritedFloat( sAirFrictionTargetTime, DefaultAirFrictionTargetTime, sDefinitionName );
	ASSERT( m_AirFrictionTargetTime > 0.0f );

	STATICHASH( AirFrictionTargetPercentVelocity );
	const float DefaultAirFrictionTargetPercentVelocity = ConfigManager::GetFloat( sAirFrictionTargetPercentVelocity, 0.0f, sRosaCollision );
	m_AirFrictionTargetPercentVelocity = ConfigManager::GetInheritedFloat( sAirFrictionTargetPercentVelocity, DefaultAirFrictionTargetPercentVelocity, sDefinitionName );

	STATICHASH( StandOnSlopes );
	m_StandOnSlopes = ConfigManager::GetInheritedBool( sStandOnSlopes, false, sDefinitionName );

	STATICHASH( MaxStepHeight );
	m_MaxStepHeight = ConfigManager::GetInheritedFloat( sMaxStepHeight, 0.0f, sDefinitionName );
	m_CanStepUp = ( m_MaxStepHeight > 0.0f );

	STATICHASH( Surface );
	m_Surface = ConfigManager::GetInheritedHash( sSurface, HashedString::NullString, sDefinitionName );

	STATICHASH( BlocksWorld );
	const bool BlocksWorld = ConfigManager::GetInheritedBool( sBlocksWorld, true, sDefinitionName );
	SetCollisionFlags( BlocksWorld ? EECF_BlocksWorld : EECF_None, EECF_BlocksWorld, false, false );

	STATICHASH( BlocksEntities );
	const bool BlocksEntities = ConfigManager::GetInheritedBool( sBlocksEntities, false, sDefinitionName );
	if( BlocksEntities )
	{
		if( IsStatic )
		{
			SetCollisionFlags( EECF_BlocksEntities | EECF_BlocksBlockers, EECF_BlocksEntities | EECF_BlocksBlockers, false, false );
		}
		else
		{
			// This entity is a blocker
			SetCollisionFlags( EECF_BlocksBlockers, EECF_BlocksBlockers, false, false );
		}
	}

	STATICHASH( BlocksOcclusion );
	const bool BlocksOcclusion = ConfigManager::GetInheritedBool( sBlocksOcclusion, false, sDefinitionName );
	SetCollisionFlags( BlocksOcclusion ? EECF_BlocksOcclusion : EECF_None, EECF_BlocksOcclusion, false, false );

	STATICHASH( BlocksTrace );
	const bool BlocksTrace = ConfigManager::GetInheritedBool( sBlocksTrace, false, sDefinitionName );
	SetCollisionFlags( BlocksTrace ? EECF_BlocksTrace : EECF_None, EECF_BlocksTrace, false, false );

	STATICHASH( BlocksRagdolls );
	const bool BlocksRagdolls = ConfigManager::GetInheritedBool( sBlocksRagdolls, false, sDefinitionName );
	SetCollisionFlags( BlocksRagdolls ? EECF_BlocksRagdolls : EECF_None, EECF_BlocksRagdolls, false, false );

	STATICHASH( BlocksNav );
	const bool BlocksNav = ConfigManager::GetInheritedBool( sBlocksNav, false, sDefinitionName );
	SetCollisionFlags( BlocksNav ? EECF_BlocksNav : EECF_None, EECF_BlocksNav, false, false );

	// By default, anything static that blocks a trace also blocks audio
	const bool DefaultBlocksAudio = IsStatic && BlocksTrace;

	STATICHASH( BlocksAudio );
	const bool BlocksAudio = ConfigManager::GetInheritedBool( sBlocksAudio, DefaultBlocksAudio, sDefinitionName );
	SetCollisionFlags( BlocksAudio ? EECF_BlocksAudio : EECF_None, EECF_BlocksAudio, false, false );

	m_DefaultCollisionFlags = m_CollisionFlags;

	AddToCollisionMap();
	AddToTouchingArray();
}

void WBCompRosaCollision::SetCollisionFlags( const uint Flags, const uint Mask /*= 0xffffffff*/, const bool SendEvents /*= false*/, const bool UpdateCollisionMap /*= true*/ )
{
	XTRACE_FUNCTION;

	// Make sure the mask includes all the bits in Flags
	ASSERT( CountBits( Mask ) >= CountBits( Flags | Mask ) );

	if( UpdateCollisionMap )
	{
		RemoveFromCollisionMap();
	}

	// Lower all the bits in the mask
	m_CollisionFlags &= ~Mask;

	// Raise all the bits in the flag and mask
	m_CollisionFlags |= Flags & Mask;

	if( UpdateCollisionMap )
	{
		AddToCollisionMap();
	}

	if( SendEvents )
	{
		ConditionalSendStaticCollisionChangedEvent();
	}
}

float WBCompRosaCollision::GetFrictionCoefficient() const
{
	if( m_FrictionTargetPercentVelocity >= 1.0f )
	{
		return 1.0f;
	}

	// Relationship between friction and the velocity we want to be reduced to at time t:
	//	TargetPercentVelocity = Pow( Mu, TargetTime / DeltaTime )
	//	Mu = Pow( TargetPercentVelocity, DeltaTime / TargetTime )
	const float DeltaTime = GetFramework()->GetClock()->GetGameDeltaTime();
	const float Mu = Pow( m_FrictionTargetPercentVelocity, DeltaTime / m_FrictionTargetTime );	// Friction coefficient
	return Mu;
}

float WBCompRosaCollision::GetAirFrictionCoefficient() const
{
	if( !m_UseAirFriction )
	{
		WARNDESC( "We shouldn't be calling this if we're not using it anymore." );
		return 1.0f;
	}

	if( m_AirFrictionTargetPercentVelocity >= 1.0f )
	{
		return 1.0f;
	}

	// Relationship between friction and the velocity we want to be reduced to at time t:
	//	TargetPercentVelocity = Pow( Mu, TargetTime / DeltaTime )
	//	Mu = Pow( TargetPercentVelocity, DeltaTime / TargetTime )
	const float DeltaTime = GetFramework()->GetClock()->GetGameDeltaTime();
	const float Mu = Pow( m_AirFrictionTargetPercentVelocity, DeltaTime / m_AirFrictionTargetTime );	// Friction coefficient
	return Mu;
}

bool WBCompRosaCollision::Collide( const Vector& StartLocation, Vector& InOutMovement )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	static const Vector	skUpVector			= Vector( 0.0f, 0.0f, 1.0f );
	static const float	skMoveLimit			= 1.0e-8f;	// At this threshold, we just don't move at all
	static const float	skLoIterationLimit	= 1.0e-3f;	// Below this threshold, zero component at sIterationLimitLo
	static const int	skIterationLimitLo	= 7;		// At this many collisions, zero any
	static const int	skIterationLimitHi	= 10;		// At this many collisions, bail out with no movement

	RosaWorld* const		pWorld = GetWorld();

	WBCompRosaTransform* const	pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );	// We shouldn't have a collision without a transform

	const float				FrictionCoefficient		= GetFrictionCoefficient();

	const bool WasFalling = InOutMovement.Dot( skUpVector ) < 0.0f;
	if( WasFalling || m_UseAirFriction ) // ACIDHACK
	{
		Fall();
	}

	// DLP 2 Dec 2021: No collision is fine; there's still physics sim stuff we want in here (HACKHACK for ghosting)
	const bool NonColliding = GetCollisionFlags() == EECF_None;
	DEVASSERT( NonColliding || MatchesAllCollisionFlags( EECF_IsDynamic ) );
	const uint	CollisionFlags	= IsDynamicBlocker() ? EECF_BlockerCollision : EECF_EntityCollision;

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= CollisionFlags;

	bool AnyCollision		= false;
	uint NumIterations		= 0;

#if BUILD_DEV
	// Make sure we're not starting inside geometry.
	if( !NonColliding && pWorld->CheckClearance( StartLocation, m_HalfExtents, Info ) )
	{
		PRINTF( "%s stuck at %s\n", GetEntity()->GetUniqueName().CStr(), StartLocation.GetString().CStr() );
		WARNDESC( "WBCompRosaCollision::Collide: Entity starting movement inside geometry" );
	}
#endif

	for(;;)
	{
		NumIterations++;

		const Segment SweepSegment = Segment( StartLocation, StartLocation + InOutMovement );
		if( NonColliding || !pWorld->Sweep( SweepSegment, m_HalfExtents, Info ) )
		{
			// ACIDHACK: Apply air friction
			if( m_UseAirFriction )
			{
				const float AirFrictionCoefficient = GetAirFrictionCoefficient();
				pTransform->SetVelocity( AirFrictionCoefficient * pTransform->GetVelocity() );
			}
			break;
		}

		DEVASSERT( !NonColliding );
		AnyCollision = true;

		// Complete collision, probably started the move inside geometry.
		if( Info.m_Out_Plane.m_Normal.LengthSquared() == 0.0f )
		{
			DEBUGPRINTF( "COMPLETE COLLISION: %s at %s\n", GetEntity()->GetUniqueName().CStr(), StartLocation.GetString().CStr() );
			DEBUGWARNDESC( "Complete collision, see log for details" );
			InOutMovement.Zero();
			// This was commented out for a long time... but I can't see any reason not to do it,
			// and it fixes things acquiring a huge downward velocity from gravity while they're stuck.
			pTransform->SetVelocity( Vector() );
			break;
		}

		const Vector OldInOutMovement = InOutMovement;
		InOutMovement = Info.m_Out_Plane.ProjectVector( InOutMovement );

		const float	FloorDot		= Info.m_Out_Plane.m_Normal.Dot( skUpVector );
		const bool	LandedOnFloor	= ( FloorDot >= 0.5f );	// 60 degree slope or shallower is walkable
		const bool	LandedOnSlope	= LandedOnFloor && ( FloorDot < 0.99f );
		const float	Friction		= LandedOnFloor ? FrictionCoefficient : 1.0f;

		// If we landed on a slope, fake that we landed on a flat floor and stop sliding.
		if( m_StandOnSlopes && LandedOnSlope )
		{
			const Plane		SlopePlane	= Info.m_Out_Plane;
			const Vector	UpSlope		= Info.m_Out_Plane.ProjectVector( skUpVector );

			// New Rosa method, should fix getting stuck in slopes
			const Vector	UpSlope2D	= Vector( UpSlope.x, UpSlope.y, 0.0f );
			const bool		IsAscending	= OldInOutMovement.Dot( UpSlope2D ) >= 0.0f;

			Info.m_Out_Plane.m_Normal	= skUpVector;

			// If we intersected with slope in a downward direction, recalculate projected movement
			// onto the new faked plane (the XY plane) so we don't slide down.
			if( !IsAscending )
			{
				InOutMovement = Info.m_Out_Plane.ProjectVector( OldInOutMovement );
			}
			else
			{
				// Reproject old movement vector onto flat plane (negating element of gravity)
				// and then reproject onto slope plane. This should give desired movement.
				InOutMovement = Info.m_Out_Plane.ProjectVector( OldInOutMovement );
				InOutMovement = SlopePlane.ProjectVector( InOutMovement );

				// Boost the component of the new movement in the direction of the slope
				// to compensate for the projection. Helps characters get up slopes a bit.
				const Vector SlopeComponent			= InOutMovement.ProjectionOnto( UpSlope );
				const Vector PerpendicularComponent	= InOutMovement - SlopeComponent;	// If you hit the slope dead-on, this would be zero
				const Vector BoostedSlopeComponent	= SlopeComponent / FloorDot;
				InOutMovement = PerpendicularComponent + BoostedSlopeComponent;
			}
		}

		// Step-up costs two additional sweeps! Use sparingly.
		if( m_CanStepUp && !LandedOnFloor && IsRecentlyLanded( 0.0f ) )
		{
			const Vector	StepUpExtents		= Vector( 0.0f, 0.0f, m_HalfExtents.z + 0.01f );	// Add a small amount so we don't end up flush against geo
			const Vector	TraceDownExtents	= Vector( m_HalfExtents.x, m_HalfExtents.y, 0.0f );	// Flattened so we find actual point of contact with the ground
			const Vector&	TraceDownStart		= SweepSegment.m_Point2;
			const Vector	TraceDownEnd		= TraceDownStart - StepUpExtents;
			const Segment	TraceDownSegment	= Segment( TraceDownStart, TraceDownEnd );

			CollisionInfo	StepUpInfo;
			StepUpInfo.m_In_CollideWorld		= true;
			StepUpInfo.m_In_CollideEntities		= true;
			StepUpInfo.m_In_CollidingEntity		= GetEntity();
			StepUpInfo.m_In_UserFlags			= CollisionFlags;

			// Make sure we actually hit anything when we trace down, else we don't know where to step
			if( pWorld->Sweep( TraceDownSegment, TraceDownExtents, StepUpInfo ) )
			{
				// Check if our trace down landed on a floor, else we can't step up
				const float	StepUpFloorDot			= StepUpInfo.m_Out_Plane.m_Normal.Dot( skUpVector );
				const bool	StepUpLandedOnFloor		= ( StepUpFloorDot >= 0.5f );	// 60 degree slope or shallower is walkable

				// Check how large of a step we took
				const Vector	StepUpEnd			= StepUpInfo.m_Out_Intersection + StepUpExtents;
				const float		StepHeight			= StepUpEnd.z - TraceDownStart.z;
				DEVASSERT( StepHeight > 0.0f );

				if( StepUpLandedOnFloor && StepHeight <= m_MaxStepHeight )
				{
					const Vector	StepUpStart		= StartLocation + Vector( 0.0f, 0.0f, StepHeight );
					const Segment	StepUpSegment	= Segment( StepUpStart, StepUpEnd );

					// Check if we can actually move to the stepped-up spot.
					if( !pWorld->Sweep( StepUpSegment, m_HalfExtents, StepUpInfo ) )
					{
						InOutMovement = StepUpEnd - StartLocation;
						OnSteppedUp( StepHeight );

						// Break out of collision iteration. We have a valid endpoint, but
						// unless we change StartLocation, this InOutMovement will collide again.
						break;
					}
				}
			}
		}

		// Something went wrong
		if( OldInOutMovement == InOutMovement )
		{
			DEBUGWARN;
			InOutMovement.Zero();
			break;
		}

		const Vector OldVelocity					= pTransform->GetVelocity();
		const Vector VelocityInCollisionPlane		= Info.m_Out_Plane.ProjectVector( OldVelocity );
		const Vector OrthogonalVelocityComponent	= VelocityInCollisionPlane * Friction;

		const Vector VelocityInCollisionNormal		= OldVelocity.ProjectionOnto( Info.m_Out_Plane.m_Normal );
		const Vector ParallelVelocityComponent		= VelocityInCollisionNormal * -m_Elasticity;

		const Vector NewVelocity					= OrthogonalVelocityComponent + ParallelVelocityComponent;
		pTransform->SetVelocity( NewVelocity );

		WBEntity* const		pHitEntity			= static_cast<WBEntity*>( Info.m_Out_HitEntity );
		const HashedString	CollisionSurface	= pWorld->GetCollisionSurface( Info );

		if( LandedOnFloor )
		{
			const float LandedMagnitude = VelocityInCollisionNormal.Length();
			DEVASSERT( LandedMagnitude >= 0.0f );
			OnLanded( LandedMagnitude, pHitEntity, CollisionSurface );
		}
		else
		{
			OnCollided( Info.m_Out_Plane.m_Normal, pHitEntity, CollisionSurface, VelocityInCollisionNormal );
		}

		if( NumIterations >= skIterationLimitHi )
		{
			InOutMovement.Zero();
			// Same reasons as above.
			pTransform->SetVelocity( Vector() );
			break;
		}
		else if( NumIterations >= skIterationLimitLo )
		{
			if( Abs( InOutMovement.x ) <= skLoIterationLimit ) { InOutMovement.x = 0.0f; }
			if( Abs( InOutMovement.y ) <= skLoIterationLimit ) { InOutMovement.y = 0.0f; }
			if( Abs( InOutMovement.z ) <= skLoIterationLimit ) { InOutMovement.z = 0.0f; }

			if( InOutMovement.x == 0.0f &&
				InOutMovement.y == 0.0f &&
				InOutMovement.z == 0.0f )
			{
				// Same reasons as above.
				pTransform->SetVelocity( Vector() );
				break;
			}
		}

		if( InOutMovement.LengthSquared() <= skMoveLimit )
		{
			InOutMovement.Zero();
			break;
		}
	}

	if( !NonColliding && InOutMovement != Vector() && pWorld->CheckClearance( StartLocation + InOutMovement, m_HalfExtents, Info ) )
	{
		// Expensive, but I need a final check to forbid any movement that would get things stuck in geo.
		// I'm seeing cases where AnyCollision is false and InOutMovement is nonzero, but the sweep didn't
		// detect a collision where this *does*. Probably a problem with ConvexHull::Sweep.
		InOutMovement.Zero();
		// Same reasons as above.
		pTransform->SetVelocity( Vector() );
	}

	// If we were falling in this movement, and now we're not moving, land the entity.
	if( WasFalling && InOutMovement == Vector() )
	{
		OnLanded( 0.0f, NULL, HashedString::NullString );
	}

	return AnyCollision;
}

void WBCompRosaCollision::OnLanded( const float LandedMagnitude, WBEntity* const pCollidedEntity, const HashedString& CollisionSurface )
{
	XTRACE_FUNCTION;

	if( !IsRecentlyLanded( 0.0f ) )
	{
		{
			WB_MAKE_EVENT( OnLanded, GetEntity() );
			WB_SET_AUTO( OnLanded, Float, LandedMagnitude, LandedMagnitude );
			WB_DISPATCH_EVENT( GetEventManager(), OnLanded, GetEntity() );
		}

		// OnAnyCollision is fired from both OnLanded and OnCollided.
		{
			WB_MAKE_EVENT( OnAnyCollision, GetEntity() );
			WB_SET_AUTO( OnAnyCollision, Entity, CollidedEntity, pCollidedEntity );
			WB_SET_AUTO( OnAnyCollision, Hash, CollisionSurface, CollisionSurface );
			WB_DISPATCH_EVENT( GetEventManager(), OnAnyCollision, GetEntity() );
		}
	}

	// HACKHACK: 1/4s buffer on refiring this. I'm adding it for the Dream Piano sequence in Rosa.
	if( !IsRecentlyLanded( 0.25f ) )
	{
		// Notify the collided object too
		if( pCollidedEntity )
		{
			WB_MAKE_EVENT( OnLandedUpon, pCollidedEntity );
			WB_SET_AUTO( OnLandedUpon, Entity, CollidedEntity, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), OnLandedUpon, pCollidedEntity );
		}
	}

	// Notify the collided object too (regardless of this entity's landed state)
	if( pCollidedEntity )
	{
		WB_MAKE_EVENT( OnAnyCollision, pCollidedEntity );
		WB_SET_AUTO( OnAnyCollision, Entity, CollidedEntity, GetEntity() );
		WB_SET_AUTO( OnAnyCollision, Hash, CollisionSurface, GetSurface() );
		WB_DISPATCH_EVENT( GetEventManager(), OnAnyCollision, pCollidedEntity );
	}

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	if( pStatMod )
	{
		STATIC_HASHED_STRING( InAir );
		pStatMod->UnTriggerEvent( sInAir );
	}

	m_Landed = true;
}

void WBCompRosaCollision::OnCollided( const Vector& CollisionNormal, WBEntity* const pCollidedEntity, const HashedString& CollisionSurface, const Vector& CollisionVelocity )
{
	XTRACE_FUNCTION;

	// OnCollided actually means *on collided with non-ground surface*, as distinguished from OnLanded.
	{
		WB_MAKE_EVENT( OnCollided, GetEntity() );
		WB_SET_AUTO( OnCollided, Vector, CollisionNormal, CollisionNormal );
		WB_SET_AUTO( OnCollided, Vector, CollisionVelocity, CollisionVelocity );
		WB_SET_AUTO( OnCollided, Entity, CollidedEntity, pCollidedEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnCollided, GetEntity() );
	}

	// OnAnyCollision handles both.
	{
		WB_MAKE_EVENT( OnAnyCollision, GetEntity() );
		WB_SET_AUTO( OnAnyCollision, Vector, CollisionNormal, CollisionNormal );
		WB_SET_AUTO( OnAnyCollision, Vector, CollisionVelocity, CollisionVelocity );
		WB_SET_AUTO( OnAnyCollision, Entity, CollidedEntity, pCollidedEntity );
		WB_SET_AUTO( OnAnyCollision, Hash, CollisionSurface, CollisionSurface );
		WB_DISPATCH_EVENT( GetEventManager(), OnAnyCollision, GetEntity() );
	}

	// Notify the collided object too.
	if( pCollidedEntity )
	{
		// OnCollidedWith is the complement to OnCollided; this is a new addition for Rosa (for pushable crates),
		// and I don't want to break old scripts by making both entities receive OnCollided
		{
			WB_MAKE_EVENT( OnCollidedWith, pCollidedEntity );
			WB_SET_AUTO( OnCollidedWith, Vector, CollisionNormal, CollisionNormal );
			WB_SET_AUTO( OnCollidedWith, Vector, CollisionVelocity, CollisionVelocity );
			WB_SET_AUTO( OnCollidedWith, Entity, CollidedEntity, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), OnCollidedWith, pCollidedEntity );
		}

		{
			WB_MAKE_EVENT( OnAnyCollision, pCollidedEntity );
			WB_SET_AUTO( OnAnyCollision, Vector, CollisionNormal, CollisionNormal );
			WB_SET_AUTO( OnAnyCollision, Vector, CollisionVelocity, CollisionVelocity );
			WB_SET_AUTO( OnAnyCollision, Entity, CollidedEntity, GetEntity() );
			WB_SET_AUTO( OnAnyCollision, Hash, CollisionSurface, GetSurface() );
			WB_DISPATCH_EVENT( GetEventManager(), OnAnyCollision, pCollidedEntity );
		}
	}
}

void WBCompRosaCollision::OnSteppedUp( const float StepHeight )
{
	XTRACE_FUNCTION;

	WB_MAKE_EVENT( OnSteppedUp, GetEntity() );
	WB_SET_AUTO( OnSteppedUp, Float, StepHeight, StepHeight );
	WB_DISPATCH_EVENT( GetEventManager(), OnSteppedUp, GetEntity() );
}

void WBCompRosaCollision::Jump()
{
	// NOTE: I used to do this only if m_Landed was true; that allowed the player to double
	// jump off their landing bounce. It was introduced with the Lift power, without comment,
	// but doesn't seem to have been necessary.
	m_Landed = false;
	m_UnlandedTime = 0.0f;	// Set unlanded time to zero (i.e., "started falling a long time ago") so we can't do a rapid double jump

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	if( pStatMod )
	{
		STATIC_HASHED_STRING( InAir );
		pStatMod->TriggerEvent( sInAir );
	}
}

void WBCompRosaCollision::Fall()
{
	if( m_Landed )
	{
		m_Landed = false;
		m_UnlandedTime = GetFramework()->GetClock()->GetGameCurrentTime();

		WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
		if( pStatMod )
		{
			STATIC_HASHED_STRING( InAir );
			pStatMod->TriggerEvent( sInAir );
		}
	}
}

bool WBCompRosaCollision::IsRecentlyLanded( const float TimeThreshold ) const
{
	if( m_Landed )
	{
		return true;
	}
	else
	{
		const float CurrentTime			= GetFramework()->GetClock()->GetGameCurrentTime();
		const float TimeSinceUnlanded	= CurrentTime - m_UnlandedTime;
		return ( TimeSinceUnlanded <= TimeThreshold );
	}
}

AABB WBCompRosaCollision::GetCurrentBounds() const
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	return AABB::CreateFromCenterAndExtents( pTransform->GetLocation(), m_HalfExtents );
}

AABB WBCompRosaCollision::GetCurrentNavBounds() const
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	return AABB::CreateFromCenterAndExtents( pTransform->GetLocation(), m_NavExtents );
}

bool WBCompRosaCollision::FindNavNodesUnder( Array<uint>& OutNavNodeIndices ) const
{
	RosaWorld* const	pWorld			= GetWorld();
	const AABB			EntityBounds	= GetNavBounds();

	uint				TempNavNodeIndex;
	const Vector		BoundsCenter	= EntityBounds.GetCenter();
	const Vector		QuarterExtents	= 0.5f * EntityBounds.GetExtents();

	// Sample 4 points on the navmesh to find the tris we're likely to block.
	const Vector		TestPoint0		= BoundsCenter + Vector(  QuarterExtents.x,  QuarterExtents.y, 0.0f );
	const Vector		TestPoint1		= BoundsCenter + Vector( -QuarterExtents.x,  QuarterExtents.y, 0.0f );
	const Vector		TestPoint2		= BoundsCenter + Vector(  QuarterExtents.x, -QuarterExtents.y, 0.0f );
	const Vector		TestPoint3		= BoundsCenter + Vector( -QuarterExtents.x, -QuarterExtents.y, 0.0f );

	if( pWorld->FindNavNodeUnder( TestPoint0, TempNavNodeIndex ) ) { OutNavNodeIndices.PushBackUnique( TempNavNodeIndex ); }
	if( pWorld->FindNavNodeUnder( TestPoint1, TempNavNodeIndex ) ) { OutNavNodeIndices.PushBackUnique( TempNavNodeIndex ); }
	if( pWorld->FindNavNodeUnder( TestPoint2, TempNavNodeIndex ) ) { OutNavNodeIndices.PushBackUnique( TempNavNodeIndex ); }
	if( pWorld->FindNavNodeUnder( TestPoint3, TempNavNodeIndex ) ) { OutNavNodeIndices.PushBackUnique( TempNavNodeIndex ); }

	return !OutNavNodeIndices.Empty();
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaCollision::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const		pFramework		= GetFramework();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();

	WBCompRosaTransform* const	pTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector Location = pTransform->GetLocation();
	pRenderer->DEBUGDrawBox( Location - m_HalfExtents, Location + m_HalfExtents, ARGB_TO_COLOR( 255, 255, 255, 255 ) );

	if( m_IsNavBlocking )
	{
		Array<uint> NavNodeIndices;
		if( FindNavNodesUnder( NavNodeIndices ) )
		{
			FOR_EACH_ARRAY( NavNodeIndexIter, NavNodeIndices, uint )
			{
				const uint&	NavNodeIndex	= NavNodeIndexIter.GetValue();

				// Copied from WBCompRosaPlayer, maybe wrap this up somewhere
				static const Vector	skNavNodeOffset = Vector( 0.0f, 0.0f, 0.01f );
				const SNavNode& NavNode = GetWorld()->GetNavNode( NavNodeIndex );

				// Draw the tri raised slightly and with each vert moved slightly toward centroid so it doesn't z-fight geo
				pRenderer->DEBUGDrawTriangle(
					NavNode.m_Tri.m_Vec1 + skNavNodeOffset + 0.01f * ( NavNode.m_Centroid - NavNode.m_Tri.m_Vec1 ).GetFastNormalized(),
					NavNode.m_Tri.m_Vec2 + skNavNodeOffset + 0.01f * ( NavNode.m_Centroid - NavNode.m_Tri.m_Vec2 ).GetFastNormalized(),
					NavNode.m_Tri.m_Vec3 + skNavNodeOffset + 0.01f * ( NavNode.m_Centroid - NavNode.m_Tri.m_Vec3 ).GetFastNormalized(),
					ARGB_TO_COLOR( 255, 255, 16, 16 ) );
			}
		}
	}
}
#endif

/*virtual*/ void WBCompRosaCollision::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnJumped );
	STATIC_HASHED_STRING( OnMoved );
	STATIC_HASHED_STRING( OnInitialTransformSet );
	STATIC_HASHED_STRING( OnLoaded );
	STATIC_HASHED_STRING( OnMeshUpdated );
	STATIC_HASHED_STRING( OnDestroyed );
	STATIC_HASHED_STRING( StopTouching );
	STATIC_HASHED_STRING( StartTouching );
	STATIC_HASHED_STRING( StopBlockingWorld );
	STATIC_HASHED_STRING( StopBlockingBlockers );
	STATIC_HASHED_STRING( SetDefaultFriction );
	STATIC_HASHED_STRING( DisableCollision );
	STATIC_HASHED_STRING( EnableCollision );
	STATIC_HASHED_STRING( ToggleCollision );
	STATIC_HASHED_STRING( SetCollisionExtentsZ );
	STATIC_HASHED_STRING( EnableAirFriction );
	STATIC_HASHED_STRING( DisableAirFriction );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnJumped )
	{
		Jump();
	}
	else if( EventName == sOnLoaded )
	{
		UpdateBounds();

		// If this entity is a nav blocker, add its bounds during loading.
		ConditionalSetNavBlocking( true );
	}
	else if( EventName == sOnMoved )
	{
		// Remove nav blocker at old location
		ConditionalSetNavBlocking( false );

		UpdateBounds();
		UpdateTouching();

		// Add nav blocker at new location
		ConditionalSetNavBlocking( true );
		ConditionalSendStaticCollisionChangedEvent();
	}
	else if( EventName == sOnInitialTransformSet )
	{
		// For initialization of certain classes of static entities, fix up extents based on orientation.
		if( m_HalfExtents.x != m_HalfExtents.y ||
			m_NavExtents.x != m_NavExtents.y )
		{
			STATIC_HASHED_STRING( Orientation );
			const Angles	Orientation	= Event.GetAngles( sOrientation );
			const float		Yaw			= Mod( TWOPI + Orientation.Yaw, TWOPI );

			if( Equal( Yaw, DEGREES_TO_RADIANS( 90.0f ) ) ||
				Equal( Yaw, DEGREES_TO_RADIANS( 270.0f ) ) )
			{
				// Remove nav blocker at old location
				ConditionalSetNavBlocking( false );

				Swap( m_HalfExtents.x, m_HalfExtents.y );
				Swap( m_NavExtents.x, m_NavExtents.y );

				UpdateBounds();
				UpdateTouching();

				// Add nav blocker at new orientation
				ConditionalSetNavBlocking( true );
			}
		}
	}
	else if( EventName == sOnMeshUpdated )
	{
		ASSERT( m_UseMeshExtents );

		WBCompRosaMesh* const		pMeshComponent	= WB_GETCOMP( GetEntity(), RosaMesh );
		DEVASSERT( pMeshComponent );

		RosaMesh* const			pMesh			= pMeshComponent->GetMesh();
		DEVASSERT( pMesh );

		// Remove nav blocker at old location
		ConditionalSetNavBlocking( false );

		m_HalfExtents	= pMesh->m_AABB.GetExtents() + Vector( m_ExtentsFatten, m_ExtentsFatten, m_ExtentsFatten );
		m_NavExtents	= pMesh->m_AABB.GetExtents() + Vector( m_NavExtentsFatten, m_NavExtentsFatten, m_NavExtentsFatten );

		UpdateBounds();
		UpdateTouching();

		// Add nav blocker at new orientation
		ConditionalSetNavBlocking( true );
	}
	else if( EventName == sOnDestroyed )
	{
		UpdateTouching();

		// If this entity is a nav blocker, remove its bounds when it is destroyed.
		ConditionalSetNavBlocking( false );

		ConditionalSendStaticCollisionChangedEvent();
	}
	else if( EventName == sStopTouching )
	{
		if( m_CanTouch )
		{
			RemoveFromTouchingArray();

			m_CanTouch = false;
			SetCollisionFlags( EECF_None, EECF_BlocksTrace );

			UpdateTouching();
		}
	}
	else if( EventName == sStartTouching )
	{
		if( !m_CanTouch )
		{
			m_CanTouch = true;
			SetCollisionFlags( m_DefaultCollisionFlags & EECF_BlocksTrace, EECF_BlocksTrace );

			AddToTouchingArray();
			UpdateTouching();
		}
	}
	else if( EventName == sStopBlockingWorld )
	{
		SetCollisionFlags( EECF_None, EECF_BlocksWorld );
	}
	else if( EventName == sStopBlockingBlockers )
	{
		SetCollisionFlags( EECF_None, EECF_BlocksBlockers );
	}
	else if( EventName == sDisableCollision )
	{
		DisableCollision();
	}
	else if( EventName == sEnableCollision )
	{
		EnableCollision();
	}
	else if( EventName == sToggleCollision )
	{
		if( HasDefaultCollision() )
		{
			DisableCollision();
		}
		else
		{
			EnableCollision();
		}
	}
	else if( EventName == sSetDefaultFriction )
	{
		STATICHASH( RosaCollision );

		STATICHASH( FrictionTargetTime );
		const float DefaultFrictionTargetTime = ConfigManager::GetFloat( sFrictionTargetTime, 0.0f, sRosaCollision );

		STATICHASH( FrictionTargetPercentVelocity );
		const float DefaultFrictionTargetPercentVelocity = ConfigManager::GetFloat( sFrictionTargetPercentVelocity, 0.0f, sRosaCollision );

		m_FrictionTargetTime = DefaultFrictionTargetTime;
		ASSERT( m_FrictionTargetTime > 0.0f );

		m_FrictionTargetPercentVelocity = DefaultFrictionTargetPercentVelocity;
	}
	else if( EventName == sSetCollisionExtentsZ )
	{
		Vector Extents	= GetExtents();

		STATIC_HASHED_STRING( ExtentsZ );
		Extents.z		= Event.GetFloat( sExtentsZ );

		SetExtents( Extents );
	}
	else if( EventName == sEnableAirFriction )
	{
		m_UseAirFriction = true;
	}
	else if( EventName == sDisableAirFriction )
	{
		m_UseAirFriction = false;
	}
}

// Disable/EnableCollision are used for static world entities.
void WBCompRosaCollision::DisableCollision()
{
	// If this entity is a nav blocker, remove its bounds when collision is disabled.
	ConditionalSetNavBlocking( false );

	SetCollisionFlags( EECF_None );
}

void WBCompRosaCollision::EnableCollision()
{
	ResetCollisionFlags();

	// If this entity is a nav blocker, re-add its bounds when collision is enabled.
	ConditionalSetNavBlocking( true );
}

/*virtual*/ void WBCompRosaCollision::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsLanded, IsRecentlyLanded( 0.0f ) );
	WB_SET_CONTEXT( Event, Bool, IsStatic, MatchesAllCollisionFlags( EECF_IsStatic ) );
}

void WBCompRosaCollision::ConditionalSetNavBlocking( const bool NavBlocking )
{
	if( m_IsNavBlocking == NavBlocking )
	{
		return;
	}

	if( MatchesAllCollisionFlags( EECF_Nav ) )
	{
		RosaNav::GetInstance()->UpdateWorldFromEntity( this, NavBlocking );
		m_IsNavBlocking = NavBlocking;
	}
}

void WBCompRosaCollision::ConditionalSendStaticCollisionChangedEvent()
{
	XTRACE_FUNCTION;

	if( MatchesAllCollisionFlags( EECF_IsStatic ) )
	{
		WB_MAKE_EVENT( OnStaticCollisionChanged, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), OnStaticCollisionChanged, GetEntity() );
	}
}

void WBCompRosaCollision::SetExtents( const Vector& HalfExtents )
{
	// If this entity is a nav blocker, remove its old bounds before updating the extents.
	ConditionalSetNavBlocking( false );

	m_HalfExtents = HalfExtents;

	UpdateBounds();
	UpdateTouching();

	// If this entity is a nav blocker, adds its new bounds after updating the extents.
	ConditionalSetNavBlocking( true );
}

void WBCompRosaCollision::GatherTouching( Array<WBEntityRef>& OutTouching ) const
{
	WBEntity* const pThisEntity = GetEntity();

	if( pThisEntity->IsDestroyed() )
	{
		// Special case, untouch everything.
		return;
	}

	if( !m_CanTouch )
	{
		return;
	}

	WBCompRosaTransform* const pThisTransform = pThisEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pThisTransform );

	const AABB ThisBox = AABB::CreateFromCenterAndExtents( pThisTransform->GetLocation(), m_HalfExtents );

	const Array<WBCompRosaCollision*>&	CollisionComponents	= GetTouchingArray();
	const uint							NumEntities			= CollisionComponents.Size();
	for( uint CollisionEntityIndex = 0; CollisionEntityIndex < NumEntities; ++CollisionEntityIndex )
	{
		WBCompRosaCollision* const pCollision = CollisionComponents[ CollisionEntityIndex ];

		ASSERT( pCollision->m_CanTouch );

		WBEntity* const pEntity = pCollision->GetEntity();

		if( pEntity == pThisEntity )
		{
			continue;
		}

		if( pEntity->IsDestroyed() )
		{
			continue;
		}

		const AABB& Box = pCollision->GetBounds();
		if( Box.Intersects( ThisBox ) )
		{
			OutTouching.PushBack( pEntity );
		}
	}
}

// NOTE: This does not capture touches that occur during a sweep; as such, it would
// be possible for an object traveling very fast to tunnel through another object
// and never get a touch notification.
void WBCompRosaCollision::UpdateTouching()
{
	PROFILE_FUNCTION;

	Array<WBEntityRef> OldTouching = m_Touching;

	Array<WBEntityRef> CurrentTouching;
	GatherTouching( CurrentTouching );

	// Update m_Touching ASAP (before invoking below events)
	// so other things that check the current touching array
	// will get up to date results. (Fixes a bug with trap
	// bolts not damaging things when they spawn touching.)
	m_Touching = CurrentTouching;

	// Untouch anything that was touched and now isn't
	FOR_EACH_ARRAY( TouchingIter, OldTouching, WBEntityRef )
	{
		const WBEntityRef& Touching = TouchingIter.GetValue();

		if( CurrentTouching.Search( Touching ).IsNull() )
		{
			SendUntouchEvent( Touching );
		}
	}

	// Touch anything that was not touched and now is
	FOR_EACH_ARRAY( TouchingIter, CurrentTouching, WBEntityRef )
	{
		const WBEntityRef& Touching = TouchingIter.GetValue();

		if( OldTouching.Search( Touching ).IsNull() )
		{
			SendTouchEvent( Touching );
		}
	}
}

void WBCompRosaCollision::SendTouchEvent( const WBEntityRef& TouchingEntity )
{
	XTRACE_FUNCTION;

	WBEntity* const pThisEntity = GetEntity();
	WBEntity* const pTouchingEntity = TouchingEntity.Get();
	DEVASSERT( pTouchingEntity );
	WBCompRosaCollision* const pTouchingCollision = WB_GETCOMP( pTouchingEntity, RosaCollision );
	DEVASSERT( pTouchingCollision );

	pTouchingCollision->AddTouching( pThisEntity );

	{
		WB_MAKE_EVENT( OnTouched, pThisEntity );
		WB_SET_AUTO( OnTouched, Entity, Touched, pTouchingEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnTouched, pThisEntity );
	}

	{
		WB_MAKE_EVENT( OnTouched, pTouchingEntity );
		WB_SET_AUTO( OnTouched, Entity, Touched, pThisEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnTouched, pTouchingEntity );
	}
}

void WBCompRosaCollision::SendUntouchEvent( const WBEntityRef& TouchingEntity )
{
	XTRACE_FUNCTION;

	WBEntity* const pThisEntity = GetEntity();
	WBEntity* const pTouchingEntity = TouchingEntity.Get();
	WBCompRosaCollision* const pTouchingCollision = WB_GETCOMP_SAFE( pTouchingEntity, RosaCollision );

	if( pTouchingCollision )
	{
		pTouchingCollision->RemoveTouching( pThisEntity );
	}

	{
		WB_MAKE_EVENT( OnUntouched, pThisEntity );
		WB_SET_AUTO( OnUntouched, Entity, Untouched, pTouchingEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnUntouched, pThisEntity );
	}

	if( pTouchingEntity )
	{
		WB_MAKE_EVENT( OnUntouched, pTouchingEntity );
		WB_SET_AUTO( OnUntouched, Entity, Untouched, pThisEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnUntouched, pTouchingEntity );
	}
}

void WBCompRosaCollision::GetTouchingEntities( Array<WBEntity*>& OutTouchingEntities ) const
{
	FOR_EACH_ARRAY( TouchingIter, m_Touching, WBEntityRef )
	{
		const WBEntityRef& Touching = TouchingIter.GetValue();
		WBEntity* const pTouchingEntity = Touching.Get();
		if( pTouchingEntity )
		{
			OutTouchingEntities.PushBack( pTouchingEntity );
		}
	}
}

void WBCompRosaCollision::AddToCollisionMap()
{
	// NOTE: For now, I'm just sorting by blocking type.
	// I could sort by complete flag if I need.
#define CONDITIONAL_ADD_TO_COLLISION_MAP( flag ) if( MatchesAllCollisionFlags( flag ) ) { AddToCollisionArray( flag ); }

	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksWorld );
	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksEntities );
	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksBlockers );
	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksOcclusion );
	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksAudio );
	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksTrace );
	CONDITIONAL_ADD_TO_COLLISION_MAP( EECF_BlocksRagdolls );

#undef CONDITIONAL_ADD_TO_COLLISION_MAP
}

void WBCompRosaCollision::AddToCollisionArray( const uint Flags )
{
	TCollisionArray& CollisionArray = sm_CollisionMap[ Flags ];
	ASSERT( !CollisionArray.Find( this, NULL ) );
	CollisionArray.PushBack( this );
}

void WBCompRosaCollision::AddToTouchingArray()
{
	if( m_CanTouch )
	{
		ASSERT( !sm_TouchingArray.Find( this, NULL ) );
		sm_TouchingArray.PushBack( this );
	}
}

void WBCompRosaCollision::RemoveFromCollisionMap()
{
#define CONDITIONAL_REMOVE_FROM_COLLISION_MAP( flag ) if( MatchesAllCollisionFlags( flag ) ) { RemoveFromCollisionArray( flag ); }

	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksWorld );
	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksEntities );
	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksBlockers );
	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksOcclusion );
	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksAudio );
	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksTrace );
	CONDITIONAL_REMOVE_FROM_COLLISION_MAP( EECF_BlocksRagdolls );

#undef CONDITIONAL_REMOVE_FROM_COLLISION_MAP
}

void WBCompRosaCollision::RemoveFromCollisionArray( const uint Flags )
{
	ASSERT( sm_CollisionMap.Search( Flags ).IsValid() );
	TCollisionArray& CollisionArray = sm_CollisionMap[ Flags ];
	ASSERT( CollisionArray.Find( this, NULL ) );
	CollisionArray.FastRemoveItem( this );

	// Clean up after array so we don't need to manage static memory.
	if( CollisionArray.Empty() )
	{
		sm_CollisionMap.Remove( Flags );
	}
}

void WBCompRosaCollision::RemoveFromTouchingArray()
{
	if( m_CanTouch )
	{
		ASSERT( sm_TouchingArray.Find( this, NULL ) );
		sm_TouchingArray.FastRemoveItem( this );
	}
}

/*static*/ const WBCompRosaCollision::TCollisionArray* WBCompRosaCollision::GetCollisionArray( const uint Flags )
{
	TCollisionMap::Iterator CollisionMapIter = sm_CollisionMap.Search( Flags );
	if( CollisionMapIter.IsValid() )
	{
		const TCollisionArray& CollisionArray = CollisionMapIter.GetValue();
		return &CollisionArray;
	}
	else
	{
		return NULL;
	}
}

#define VERSION_EMPTY			0
#define VERSION_TOUCHING		1
#define VERSION_HALFEXTENTS		2
#define VERSION_CANTOUCH		3
#define VERSION_FLAGS			4
#define VERSION_FRICTION		5
#define VERSION_NAVEXTENTS		6
#define VERSION_AIRFRICTION		7
#define VERSION_USEAIRFRICTION	8
#define VERSION_CURRENT			8

uint WBCompRosaCollision::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;											// Version
	Size += sizeof( c_uint32 );							// m_Touching.Size()
	Size += sizeof( WBEntityRef ) * m_Touching.Size();	// m_Touching
	Size += sizeof( Vector );							// m_HalfExtents
	Size += 1;											// m_CanTouch
	Size += 4;											// m_CollisionFlags
	Size += 4;											// m_FrictionTargetTime
	Size += 4;											// m_FrictionTargetPercentVelocity
	Size += sizeof( Vector );							// m_NavExtents
	Size += 4;											// m_AirFrictionTargetTime
	Size += 4;											// m_AirFrictionTargetPercentVelocity
	Size += 1;											// m_UseAirFriction

	return Size;
}

void WBCompRosaCollision::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_Touching.Size() );
	FOR_EACH_ARRAY( TouchingIter, m_Touching, WBEntityRef )
	{
		const WBEntityRef& Touching = TouchingIter.GetValue();
		Stream.Write( sizeof( WBEntityRef ), &Touching );
	}

	Stream.Write( sizeof( Vector ), &m_HalfExtents );

	Stream.WriteBool( m_CanTouch );

	Stream.WriteUInt32( m_CollisionFlags );

	Stream.WriteFloat( m_FrictionTargetTime );
	Stream.WriteFloat( m_FrictionTargetPercentVelocity );

	Stream.Write( sizeof( Vector ), &m_NavExtents );

	Stream.WriteFloat( m_AirFrictionTargetTime );
	Stream.WriteFloat( m_AirFrictionTargetPercentVelocity );
	Stream.WriteBool( m_UseAirFriction );
}

void WBCompRosaCollision::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_TOUCHING )
	{
		ASSERT( m_Touching.Empty() );
		const uint NumTouching = Stream.ReadUInt32();
		for( uint TouchingIndex = 0; TouchingIndex < NumTouching; ++TouchingIndex )
		{
			WBEntityRef Touching;
			Stream.Read( sizeof( WBEntityRef ), &Touching );
			m_Touching.PushBack( Touching );
		}
	}

	if( Version >= VERSION_HALFEXTENTS )
	{
		Stream.Read( sizeof( Vector ), &m_HalfExtents );
	}

	if( Version >= VERSION_CANTOUCH )
	{
		RemoveFromTouchingArray();

		m_CanTouch = Stream.ReadBool();

		AddToTouchingArray();
	}

	if( Version >= VERSION_FLAGS )
	{
		SetCollisionFlags( Stream.ReadUInt32() );
	}

	if( Version >= VERSION_FRICTION )
	{
		m_FrictionTargetTime			= Stream.ReadFloat();
		m_FrictionTargetPercentVelocity	= Stream.ReadFloat();
	}

	if( Version >= VERSION_NAVEXTENTS )
	{
		Stream.Read( sizeof( Vector ), &m_NavExtents );
	}

	if( Version >= VERSION_AIRFRICTION )
	{
		m_AirFrictionTargetTime				= Stream.ReadFloat();
		m_AirFrictionTargetPercentVelocity	= Stream.ReadFloat();
	}

	if( Version >= VERSION_USEAIRFRICTION )
	{
		m_UseAirFriction = Stream.ReadBool();
	}
}
