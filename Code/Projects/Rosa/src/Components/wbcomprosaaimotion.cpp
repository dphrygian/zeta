#include "core.h"
#include "wbcomprosaaimotion.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosadoor.h"
#include "wbcomprosarepulsor.h"
#include "wbcomponentarrays.h"
#include "wbcomprosahealth.h"
#include "wbcomprosafaction.h"
#include "Components/wbcomprodinknowledge.h"
#include "Components/wbcomprodinresourcemap.h"
#include "Components/wbcompstatmod.h"
#include "configmanager.h"
#include "mathcore.h"
#include "wbeventmanager.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "segment.h"
#include "collisioninfo.h"
#include "idatastream.h"
#include "reversehash.h"
#include "mathfunc.h"
#include "fontmanager.h"

// NOTE: This *never* releases its animation resource. That's ok.

WBCompRosaAIMotion::WBCompRosaAIMotion()
:	m_CanMove( false )
,	m_LandAcceleration( 0.0f )
,	m_AirAcceleration( 0.0f )
,	m_TurnSpeed( 0.0f )
,	m_FollowValidateTime( 0.0f )
,	m_NextFollowValidateTime( 0.0f )
,	m_StepReachedThresholdSq( 0.0f )
,	m_TurnReachedThreshold( 0.0f )
,	m_RepathOnNextTick( false )
,	m_Paused( false )
,	m_IsFlying( false )
,	m_CanOpenDoors( false )
,	m_CanUnlockDoors( false )
,	m_CanBreakDoors( false )
,	m_MaxPathSteps( 0 )
,	m_IdleAnimationName()
,	m_WalkAnimationName()
,	m_StanceAnimationNames()
,	m_AnimationResource()
,	m_HasAnimationResource( false )
,	m_IdleAnimRateBase( 0.0f )
,	m_WalkAnimRateBase( 0.0f )
,	m_PathData()
,	m_PathIndex( 0 )
,	m_PathBound()
,	m_MotionStatus( EMS_Still )
,	m_MotionStance( HashedString::NullString )
,	m_Cautious( false )
,	m_Wander( false )
,	m_WanderTargetDistance( 0.0f )
,	m_Flee( false )
,	m_FleeTargetDistance( 0.0f )
,	m_UseTether( false )
,	m_TetherLocation()
,	m_TetherDistance( 0.0f )
,	m_TetherDistanceZ( 0.0f )
,	m_ApproachDistance( 0.0f )
,	m_UseActualTargetLocation( false )
,	m_ReachedThresholdMinSq( 0.0f )
,	m_ReachedThresholdMaxSq( 0.0f )
,	m_FlyingDeflectionRadiusSq( 0.0f )
,	m_PostDeflectionEndTime( 0.0f )
,	m_DeflectionEndTime( 0.0f )
,	m_FlyingDestinationOffsetZ( 0.0f )
,	m_LastDestination()
,	m_LastDestinationIndex( 0 )
,	m_LastDestinationEntity()
,	m_WaitingToPlayIdleAnim( false )
,	m_PlayIdleAnimTime( 0.0f )
,	m_UseTurnTargetDirection( false )
,	m_TurnTargetDirection()
,	m_CurrentRepulsor()
,	m_StuckWatchdogLocation()
,	m_StepUpTime( 0.0f )
,	m_StepUpZInterpolator()
#if BUILD_DEV
,	m_WarnMaxSteps( false )
#endif
{
	STATIC_HASHED_STRING( OnNavChanged );
	GetEventManager()->AddObserver( sOnNavChanged, this );
}

WBCompRosaAIMotion::~WBCompRosaAIMotion()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnNavChanged );
		pEventManager->RemoveObserver( sOnNavChanged, this );
	}
}

void WBCompRosaAIMotion::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( CanMove );
	m_CanMove = ConfigManager::GetInheritedBool( sCanMove, true, sDefinitionName );

	STATICHASH( LandAcceleration );
	m_LandAcceleration = ConfigManager::GetInheritedFloat( sLandAcceleration, 0.0f, sDefinitionName );

	STATICHASH( AirAcceleration );
	m_AirAcceleration = ConfigManager::GetInheritedFloat( sAirAcceleration, 0.0f, sDefinitionName );

	STATICHASH( TurnSpeed );
	m_TurnSpeed = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sTurnSpeed, 0.0f, sDefinitionName ) );

	STATICHASH( FollowValidateTime );
	m_FollowValidateTime = ConfigManager::GetInheritedFloat( sFollowValidateTime, 0.0f, sDefinitionName );

	STATICHASH( StepReachedThreshold );
	m_StepReachedThresholdSq = Square( ConfigManager::GetInheritedFloat( sStepReachedThreshold, 0.0f, sDefinitionName ) );

	STATICHASH( TurnReachedThreshold );
	const float TurnReachedComplementDegrees = 90.0f - ConfigManager::GetInheritedFloat( sTurnReachedThreshold, 0.0f, sDefinitionName );
	m_TurnReachedThreshold = Cos( DEGREES_TO_RADIANS( TurnReachedComplementDegrees ) );

	STATICHASH( IsFlying );
	m_IsFlying = ConfigManager::GetInheritedBool( sIsFlying, false, sDefinitionName );

	STATICHASH( CanUnlockDoors );
	m_CanUnlockDoors = ConfigManager::GetInheritedBool( sCanUnlockDoors, false, sDefinitionName );

	STATICHASH( CanBreakDoors );
	m_CanBreakDoors = ConfigManager::GetInheritedBool( sCanBreakDoors, false, sDefinitionName );

	STATICHASH( CanOpenDoors );
	m_CanOpenDoors = m_CanUnlockDoors || m_CanBreakDoors || ConfigManager::GetInheritedBool( sCanOpenDoors, false, sDefinitionName );

	STATICHASH( MaxPathSteps );
	m_MaxPathSteps = ConfigManager::GetInheritedInt( sMaxPathSteps, 0, sDefinitionName );

	STATICHASH( IdleAnimation );
	m_IdleAnimationName = ConfigManager::GetInheritedHash( sIdleAnimation, HashedString::NullString, sDefinitionName );

	STATICHASH( WalkAnimation );
	m_WalkAnimationName = ConfigManager::GetInheritedHash( sWalkAnimation, HashedString::NullString, sDefinitionName );

	STATICHASH( NumStanceAnimations );
	const uint NumStanceAnimations = ConfigManager::GetInheritedInt( sNumStanceAnimations, 0, sDefinitionName );
	for( uint StanceAnimationIndex = 0; StanceAnimationIndex < NumStanceAnimations; ++StanceAnimationIndex )
	{
		const HashedString Stance				= ConfigManager::GetInheritedSequenceHash( "Stance%d", StanceAnimationIndex, HashedString::NullString, sDefinitionName );
		const HashedString StanceAnimationName	= ConfigManager::GetInheritedSequenceHash( "Stance%dAnimation", StanceAnimationIndex, HashedString::NullString, sDefinitionName );

		m_StanceAnimationNames.Insert( Stance, StanceAnimationName );
	}

	STATICHASH( AnimationResource );
	m_AnimationResource = ConfigManager::GetInheritedHash( sAnimationResource, HashedString::NullString, sDefinitionName );

	STATICHASH( IdleAnimRateLo );
	const float IdleAnimRateLo = ConfigManager::GetInheritedFloat( sIdleAnimRateLo, 1.0f, sDefinitionName );

	STATICHASH( IdleAnimRateHi );
	const float IdleAnimRateHi = ConfigManager::GetInheritedFloat( sIdleAnimRateHi, 1.0f, sDefinitionName );

	m_IdleAnimRateBase = Math::Random( IdleAnimRateLo, IdleAnimRateHi );

	STATICHASH( WalkAnimRateLo );
	const float WalkAnimRateLo = ConfigManager::GetInheritedFloat( sWalkAnimRateLo, 1.0f, sDefinitionName );

	STATICHASH( WalkAnimRateHi );
	const float WalkAnimRateHi = ConfigManager::GetInheritedFloat( sWalkAnimRateHi, 1.0f, sDefinitionName );

	m_WalkAnimRateBase = Math::Random( WalkAnimRateLo, WalkAnimRateHi );

	STATICHASH( StepUpTime );
	m_StepUpTime = ConfigManager::GetInheritedFloat( sStepUpTime, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaAIMotion::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetIdleAnimation );
	STATIC_HASHED_STRING( OnCollided );
	STATIC_HASHED_STRING( DoorOpenFailed );
	STATIC_HASHED_STRING( OnNavChanged );
	STATIC_HASHED_STRING( OnLanded );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( StopAIMotion );
	STATIC_HASHED_STRING( PauseAIMotion );
	STATIC_HASHED_STRING( UnpauseAIMotion );
	STATIC_HASHED_STRING( Follow );
	STATIC_HASHED_STRING( SetStance );
	STATIC_HASHED_STRING( OnSteppedUp );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetIdleAnimation )
	{
		STATIC_HASHED_STRING( IdleAnimation );
		m_IdleAnimationName = Event.GetHash( sIdleAnimation );

		// HACKHACK: Slam the anim on, since I'm just using this
		// in the hub currently and I know it's the desired result
		PlayIdleAnimation( true );
	}
	else if( EventName == sOnCollided )
	{
		if( m_Paused )
		{
			// Don't push doors if we're paused
		}
		else
		{
			STATIC_HASHED_STRING( CollidedEntity );
			WBEntity* const			pCollidedEntity	= Event.GetEntity( sCollidedEntity );
			WBCompRosaDoor* const	pCollidedDoor	= WB_GETCOMP_SAFE( pCollidedEntity, RosaDoor );

			if( m_CanOpenDoors && pCollidedDoor && pCollidedDoor->ShouldAITryHandle( GetEntity() ) )
			{
				// This was intended to be more general purpose, but doors are the only thing that can be pushed.
				WB_MAKE_EVENT( OnPushed, pCollidedEntity );
				WB_SET_AUTO( OnPushed, Entity, Pusher, GetEntity() );
				WB_DISPATCH_EVENT( GetEventManager(), OnPushed, pCollidedEntity );
			}
		}
	}
	else if( EventName == sDoorOpenFailed )
	{
		if( IsLocomoting() )
		{
			m_RepathOnNextTick = true;
		}
	}
	else if( EventName == sOnNavChanged )
	{
		if( IsLocomoting() )
		{
			STATIC_HASHED_STRING( BoxMin );
			const Vector BoxMin = Event.GetVector( sBoxMin );

			STATIC_HASHED_STRING( BoxMax );
			const Vector BoxMax = Event.GetVector( sBoxMax );

			const AABB ChangedBox( BoxMin, BoxMax );
			m_RepathOnNextTick = ChangedBox.Intersects( m_PathBound );
		}
	}
	else if( EventName == sOnInitialized )
	{
		PlayIdleAnimation( true );
	}
	else if( EventName == sOnLanded )
	{
		if( IsLocomoting() )
		{
			PlayMotionAnimation();
		}
		else
		{
			PlayIdleAnimation( false );
		}
	}
	else if( EventName == sStopAIMotion )
	{
		StopMove();
	}
	else if( EventName == sPauseAIMotion )
	{
		StopMove();
		PlayIdleAnimation( false );
		m_Paused = true;
	}
	else if( EventName == sUnpauseAIMotion )
	{
		m_Paused = false;
	}
	else if( EventName == sFollow )
	{
		// HACKHACK: This is mostly for Yog in the expansion. Normally, AIs should
		// invoke motion through their behavior trees (i.e., RodinBTNodeRosaMoveTo).

		STATIC_HASHED_STRING( FollowEntity );
		WBEntity* const	pFollowEntity				= Event.GetEntity( sFollowEntity );

		STATIC_HASHED_STRING( ReachedThresholdMin );
		const float		ReachedThresholdMin			= Event.GetFloat( sReachedThresholdMin );

		STATIC_HASHED_STRING( ReachedThresholdMax );
		const float		ReachedThresholdMax			= Event.GetFloat( sReachedThresholdMax );

		STATIC_HASHED_STRING( DeflectionRadius );
		const float		DeflectionRadius			= Event.GetFloat( sDeflectionRadius );

		STATIC_HASHED_STRING( PostDeflectionEndTime );
		const float		PostDeflectionEndTime		= Event.GetFloat( sPostDeflectionEndTime );

		STATIC_HASHED_STRING( FlyingDestinationOffsetZ );
		const float		FlyingDestinationOffsetZ	= Event.GetFloat( sFlyingDestinationOffsetZ );

		SetReachedThreshold( ReachedThresholdMin, ReachedThresholdMax );
		SetDeflectionRadius( DeflectionRadius );
		SetPostDeflectionEndTime( PostDeflectionEndTime );
		SetFlyingDestinationOffsetZ( FlyingDestinationOffsetZ );
		StartFollow( pFollowEntity, 0.0f /*approach distance*/, false /*use actual target location*/ );
	}
	else if( EventName == sSetStance )
	{
		STATIC_HASHED_STRING( Stance );
		SetStance( Event.GetHash( sStance ) );
	}
	else if( EventName == sOnSteppedUp )
	{
		STATIC_HASHED_STRING( StepHeight );
		const float StepHeight		= Event.GetFloat( sStepHeight );

		OnSteppedUp( StepHeight );
	}
}

/*virtual*/ void WBCompRosaAIMotion::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, CanOpenDoors,		m_CanOpenDoors );
	WB_SET_CONTEXT( Event, Bool, CanUnlockDoors,	m_CanUnlockDoors );
	WB_SET_CONTEXT( Event, Bool, CanBreakDoors,		m_CanBreakDoors );
}

/*virtual*/ void WBCompRosaAIMotion::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	// Only send this event when step-up Z has changed
	const float LastStepUpZ = m_StepUpZInterpolator.GetValue();
	m_StepUpZInterpolator.Tick( DeltaTime );
	if( LastStepUpZ != m_StepUpZInterpolator.GetValue() )
	{
		WB_MAKE_EVENT( SetTransientMeshOffsetZ, GetEntity() );
		WB_SET_AUTO( SetTransientMeshOffsetZ, Float, OffsetZ, m_StepUpZInterpolator.GetValue() );
		WB_DISPATCH_EVENT( GetEventManager(), SetTransientMeshOffsetZ, GetEntity() );
	}

	if( m_RepathOnNextTick )
	{
		m_RepathOnNextTick = false;
		Repath();
	}

	// NOTE: This also works for fleeing, which borrows a lot from following
	if( m_MotionStatus == EMS_Following && GetTime() > m_NextFollowValidateTime )
	{
		m_NextFollowValidateTime = GetTime() + m_FollowValidateTime;
		ValidateFollow();
	}

	TickMove();

	if( m_MotionStatus == EMS_TurningToFace )
	{
		TickTurn();
	}

	if( m_WaitingToPlayIdleAnim && !IsMoving() )
	{
		const float					CurrentTime	= GetTime();
		WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		if( CurrentTime > m_PlayIdleAnimTime || pTransform->GetVelocity().LengthSquared() < EPSILON )
		{
			m_WaitingToPlayIdleAnim = false;
			PlayIdleAnimation( false );
		}
	}
}

bool WBCompRosaAIMotion::GetNextStep( Vector& OutStep )
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector CurrentLocation	= pTransform->GetLocation();

	for( ; m_PathIndex < m_PathData.m_Path.Size(); ++m_PathIndex )
	{
		const Vector&	Step		= m_PathData.m_Path[ m_PathIndex ];

		const float		StepDistSq	= ( CurrentLocation - Step ).LengthSquared2D();
		if( StepDistSq < m_StepReachedThresholdSq )
		{
			// We're within a reasonable tolerance, go to the next step.
			continue;
		}

		// We're still on our way to this step.
		OutStep = Step;

		return true;
	}

	return false;
}

// Bastardized from TickMove. I should refactor to handle this case properly within that function.
void WBCompRosaAIMotion::TickTurn()
{
	ASSERT( m_MotionStatus == EMS_TurningToFace );

	WBEntity* const pEntity = GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector		MovementOffset		= m_LastDestination - pTransform->GetLocation();
	const Vector		MovementDirection	= MovementOffset.Get2D().GetNormalized();
	const Vector		TargetDirection		= m_UseTurnTargetDirection ? m_TurnTargetDirection : MovementDirection;
	const bool			NoTargetDirection	= TargetDirection.IsZero();

	if( TargetDirection.IsZero() )
	{
		// Nowhere to turn; we may be at the target location with no turn target direction set
		m_MotionStatus = EMS_MoveSucceeded;
		QueueIdleAnimation();
		return;
	}

	const Angles		CurrentOrientation	= pTransform->GetOrientation();
	const Vector		CurrentFacing		= CurrentOrientation.ToVector();
	static const Vector	Up					= Vector( 0.0f, 0.0f, 1.0f );
	const Vector		CurrentRight		= CurrentFacing.Cross( Up ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
	const bool			FacingTarget		= NoTargetDirection || TargetDirection.Dot( CurrentFacing ) > 0.0f;
	const float			CosTurnAngle		= TargetDirection.Dot( CurrentRight );
	const float			TurnEpsilon			= FacingTarget ? m_TurnReachedThreshold : 0.0f;
	const bool			NeedsRightTurn		= CosTurnAngle >= TurnEpsilon;
	const bool			NeedsLeftTurn		= CosTurnAngle <= -TurnEpsilon;

	if( NeedsLeftTurn || NeedsRightTurn )
	{
		const float		TurnYaw				= NeedsRightTurn ? -1.0f : ( NeedsLeftTurn ? 1.0f : 0.0f );
		const Angles	TurnVelocity		= Angles( 0.0f, 0.0f, TurnYaw * GetTurnSpeed() );
		pTransform->SetRotationalVelocity( TurnVelocity );
	}
	else
	{
		// We've finished turning to face. Movement is all done.
		m_MotionStatus = EMS_MoveSucceeded;
		QueueIdleAnimation();
	}
}

void WBCompRosaAIMotion::TickMove()
{
	m_IsFlying ? TickFlying() : TickWalking();
}

void WBCompRosaAIMotion::DoLocalAvoidance( const Vector& CurrentLocation, Vector& InOutMovementDirection )
{
	// Local avoidance: steer away from repulsors

	WBEntity* const pDestinationEntity	= m_LastDestinationEntity.Get();

	WBEntity* const pCurrentRepulsor	= m_CurrentRepulsor.Get();
	m_CurrentRepulsor = static_cast<WBEntity*>( NULL );

	const Array<WBCompRosaRepulsor*>* const pRepulsors = WBComponentArrays::GetComponents<WBCompRosaRepulsor>();
	if( !pRepulsors )
	{
		return;
	}

	WBEntity* const						pEntity				= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaCollision* const			pCollision			= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	const float							ExtentsRadius		= pCollision->GetExtents().Length2D();

	// Find nearest repulsor; we only avoid that one
	WBCompRosaRepulsor*					pNearestRepulsor	= NULL;
	float								NearestDistanceSq	= 0.0f;
	const Array<WBCompRosaRepulsor*>&	Repulsors			= *pRepulsors;
	FOR_EACH_ARRAY( RepulsorIter, Repulsors, WBCompRosaRepulsor* )
	{
		WBCompRosaRepulsor* const		pRepulsor			= RepulsorIter.GetValue();
		WBEntity* const					pRepulsorEntity		= pRepulsor->GetEntity();

		// Ignore inactive repulsors
		if( !pRepulsor->IsActive() )
		{
			continue;
		}

		// Don't avoid self
		if( pRepulsorEntity == pEntity )
		{
			continue;
		}

		// Don't avoid destination entity (unless we're fleeing them)
		if( !m_Flee && pRepulsorEntity == pDestinationEntity )
		{
			continue;
		}

		WBCompRosaTransform* const		pRepulsorTransform	= pRepulsorEntity->GetTransformComponent<WBCompRosaTransform>();
		const Vector					RepulsorLocation	= pRepulsorTransform->GetLocation();

		if( RepulsorLocation.z < CurrentLocation.z - pCollision->GetExtents().z ||
			RepulsorLocation.z > CurrentLocation.z + pCollision->GetExtents().z )
		{
			// Ignore repulsors outside our vertical extents
			continue;
		}

		const Vector					OffsetToRepulsor	= RepulsorLocation - CurrentLocation;
		const float						DistanceSq2D		= OffsetToRepulsor.LengthSquared2D();
		// DLP 13 Sept 2016: Adding entity's radius to the repulsor radius so we account for entity size in the overlap test
		const float						RepulsorRadiusSq	= Square( pRepulsor->GetRadius() + ExtentsRadius );

		// Only avoid within repulsor radius
		if( DistanceSq2D > RepulsorRadiusSq )
		{
			continue;
		}

		// DLP 13 Sept 2016: This used to test against current facing instead of
		// direction of movement, but this seems more correct and fixed a specific
		// case where AIs were getting stuck on coffins.
		// Only avoid if repulsor is on front side of movement
		if( OffsetToRepulsor.Dot( InOutMovementDirection ) <= 0.0f )
		{
			continue;
		}

		if( !pNearestRepulsor || DistanceSq2D < NearestDistanceSq )
		{
			pNearestRepulsor = pRepulsor;
		}
	}

	// Steer away from the nearest repulsor to the point on its radius orthogonal to our current direction of travel
	if( !pNearestRepulsor )
	{
		return;
	}

	WBEntity* const					pRepulsorEntity		= pNearestRepulsor->GetEntity();
	WBCompRosaTransform* const		pRepulsorTransform	= pRepulsorEntity->GetTransformComponent<WBCompRosaTransform>();
	const Vector					RepulsorLocation	= pRepulsorTransform->GetLocation();
	const Vector					RepulsorFacing		= pRepulsorTransform->GetOrientation().ToVector2D();
	const Vector					OffsetToRepulsor	= RepulsorLocation - CurrentLocation;
	// DLP 13 Sept 2016: Adding entity's radius to the repulsor radius so we account for entity size in the overlap test
	const float						RepulsorRadius		= pNearestRepulsor->GetRadius() + ExtentsRadius;

	static const Vector				kUp					= Vector( 0.0f, 0.0f, 1.0f );
	const Vector					MovementRight		= InOutMovementDirection.Cross( kUp ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem

	// If the repulsor is directed, steer toward its front
	// Else steer toward whichever side we're already moving toward
	const bool						SteerRight			= pNearestRepulsor->IsDirected() ?
															( MovementRight.Dot( RepulsorFacing ) >= 0.0f ) :
															( MovementRight.Dot( OffsetToRepulsor ) <= 0.0f );

	const float						SteerSign			= SteerRight ? 1.0f : -1.0f;
	const Vector					LocalDestination	= RepulsorLocation + ( MovementRight * RepulsorRadius * SteerSign );
	const Vector					LocalMovementOffset	= LocalDestination - CurrentLocation;

	// And update movement direction for motion
	InOutMovementDirection								= LocalMovementOffset.Get2D().GetNormalized();

	// Notify the AI it is being repulsed
	m_CurrentRepulsor = pRepulsorEntity;
	if( pRepulsorEntity != pCurrentRepulsor )
	{
		WB_MAKE_EVENT( OnRepulsed, NULL );
		WB_SET_AUTO( OnRepulsed, Entity, RepulsorEntity, pRepulsorEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnRepulsed, pEntity );
	}
}

bool WBCompRosaAIMotion::TeleportFinishMove()
{
	// Regardless of whether the teleport succeeds, this stops current movement.
	m_MotionStatus = EMS_MoveSucceeded;
	QueueIdleAnimation();

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	RosaWorld* const			pWorld		= GetWorld();
	DEVASSERT( pWorld );

	Vector						TeleportLoc	= m_LastDestination;

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pEntity;
	Info.m_In_UserFlags			= EECF_BlockerCollision;
	if( !pWorld->FindSpot( TeleportLoc, pCollision->GetExtents(), Info ) )
	{
		return false;
	}

	if( !pWorld->IsNavMeshUnder( TeleportLoc ) )
	{
		// Hopefully prevent falling out of the world if FindSpot pushed out of bounds!
		DEVWARNDESC( "WBCompRosaAIMotion::TeleportFinishMove: We prevented falling out of world!" );
		return false;
	}

	WB_MAKE_EVENT( OnPreTeleportMove, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnPreTeleportMove, pEntity );

	pTransform->SetAcceleration( Vector() );
	pTransform->SetVelocity( Vector() );
	pTransform->SetLocation( TeleportLoc );
	pTransform->SetRotationalVelocity( Angles() );
	if( m_UseTurnTargetDirection )
	{
		pTransform->SetOrientation( m_TurnTargetDirection.ToAngles() );
	}

	// This notifies the mesh to update its transform
	WB_MAKE_EVENT( OnTeleported, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnTeleported, pEntity );

	// This is the game script hook for VFX/SFX after mesh is updated
	WB_MAKE_EVENT( OnPostTeleportMove, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnPostTeleportMove, pEntity );

	return true;
}

void WBCompRosaAIMotion::OnSteppedUp( const float StepHeight )
{
	WBEntity* const			pEntity		= GetEntity();
	DEVASSERT( pEntity );

	const float NewViewOffsetZ	= m_StepUpZInterpolator.GetEndValue();
	const float OldViewOffsetZ	= NewViewOffsetZ - StepHeight;
	m_StepUpZInterpolator.Reset( Interpolator<float>::EIT_EaseOut, OldViewOffsetZ, NewViewOffsetZ, m_StepUpTime );
	{
		WB_MAKE_EVENT( SetTransientMeshOffsetZ, pEntity );
		WB_SET_AUTO( SetTransientMeshOffsetZ, Float, OffsetZ, m_StepUpZInterpolator.GetValue() );
		WB_DISPATCH_EVENT( GetEventManager(), SetTransientMeshOffsetZ, pEntity );
	}
}

void WBCompRosaAIMotion::TickWalking()
{
	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	pTransform->SetAcceleration( Vector() );
	pTransform->SetRotationalVelocity( Angles() );

	if( !IsLocomoting() )
	{
		return;
	}

	const Vector	CurrentLocation		= pTransform->GetLocation();
	const Vector	RemainingOffset		= CurrentLocation - m_LastDestination;
	const float		DistanceRemainingSq	= RemainingOffset.LengthSquared2D();
	const float		DistanceRemainingZ	= Abs( RemainingOffset.z );
	if( DistanceRemainingSq <= m_ReachedThresholdMinSq && DistanceRemainingZ <= pCollision->GetExtents().z )
	{
		// We're within our reached threshold, we're done!
		m_MotionStatus			= EMS_TurningToFace;
		return;
	}

	// Bit of a hack; if we're following, also check reached distance to actual current target location,
	// disregarding last known location. This should help to prevent running into the target (especially
	// for larger guys like Tarrare).
	WBEntity* const	pDestinationEntity	= m_LastDestinationEntity.Get();
	if( pDestinationEntity )
	{
		const Vector	ActualTargetLoc				= pDestinationEntity->GetTransformComponent<WBCompRosaTransform>()->GetLocation();
		const Vector	ActualRemainingOffset		= CurrentLocation - ActualTargetLoc;
		const float		ActualDistanceRemainingSq	= ActualRemainingOffset.LengthSquared2D();
		const float		ActualDistanceRemainingZ	= Abs( ActualRemainingOffset.z );
		if( ActualDistanceRemainingSq <= m_ReachedThresholdMinSq && ActualDistanceRemainingZ <= pCollision->GetExtents().z )
		{
			// We're within our reached threshold, we're done!
			m_MotionStatus			= EMS_TurningToFace;
			return;
		}
	}

	Vector NextStep;
	if( !GetNextStep( NextStep ) )
	{
		// Path has run out, we're done!
		m_MotionStatus			= EMS_TurningToFace;
		return;
	}

	const bool	Stuck		= ( CurrentLocation == m_StuckWatchdogLocation );
	m_StuckWatchdogLocation	= CurrentLocation;
	if( Stuck )
	{
		// DLP 13 Sept 2016: Instead of a random move, just pretend we're done and let the behavior fix it
		m_MotionStatus			= EMS_TurningToFace;
		return;
	}

	static const Vector	kUp					= Vector( 0.0f, 0.0f, 1.0f );
	const Angles		CurrentOrientation	= pTransform->GetOrientation();
	const Vector		CurrentFacing		= CurrentOrientation.ToVector();
	const Vector		CurrentRight		= CurrentFacing.Cross( kUp ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem

	const Vector		MovementOffset		= NextStep - CurrentLocation;
	Vector				MovementDirection	= MovementOffset.Get2D().GetNormalized();
	DoLocalAvoidance( CurrentLocation, MovementDirection );

	const float			AccelerationSize	= pCollision->IsRecentlyLanded( 0.0f ) ? GetLandAcceleration() : m_AirAcceleration;
	const Vector		Acceleration		= MovementDirection * AccelerationSize;

	const bool			FacingMovement		= MovementDirection.Dot( CurrentFacing ) > 0.0f;
	const float			CosTurnAngle		= MovementDirection.Dot( CurrentRight );
	const float			TurnEpsilon			= FacingMovement ? m_TurnReachedThreshold : 0.0f;
	const bool			NeedsRightTurn		= CosTurnAngle >= TurnEpsilon;
	const bool			NeedsLeftTurn		= CosTurnAngle <= -TurnEpsilon;
	const float			TurnYaw				= NeedsRightTurn ? -1.0f : ( NeedsLeftTurn ? 1.0f : 0.0f );
	const Angles		TurnVelocity		= Angles( 0.0f, 0.0f, TurnYaw * GetTurnSpeed() );

	pTransform->SetAcceleration( Acceleration );
	pTransform->SetRotationalVelocity( TurnVelocity );
}

void WBCompRosaAIMotion::TickFlying()
{
	WBEntity* const pEntity = GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision = WB_GETCOMP( pEntity, RosaCollision );

	pTransform->SetAcceleration( Vector() );
	pTransform->SetRotationalVelocity( Angles() );

	// HACK to fix pigeons; same as in TickWalking, not sure why this wasn't here before?
	if( !IsLocomoting() )
	{
		return;
	}

	if( m_Paused )
	{
		return;
	}

	const Vector	CurrentLocation		= pTransform->GetLocation();
	const Vector	RemainingOffset		= CurrentLocation - m_LastDestination;
	const float		DistanceRemainingSq	= RemainingOffset.LengthSquared2D();
	const float		DistanceRemainingZ	= Abs( RemainingOffset.z );
	if( DistanceRemainingSq <= m_ReachedThresholdMinSq && ( !pCollision || DistanceRemainingZ <= pCollision->GetExtents().z ) )
	{
		// We're within our reached threshold.
		// Signal that we're done, though we'll actually continue circling target.
		m_MotionStatus			= EMS_MoveSucceeded;
		QueueIdleAnimation();
	}

	// Bit of a hack; if we're following, also check reached distance to actual current target location,
	// disregarding last known location. This should help to prevent running into the target (especially
	// for larger guys like Tarrare).
	WBEntity* const	pDestinationEntity	= m_LastDestinationEntity.Get();
	if( pDestinationEntity )
	{
		const Vector	ActualTargetLoc				= pDestinationEntity->GetTransformComponent<WBCompRosaTransform>()->GetLocation();
		const Vector	ActualRemainingOffset		= CurrentLocation - ActualTargetLoc;
		const float		ActualDistanceRemainingSq	= ActualRemainingOffset.LengthSquared2D();
		const float		ActualDistanceRemainingZ	= Abs( ActualRemainingOffset.z );
		if( ActualDistanceRemainingSq <= m_ReachedThresholdMinSq && ( !pCollision || ActualDistanceRemainingZ <= pCollision->GetExtents().z ) )
		{
			// Signal that we're done, though we'll actually continue circling target.
			m_MotionStatus			= EMS_MoveSucceeded;
			QueueIdleAnimation();
		}
	}

	Vector NextStep = m_LastDestination;
	if( IsLocomoting() )
	{
		GetNextStep( NextStep );
	}

	Vector				CombinedAcceleration;

	// Compensate for current velocity
	static const float	kCompensateTime			= 0.5f;
	static const float	kCompensateReciprocal	= 1.0f / kCompensateTime;
	const Vector		CompensateAcceleration	= -pTransform->GetVelocity() * kCompensateReciprocal;
	CombinedAcceleration						+= CompensateAcceleration;

	const Vector		MovementOffset			= NextStep - CurrentLocation;
	const Vector		TargetDirection			= MovementOffset.GetNormalized();
	const bool			NoTargetDirection		= TargetDirection.IsZero();

	// Figure out which way we need to turned
	static const Vector	kUp						= Vector( 0.0f, 0.0f, 1.0f );
	const Angles		CurrentOrientation		= pTransform->GetOrientation();
	const Vector		CurrentFacing			= CurrentOrientation.ToVector();
	const Vector		CurrentRight			= CurrentFacing.Cross( kUp ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
	const bool			FacingTarget			= NoTargetDirection || TargetDirection.Dot( CurrentFacing ) > 0.0f;
	const float			CosTurnAngle			= TargetDirection.Dot( CurrentRight );
	const float			TurnEpsilon				= FacingTarget ? m_TurnReachedThreshold : 0.0f;
	const bool			NeedsRightTurn			= CosTurnAngle >= TurnEpsilon;
	const bool			NeedsLeftTurn			= CosTurnAngle <= -TurnEpsilon;

	// Add deflection acceleration to circle the target, if desired
	bool Deflecting = false;
	if( m_MotionStatus == EMS_Following )
	{
		if( DistanceRemainingSq < m_FlyingDeflectionRadiusSq )
		{
			if( m_PostDeflectionEndTime > 0.0f )
			{
				const float CurrentTime = GetTime();
				if( m_DeflectionEndTime == 0.0f )
				{
					m_DeflectionEndTime = CurrentTime + m_PostDeflectionEndTime;
				}
				else if( CurrentTime > m_DeflectionEndTime )
				{
					// Finish move if we've been constantly deflecting for m_PostDeflectionEndTime
					m_MotionStatus = EMS_MoveSucceeded;
					QueueIdleAnimation();
				}
			}

			Deflecting								= true;
		}
		else
		{
			m_DeflectionEndTime = 0.0f;
		}
	}

	if( Deflecting )
	{
		const Vector	TargetRight				= TargetDirection.Cross( kUp ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
		const bool		DeflectRight			= CosTurnAngle <= 0.0f;
		const Vector	DeflectionNormal		= DeflectRight ? TargetRight : -TargetRight;
		const Vector	DeflectionAcceleration	= DeflectionNormal * m_AirAcceleration;
		CombinedAcceleration					+= DeflectionAcceleration;
	}
	else
	{
		// Accelerate toward destination
		const Vector	PrimaryAcceleration		= TargetDirection * m_AirAcceleration;
		CombinedAcceleration					+= PrimaryAcceleration;
	}

	// Combine all accelerations
	Vector				CombinedNormal;
	float				CombinedSize;
	float				CombinedSizeReciprocal;
	CombinedAcceleration.GetNormalized( CombinedNormal, CombinedSize, CombinedSizeReciprocal );
	const float			AccelerationSize		= Min( CombinedSize, m_AirAcceleration );
	const Vector		Acceleration			= CombinedNormal * AccelerationSize;

	const float			TurnYaw					= NeedsRightTurn ? -1.0f : ( NeedsLeftTurn ? 1.0f : 0.0f );
	const Angles		TurnVelocity			= Angles( 0.0f, 0.0f, TurnYaw * GetTurnSpeed() );

	pTransform->SetAcceleration( Acceleration );
	pTransform->SetRotationalVelocity( TurnVelocity );
}

float WBCompRosaAIMotion::GetLandAcceleration() const
{
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );

	WB_MODIFY_FLOAT_SAFE( LandAcceleration, m_LandAcceleration, pStatMod );
	return WB_MODDED( LandAcceleration );
}

float WBCompRosaAIMotion::GetTurnSpeed() const
{
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );

	WB_MODIFY_FLOAT_SAFE( TurnSpeed, m_TurnSpeed, pStatMod );
	return WB_MODDED( TurnSpeed );
}

float WBCompRosaAIMotion::GetMotionAnimationPlayRate() const
{
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );

	WB_MODIFY_FLOAT_SAFE( WalkAnimRate, m_WalkAnimRateBase, pStatMod );
	return WB_MODDED( WalkAnimRate );
}

void WBCompRosaAIMotion::ValidateFollow()
{
	WBEntity* const pDestinationEntity = m_LastDestinationEntity();

	if( !pDestinationEntity )
	{
		// We no longer have a target to follow.
		m_MotionStatus			= EMS_MoveFailed;
		QueueIdleAnimation();
		return;
	}

	WBCompRosaHealth* const pHealth = WB_GETCOMP( pDestinationEntity, RosaHealth );
	if( pHealth && pHealth->IsDead() )
	{
		WBEntity* const							pEntity			= GetEntity();
		const RosaFactions::EFactionCon			Con				= WBCompRosaFaction::GetCon( pEntity, pDestinationEntity );
		WBCompRodinKnowledge* const				pKnowledge		= WB_GETCOMP( GetEntity(), RodinKnowledge );
		WBCompRodinKnowledge::TKnowledge* const	pKnowledgeEntry	= pKnowledge->GetKnowledge( pDestinationEntity );
		STATIC_HASHED_STRING( RegardAsHostile );
		const bool								RegardAsHostile	= pKnowledgeEntry && pKnowledgeEntry->GetBool( sRegardAsHostile );

		if( Con == RosaFactions::EFR_Hostile ||
			( RegardAsHostile && Con == RosaFactions::EFR_Neutral ) )
		{
			// Target is an enemy that has died. Stop following.
			m_MotionStatus			= EMS_MoveFailed;
			QueueIdleAnimation();
			return;
		}
		else
		{
			// Target is a body search target, keep going
		}
	}

	Vector ActualDestination;
	if( !GetLocationFor( pDestinationEntity, ActualDestination ) )
	{
		// We no longer have a last known location for this target.
		m_MotionStatus			= EMS_MoveFailed;
		QueueIdleAnimation();
		return;
	}

	// HACKHACK: For drones, add an offset as needed
	ActualDestination.z += m_FlyingDestinationOffsetZ;

	uint NavNodeIndex;
	if( GetWorld()->FindNavNodeUnder( ActualDestination, NavNodeIndex ) )
	{
		if( NavNodeIndex == m_LastDestinationIndex )
		{
			// We're still on track.
			return;
		}
	}

	// Repath because the follow target has moved.
	RepathFollow();
}

void WBCompRosaAIMotion::SetTurnTarget( const bool UseTurnTarget, const Angles& TurnTarget )
{
	m_UseTurnTargetDirection	= UseTurnTarget;
	m_TurnTargetDirection		= TurnTarget.ToVector();
}

void WBCompRosaAIMotion::SetReachedThreshold( const float ReachedThresholdMin, const float ReachedThresholdMax )
{
	m_ReachedThresholdMinSq		= Square( ReachedThresholdMin );
	m_ReachedThresholdMaxSq		= Square( ReachedThresholdMax );
}

void WBCompRosaAIMotion::UnsetTether()
{
	m_UseTether = false;
}

void WBCompRosaAIMotion::SetTether( const Vector& TetherLocation, const float TetherDistance, const float TetherDistanceZ )
{
	m_UseTether			= true;
	m_TetherLocation	= TetherLocation;
	m_TetherDistance	= TetherDistance;
	m_TetherDistanceZ	= TetherDistanceZ;
}

void WBCompRosaAIMotion::SetDeflectionRadius( const float DeflectionRadius )
{
	m_FlyingDeflectionRadiusSq	= Square( DeflectionRadius );
}

void WBCompRosaAIMotion::StartMove( const Vector& Destination, const float ApproachDistance )
{
	m_MotionStatus			= EMS_MoveFailed;
	m_Wander				= false;
	m_Flee					= false;
	m_ApproachDistance		= ApproachDistance;
	m_StuckWatchdogLocation	= Vector();

	InternalStartMove( Destination, NULL );
}

void WBCompRosaAIMotion::StartTurn( const Vector& TurnTarget )
{
	m_MotionStatus			= EMS_TurningToFace;
	m_LastDestination		= TurnTarget;
	GetWorld()->FindNavNodeUnder( TurnTarget, m_LastDestinationIndex );
	m_LastDestinationEntity	= static_cast<WBEntity*>( NULL );
	m_CurrentRepulsor		= static_cast<WBEntity*>( NULL );

	PlayMotionAnimation();
}

void WBCompRosaAIMotion::StartFollow( const WBEntity* const pDestinationEntity, const float ApproachDistance, const bool UseActualTargetLocation )
{
	m_MotionStatus				= EMS_MoveFailed;
	m_Wander					= false;
	m_Flee						= false;
	m_ApproachDistance			= ApproachDistance;
	m_UseActualTargetLocation	= UseActualTargetLocation;
	m_StuckWatchdogLocation		= Vector();

	if( !pDestinationEntity )
	{
		return;
	}

	InternalStartMove( Vector(), pDestinationEntity );
}

void WBCompRosaAIMotion::StartWander( const float WanderTargetDistance )
{
	m_MotionStatus			= EMS_MoveFailed;
	m_Flee					= false;
	m_Wander				= true;
	m_WanderTargetDistance	= WanderTargetDistance;
	m_StuckWatchdogLocation	= Vector();

	InternalStartMove( Vector(), NULL );
}

void WBCompRosaAIMotion::StartFlee( const WBEntity* const pFleeEntity, const float FleeTargetDistance )
{
	m_MotionStatus			= EMS_MoveFailed;
	m_Wander				= false;
	m_Flee					= true;
	m_FleeTargetDistance	= FleeTargetDistance;
	m_StuckWatchdogLocation	= Vector();

	InternalStartMove( Vector(), pFleeEntity );
}

void WBCompRosaAIMotion::RepathFollow()
{
	m_MotionStatus = EMS_MoveFailed;

	WBEntity* const pDestinationEntity = m_LastDestinationEntity();
	if( !pDestinationEntity )
	{
		return;
	}

	InternalStartMove( Vector(), pDestinationEntity );
}

// Simply repath with the same inputs as the previous request.
void WBCompRosaAIMotion::Repath()
{
	m_MotionStatus = EMS_MoveFailed;

	InternalStartMove( m_LastDestination, m_LastDestinationEntity() );
}

bool WBCompRosaAIMotion::GetLocationFor( const WBEntity* const pFollowEntity, Vector& OutLocation ) const
{
	if( m_UseActualTargetLocation )
	{
		return GetActualLocationFor( pFollowEntity, OutLocation );
	}
	else
	{
		return GetLastKnownLocationFor( pFollowEntity, OutLocation );
	}
}

bool WBCompRosaAIMotion::GetLastKnownLocationFor( const WBEntity* const pFollowEntity, Vector& OutLocation ) const
{
	DEVASSERT( pFollowEntity );

	WBCompRodinKnowledge* const pKnowledge = WB_GETCOMP( GetEntity(), RodinKnowledge );
	if( pKnowledge )
	{
		return pKnowledge->GetLastKnownLocationFor( pFollowEntity, OutLocation );
	}
	else
	{
		// HACKHACK: For Yog, who doesn't need to actually acquire knowledge of the player.
		return GetActualLocationFor( pFollowEntity, OutLocation );
	}
}

bool WBCompRosaAIMotion::GetActualLocationFor( const WBEntity* const pFollowEntity, Vector& OutLocation ) const
{
	DEVASSERT( pFollowEntity );

	WBCompRosaTransform* const pFollowTransform = pFollowEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pFollowTransform );

	OutLocation = pFollowTransform->GetLocation();
	return true;
}

void WBCompRosaAIMotion::InternalStartMove( const Vector& Destination, const WBEntity* const pDestinationEntity )
{
	// Just make sure we're calling this from a clean slate.
	ASSERT( m_MotionStatus == EMS_MoveFailed );

	if( !m_CanMove )
	{
		m_MotionStatus = EMS_MoveSucceeded;
		QueueIdleAnimation();
		return;
	}

	RosaWorld* const			pWorld				= GetWorld();
	ASSERT( pWorld );

	RosaNav* const				pNav				= RosaNav::GetInstance();
	ASSERT( pNav );

	WBEntity* const				pEntity				= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform			= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision			= WB_GETCOMP( pEntity, RosaCollision );
	const bool					BypassPathing		= ( m_IsFlying && pCollision == NULL );
	ASSERT( pCollision || BypassPathing );

	const Vector				CurrentLocation		= pTransform->GetLocation();
	Vector						ActualDestination	= Destination;
	if( pDestinationEntity )
	{
		if( GetLocationFor( pDestinationEntity, ActualDestination ) )
		{
			// We're good.
		}
		else
		{
			// No last known position for this entity.
			// Call this a success. It's not really, but it's not a pathing failure; it's more like
			// the request wasn't valid in the first place. This prevents stalling in BT loops that
			// wait for the move to be successful.
			m_MotionStatus = EMS_MoveSucceeded;
			QueueIdleAnimation();
			return;
		}
	}

	// ROSAHACK: For drones, add an offset as needed
	ActualDestination.z += m_FlyingDestinationOffsetZ;

	if( !m_Wander )
	{
		// If we're already at our destination, exit early.
		const float DistanceRemainingSq = ( CurrentLocation - ActualDestination ).LengthSquared();
		if( DistanceRemainingSq <= m_ReachedThresholdMaxSq )
		{
			// We're within our reached threshold, we're done!
			m_MotionStatus			= EMS_TurningToFace;
			m_LastDestination		= ActualDestination;
			GetWorld()->FindNavNodeUnder( ActualDestination, m_LastDestinationIndex );
			m_LastDestinationEntity	= pDestinationEntity;
			m_CurrentRepulsor		= static_cast<WBEntity*>( NULL );
			return;
		}
	}

	const bool	IsFollowing	= ( pDestinationEntity != NULL );
	Vector		ActualStart	= CurrentLocation;

	if( !m_IsFlying )
	{
		// Get endpoints on ground.
		CollisionInfo Info;
		Info.m_In_CollideWorld		= true;
		Info.m_In_CollideEntities	= true;
		Info.m_In_CollidingEntity	= pEntity;
		Info.m_In_UserFlags			= EECF_EntityCollision;

		const float			TraceDistance			= pWorld->GetRayTraceLength();
		const Vector		Extents					= pCollision->GetExtents();

		// ROSATODO: Do I need these sweeps for navmesh?

		// Sweep down to get actual start.
		const Segment		SweepStartSegment		= Segment( CurrentLocation, CurrentLocation + Vector( 0.0f, 0.0f, -TraceDistance ) );
		pWorld->Sweep( SweepStartSegment, Extents, Info );
		if( Info.m_Out_Collision )
		{
			ActualStart								= Info.m_Out_Intersection;
			ActualStart.z							+= 0.5f - Extents.z;	// Start in the center of the voxel touching the ground; fixes tall AIs pathing funny
		}

		// Sweep down to get actual destination.
		// Don't assume extents actually fits at ActualDestination yet!
		const Segment		SweepDestinationSegment	= Segment( ActualDestination, ActualDestination + Vector( 0.0f, 0.0f, -TraceDistance ) );
		pWorld->Trace( SweepDestinationSegment, Info );
		if( Info.m_Out_Collision )
		{
			ActualDestination						= Info.m_Out_Intersection;
			ActualDestination.z						+= + 0.5f;
		}
	}

	// Do pathfinding and smoothing
	if( BypassPathing )
	{
		m_PathData.m_Path.Clear();
		m_PathData.m_Path.PushBack( ActualStart );
		m_PathData.m_Path.PushBack( ActualDestination );
	}
	else
	{
		m_PathData.m_Params.m_Entity			= pEntity;
		m_PathData.m_Params.m_Start				= ActualStart;

		m_PathData.m_Params.m_UseTether			= m_UseTether;
		m_PathData.m_Params.m_TetherLocation	= m_TetherLocation;
		m_PathData.m_Params.m_TetherDistanceSq	= Square( m_TetherDistance );
		m_PathData.m_Params.m_TetherDistanceZ	= m_TetherDistanceZ;

		if( m_Wander )
		{
			m_PathData.m_Params.m_TargetDistance	= m_WanderTargetDistance;
		}
		else if( m_Flee )
		{
			m_PathData.m_Params.m_TargetDistance	= m_FleeTargetDistance;
			m_PathData.m_Params.m_Destination		= ActualDestination;
		}
		else
		{
			m_PathData.m_Params.m_TargetDistance	= m_ApproachDistance;
			m_PathData.m_Params.m_Destination		= ActualDestination;
		}

		m_PathData.m_Params.m_PathMode			= m_Wander ? RosaNav::EPM_Wander : ( m_Flee ? RosaNav::EPM_Flee : RosaNav::EPM_Search );
		const float StepReachedThreshold		= SqRt( m_StepReachedThresholdSq );
		// Add a little bit for extra headroom
		m_PathData.m_Params.m_AgentHeight		= ( pCollision->GetExtents().z * 2.0f ) + StepReachedThreshold;
		// Add a little bit for extra elbow room, and account for box shape on diagonals
		// (Multiplying by 2 would be more appropriate, because that additionally accounts
		// for rounding corners on the navmesh instead of cutting straight across; but
		// that also makes it really hard for anything to fit through 1m passages.)
		// Update for Rosa: this currently allows 2 ordinary sized agents to fit side by
		// side through a door, so... it's fine? And the addition of StepReachedThreshold
		// helps them stop running into corners due to turning early.
		m_PathData.m_Params.m_AgentRadius		= ( pCollision->GetExtents().x * 1.414f ) + StepReachedThreshold;
		m_PathData.m_Params.m_MotionType		= m_IsFlying ? RosaNav::EMT_Flying : RosaNav::EMT_Walking;
		m_PathData.m_Params.m_CanOpenDoors		= m_CanOpenDoors;
		m_PathData.m_Params.m_CanUnlockDoors	= m_CanUnlockDoors;
		m_PathData.m_Params.m_CanBreakDoors		= m_CanBreakDoors;
		m_PathData.m_Params.m_Cautious			= m_Cautious;
		m_PathData.m_Params.m_MaxSteps			= m_MaxPathSteps;
		m_PathData.m_Params.m_UsePartialPath	= true;

#if BUILD_DEV
		m_PathData.m_Params.m_WarnMaxSteps	= m_WarnMaxSteps;
#endif

		const RosaNav::EPathStatus Status = pNav->FindPath( m_PathData );
		if( Status == RosaNav::EPS_NoPathFound )
		{
			DEBUGPRINTF( "WBCompRosaAIMotion::InternalStartMove: No path found, motion will fail.\n" );
			return;
		}

		// If we're already at our (path found) destination, exit early.
		const float DistanceRemainingSq = ( CurrentLocation - m_PathData.m_Path.Last() ).LengthSquared();
		if( DistanceRemainingSq <= m_ReachedThresholdMaxSq )
		{
			// We're within our reached threshold, we're done!
			m_MotionStatus			= EMS_TurningToFace;
			m_LastDestination		= ActualDestination;
			GetWorld()->FindNavNodeUnder( ActualDestination, m_LastDestinationIndex );
			m_LastDestinationEntity	= pDestinationEntity;
			m_CurrentRepulsor		= static_cast<WBEntity*>( NULL );
			return;
		}
	}

	// Cache the bound of our path so we can easily invalidate the path when the world changes.
	if( pCollision )
	{
		DEVASSERT( !m_PathData.m_Path.Empty() );
		const uint PathSize = m_PathData.m_Path.Size();
		m_PathBound = AABB( m_PathData.m_Path[0], m_PathData.m_Path[0] );
		for( uint PathIndex = 1; PathIndex < PathSize; ++PathIndex )
		{
			m_PathBound.ExpandTo( m_PathData.m_Path[ PathIndex ] );
		}
		m_PathBound.ExpandBy( pCollision->GetExtents() );	// Expand by collision extents so we get all possible changes.
		m_PathBound.m_Min.z -= 1.0f;						// Also expand down by one unit so we detect changes to the floor.
	}

	// Set parameters and do side effects
	{
		m_WaitingToPlayIdleAnim		= false;
		m_MotionStatus				= IsFollowing ? EMS_Following : EMS_Moving;
		m_PathIndex					= 1;	// The first node is the start; no need to move to it.
		m_LastDestination			= m_PathData.m_Path.Last();
		GetWorld()->FindNavNodeUnder( m_LastDestination, m_LastDestinationIndex );
		m_LastDestinationEntity		= pDestinationEntity;
		m_CurrentRepulsor			= static_cast<WBEntity*>( NULL );
		m_NextFollowValidateTime	= GetTime() + m_FollowValidateTime;
		m_DeflectionEndTime			= 0.0f;

		PlayMotionAnimation();
	}
}

void WBCompRosaAIMotion::PlayAnimation( const HashedString& AnimationName, const bool Loop, const bool IgnoreIfAlreadyPlaying, const float PlayRate, const bool NoBlend )
{
	if( m_Paused )
	{
		return;
	}

	DEVASSERT( AnimationName != HashedString::NullString );

	WBCompRodinResourceMap* const pResourceMap = WB_GETCOMP( GetEntity(), RodinResourceMap );
	if( pResourceMap && pResourceMap->ClaimResource( this, m_AnimationResource, false ) )
	{
		m_HasAnimationResource = true;

		WB_MAKE_EVENT( PlayAnim, GetEntity() );
		WB_SET_AUTO( PlayAnim, Hash, AnimationName, AnimationName );
		WB_SET_AUTO( PlayAnim, Bool, Loop, Loop );
		WB_SET_AUTO( PlayAnim, Bool, IgnoreIfAlreadyPlaying, IgnoreIfAlreadyPlaying );
		WB_SET_AUTO( PlayAnim, Float, PlayRate, PlayRate );
		WB_SET_AUTO( PlayAnim, Float, BlendTime, NoBlend ? -1.0f : 0.0f );
		WB_DISPATCH_EVENT( GetEventManager(), PlayAnim, GetEntity() );
	}
}

/*virtual*/ bool WBCompRosaAIMotion::OnResourceStolen( const HashedString& Resource )
{
	Unused( Resource );
	DEVASSERT( Resource == m_AnimationResource );

	m_HasAnimationResource = false;

	// DLP 5 Dec 2021: I feel like this should probably also stop movement,
	// but there's stuff with the way the BTs use the Anim resources that
	// I'd have to revisit to make it work right, and there could be cases
	// where I *don't* want to stop, like during a dodge that continues
	// moving toward a target. So I'm leaving this alone for now, but also
	// leaving this reminder that it deserves another look. (The problem
	// I was seeing was foot sliding when a moving AI used a ranged attack,
	// but I've switched to layered upper-body animations now to fix that.)

	return true;
}

/*virtual*/ void WBCompRosaAIMotion::OnResourceReturned( const HashedString& Resource )
{
	Unused( Resource );
	DEVASSERT( Resource == m_AnimationResource );

	m_HasAnimationResource = true;

	if( IsLocomoting() )
	{
		PlayMotionAnimation();
	}
	else
	{
		PlayIdleAnimation( false );
	}
}

void WBCompRosaAIMotion::QueueIdleAnimation()
{
	if( !m_WaitingToPlayIdleAnim )
	{
		m_WaitingToPlayIdleAnim	= true;
		m_PlayIdleAnimTime		= GetTime() + 0.1f;	// HACKHACK: Hard-coded wait before playing the idle anim (i.e., after stopping motion)
	}
}

void WBCompRosaAIMotion::StopMove()
{
	m_MotionStatus = EMS_Still;

	// ROSANOTE: Don't play idle animation immediately, else the motion animation
	// can start/stop unnaturally when moving at reached threshold
	QueueIdleAnimation();
}

void WBCompRosaAIMotion::PlayIdleAnimation( const bool NoBlend )
{
	PlayAnimation( m_IdleAnimationName, true, true, GetIdleAnimationPlayRate(), NoBlend );
}

void WBCompRosaAIMotion::PlayMotionAnimation()
{
	PlayAnimation( GetMotionStanceAnimationName(), true, true, GetMotionAnimationPlayRate(), false );
}

const HashedString&	WBCompRosaAIMotion::GetMotionStanceAnimationName() const
{
	Map<HashedString, HashedString>::Iterator StanceIter = m_StanceAnimationNames.Search( m_MotionStance );
	if( StanceIter.IsValid() )
	{
		return StanceIter.GetValue();
	}
	else
	{
		// Fall back to a scaled walk animation if requested stance isn't defined
		return m_WalkAnimationName;
	}
}

#if BUILD_DEV
/*static*/ SimpleString WBCompRosaAIMotion::GetStatusFromEnum( const EMotionStatus Status )
{
	switch( Status )
	{
	case EMS_Still:			return "EMS_Still";
	case EMS_Moving:		return "EMS_Moving";
	case EMS_Following:		return "EMS_Following";
	case EMS_TurningToFace:	return "EMS_TurningToFace";
	case EMS_MoveSucceeded:	return "EMS_MoveSucceeded";
	case EMS_MoveFailed:	return "EMS_MoveFailed";
	default:				return "Unknown";
	}
}
/*virtual*/ void WBCompRosaAIMotion::Report() const
{
	Super::Report();

	PRINTF( WBPROPERTY_REPORT_PREFIX "Status:   %s\n", GetStatusFromEnum( m_MotionStatus ).CStr() );
	PRINTF( WBPROPERTY_REPORT_PREFIX "Stance:   %s\n", ReverseHash::ReversedHash( m_MotionStance ).CStr() );
	PRINTF( WBPROPERTY_REPORT_PREFIX "  Anim:   %s\n", ReverseHash::ReversedHash( GetMotionStanceAnimationName() ).CStr() );
	PRINTF( WBPROPERTY_REPORT_PREFIX "AnimRate: %f\n", GetMotionAnimationPlayRate() );
}

/*virtual*/ void WBCompRosaAIMotion::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const		pFramework	= GetFramework();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();

	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector				Location	= pTransform->GetLocation();

	pRenderer->DEBUGPrint( SimpleString::PrintF( "%sStatus: %s",		DebugRenderLineFeed().CStr(),	GetStatusFromEnum( m_MotionStatus ).CStr() ),							Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 128, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%sStance: %s",		DebugRenderLineFeed().CStr(),	ReverseHash::ReversedHash( m_MotionStance ).CStr() ),					Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 128, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%sAnim: %s\n",		DebugRenderLineFeed().CStr(),	ReverseHash::ReversedHash( GetMotionStanceAnimationName() ).CStr() ),	Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 128, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%sAnimRate: %f\n",	DebugRenderLineFeed().CStr(),	GetMotionAnimationPlayRate() ),											Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 128, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );

	if( !IsMoving() )
	{
		return;
	}

	const uint NumPathSteps = m_PathData.m_Path.Size();
	for( uint PathIndex = 1; PathIndex < NumPathSteps; ++PathIndex )
	{
		const Vector& PrevStep = m_PathData.m_Path[ PathIndex - 1 ];
		const Vector& NextStep = m_PathData.m_Path[ PathIndex ];

		pRenderer->DEBUGDrawLine( PrevStep, NextStep, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
	}

	if( m_PathIndex < NumPathSteps )
	{
		pRenderer->DEBUGDrawCross( m_PathData.m_Path[ m_PathIndex ], 0.5f, ARGB_TO_COLOR( 255, 255, 128, 0 ) );
	}
}
#endif

#define VERSION_EMPTY				0
#define VERSION_PAUSED				1
#define VERSION_STANCE				2
#define VERSION_WALKANIMRATEBASE	3
#define VERSION_IDLEANIMRATEBASE	4
#define VERSION_IDLEANIMATIONNAME	5
#define VERSION_CURRENT				5

uint WBCompRosaAIMotion::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += 1;						// m_Paused
	Size += sizeof( HashedString );	// m_MotionStance

	Size += 4;						// m_WalkAnimRateBase

	Size += 4;						// m_IdleAnimRateBase

	Size += sizeof( HashedString );	// m_IdleAnimationName

	return Size;
}

void WBCompRosaAIMotion::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Paused );

	Stream.WriteHashedString( m_MotionStance );

	Stream.WriteFloat( m_WalkAnimRateBase );

	Stream.WriteFloat( m_IdleAnimRateBase );

	Stream.WriteHashedString( m_IdleAnimationName );
}

void WBCompRosaAIMotion::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_PAUSED )
	{
		m_Paused = Stream.ReadBool();
	}

	if( Version >= VERSION_STANCE )
	{
		m_MotionStance = Stream.ReadHashedString();
	}

	if( Version >= VERSION_WALKANIMRATEBASE )
	{
		m_WalkAnimRateBase = Stream.ReadFloat();
	}

	if( Version >= VERSION_IDLEANIMRATEBASE )
	{
		m_IdleAnimRateBase = Stream.ReadFloat();
	}

	if( Version >= VERSION_IDLEANIMATIONNAME )
	{
		m_IdleAnimationName = Stream.ReadHashedString();
	}
}
