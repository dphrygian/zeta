#include "core.h"
#include "wbcomprosaplayer.h"
#include "keyboard.h"
#include "rosaworld.h"
#include "rosaframework.h"
#include "wbeventmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosacamera.h"
#include "wbcomprosafootsteps.h"
#include "wbcomprosahealth.h"
#include "wbcomprosainventory.h"
#include "wbcomprosaweapon.h"
#include "wbcomprosafrobber.h"
#include "wbcomprosafrobbable.h"
#include "wbcomprosamesh.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcomplabel.h"
#include "configmanager.h"
#include "idatastream.h"
#include "ray.h"
#include "collisioninfo.h"
#include "wbscene.h"
#include "inputsystem.h"
#include "configmanager.h"
#include "mathcore.h"
#include "rosagame.h"
#include "rosasaveload.h"
#include "reversehash.h"
#include "matrix.h"
#include "irenderer.h"
#include "Common/uimanagercommon.h"
#include "noise.h"
#include "rosadifficulty.h"
#include "rosacampaign.h"

#if BUILD_DEV
#if BUILD_WINDOWS
#include "console.h"
#endif
#include "wbcomprosainventory.h"
#include "wbcomprosarope.h"
#include "wbcomprosavisible.h"
#include "wbcomprosawallet.h"
#include "wbcomprosakeyring.h"
#include "wbcomprosalinkedentities.h"
#include "wbcomponentarrays.h"
#include "wbcomprosafaction.h"
#include "fontmanager.h"
#endif

#define CAMERA_RELATIVE_CLIMBING		1
#define CAMERA_RELATIVE_CLIMBING_BIAS	1

WBCompRosaPlayer::WBCompRosaPlayer()
:	m_DeferMusic( false )
,	m_InitialMusicTrackBits( 0 )
,	m_RollingAutosaveDelay( 0.0f )
,	m_RetryAutosaveDelay( 0.0f )
,	m_AutosaveSuppRefsSerialized( 0 )
,	m_AutosaveSuppRefsTransient( 0 )
,	m_UnlandedForceFootstepWindow( 0.0f )
,	m_UnlandedJumpWindow( 0.0f )
,	m_UnlandedLeanWindow( 0.0f )
,	m_LandAcceleration( 0.0f )
,	m_AirAcceleration( 0.0f )
,	m_TurnSpeed( 0.0f )
,	m_JumpHeight( 0.0f )
,	m_BackpedalScalar( 0.0f )
,	m_UncrouchOnSprint( false )
,	m_IsCrouched( false )
,	m_IsUncrouching( false )
,	m_IsForceCrouched( false )
,	m_StandExtentsZ( 0.0f )
,	m_CrouchExtentsZ( 0.0f )
,	m_StandViewOffsetZ( 0.0f )
,	m_CrouchViewOffsetZ( 0.0f )
,	m_CrouchViewInterpTime( 0.0f )
,	m_ViewOffsetZInterpolator()
,	m_PowerSlideRollInterpolator()
,	m_StepUpZInterpolator()
,	m_ViewBobOffsetInterpolator()
,	m_ViewBobAngleOffsetInterpolator()
,	m_ViewSwayOffsetInterpolator()
,	m_ViewSwayAngleOffsetInterpolator()
,	m_ViewHandsAcceleration()
,	m_ViewHandsVelocity()
,	m_ViewHandsRotationalAcceleration()
,	m_ViewHandsRotationalVelocity()
,	m_KickSpringK( 0.0f )
,	m_KickDamperC( 0.0f )
,	m_KickVelocity()
,	m_KickPosition()
,	m_ViewHandsSpringK( 0.0f )
,	m_ViewHandsDamperC( 0.0f )
,	m_PushToSprint( false )
,	m_IsSprinting( false )
,	m_IsSprintingWithMovement( false )
,	m_SprintStartTime( 0.0f )
,	m_SprintFOVScale( 0.0f )
,	m_SprintFOVTime( 0.0f )
,	m_DoubleJumpHeight( 0.0f )
,	m_HasDoubleJumped( false )
,	m_PowerJumpRatio( 0.0f )
,	m_IsPowerSliding( false )
,	m_PowerSlideDuration( 0.0f )
,	m_PowerSlideEndTime( 0.0f )
,	m_PowerSlideY()
,	m_PowerSlideInputContext()
,	m_PowerSlideReqVelocitySq( 0.0f )
,	m_PowerSlideRoll( 0.0f )
,	m_PowerSlideRollInterpTime( 0.0f )
,	m_IsDragging( false )
,	m_DragInputContext()
,	m_DragDropOffset()
,	m_DragDropOrientation()
,	m_DragDropSpawnImpulse( 0.0f )
,	m_DragDropSpawnImpulseZ( 0.0f )
,	m_DraggedEntity()
,	m_ClimbRefs( 0 )
,	m_IsClimbing( false )
,	m_ClimbInputContext()
,	m_ClimbOffImpulse( 0.0f )
,	m_ClimbFacingBiasAngle( 0.0f )
,	m_ClimbFacingBiasScale( 0.0f )
,	m_IsMantling( false )
,	m_MantleInputContext()
,	m_MantleVelocity( 0.0f )
,	m_MantleVector()
,	m_MantleDestination()
,	m_CanMantle( false )
,	m_AutoAimEnabled( false )
,	m_AutoAimMaxTurn( 0.0f )
,	m_SprintFOVEnabled( false )
,	m_CurrentFOV()
,	m_CurrentFGFOV()
,	m_IsDisablingPause( false )
,	m_HasSetSpawnPoint( false )
,	m_SpawnLocation()
,	m_SpawnOrientation()
,	m_IsFlying( false )
,	m_MaxViewBobOffset()
,	m_MaxViewBobAngleOffset()
,	m_UnlandedViewBobWindow( 0.0f )
,	m_ViewBobAngleExponent( 0.0f )
,	m_ViewBobInterpolateTime( 0.0f )
,	m_MaxViewSwayOffset()
,	m_MaxViewSwayAngleOffset()
,	m_ViewSwayInterpolateTime( 0.0f )
,	m_ViewSwayNoiseOctaves( 0 )
,	m_ViewSwayNoiseScalars()
,	m_ViewSwayNoiseAngleScalars()
#if BUILD_DEV
,	m_DEVHACKDebugTarget()
,	m_DEVHACKClockMultiplier( NULL )
,	m_CAMHACKPathData()
,	m_CAMHACKCamActive( false )
,	m_CAMHACKCamVelocity( 1.0f )
,	m_CAMHACKCamLocation()
,	m_CAMHACKCamOrientation()
,	m_CAMHACKCamStartLocation()
,	m_CAMHACKCamEndLocation()
,	m_CAMHACKCamStartOrientation()
,	m_CAMHACKCamEndOrientation()
,	m_DebugSpawners()
,	m_DebugSpawnerIndex( 0 )
,	m_CACHED_LastAINoiseSourceLocation()
,	m_CACHED_LastAINoiseRadius( 0.0f )
#endif
{
	STATIC_HASHED_STRING( PreLevelTransition );
	GetEventManager()->AddObserver( sPreLevelTransition, this );

	STATIC_HASHED_STRING( PostLevelTransition );
	GetEventManager()->AddObserver( sPostLevelTransition, this );

	STATIC_HASHED_STRING( OnAINoise );
	GetEventManager()->AddObserver( sOnAINoise, this );

	GetFramework()->GetInputSystem()->AddInputSystemObserver( this );
}

WBCompRosaPlayer::~WBCompRosaPlayer()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( PreLevelTransition );
		pEventManager->RemoveObserver( sPreLevelTransition, this );

		STATIC_HASHED_STRING( PostLevelTransition );
		pEventManager->RemoveObserver( sPostLevelTransition, this );

		STATIC_HASHED_STRING( OnAINoise );
		pEventManager->RemoveObserver( sOnAINoise, this );
	}

	InputSystem* const pInputSystem = GetFramework()->GetInputSystem();
	if( pInputSystem )
	{
		pInputSystem->RemoveInputSystemObserver( this );
	}
}

/*virtual*/ void WBCompRosaPlayer::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	RosaCampaign* const pCampaign = RosaCampaign::GetCampaign();

	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( DeferMusic );
	m_DeferMusic = ConfigManager::GetInheritedBool( sDeferMusic, false, sDefinitionName );

	STATICHASH( InitialMusicTrackBits );
	m_InitialMusicTrackBits = pCampaign->OverrideInt( sInitialMusicTrackBits, ConfigManager::GetInheritedInt( sInitialMusicTrackBits, 0, sDefinitionName ) );

	STATICHASH( RollingAutosaveDelay );
	m_RollingAutosaveDelay = ConfigManager::GetInheritedFloat( sRollingAutosaveDelay, 0.0f, sDefinitionName );

	STATICHASH( RetryAutosaveDelay );
	m_RetryAutosaveDelay = ConfigManager::GetInheritedFloat( sRetryAutosaveDelay, 0.0f, sDefinitionName );

	STATICHASH( UnlandedForceFootstepWindow );
	m_UnlandedForceFootstepWindow = ConfigManager::GetInheritedFloat( sUnlandedForceFootstepWindow, 0.0f, sDefinitionName );

	STATICHASH( UnlandedJumpWindow );
	m_UnlandedJumpWindow = ConfigManager::GetInheritedFloat( sUnlandedJumpWindow, 0.0f, sDefinitionName );

	STATICHASH( UnlandedLeanWindow );
	m_UnlandedLeanWindow = ConfigManager::GetInheritedFloat( sUnlandedLeanWindow, 0.0f, sDefinitionName );

	STATICHASH( LandAcceleration );
	m_LandAcceleration = ConfigManager::GetInheritedFloat( sLandAcceleration, 0.0f, sDefinitionName );

	STATICHASH( AirAcceleration );
	m_AirAcceleration = ConfigManager::GetInheritedFloat( sAirAcceleration, 0.0f, sDefinitionName );

	STATICHASH( TurnSpeed );
	m_TurnSpeed = ConfigManager::GetInheritedFloat( sTurnSpeed, 0.0f, sDefinitionName );

	STATICHASH( JumpHeight );
	m_JumpHeight = ConfigManager::GetInheritedFloat( sJumpHeight, 0.0f, sDefinitionName );

	STATICHASH( BackpedalScalar );
	m_BackpedalScalar = ConfigManager::GetInheritedFloat( sBackpedalScalar, 1.0f, sDefinitionName );

	STATICHASH( DoubleJumpHeight );
	m_DoubleJumpHeight = ConfigManager::GetInheritedFloat( sDoubleJumpHeight, 0.0f, sDefinitionName );

	STATICHASH( PowerJumpRatio );
	m_PowerJumpRatio = ConfigManager::GetInheritedFloat( sPowerJumpRatio, 0.0f, sDefinitionName );

	STATICHASH( UncrouchOnSprint );
	m_UncrouchOnSprint = ConfigManager::GetInheritedBool( sUncrouchOnSprint, false, sDefinitionName );

	STATICHASH( SprintFOVScale );
	m_SprintFOVScale = ConfigManager::GetInheritedFloat( sSprintFOVScale, 1.0f, sDefinitionName );

	STATICHASH( SprintFOVTime );
	m_SprintFOVTime = ConfigManager::GetInheritedFloat( sSprintFOVTime, 0.0f, sDefinitionName );

	STATICHASH( StandExtentsZ );
	m_StandExtentsZ = ConfigManager::GetInheritedFloat( sStandExtentsZ, 0.0f, sDefinitionName );

	STATICHASH( CrouchExtentsZ );
	m_CrouchExtentsZ = ConfigManager::GetInheritedFloat( sCrouchExtentsZ, 0.0f, sDefinitionName );

	STATICHASH( StandViewOffsetZ );
	m_StandViewOffsetZ = ConfigManager::GetInheritedFloat( sStandViewOffsetZ, 0.0f, sDefinitionName );
	m_ViewOffsetZInterpolator.Reset( m_StandViewOffsetZ );

	STATICHASH( CrouchViewOffsetZ );
	m_CrouchViewOffsetZ = ConfigManager::GetInheritedFloat( sCrouchViewOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( CrouchViewInterpTime );
	m_CrouchViewInterpTime = ConfigManager::GetInheritedFloat( sCrouchViewInterpTime, 0.0f, sDefinitionName );

	STATICHASH( PushToSprint );
	m_PushToSprint = ConfigManager::GetInheritedBool( sPushToSprint, false, sDefinitionName );

	STATICHASH( PowerSlideDuration );
	m_PowerSlideDuration = ConfigManager::GetInheritedFloat( sPowerSlideDuration, 0.0f, sDefinitionName );

	STATICHASH( PowerSlideInputContext );
	m_PowerSlideInputContext = ConfigManager::GetInheritedHash( sPowerSlideInputContext, HashedString::NullString, sDefinitionName );

	STATICHASH( PowerSlideReqVelocity );
	m_PowerSlideReqVelocitySq = Square( ConfigManager::GetInheritedFloat( sPowerSlideReqVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( PowerSlideRoll );
	m_PowerSlideRoll = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sPowerSlideRoll, 0.0f, sDefinitionName ) );

	STATICHASH( PowerSlideRollInterpTime );
	m_PowerSlideRollInterpTime = ConfigManager::GetInheritedFloat( sPowerSlideRollInterpTime, 0.0f, sDefinitionName );

	STATICHASH( DragInputContext );
	m_DragInputContext = ConfigManager::GetInheritedHash( sDragInputContext, HashedString::NullString, sDefinitionName );

	STATICHASH( DragDropOffsetZ );
	m_DragDropOffset.z = ConfigManager::GetInheritedFloat( sDragDropOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( DragDropOrientationYaw );
	m_DragDropOrientation.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sDragDropOrientationYaw, 0.0f, sDefinitionName ) );

	STATICHASH( DragDropSpawnImpulse );
	m_DragDropSpawnImpulse = ConfigManager::GetInheritedFloat( sDragDropSpawnImpulse, 0.0f, sDefinitionName );

	STATICHASH( DragDropSpawnImpulseZ );
	m_DragDropSpawnImpulseZ = ConfigManager::GetInheritedFloat( sDragDropSpawnImpulseZ, 0.0f, sDefinitionName );

	STATICHASH( ClimbInputContext );
	m_ClimbInputContext = ConfigManager::GetInheritedHash( sClimbInputContext, HashedString::NullString, sDefinitionName );

	STATICHASH( ClimbOffImpulse );
	m_ClimbOffImpulse = ConfigManager::GetInheritedFloat( sClimbOffImpulse, 0.0f, sDefinitionName );

	STATICHASH( ClimbFacingBiasAngle );
	m_ClimbFacingBiasAngle = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sClimbFacingBiasAngle, 0.0f, sDefinitionName ) );

	STATICHASH( ClimbFacingBiasScale );
	m_ClimbFacingBiasScale = ConfigManager::GetInheritedFloat( sClimbFacingBiasScale, 0.0f, sDefinitionName );

	STATICHASH( MantleInputContext );
	m_MantleInputContext = ConfigManager::GetInheritedHash( sMantleInputContext, HashedString::NullString, sDefinitionName );

	STATICHASH( MantleVelocity );
	m_MantleVelocity = ConfigManager::GetInheritedFloat( sMantleVelocity, 0.0f, sDefinitionName );

	// Not a config var of the component!
	STATICHASH( AutoAim );
	m_AutoAimEnabled = ConfigManager::GetBool( sAutoAim );

	STATICHASH( AutoAimMaxTurn );
	m_AutoAimMaxTurn = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sAutoAimMaxTurn, 0.0f, sDefinitionName ) );

	// Not a config var of the component!
	STATICHASH( SprintFOV );
	m_SprintFOVEnabled = ConfigManager::GetBool( sSprintFOV );

	STATICHASH( MaxViewBobOffsetX );
	m_MaxViewBobOffset.x = ConfigManager::GetInheritedFloat( sMaxViewBobOffsetX, 0.0f, sDefinitionName );

	STATICHASH( MaxViewBobOffsetY );
	m_MaxViewBobOffset.y = ConfigManager::GetInheritedFloat( sMaxViewBobOffsetY, 0.0f, sDefinitionName );

	STATICHASH( MaxViewBobOffsetZ );
	m_MaxViewBobOffset.z = ConfigManager::GetInheritedFloat( sMaxViewBobOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( MaxViewBobAngleOffsetPitch );
	m_MaxViewBobAngleOffset.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxViewBobAngleOffsetPitch, 0.0f, sDefinitionName ) );

	STATICHASH( MaxViewBobAngleOffsetRoll );
	m_MaxViewBobAngleOffset.Roll = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxViewBobAngleOffsetRoll, 0.0f, sDefinitionName ) );

	STATICHASH( MaxViewBobAngleOffsetYaw );
	m_MaxViewBobAngleOffset.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxViewBobAngleOffsetYaw, 0.0f, sDefinitionName ) );

	STATICHASH( UnlandedViewBobWindow );
	m_UnlandedViewBobWindow = ConfigManager::GetInheritedFloat( sUnlandedViewBobWindow, 0.0f, sDefinitionName );

	STATICHASH( ViewBobAngleExponent );
	m_ViewBobAngleExponent = ConfigManager::GetInheritedFloat( sViewBobAngleExponent, 0.0f, sDefinitionName );

	STATICHASH( ViewBobInterpolateTime );
	m_ViewBobInterpolateTime = ConfigManager::GetInheritedFloat( sViewBobInterpolateTime, 0.0f, sDefinitionName );

	STATICHASH( MaxViewSwayOffsetX );
	m_MaxViewSwayOffset.x = ConfigManager::GetInheritedFloat( sMaxViewSwayOffsetX, 0.0f, sDefinitionName );

	STATICHASH( MaxViewSwayOffsetY );
	m_MaxViewSwayOffset.y = ConfigManager::GetInheritedFloat( sMaxViewSwayOffsetY, 0.0f, sDefinitionName );

	STATICHASH( MaxViewSwayOffsetZ );
	m_MaxViewSwayOffset.z = ConfigManager::GetInheritedFloat( sMaxViewSwayOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( MaxViewSwayAngleOffsetPitch );
	m_MaxViewSwayAngleOffset.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxViewSwayAngleOffsetPitch, 0.0f, sDefinitionName ) );

	STATICHASH( MaxViewSwayAngleOffsetRoll );
	m_MaxViewSwayAngleOffset.Roll = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxViewSwayAngleOffsetRoll, 0.0f, sDefinitionName ) );

	STATICHASH( MaxViewSwayAngleOffsetYaw );
	m_MaxViewSwayAngleOffset.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxViewSwayAngleOffsetYaw, 0.0f, sDefinitionName ) );

	STATICHASH( ViewSwayInterpolateTime );
	m_ViewSwayInterpolateTime = ConfigManager::GetInheritedFloat( sViewSwayInterpolateTime, 0.0f, sDefinitionName );

	STATICHASH( ViewSwayNoiseOctaves );
	m_ViewSwayNoiseOctaves = ConfigManager::GetInheritedInt( sViewSwayNoiseOctaves, 0, sDefinitionName );

	STATICHASH( ViewSwayNoiseScalarsX );
	m_ViewSwayNoiseScalars.x = ConfigManager::GetInheritedFloat( sViewSwayNoiseScalarsX, 0.0f, sDefinitionName );

	STATICHASH( ViewSwayNoiseScalarsY );
	m_ViewSwayNoiseScalars.y = ConfigManager::GetInheritedFloat( sViewSwayNoiseScalarsY, 0.0f, sDefinitionName );

	STATICHASH( ViewSwayNoiseScalarsZ );
	m_ViewSwayNoiseScalars.z = ConfigManager::GetInheritedFloat( sViewSwayNoiseScalarsZ, 0.0f, sDefinitionName );

	STATICHASH( ViewSwayNoiseAngleScalarsPitch );
	m_ViewSwayNoiseAngleScalars.x = ConfigManager::GetInheritedFloat( sViewSwayNoiseAngleScalarsPitch, 0.0f, sDefinitionName );

	STATICHASH( ViewSwayNoiseAngleScalarsRoll );
	m_ViewSwayNoiseAngleScalars.y = ConfigManager::GetInheritedFloat( sViewSwayNoiseAngleScalarsRoll, 0.0f, sDefinitionName );

	STATICHASH( ViewSwayNoiseAngleScalarsYaw );
	m_ViewSwayNoiseAngleScalars.z = ConfigManager::GetInheritedFloat( sViewSwayNoiseAngleScalarsYaw, 0.0f, sDefinitionName );

	STATICHASH( KickSpringK );
	m_KickSpringK = ConfigManager::GetInheritedFloat( sKickSpringK, 0.0f, sDefinitionName );

	STATICHASH( KickDamperC );
	m_KickDamperC = ConfigManager::GetInheritedFloat( sKickDamperC, 0.0f, sDefinitionName );

	STATICHASH( ViewHandsSpringK );
	m_ViewHandsSpringK = ConfigManager::GetInheritedFloat( sViewHandsSpringK, 0.0f, sDefinitionName );

	STATICHASH( ViewHandsDamperC );
	m_ViewHandsDamperC = ConfigManager::GetInheritedFloat( sViewHandsDamperC, 0.0f, sDefinitionName );

#if BUILD_DEV
	const uint NumActiveContexts	= GetFramework()->GetInputSystem()->GetNumActiveContexts();
	const bool IsInTitleScreen		= GetGame()->IsInTitleScreen();
	// This isn't really the player component's domain, it's just a convenient place to validate this.
	// Title screen has a null input context, so it is excepted.
	DEVASSERT( NumActiveContexts == 0 || IsInTitleScreen );
#endif // BUILD_DEV

#if BUILD_DEV
	STATICHASH( RosaPlayer_DebugSpawner );

	STATICHASH( NumEntities );
	const uint NumEntities = ConfigManager::GetInt( sNumEntities, 0, sRosaPlayer_DebugSpawner );

	for( uint EntityIndex = 0; EntityIndex < NumEntities; ++EntityIndex )
	{
		SDebugSpawner& Spawner		= m_DebugSpawners.PushBack();
		Spawner.m_Entity			= ConfigManager::GetSequenceString(	"Entity%d",		EntityIndex, "",	sRosaPlayer_DebugSpawner );
		Spawner.m_Offset.x			= ConfigManager::GetSequenceFloat(	"Entity%dX",	EntityIndex, 0.0f,	sRosaPlayer_DebugSpawner );
		Spawner.m_Offset.y			= ConfigManager::GetSequenceFloat(	"Entity%dY",	EntityIndex, 0.0f,	sRosaPlayer_DebugSpawner );
		Spawner.m_Offset.z			= ConfigManager::GetSequenceFloat(	"Entity%dZ",	EntityIndex, 0.0f,	sRosaPlayer_DebugSpawner );
		Spawner.m_NormalDistance	= ConfigManager::GetSequenceFloat(	"Entity%dN",	EntityIndex, 0.0f,	sRosaPlayer_DebugSpawner );
	}
#endif
}

void WBCompRosaPlayer::TickViewBobAndSway()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	WBEntity* const				pEntity		= GetEntity();
	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	WBCompRosaHealth* const		pHealth		= WB_GETCOMP( pEntity, RosaHealth );
	WBCompRosaFootsteps* const	pFootsteps	= WB_GETCOMP( pEntity, RosaFootsteps );
	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );

	// NOTE: This uses the transform's *default* speed limit, not the modded speed limit.
	// Which is what I want here, because then it saturates while running even if speed limit is increased.
	const bool		DoViewBob				= pHealth->IsAlive() && pCollision->IsRecentlyLanded( m_UnlandedViewBobWindow ) && !m_IsPowerSliding && !m_IsClimbing && !m_IsMantling;
	const float		ViewBobScalar			= Saturate( pTransform->GetVelocity().Length2D() / pTransform->GetSpeedLimit() );
	const float		ViewBobAngleScalar		= Pow( ViewBobScalar, m_ViewBobAngleExponent );
	const float		ViewBobAlphaX			= pFootsteps ? pFootsteps->GetStepPhaseSignedAlpha() : 0.0f;	// [-1, 1]
	const float		ViewBobAlphaYZ			= Abs( ViewBobAlphaX );											// [0, 1]
	const Vector	ViewBobAlpha			= Vector( ViewBobAlphaX, ViewBobAlphaYZ, ViewBobAlphaYZ );
	const Angles	ViewBobAngleAlpha		= Angles( ViewBobAlphaYZ, ViewBobAlphaX, ViewBobAlphaX );
	const Vector	ViewBobOffset			= ( ViewBobScalar * ViewBobAlpha * m_MaxViewBobOffset ) * pTransform->GetOrientation().ToMatrix();
	const Angles	ViewBobAngleOffset		= ( ViewBobAngleScalar * ViewBobAngleAlpha * m_MaxViewBobAngleOffset );

	const bool		DoViewSway				= true;
	const Vector	ViewSwayAlpha			= Vector(
												Noise::SumNoise1( GetTime() * m_ViewSwayNoiseScalars.x, m_ViewSwayNoiseOctaves, Noise::CubicNoise1 ),
												Noise::SumNoise1( GetTime() * m_ViewSwayNoiseScalars.y, m_ViewSwayNoiseOctaves, Noise::CubicNoise1 ),
												Noise::SumNoise1( GetTime() * m_ViewSwayNoiseScalars.z, m_ViewSwayNoiseOctaves, Noise::CubicNoise1 ) );
	const Angles	ViewSwayAngleAlpha		= Angles(
												Noise::SumNoise1( GetTime() * m_ViewSwayNoiseAngleScalars.x, m_ViewSwayNoiseOctaves, Noise::CubicNoise1 ),
												Noise::SumNoise1( GetTime() * m_ViewSwayNoiseAngleScalars.y, m_ViewSwayNoiseOctaves, Noise::CubicNoise1 ),
												Noise::SumNoise1( GetTime() * m_ViewSwayNoiseAngleScalars.z, m_ViewSwayNoiseOctaves, Noise::CubicNoise1 ) );
	WB_MODIFY_FLOAT( ViewSwayOffsetScalar, 1.0f, pStatMod );
	const float ViewSwayOffsetScalar		= WB_MODDED( ViewSwayOffsetScalar );
	WB_MODIFY_FLOAT( ViewSwayAngleOffsetScalar, 1.0f, pStatMod );
	const float ViewSwayAngleOffsetScalar	= WB_MODDED( ViewSwayAngleOffsetScalar );
	const Vector	ViewSwayOffset			= ViewSwayOffsetScalar * ( ( ViewSwayAlpha * m_MaxViewSwayOffset ) * pTransform->GetOrientation().ToMatrix() );
	const Angles	ViewSwayAngleOffset		= ViewSwayAngleOffsetScalar * ( ViewSwayAngleAlpha * m_MaxViewSwayAngleOffset );

	// Constantly resetting from current value produces an ease-out effect
	m_ViewBobOffsetInterpolator.Reset(
		Interpolator<Vector>::EIT_Linear,
		m_ViewBobOffsetInterpolator.GetValue(),
		DoViewBob ? ViewBobOffset : Vector(),
		m_ViewBobInterpolateTime );

	m_ViewBobAngleOffsetInterpolator.Reset(
		Interpolator<Angles>::EIT_Linear,
		m_ViewBobAngleOffsetInterpolator.GetValue(),
		DoViewBob ? ViewBobAngleOffset : Angles(),
		m_ViewBobInterpolateTime );

	m_ViewSwayOffsetInterpolator.Reset(
		Interpolator<Vector>::EIT_Linear,
		m_ViewSwayOffsetInterpolator.GetValue(),
		DoViewSway ? ViewSwayOffset : Vector(),
		m_ViewSwayInterpolateTime );

	m_ViewSwayAngleOffsetInterpolator.Reset(
		Interpolator<Angles>::EIT_Linear,
		m_ViewSwayAngleOffsetInterpolator.GetValue(),
		DoViewSway ? ViewSwayAngleOffset : Angles(),
		m_ViewSwayInterpolateTime );
}

WBCompRosaWeapon* WBCompRosaPlayer::GetWeapon() const
{
	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaInventory* const	pInventory		= WB_GETCOMP( pEntity, RosaInventory );
	DEVASSERT( pInventory );

	WBEntity* const				pWeaponEntity	= pInventory->GetCycleItem();

	WBCompRosaWeapon* const		pWeapon			= WB_GETCOMP_SAFE( pWeaponEntity, RosaWeapon );

	return pWeapon;
}

void WBCompRosaPlayer::TickKick( const float DeltaTime )
{
	WBCompRosaWeapon* const		pWeapon			= GetWeapon();
	const float					SpringK			= ( pWeapon && pWeapon->GetKickSpringK() > 0.0f ) ? pWeapon->GetKickSpringK() : m_KickSpringK;
	const float					DamperC			= ( pWeapon && pWeapon->GetKickDamperC() > 0.0f ) ? pWeapon->GetKickDamperC() : m_KickDamperC;

	// Spring-damping system (ignoring mass because it's just a multiple of this equation)
	const Angles KickAcceleration = ( -SpringK * m_KickPosition ) + ( -DamperC * m_KickVelocity );

	m_KickVelocity += KickAcceleration	* DeltaTime;
	m_KickPosition += m_KickVelocity	* DeltaTime;
}

void WBCompRosaPlayer::TickHandsVelocity( const float DeltaTime, const Vector& TargetVelocity, const Angles& TargetRotationalVelocity )
{
	// Here, the velocity *is* the position for the spring system; so "velocity" becomes acceleration, and "acceleration" becomes jerk.

	// This system typically tries to center at the origin instead of a target;
	// so subtract to get our "position" (velocity).
	const Vector	CurrentVelocity				= m_ViewHandsVelocity			- TargetVelocity;
	const Angles	CurrentRotationalVelocity	= m_ViewHandsRotationalVelocity	- TargetRotationalVelocity;
	const Vector	ViewHandsJerk				= ( -m_ViewHandsSpringK * CurrentVelocity )				+ ( -m_ViewHandsDamperC * m_ViewHandsAcceleration );
	const Angles	ViewHandsRotationalJerk		= ( -m_ViewHandsSpringK * CurrentRotationalVelocity )	+ ( -m_ViewHandsDamperC * m_ViewHandsRotationalAcceleration );

	m_ViewHandsAcceleration				+= ViewHandsJerk						* DeltaTime;
	m_ViewHandsRotationalAcceleration	+= ViewHandsRotationalJerk				* DeltaTime;
	m_ViewHandsVelocity					+= m_ViewHandsAcceleration				* DeltaTime;
	m_ViewHandsRotationalVelocity		+= m_ViewHandsRotationalAcceleration	* DeltaTime;
}

// Sticky targeting (lock-on, auto aim, aim assist, whatever)
// Orient to face aim target, if any; only if we're using the controller.
// This isn't dependent on input per se, but it's probably best
// if we only do it in cases where turning isn't suppressed.
// ROSATODO: This should probably still be an option, even if it's controller-only.
void WBCompRosaPlayer::TickAutoAim( const float DeltaTime )
{
	if( !m_AutoAimEnabled )
	{
		return;
	}

	STATIC_HASHED_STRING( TurnX );
	STATIC_HASHED_STRING( TurnY );

	InputSystem* const			pInputSystem	= GetFramework()->GetInputSystem();
	if( !pInputSystem->IsUsingControllerExclusively() ||
		pInputSystem->IsSuppressed( sTurnX ) ||
		pInputSystem->IsSuppressed( sTurnY ) )
	{
		return;
	}

	WBCompRosaWeapon* const		pWeapon			= GetWeapon();
	if( !pWeapon || !pWeapon->CanAutoAim() )
	{
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaFrobber* const	pFrobber		= WB_GETCOMP( pEntity, RosaFrobber );

	WBEntity*		pAimTarget;
	HashedString	AimTargetBone;
	pFrobber->GetAimTarget( pAimTarget, AimTargetBone );
	if( !pAimTarget )
	{
		return;
	}

	WBCompRosaFrobbable* const	pFrobbable	= WB_GETCOMP( pAimTarget, RosaFrobbable );
	DEVASSERT( pFrobbable );
	if( !pFrobbable->CanBeAutoAimedAt() )
	{
		return;
	}

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaCamera* const		pCamera			= WB_GETCOMP( pEntity, RosaCamera );
	WBCompRosaMesh* const		pAimTargetMesh	= WB_GETCOMP( pAimTarget, RosaMesh );
	const Vector				BoneLocation	= pAimTargetMesh->GetBoneLocation( pAimTargetMesh->GetBoneIndex( AimTargetBone ) );
	const Vector				CameraLocation	= pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pTransform->GetLocation() );
	const Angles				OldOrientation	= pTransform->GetOrientation();
	const Vector				AimOffset		= BoneLocation - CameraLocation;
	const Angles				AimOrientation	= AimOffset.ToAngles();
	Angles						TurnOffset		= ( AimOrientation - pTransform->GetOrientation() ).GetShortestRotation();
	const float					MaxTurn			= DeltaTime * m_AutoAimMaxTurn;
	TurnOffset.Yaw								= Clamp( TurnOffset.Yaw,	-MaxTurn, MaxTurn );
	TurnOffset.Pitch							= Clamp( TurnOffset.Pitch,	-MaxTurn, MaxTurn );
	const Angles				NewOrientation	= WBCompRosaTransform::ClampPitch( OldOrientation + TurnOffset );
	pTransform->SetOrientation( NewOrientation );
}

/*virtual*/ void WBCompRosaPlayer::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaCollision* const	pCollision		= WB_GETCOMP( pEntity, RosaCollision );
	WBCompRosaCamera* const		pCamera			= WB_GETCOMP( pEntity, RosaCamera );
	WBCompRosaHealth* const		pHealth			= WB_GETCOMP( pEntity, RosaHealth );
	WBCompRosaInventory* const	pInventory		= WB_GETCOMP( pEntity, RosaInventory );
	WBCompStatMod* const		pStatMod		= WB_GETCOMP( pEntity, StatMod );
	WBCompRosaWeapon* const		pWeapon			= GetWeapon();
	WBEventManager* const		pEventManager	= GetEventManager();
	InputSystem* const			pInputSystem	= GetFramework()->GetInputSystem();

#if BUILD_DEV
	if( m_CAMHACKCamActive )
	{
		m_CAMHACKCamLocation.Tick( DeltaTime );
		m_CAMHACKCamOrientation.Tick( DeltaTime );

		WBCompRosaFootsteps* const	pFootsteps	= WB_GETCOMP( pEntity, RosaFootsteps );

		pTransform->SetLocation( m_CAMHACKCamLocation.GetValue() );
		pTransform->SetOrientation( m_CAMHACKCamOrientation.GetValue() );

		if( m_CAMHACKCamLocation.GetT() == 1.0f )
		{
			m_CAMHACKCamActive = false;

			pCollision->ResetCollisionFlags();
			pTransform->SetDefaultGravity();

			if( pFootsteps )
			{
				pFootsteps->SetFootstepsDisabled( false );
			}

			WB_MAKE_EVENT( ShowHands, GetEntity() );
			WB_DISPATCH_EVENT( pEventManager, ShowHands, GetEntity() );

			STATIC_HASHED_STRING( HUD );
			WB_MAKE_EVENT( RepushUIScreen, NULL );
			WB_SET_AUTO( RepushUIScreen, Hash, Screen, sHUD );
			WB_DISPATCH_EVENT( GetEventManager(), RepushUIScreen, NULL );
		}
	}
#endif	// BUILD_DEV

	TickViewBobAndSway();

	TickKick( DeltaTime );
	TickHandsVelocity( DeltaTime, pTransform->GetVelocity(), pTransform->GetRotationalVelocity() );

	if( pHealth->IsAlive() )
	{
		m_ViewOffsetZInterpolator.Tick( DeltaTime );
		pCamera->SetViewOffsetZ( m_ViewOffsetZInterpolator.GetValue() );

		m_PowerSlideRollInterpolator.Tick( DeltaTime );
		pCamera->SetSlideRoll( m_PowerSlideRollInterpolator.GetValue() );

		m_StepUpZInterpolator.Tick( DeltaTime );
		pCamera->SetStepUpZ( m_StepUpZInterpolator.GetValue() );

		// Stat mod kick at the last minute instead of for each impulse
		WB_MODIFY_FLOAT( KickScalar, 1.0f, pStatMod );
		pCamera->SetKickAngleOffset( m_KickPosition * WB_MODDED( KickScalar ) );
	}
	else
	{
		pCamera->SetKickAngleOffset( Angles() );
	}

	// Do this part regardless of being alive, I guess
	{
		m_ViewBobOffsetInterpolator.Tick( DeltaTime );
		pCamera->SetViewBobOffset( m_ViewBobOffsetInterpolator.GetValue() );

		m_ViewBobAngleOffsetInterpolator.Tick( DeltaTime );
		pCamera->SetViewBobAngleOffset( m_ViewBobAngleOffsetInterpolator.GetValue() );

		m_ViewSwayOffsetInterpolator.Tick( DeltaTime );
		pCamera->SetViewSwayOffset( m_ViewSwayOffsetInterpolator.GetValue() );

		m_ViewSwayAngleOffsetInterpolator.Tick( DeltaTime );
		pCamera->SetViewSwayAngleOffset( m_ViewSwayAngleOffsetInterpolator.GetValue() );

		pCamera->SetHandsVelocity( m_ViewHandsVelocity, m_ViewHandsRotationalVelocity );
	}

	// Update FOVs, factoring in weapon aim and any other stat mods
	// Weapon aim FOV takes priority, then fall back to sprint/normal, then apply stat mods
	const bool				UseSprintFOV	= m_SprintFOVEnabled && ( m_IsSprintingWithMovement || m_IsPowerSliding );
	const float				BaseFOVScale	= ( pWeapon && pWeapon->IsAiming() )		? pWeapon->GetAimZoom()		: ( UseSprintFOV ? m_SprintFOVScale : 1.0f );
	const float				BaseFGFOVScale	= ( pWeapon && pWeapon->IsAiming() )		? pWeapon->GetAimZoomFG()	: 1.0f;
	const float				BaseFOVTime		= ( pWeapon && pWeapon->IsAimChanging() )	? pWeapon->GetAimTime()		: m_SprintFOVTime;
	WB_MODIFY_FLOAT( FOVScale,		BaseFOVScale,	pStatMod );
	WB_MODIFY_FLOAT( FGFOVScale,	BaseFGFOVScale,	pStatMod );
	WB_MODIFY_FLOAT( FOVTime,		BaseFOVTime,	pStatMod );
	STATICHASH( FOV );
	STATICHASH( ForegroundFOV );
	TickScaleFOV( m_CurrentFOV,		ConfigManager::GetFloat( sFOV ),			WB_MODDED( FOVScale ),		WB_MODDED( FOVTime ), DeltaTime );
	TickScaleFOV( m_CurrentFGFOV,	ConfigManager::GetFloat( sForegroundFOV ),	WB_MODDED( FGFOVScale ),	WB_MODDED( FOVTime ), DeltaTime );
	GetFramework()->SetFOV(		m_CurrentFOV.GetValue() );
	GetFramework()->SetFGFOV(	m_CurrentFGFOV.GetValue() );

	if( m_IsUncrouching )
	{
		TryUncrouch();
	}

	if( m_IsPowerSliding )
	{
		if( GetTime() >= m_PowerSlideEndTime )
		{
			EndPowerSlide();
		}
	}



	// *********************************************************************************************
	// *********************************************************************************************
	// *********************************************************************************************
	//
	// Everything before this point is independent of current input and must be done every tick.
	// Everything after this point is dependent on input and should be done only when we have focus.
	//
	// *********************************************************************************************
	// *********************************************************************************************
	// *********************************************************************************************

	if( !GetFramework()->HasFocus() )
	{
		return;
	}

	TickAutoAim( DeltaTime );

	DEVASSERT( pTransform );
	DEVASSERT( pCollision );
	DEVASSERT( pCamera );
	DEVASSERT( pStatMod );

	// Get 2D orientation; we don't want to move as if we're flying.
	Angles PlayerOrientation;
	Vector X, Y, Z;
	const Vector UpVector		= Vector( 0.0f, 0.0f, 1.0f );
	const Vector PlayerFacing	= pTransform->GetOrientation().ToVector();
	const Vector PlayerRight	= PlayerFacing.Cross( UpVector ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
	if( m_IsFlying )
	{
		PlayerOrientation		= pTransform->GetOrientation();
	}
	else
	{
		PlayerOrientation.Yaw	= pTransform->GetOrientation().Yaw;
	}
	PlayerOrientation.GetAxes( X, Y, Z );

	Vector MovementVector;
	Angles TurnAngles;
	Vector ImpulseVector;

	STATIC_HASHED_STRING( Right );
	STATIC_HASHED_STRING( Left );
	STATIC_HASHED_STRING( Forward );
	STATIC_HASHED_STRING( Back );
	STATIC_HASHED_STRING( MoveX );
	STATIC_HASHED_STRING( MoveY );
	STATIC_HASHED_STRING( Jump );
	STATIC_HASHED_STRING( Heal );
	STATIC_HASHED_STRING( UseWeapon );
	STATIC_HASHED_STRING( Shove );
	STATIC_HASHED_STRING( Reload );
	STATIC_HASHED_STRING( Zoom );
	STATIC_HASHED_STRING( Light );
	STATIC_HASHED_STRING( CycleMag );
	STATIC_HASHED_STRING( Radial );
	STATIC_HASHED_STRING( CycleSlot0 );
	STATIC_HASHED_STRING( CycleSlot1 );
	STATIC_HASHED_STRING( CycleSlot2 );
	STATIC_HASHED_STRING( CycleSlot3 );
	STATIC_HASHED_STRING( CyclePrev );
	STATIC_HASHED_STRING( CycleNext );
	STATIC_HASHED_STRING( Frob );
	STATIC_HASHED_STRING( LeanLeft );
	STATIC_HASHED_STRING( LeanRight );
	STATIC_HASHED_STRING( TurnX );
	STATIC_HASHED_STRING( TurnY );
	STATIC_HASHED_STRING( Run );
	STATIC_HASHED_STRING( Crouch );
	STATIC_HASHED_STRING( ClimbForward );
	STATIC_HASHED_STRING( ClimbBack );
	STATIC_HASHED_STRING( ClimbDown );
	STATIC_HASHED_STRING( ClimbY );
	STATIC_HASHED_STRING( ClimbJump );
	STATIC_HASHED_STRING( Mantle );

	if( pInputSystem->IsHigh( sRight ) )	{ MovementVector += X; }
	if( pInputSystem->IsHigh( sLeft ) )		{ MovementVector -= X; }
	if( pInputSystem->IsHigh( sForward ) )	{ MovementVector += Y; }
	if( pInputSystem->IsHigh( sBack ) )		{ MovementVector -= Y * m_BackpedalScalar; }
	if( m_IsFlying )
	{
		// TODO: Maybe do this with an input context? It's really just a borrowed hack from Acid for ghosting right now, whatever.
		if( pInputSystem->IsHigh( sJump ) )
		{
			MovementVector += UpVector;
		}
		if( pInputSystem->IsHigh( sCrouch ) )
		{
			MovementVector -= UpVector;
		}
	}

	const float MoveX = pInputSystem->GetPosition( sMoveX );
	const float MoveY = pInputSystem->GetPosition( sMoveY );
	MovementVector += X * MoveX;
	MovementVector += Y * MoveY * ( ( MoveY < 0.0f ) ? m_BackpedalScalar : 1.0f );

	const bool HasMovementInput = MovementVector.LengthSquared() >= 0.1f || pInputSystem->IsHigh( sMantle ); // HACKHACK to keep spring on during mantle, since the input context disabled MoveX/Y

	const bool WasSprinting = m_IsSprinting;
	if( m_PushToSprint )
	{
		// In this mode, latch the sprinting flag until movement stops.
		if( pInputSystem->IsHigh( sRun ) )
		{
			m_IsSprinting = true;
		}
		else if( !HasMovementInput )
		{
			m_IsSprinting = false;
		}
	}
	else
	{
		m_IsSprinting = pInputSystem->IsHigh( sRun );
	}

	m_IsSprintingWithMovement = m_IsSprinting && HasMovementInput;

	if( !WasSprinting && m_IsSprinting )
	{
		m_SprintStartTime = GetTime();
	}

	// If we're crouched and we start sprinting, even if we don't have any movement input, then uncrouch.
	// (Surprisingly, this feels more correct than requiring movement input. Feels weird to be crouched,
	// press shift, start moving, and not be sprinting.)
	// Note: This behavior doesn't change for push-to-sprint.
	if( m_UncrouchOnSprint && m_IsCrouched && pInputSystem->OnRise( sRun ) )
	{
		BeginUncrouch();
	}

	if( m_IsPowerSliding )
	{
		MovementVector += m_PowerSlideY;
	}

	float LeanTarget = 0.0f;
	const bool CanLean = pCollision->IsRecentlyLanded( m_UnlandedLeanWindow );
	if( m_IsFlying )
	{
		// Don't lean
	}
	else if( CanLean )
	{
		if( pInputSystem->IsHigh( sLeanLeft ) )		{ LeanTarget -= 1.0f; }
		if( pInputSystem->IsHigh( sLeanRight ) )	{ LeanTarget += 1.0f; }
	}

	float StrafeRollTarget = 0.0f;
	if( pInputSystem->IsHigh( sLeft ) )		{ StrafeRollTarget -= 1.0f; }
	if( pInputSystem->IsHigh( sRight ) )	{ StrafeRollTarget += 1.0f; }
	StrafeRollTarget += pInputSystem->GetPosition( sMoveX );

	if( pInputSystem->OnRise( sHeal ) )
	{
		WB_MAKE_EVENT( TryUseBandage, pEntity );
		WB_DISPATCH_EVENT( pEventManager, TryUseBandage, pEntity );
	}

	// NOTE: Always send weapon input, even when it's low. This way, we
	// untrigger automatic weapons even if the OnFall edge is missed.
	const uint UseWeaponInput = pInputSystem->OnInput( sUseWeapon );
	{
		WB_MAKE_EVENT( UseWeapon, pEntity );
		WB_SET_AUTO( UseWeapon, Int, Input, UseWeaponInput );
		WB_DISPATCH_EVENT( pEventManager, UseWeapon, pEntity );
	}

	if( pInputSystem->OnRise( sShove ) )
	{
		WB_MAKE_EVENT( TryShove, pEntity );
		WB_DISPATCH_EVENT( pEventManager, TryShove, pEntity );
	}

	if( pInputSystem->OnRise( sReload ) )
	{
		WB_MAKE_EVENT( TryReload, pEntity );
		WB_DISPATCH_EVENT( pEventManager, TryReload, pEntity );
	}

	if( pInputSystem->OnRise( sLight ) )
	{
		WB_MAKE_EVENT( ToggleFlashlight, NULL );
		WB_DISPATCH_EVENT( pEventManager, ToggleFlashlight, NULL );
	}

	if( pInputSystem->OnRise( sCycleMag ) )
	{
		WB_MAKE_EVENT( TryCycleMagazine, pEntity );
		WB_DISPATCH_EVENT( pEventManager, TryCycleMagazine, pEntity );
	}

	// NOTE: TryAim/TryUnAim are sent continuously, to make sure they aren't missed
	// during weapon transitional states and stuff.
	if( pInputSystem->IsHigh( sZoom ) )
	{
		WB_MAKE_EVENT( TryAim, pEntity );
		WB_SET_AUTO( TryAim, Float, ZoomTime, WB_MODDED( FOVTime ) );
		WB_DISPATCH_EVENT( pEventManager, TryAim, pEntity );
	}
	else
	{
		WB_MAKE_EVENT( TryUnAim, pEntity );
		WB_SET_AUTO( TryUnAim, Float, ZoomTime, WB_MODDED( FOVTime ) );
		WB_DISPATCH_EVENT( pEventManager, TryUnAim, pEntity );
	}

	// Only accept radial input if we have any cycle items
	if( pInputSystem->OnRise( sRadial ) && pInventory->GetNumCycleItems() > 0 )
	{
		STATIC_HASHED_STRING( RadialScreen );
		WB_MAKE_EVENT( PushUIScreen, NULL );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sRadialScreen );
		WB_DISPATCH_EVENT( GetEventManager(), PushUIScreen, NULL );
	}

#define HANDLE_CYCLESLOT_INPUT( slot ) \
	if( pInputSystem->OnRise( sCycleSlot##slot ) ) \
	{ \
		STATIC_HASHED_STRING( WeaponSlot##slot ); \
		WB_MAKE_EVENT( RequestCycleToSlot, pEntity ); \
		WB_SET_AUTO( RequestCycleToSlot, Hash, Slot, sWeaponSlot##slot ); \
		WB_DISPATCH_EVENT( pEventManager, RequestCycleToSlot, pEntity ); \
	}

	HANDLE_CYCLESLOT_INPUT( 0 );
	HANDLE_CYCLESLOT_INPUT( 1 );
	HANDLE_CYCLESLOT_INPUT( 2 );
	HANDLE_CYCLESLOT_INPUT( 3 );
#undef HANDLE_CYCLESLOT_INPUT

	if( pInputSystem->OnRise( sCyclePrev ) )
	{
		WB_MAKE_EVENT( RequestCyclePrev, pEntity );
		WB_DISPATCH_EVENT( pEventManager, RequestCyclePrev, pEntity );
	}

	if( pInputSystem->OnRise( sCycleNext ) )
	{
		WB_MAKE_EVENT( RequestCycleNext, pEntity );
		WB_DISPATCH_EVENT( pEventManager, RequestCycleNext, pEntity );
	}

	const uint FrobInput = pInputSystem->OnInput( sFrob );
	if( FrobInput )
	{
		WB_MAKE_EVENT( OnFrob, pEntity );
		WB_SET_AUTO( OnFrob, Int, Input, FrobInput );
		WB_DISPATCH_EVENT( pEventManager, OnFrob, pEntity );
	}

	bool		IsDoubleJumping		= false;
	const bool	IsRecentlyLanded	= pCollision->IsRecentlyLanded( m_UnlandedJumpWindow );
	const bool	ShouldDoubleJump	= !IsRecentlyLanded && CanDoubleJump();
	if( m_IsFlying )
	{
		// Don't jump
	}
	else if(
		( pInputSystem->OnRise( sJump ) && IsRecentlyLanded ) ||
		( pInputSystem->OnRise( sJump ) && ShouldDoubleJump ) ||
		( pInputSystem->IsHigh( sJump ) && ShouldDoubleJump && pTransform->GetVelocity().z < 0.0f ) )
	{
		if( ShouldDoubleJump )
		{
			IsDoubleJumping		= true;
			m_HasDoubleJumped	= true;
		}

		WB_MAKE_EVENT( OnJumped, pEntity );
		WB_SET_AUTO( OnJumped, Bool, DoubleJump, IsDoubleJumping );
		WB_DISPATCH_EVENT( pEventManager, OnJumped, pEntity );

		ImpulseVector.z += 1.0f;

		if( CanPowerJump() )
		{
			ImpulseVector += ( Y * m_PowerJumpRatio );

			// HACKHACK: Power jump should always uncrouch you
			if( m_IsCrouched )
			{
				BeginUncrouch();
			}

			// HACKHACK: Land out of the jump in a sprint (for push-to-sprint mode)
			m_IsSprinting				= true;
			m_IsSprintingWithMovement	= true;
		}

		if( m_IsPowerSliding )
		{
			EndPowerSlide();
		}
	}

	if( m_IsMantling && pInputSystem->IsLow( sMantle ) )
	{
		EndMantle( false );
	}

	if( m_IsFlying )
	{
		// Don't crouch, and uncrouch if we are
		if( m_IsCrouched )
		{
			BeginUncrouch();
		}
	}
	else if( pInputSystem->OnRise( sCrouch ) )
	{
		if( m_IsUncrouching )
		{
			CancelUncrouch();
		}
		else if( m_IsCrouched )
		{
			BeginUncrouch();
		}
		else
		{
			if( CanCrouch() )
			{
				Crouch();

				if( CanPowerSlide() &&
					( pInputSystem->IsHigh( sForward ) || pInputSystem->GetPosition( sMoveY ) > 0.0f ) )
				{
					BeginPowerSlide( Y );
				}
			}
		}
	}

	// CAMTODO: Is this still something we want? How will missions in Zeta end? Going to an exit might make more sense...
	// Send the Return to Hub message if we've completed the mission
	if( pInputSystem->OnHold( sHeal ) &&
		RosaCampaign::GetCampaign()->IsScenarioCompleted() )
	{
		WB_MAKE_EVENT( ActiveReturnToHub, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), ActiveReturnToHub, GetEntity() );
	}

	const bool	IsClimbForward	= pInputSystem->IsHigh( sClimbForward );
	const bool	IsClimbBack		= pInputSystem->IsHigh( sClimbBack );
	const bool	IsClimbDown		= pInputSystem->IsHigh( sClimbDown );
	const float	ClimbY			= pInputSystem->GetPosition( sClimbY );
	const bool	IsClimbY		= Abs( ClimbY ) > EPSILON;

#if CAMERA_RELATIVE_CLIMBING
	// Camera-relative climbing motion (how I prefer).
	if( IsClimbForward || IsClimbBack || IsClimbDown || IsClimbY )
	{
#if CAMERA_RELATIVE_CLIMBING_BIAS
		const Matrix		ClimbFacingMatrix		= Matrix::CreateRotation( PlayerRight, m_ClimbFacingBiasAngle );
		const Vector		ClimbFacing				= PlayerFacing * ClimbFacingMatrix;
		const float			ClimbMagnitude			= ClimbFacing.z;
		const float			ScaledClimbMagnitude	= Clamp( ClimbMagnitude * m_ClimbFacingBiasScale, -1.0f, 1.0f );
		const Vector		ClimbVector				= UpVector * ScaledClimbMagnitude;
#else
		const Vector		ClimbVector				= PlayerFacing.ProjectionOnto( UpVector );
#endif
		ASSERT( ClimbVector.x == 0.0f && ClimbVector.y == 0.0f );

		if( IsClimbForward )	{ MovementVector += ClimbVector; }
		if( IsClimbBack )		{ MovementVector -= ClimbVector; }
		if( IsClimbDown )		{ MovementVector -= UpVector; }
		if( IsClimbY )			{ MovementVector += ClimbVector * ClimbY; }
	}
#else
	if( IsClimbForward || IsClimbBack || IsClimbDown || IsClimbY )
	{
		if( IsClimbForward )	{ MovementVector += UpVector; }
		if( IsClimbBack )		{ MovementVector -= UpVector; }
		if( IsClimbDown )		{ MovementVector -= UpVector; }
		if( IsClimbY )			{ MovementVector += UpVector * ClimbY; }
	}
#endif

	if( pInputSystem->OnRise( sClimbJump ) &&
		!pInputSystem->OnFall( sJump ) )		// This prevents jumping off when we've just switched to the climb context
	{
		ZeroClimbRefs();

		WB_MAKE_EVENT( OnJumped, pEntity );
		WB_DISPATCH_EVENT( pEventManager, OnJumped, pEntity );

		ImpulseVector.z += 1.0f;
		MovementVector = Vector();

		if( IsClimbForward )	{ ImpulseVector += Y; }
		if( IsClimbBack )		{ ImpulseVector -= Y; }
		if( IsClimbY )			{ ImpulseVector += Y * ClimbY; }
	}

	ConditionalApplyRunningStatMods();

	TurnAngles.Yaw		-= pInputSystem->GetPosition( sTurnX );
	TurnAngles.Pitch	-= pInputSystem->GetPosition( sTurnY );

	float MoveSpeed = 0.0f;
	if( m_IsClimbing || pCollision->IsLanded() || m_IsFlying ) // ACIDHACK: Use land acceleration in the air if we're flying, because that's how it's tuned
	{
		WB_MODIFY_FLOAT( LandAcceleration, m_LandAcceleration, pStatMod );
		MoveSpeed = WB_MODDED( LandAcceleration );
	}
	else
	{
		MoveSpeed = m_AirAcceleration;
	}

	if( m_IsClimbing )
	{
		const float Mu = pCollision->GetFrictionCoefficient();
		pTransform->SetVelocity( pTransform->GetVelocity() * Mu );
	}

	// Don't let movement input exceed 1.0 (run + strafe doesn't double speed!),
	// but do allow it to be less than 1.0 for camera-relative climbing or analog input.
	Vector MovementVectorDirection;
	float MovementVectorLength;
	float MovementVectorLengthOverOne;
	MovementVector.GetNormalized( MovementVectorDirection, MovementVectorLength, MovementVectorLengthOverOne );
	const float AppliedMovementLength = Min( MovementVectorLength, 1.0f );

	MovementVector	= MovementVectorDirection * AppliedMovementLength * MoveSpeed;
	TurnAngles		*= m_TurnSpeed * WB_MODDED( FOVScale );	// Apply FOV scale here so zoomed view has slower turn speed

	if( m_IsMantling )
	{
		// Check if we're past the destination and end the mantle if so
		const Vector	CurrentLocation	= pTransform->GetLocation();
		const Vector	ToDestination	= ( m_MantleDestination - CurrentLocation );
		const float		CosAngle		= ToDestination.Dot( m_MantleVector );
		if( CosAngle < 0.0f )
		{
			EndMantle( true );
		}
		else
		{
			const Vector MantleVelocity = m_MantleVector * m_MantleVelocity;
			pTransform->SetVelocity( MantleVelocity );
		}
	}

	pTransform->SetAcceleration( MovementVector );
	pTransform->SetRotationalVelocity( TurnAngles );

	if( !ImpulseVector.IsZero() )
	{
		const float BaseJumpHeight = IsDoubleJumping ? m_DoubleJumpHeight : m_JumpHeight;
		WB_MODIFY_FLOAT( JumpHeight, BaseJumpHeight, pStatMod );
		const float JumpImpulse = SqRt( -2.0f * pTransform->GetGravity() * WB_MODDED( JumpHeight ) );
		ImpulseVector = ImpulseVector.GetFastNormalized() * JumpImpulse;

		// HACKHACK: Negate velocity along impulse vector, so double jumps work as expected.
		// DLP 29 Dec 2018: For power jumps, I've realized I only want to negate velocity in Z.
		//const Vector ImpulseDirectionVelocity = pTransform->GetVelocity().ProjectionOnto( ImpulseVector );
		const Vector ImpulseDirectionVelocity = pTransform->GetVelocity().ProjectionOnto( Vector::Up );
		pTransform->ApplyImpulse( -ImpulseDirectionVelocity );
		pTransform->ApplyImpulse( ImpulseVector );
	}

	pCamera->SetLeanPosition( LeanTarget );
	pCamera->SetStrafeRollPosition( StrafeRollTarget );

	if( m_IsSprintingWithMovement || m_IsPowerSliding )
	{
		pCamera->ZoomMinimapOut();
	}
	else
	{
		pCamera->ZoomMinimapIn();
	}

#if BUILD_DEV
	DEVHACKInput();
#endif
}

void WBCompRosaPlayer::SetCrouchOverlayHidden( const bool Hidden )
{
	STATIC_HASHED_STRING( HUD );
	STATIC_HASHED_STRING( CrouchOverlay );

	WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
	WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
	WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrouchOverlay );
	WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
	WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, GetFramework()->GetUIManager() );
}

void WBCompRosaPlayer::Crouch()
{
	ASSERT( !m_IsCrouched );
	ASSERT( !m_IsUncrouching );

	DEBUGASSERT( CanCrouch() );

	m_IsCrouched = true;

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	Vector Extents = pCollision->GetExtents();
	ASSERT( m_CrouchExtentsZ < Extents.z );
	Extents.z = m_CrouchExtentsZ;
	pCollision->SetExtents( Extents );

	// DLP 22 Dec 2020: If we're on the ground, immediately assume the crouched height location.
	// This fixes the long standing issue of falling to the ground which I'd papered over with
	// view interpolation and stuff. It was revealed again with the view hands velocity stuff,
	// so I'm finally just doing the thing I should've always done and setting the location
	// (after setting the smaller extents). I don't do this in the air because it jerks the
	// camera and might affect the ability to reach high ledges during crouch jumps.
	if( pCollision->IsLanded() )
	{
		Vector CrouchLocation = pTransform->GetLocation();
		CrouchLocation.z = ( CrouchLocation.z - m_StandExtentsZ ) + m_CrouchExtentsZ + EPSILON;	// ROSAHACK: Add an epsilon to fix getting stuck in ground
		pTransform->SetLocation( CrouchLocation );
	}

	m_ViewOffsetZInterpolator.Reset( Interpolator<float>::EIT_EaseOut, m_StandViewOffsetZ, m_CrouchViewOffsetZ, m_CrouchViewInterpTime );

	SetCrouchOverlayHidden( false );

	STATIC_HASHED_STRING( Crouching );
	pStatMod->TriggerEvent( sCrouching );
}

void WBCompRosaPlayer::BeginUncrouch()
{
	ASSERT( m_IsCrouched );

	m_IsUncrouching = true;
	m_IsForceCrouched = false;

	if( m_IsPowerSliding )
	{
		EndPowerSlide();
	}
}

void WBCompRosaPlayer::CancelUncrouch()
{
	ASSERT( m_IsCrouched );
	ASSERT( m_IsUncrouching );

	m_IsUncrouching = false;
}

void WBCompRosaPlayer::TryUncrouch()
{
	ASSERT( m_IsCrouched );
	ASSERT( m_IsUncrouching );

	if( CanUncrouch() )
	{
		Uncrouch();
	}
}

bool WBCompRosaPlayer::CanCrouch()
{
	DEVASSERT( !m_IsCrouched );

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	RosaWorld* const pWorld = GetWorld();
	ASSERT( pWorld );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= EECF_BlockerCollision;

	if( pWorld->CheckClearance( pTransform->GetLocation(), pCollision->GetExtents(), Info ) )
	{
		// We're inside collision (a valid case when seated at a table, for example); don't crouch
		return false;
	}
	else
	{
		return true;
	}
}

bool WBCompRosaPlayer::CanUncrouch()
{
	ASSERT( m_IsCrouched );

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	RosaWorld* const pWorld = GetWorld();
	ASSERT( pWorld );

	Vector StandLocation = pTransform->GetLocation();
	// DLP 22 Dec 2020: I used to always check the standing location because I was always
	// moving the location on Uncrouch. Not anymore, this corresponds to what Uncrouch does.
	if( pCollision->IsLanded() )
	{
		StandLocation.z = ( StandLocation.z - m_CrouchExtentsZ ) + m_StandExtentsZ + EPSILON;	// ROSAHACK: Add an epsilon to fix getting stuck in ground
	}

	Vector CheckExtents = pCollision->GetExtents();
	CheckExtents.z = m_StandExtentsZ;

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= EECF_BlockerCollision;

	if( pWorld->CheckClearance( StandLocation, CheckExtents, Info ) )
	{
		// Something is blocking the uncrouch
		return false;
	}
	else
	{
		return true;
	}
}

void WBCompRosaPlayer::Uncrouch()
{
	ASSERT( m_IsCrouched );
	ASSERT( m_IsUncrouching );

	DEBUGASSERT( CanUncrouch() );

	m_IsUncrouching = false;
	m_IsCrouched = false;

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	// If we're on land, transform back up.
	// DLP 22 Dec 2020: I used to always do this, which could be used to boost higher during a jump.
	// This introduces the possibility that you won't be able to uncrouch if you're airborne
	// just above the ground (a case I'm handling in CanUncrouch), but that's probably fine.
	if( pCollision->IsLanded() )
	{
		Vector StandLocation = pTransform->GetLocation();
		StandLocation.z = ( StandLocation.z - m_CrouchExtentsZ ) + m_StandExtentsZ + EPSILON;	// ROSAHACK: Add an epsilon to fix getting stuck in ground
		pTransform->SetLocation( StandLocation );
	}

	Vector Extents = pCollision->GetExtents();
	ASSERT( m_StandExtentsZ > Extents.z );
	Extents.z = m_StandExtentsZ;
	pCollision->SetExtents( Extents );

	m_ViewOffsetZInterpolator.Reset( Interpolator<float>::EIT_EaseOut, m_CrouchViewOffsetZ, m_StandViewOffsetZ, m_CrouchViewInterpTime );

	SetCrouchOverlayHidden( true );

	STATIC_HASHED_STRING( Crouching );
	pStatMod->UnTriggerEvent( sCrouching );
}

void WBCompRosaPlayer::RestoreCrouch()
{
	// Fix up view offset and stat mod from state.
	// Other crouch properties (collision extents, transform location) are serialized.

	WBEntity* const			pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaCamera* const	pCamera		= WB_GETCOMP( pEntity, RosaCamera );
	ASSERT( pCamera );

	WBCompStatMod* const	pStatMod	= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	const float ViewOffsetZ = m_IsCrouched ? m_CrouchViewOffsetZ : m_StandViewOffsetZ;
	m_ViewOffsetZInterpolator.Reset( ViewOffsetZ );
	pCamera->SetViewOffsetZ( ViewOffsetZ );

	SetCrouchOverlayHidden( !m_IsCrouched );

	STATIC_HASHED_STRING( Crouching );
	pStatMod->SetEventActive( sCrouching, m_IsCrouched );
}

void WBCompRosaPlayer::OnSteppedUp( const float StepHeight )
{
	WBEntity* const			pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaCamera* const	pCamera		= WB_GETCOMP( pEntity, RosaCamera );
	ASSERT( pCamera );

	const float NewViewOffsetZ	= m_StepUpZInterpolator.GetEndValue();
	const float OldViewOffsetZ	= NewViewOffsetZ - StepHeight;
	m_StepUpZInterpolator.Reset( Interpolator<float>::EIT_EaseOut, OldViewOffsetZ, NewViewOffsetZ, m_CrouchViewInterpTime );
	pCamera->SetStepUpZ( OldViewOffsetZ );
}

bool WBCompRosaPlayer::CanPowerJump() const
{
	if( !m_IsPowerSliding )
	{
		return false;
	}

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT( PowerJump, 0.0f, pStatMod );
	const bool PowerJump = ( WB_MODDED( PowerJump ) != 0.0f );

	return PowerJump;
}

bool WBCompRosaPlayer::CanPowerSlide() const
{
	if( !m_IsSprinting )
	{
		return false;
	}

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	if( pTransform->GetVelocity().LengthSquared2D() < m_PowerSlideReqVelocitySq )
	{
		return false;
	}

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT( PowerSlide, 0.0f, pStatMod );
	const bool PowerSlide = ( WB_MODDED( PowerSlide ) != 0.0f );

	return PowerSlide;
}

bool WBCompRosaPlayer::CanDoubleJump() const
{
	if( m_HasDoubleJumped )
	{
		return false;
	}

	if( m_IsMantling )
	{
		return false;
	}

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT( DoubleJump, 0.0f, pStatMod );
	const bool DoubleJump = ( WB_MODDED( DoubleJump ) != 0.0f );

	return DoubleJump;
}

void WBCompRosaPlayer::BeginPowerSlide( const Vector& PowerSlideY )
{
	ASSERT( !m_IsPowerSliding );

	WBEntity* const			pEntity		= GetEntity();
	DEVASSERT( pEntity );
	WBCompStatMod* const	pStatMod	= WB_GETCOMP( pEntity, StatMod );
	DEVASSERT( pStatMod );
	WB_MODIFY_FLOAT( PowerSlideDuration, m_PowerSlideDuration, pStatMod );

	m_IsPowerSliding	= true;
	m_PowerSlideEndTime	= GetTime() + WB_MODDED( PowerSlideDuration );
	m_PowerSlideY		= PowerSlideY;

	m_PowerSlideRollInterpolator.Reset( Interpolator<float>::EIT_Linear, 0.0f, m_PowerSlideRoll, m_PowerSlideRollInterpTime );

	STATIC_HASHED_STRING( PowerSliding );
	pStatMod->TriggerEvent( sPowerSliding );

	GetFramework()->GetInputSystem()->PushContext( m_PowerSlideInputContext );

	WB_MAKE_EVENT( OnBeginPowerSlide, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnBeginPowerSlide, pEntity );
}

void WBCompRosaPlayer::EndPowerSlide()
{
	ASSERT( m_IsPowerSliding );

	m_IsPowerSliding = false;

	InputSystem* const			pInputSystem	= GetFramework()->GetInputSystem();
	ASSERT( pInputSystem );

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompStatMod* const		pStatMod		= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	// DLP 13 Oct 2021: This used to use m_PowerSlideRoll as the StartValue, but it seems better to blend back
	// from whatever it was at, in case the roll hadn't finished interpolating when the player stopped sliding?
	m_PowerSlideRollInterpolator.Reset( Interpolator<float>::EIT_Linear, m_PowerSlideRollInterpolator.GetValue(), 0.0f, m_PowerSlideRollInterpTime );

	STATIC_HASHED_STRING( PowerSliding );
	pStatMod->UnTriggerEvent( sPowerSliding );

	pInputSystem->PopContext( m_PowerSlideInputContext );

	STATIC_HASHED_STRING( Run );
	if( pInputSystem->IsHigh( sRun ) )
	{
		BeginUncrouch();
	}
}

void WBCompRosaPlayer::RestorePowerSlide()
{
	WBEntity* const			pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompStatMod* const	pStatMod	= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	const float ViewAngleOffsetRoll = m_IsPowerSliding ? m_PowerSlideRoll : 0.0f;
	m_PowerSlideRollInterpolator.Reset( ViewAngleOffsetRoll );

	STATIC_HASHED_STRING( PowerSliding );
	pStatMod->SetEventActive( sPowerSliding, m_IsPowerSliding );

	if( m_IsPowerSliding )
	{
		GetFramework()->GetInputSystem()->PushContext( m_PowerSlideInputContext );
	}
}

void WBCompRosaPlayer::BeginDrag( WBEntity* const pDraggedEntity )
{
	DEVASSERT( !m_IsDragging );

	m_IsDragging	= true;
	m_DraggedEntity	= pDraggedEntity;

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );
	DEVASSERT( pStatMod );

	STATIC_HASHED_STRING( Dragging );
	pStatMod->TriggerEvent( sDragging );

	GetFramework()->GetInputSystem()->PushContext( m_DragInputContext );
}

// HACKHACK: Move the dragged entity and give it an impulse.
// I'm calling this a hack because all the other behavior (e.g., showing/hiding it) is scripted.
void WBCompRosaPlayer::DropDraggedEntity()
{
	WBEntity* const				pEntity				= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform			= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBEntity* const				pDraggedEntity		= m_DraggedEntity.Get();
	DEVASSERT( pDraggedEntity );

	WBCompRosaTransform* const	pDraggedTransform	= pDraggedEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pDraggedTransform );

	WBCompRosaCollision* const	pDraggedCollision	= WB_GETCOMP( pDraggedEntity, RosaCollision );
	DEVASSERT( pDraggedCollision );

	Vector						DropLocation		= pTransform->GetLocation() + m_DragDropOffset;
	const Angles				Orientation			= pTransform->GetOrientation().Get2D();
	const Angles				DropOrientation		= Orientation + m_DragDropOrientation;

	Vector						DropImpulse			= Orientation.ToVector();
	DropImpulse.z									+= m_DragDropSpawnImpulseZ;
	DropImpulse.FastNormalize();
	DropImpulse										*= m_DragDropSpawnImpulse;

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pEntity;	// Using the player, not the dragged entity
	Info.m_In_UserFlags			= EECF_EntityCollision;

	const bool FoundSpot		= GetWorld()->FindSpot( DropLocation, pDraggedCollision->GetExtents(), Info );
	Unused( FoundSpot );
	DEVASSERT( FoundSpot );

	pDraggedTransform->SetLocation(		DropLocation );
	pDraggedTransform->SetVelocity(		pTransform->GetVelocity() );
	pDraggedTransform->SetOrientation(	DropOrientation );
	pDraggedTransform->ApplyImpulse(	DropImpulse );
	pDraggedTransform->OnTeleport();
}

void WBCompRosaPlayer::EndDrag()
{
	DEVASSERT( m_IsDragging );

	DropDraggedEntity();

	m_IsDragging	= false;
	m_DraggedEntity	= static_cast<WBEntity*>( NULL );

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );
	DEVASSERT( pStatMod );

	STATIC_HASHED_STRING( Dragging );
	pStatMod->UnTriggerEvent( sDragging );

	GetFramework()->GetInputSystem()->PopContext( m_DragInputContext );
}

void WBCompRosaPlayer::RestoreDrag()
{
	WBEntity* const			pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompStatMod* const	pStatMod	= WB_GETCOMP( pEntity, StatMod );
	DEVASSERT( pStatMod );

	STATIC_HASHED_STRING( Dragging );
	pStatMod->SetEventActive( sDragging, m_IsDragging );

	if( m_IsDragging )
	{
		GetFramework()->GetInputSystem()->PushContext( m_DragInputContext );
	}
}

bool WBCompRosaPlayer::ShouldAttachToClimbable( const WBEvent& ClimbEvent )
{
	if( m_ClimbRefs > 0 )
	{
		// If we're already climbing, always attach to any further climbables.
		return true;
	}

	STATIC_HASHED_STRING( UseSnapPlane );
	const bool			UseSnapPlane				= ClimbEvent.GetBool( sUseSnapPlane );

	if( !UseSnapPlane )
	{
		// We have no snap plane, always attach.
		return true;
	}

	WBEntity* const				pEntity			= GetEntity();

	WBCompRosaCollision* const	pCollision		= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	if( !pCollision->IsLanded() )
	{
		// Always attach if jumping or falling.
		return true;
	}

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector	Orientation					= pTransform->GetOrientation().ToVector();
	const Vector	VelocityNormal				= pTransform->GetVelocity().GetFastNormalized();

	STATIC_HASHED_STRING( SnapPlaneNormal );
	const Vector	SnapPlaneNormal				= ClimbEvent.GetVector( sSnapPlaneNormal );

	// Cheap 90 degree rotation to get facing plane from snap plane
	ASSERT( SnapPlaneNormal.z == 0.0f );
	const Vector	FacingPlaneNormal			= Vector( -SnapPlaneNormal.y, SnapPlaneNormal.x, 0.0f );

	// TODO: Configurate if needed.
	static const float	kCos90	= 0.0f;	// Climbing angle
	const float			FacingDot = Orientation.Dot( FacingPlaneNormal );
	const float			MovingDot = VelocityNormal.Dot( FacingPlaneNormal );
	if( FacingDot < kCos90 &&
		MovingDot < kCos90 )
	{
		// We're facing the snap plane and moving toward the snap plane, so we should attach.
		return true;
	}

	// All cases failed, don't attach.
	return false;
}

void WBCompRosaPlayer::IncrementClimbRefs( const WBEvent& ClimbEvent )
{
	if( ++m_ClimbRefs == 1 )
	{
		if( !m_IsMantling && !m_IsDragging )
		{
			BeginClimb( ClimbEvent );
		}
	}
}

void WBCompRosaPlayer::DecrementClimbRefs( const bool AddClimbOffImpulse )
{
	if( m_ClimbRefs > 0 )
	{
		if( --m_ClimbRefs == 0 )
		{
			if( m_IsClimbing )
			{
				EndClimb( AddClimbOffImpulse );
			}
		}
	}
}

void WBCompRosaPlayer::ZeroClimbRefs()
{
	if( m_ClimbRefs > 0 )
	{
		m_ClimbRefs = 0;

		if( m_IsClimbing )
		{
			EndClimb( false );
		}
	}
}

void WBCompRosaPlayer::BeginClimb( const WBEvent& ClimbEvent )
{
	if( m_IsPowerSliding )
	{
		EndPowerSlide();
	}

	if( m_IsCrouched )
	{
		BeginUncrouch();
	}

	ASSERT( !m_IsClimbing );

	m_HasDoubleJumped = false;
	m_IsClimbing = true;

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompStatMod* const		pStatMod		= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	STATIC_HASHED_STRING( UseSnapPlane );
	const bool					UseSnapPlane	= ClimbEvent.GetBool( sUseSnapPlane );

	if( UseSnapPlane )
	{
		STATIC_HASHED_STRING( SnapPlaneDistance );
		const float		SnapPlaneDistance	= ClimbEvent.GetFloat( sSnapPlaneDistance );

		STATIC_HASHED_STRING( SnapPlaneNormal );
		const Vector	SnapPlaneNormal		= ClimbEvent.GetVector( sSnapPlaneNormal );

		const Plane		SnapPlane			= Plane( SnapPlaneNormal, SnapPlaneDistance );
		const Vector	Location			= pTransform->GetLocation();
		const Vector	SnappedLocation		= SnapPlane.ProjectPoint( Location );

		pTransform->SetLocation( SnappedLocation );
	}

	pTransform->SetGravity( 0.0f );

	pTransform->SetVelocity( Vector() );
	pTransform->SetAcceleration( Vector() );
	pTransform->SetRotationalVelocity( Angles() );

	STATIC_HASHED_STRING( Climbing );
	pStatMod->TriggerEvent( sClimbing );

	GetFramework()->GetInputSystem()->PushContext( m_ClimbInputContext );

	WB_MAKE_EVENT( HideHands, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), HideHands, pEntity );
}

void WBCompRosaPlayer::EndClimb( const bool AddClimbOffImpulse )
{
	ASSERT( m_IsClimbing );

	m_IsClimbing = false;

	GetFramework()->GetInputSystem()->PopContext( m_ClimbInputContext );

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	pTransform->SetDefaultGravity();

	if( AddClimbOffImpulse )
	{
		pTransform->ApplyImpulse( Vector( 0.0f, 0.0f, m_ClimbOffImpulse ) );
	}

	STATIC_HASHED_STRING( Climbing );
	pStatMod->UnTriggerEvent( sClimbing );

	WB_MAKE_EVENT( ShowHands, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), ShowHands, pEntity );
}

void WBCompRosaPlayer::RestoreClimb()
{
	m_IsClimbing = ( m_ClimbRefs > 0 );

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pEntity, StatMod );
	ASSERT( pStatMod );

	STATIC_HASHED_STRING( Climbing );
	pStatMod->SetEventActive( sClimbing, m_IsClimbing );

	if( m_IsClimbing )
	{
		GetFramework()->GetInputSystem()->PushContext( m_ClimbInputContext );
	}
}

void WBCompRosaPlayer::TryBeginMantle( const Vector& CollisionNormal )
{
	if( m_IsMantling || !m_CanMantle || m_ClimbRefs > 0 )
	{
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision		= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	InputSystem* const			pInputSystem	= GetFramework()->GetInputSystem();
	ASSERT( pInputSystem );

	STATIC_HASHED_STRING( Jump );

	// TODO: Configurate if needed.
	static const float kCos45	= 0.707107f;	// Mantle angle
	const Vector Facing			= pTransform->GetOrientation().ToVector2D();

	// If we're facing the collision surface and falling and holding the jump button, try to mantle.
	if( pCollision->IsRecentlyLanded( 0.0f )		||
		pTransform->GetVelocity().z >= 0.0f			||
		Facing.Dot( CollisionNormal ) >= -kCos45	||
		!pInputSystem->IsHigh( sJump )				)
	{
		return;
	}

	RosaWorld* const	pWorld				= GetWorld();
	DEVASSERT( pWorld );

	const Vector		MantleDestination	= GetMantleDestination( CollisionNormal, pTransform->GetLocation(), pCollision->GetExtents() );

	CollisionInfo		ClearanceInfo;
	ClearanceInfo.m_In_CollideWorld		= true;
	ClearanceInfo.m_In_CollideEntities	= true;
	ClearanceInfo.m_In_CollidingEntity	= pEntity;
	// Historically, this was always EECF_BlockerCollision (same as below but also colliding with dynamic entities);
	// now I'm choosing to not collide with dynamic entities so the player can smash through windows. Potentially
	// this means it could try to mantel the player up into an enemy or something, but that won't cause stuck bugs.
	ClearanceInfo.m_In_UserFlags		= EECF_CollideAsBlocker | EECF_CollideStaticEntities;

	// Check that we still have room at our destination.
	if( !pWorld->CheckClearance( MantleDestination, pCollision->GetExtents(), ClearanceInfo ) )
	{
		BeginMantle( MantleDestination );
		return;
	}

	// Something is blocking the mantle; crouch and retry
	if( m_IsCrouched || !CanCrouch() )
	{
		return;
	}

	Vector			CrouchExtents			= pCollision->GetExtents();
	CrouchExtents.z = m_CrouchExtentsZ;
	const Vector	CrouchMantleDestination	= GetMantleDestination( CollisionNormal, pTransform->GetLocation(), CrouchExtents );
	if( !pWorld->CheckClearance( CrouchMantleDestination, CrouchExtents, ClearanceInfo ) )
	{
		Crouch();
		m_IsForceCrouched = true;

		BeginMantle( CrouchMantleDestination );
		return;
	}
}

Vector WBCompRosaPlayer::GetMantleDestination( const Vector& CollisionNormal, const Vector& Location, const Vector& Extents ) const
{
	const float		MantleDestinationZ	= Location.z + ( Extents.z * 1.5f );					// We can climb up to 1.5x our height (TODO: configurate? might require a sweep down to be sure we don't overshoot)
	const Vector	MantleDestinationXY	= Location - ( CollisionNormal * Extents.x * 1.414f );	// Multiply by 1.414 to be sure we'll have clearance on any angle
	const Vector	MantleDestination	= Vector( MantleDestinationXY.x, MantleDestinationXY.y, MantleDestinationZ );

	return MantleDestination;
}

void WBCompRosaPlayer::BeginMantle( const Vector& MantleDestination )
{
	ASSERT( !m_IsMantling );
	m_IsMantling = true;
	m_HasDoubleJumped = true;	// HACKHACK: We don't want to double jump during the pop off a mantel

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	GetFramework()->GetInputSystem()->PushContext( m_MantleInputContext );

	m_MantleDestination	= MantleDestination;

	// Make sure m_MantleVector has a significant component in both X/Y and Z.
	// As such, it does *not* indicate the actual direction to the mantle point.
	m_MantleVector		= ( MantleDestination - pTransform->GetLocation() ).GetNormalized().Get2D();
	m_MantleVector.z	= 1.0f;
	m_MantleVector.FastNormalize();

	m_CanMantle = false;

	WB_MAKE_EVENT( OnBeginMantle, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnBeginMantle, pEntity );
}

void WBCompRosaPlayer::EndMantle( const bool AllowMantle )
{
	ASSERT( m_IsMantling );
	m_IsMantling = false;

	// DLP 18 May 2020: This used to only become true on landing;
	// I'm now making it so you can continue to mantle into any
	// other surface you encounter before landing, which feels
	// a lot better because you keep holding the inputs and keep
	// mantling as long as there's space to do so. I haven't seen
	// any issues with it yet, just leaving this note here as a
	// reminder in case anything wonky happens later.
	// (Addendum: I added the AllowMantle flag so this does *not*
	// get enabled if EndMantle is called because the input was
	// released during mantling; this fixes odd repeat mantling
	// if spamming the space bar to bunny hop.)
	m_CanMantle = AllowMantle;

	GetFramework()->GetInputSystem()->PopContext( m_MantleInputContext );

	if( m_IsForceCrouched )
	{
		BeginUncrouch();
	}
}

void WBCompRosaPlayer::TickScaleFOV( Interpolator<float>& FOVInterpolator, const float BaseFOVDegrees, const float Scale, const float Time, const float DeltaTime )
{
	const float HalfFOVRadians			= DEGREES_TO_RADIANS( 0.5f * BaseFOVDegrees );
	const float HalfScaledFOVRadians	= ATan( Scale * Tan( HalfFOVRadians ) );
	const float ScaledFOVDegrees		= 2.0f * RADIANS_TO_DEGREES( HalfScaledFOVRadians );

	if( FOVInterpolator.GetValue() == 0.0f )
	{
		// Instantly reset FOV to the target if we don't have a valid current value.
		FOVInterpolator.Reset( ScaledFOVDegrees );
	}

	if( FOVInterpolator.GetEndValue() != ScaledFOVDegrees )
	{
		// If the target FOV changes, restart the lerp.
		FOVInterpolator.Reset( Interpolator<float>::EIT_Linear, FOVInterpolator.GetValue(), ScaledFOVDegrees, Time );
	}

	FOVInterpolator.Tick( DeltaTime );
}

void WBCompRosaPlayer::SetSpawnPoint()
{
	ASSERT( !m_HasSetSpawnPoint );

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	m_SpawnLocation		= pTransform->GetLocation();
	m_SpawnOrientation	= pTransform->GetOrientation();
	m_HasSetSpawnPoint	= true;
}

void WBCompRosaPlayer::RestoreSpawnPoint()
{
	ASSERT( m_HasSetSpawnPoint );

	WBEntity* const				pEntity		= GetEntity();
	ASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	RosaWorld* const pWorld = GetWorld();
	DEVASSERT( pWorld );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pEntity;
	Info.m_In_UserFlags			= EECF_BlockerCollision;
	Vector SpawnLocation		= m_SpawnLocation;
	const bool FoundSpot		= pWorld->FindSpot( SpawnLocation, pCollision->GetExtents(), Info );
	ASSERT( FoundSpot );

	pTransform->SetLocation(		SpawnLocation );
	pTransform->SetVelocity(		Vector() );
	pTransform->SetAcceleration(	Vector() );
	pTransform->SetOrientation(		m_SpawnOrientation );

	if( m_IsCrouched )
	{
		BeginUncrouch();
	}
}

void WBCompRosaPlayer::TeleportTo( const HashedString& TeleportLabel )
{
	if( TeleportLabel == HashedString::NullString )
	{
		return;
	}

	WBEntity* const pTeleportTarget = WBCompLabel::GetEntityByLabel( TeleportLabel );
	if( !pTeleportTarget )
	{
		return;
	}

	WBCompRosaTransform* pTeleportTransform = pTeleportTarget->GetTransformComponent<WBCompRosaTransform>();
	if( !pTeleportTransform )
	{
		return;
	}

	WBCompRosaTransform* pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pTransform );

	pTransform->SetLocation(		pTeleportTransform->GetLocation() );
	pTransform->SetVelocity(		Vector() );
	pTransform->SetAcceleration(	Vector() );
	pTransform->SetOrientation(		pTeleportTransform->GetOrientation() );
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaPlayer::Report() const
{
	Super::Report();

	PRINTF( WBPROPERTY_REPORT_PREFIX "Autosave Suppression Refcount: %d/%d\n", m_AutosaveSuppRefsSerialized, m_AutosaveSuppRefsTransient );
}

/*virtual*/ void WBCompRosaPlayer::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const		pFramework		= GetFramework();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();
	View* const					pView			= pFramework->GetMainView();
	Display* const				pDisplay		= pFramework->GetDisplay();

	WBCompRosaTransform* const	pTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector				Location		= pTransform->GetLocation();

	const float					ClockMultiplier	= m_DEVHACKClockMultiplier ? m_DEVHACKClockMultiplier->m_Multiplier : 1.0f;
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s  Clock multiplier: %.2f", DebugRenderLineFeed().CStr(), ClockMultiplier ), Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 255, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s  Fixed time scalar: %.2f", DebugRenderLineFeed().CStr(), pFramework->DEV_GetFixedTimeScalar() ), Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 255, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );

	// (These are going to be the previous frame's stats, obviously.)
	const IRenderer::SDEV_RenderStats& RenderStats	= pRenderer->DEV_GetStats();

	// Print renderer stats in the top left, not on the player entity.
	// TODO: I don't feel like writing a whole system to display real-time stats from multiple sources right now. But I should.
	pRenderer->DEBUGPrint( SimpleString::PrintF(
		"Avg FPS: %d\nAvg frame time: %.3fms\nRender time: %.3fms\nRendered meshes: %d\nRendered draw calls: %d\n"
		"Rendered shadow lights: %d\nRendered shadow meshes: %d\nShadow time: %.3fms\n"
		"TickSim time: %.3fms (min %.3fms / max %.3fms)\n"
		"TickRender time: %.3fms (min %.3fms / max %.3fms)",
		RoundToUInt( RenderStats.m_AvgFPS ), 1000.0f / RenderStats.m_AvgFPS, GET_CLOCK_MS( RenderStats.m_RenderTime ), RenderStats.m_NumMeshes, RenderStats.m_NumDrawCalls,
		RenderStats.m_NumShadowLights, RenderStats.m_NumShadowMeshes, GET_CLOCK_MS( RenderStats.m_ShadowTime ),
		GET_CLOCK_MS( pFramework->DEV_GetSimTime() ), GET_CLOCK_MS( pFramework->DEV_GetSimTimeMin() ), GET_CLOCK_MS( pFramework->DEV_GetSimTimeMax() ),
		GET_CLOCK_MS( pFramework->DEV_GetRenderTime() ), GET_CLOCK_MS( pFramework->DEV_GetRenderTimeMin() ), GET_CLOCK_MS( pFramework->DEV_GetRenderTimeMax() ) ),
		SRect( 100.0f, 100.0f, 0.0f, 0.0f ), DEFAULT_MONOSPACED_FONT_TAG, ARGB_TO_COLOR( 255, 255, 255, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );

	// Display relevant shadow-casting lights and the number of meshes drawn for each.
	// It makes sense to do this here instead of a debug display on the light component,
	// because we want to see all lights, and also we don't have rendered numbers on the
	// light components. I might comment this out too.
	FOR_EACH_ARRAY( ShadowLightIter, RenderStats.m_ShadowLights, IRenderer::SDEV_RenderStats::SDEV_ShadowLight )
	{
		const IRenderer::SDEV_RenderStats::SDEV_ShadowLight& ShadowLight = ShadowLightIter.GetValue();
		//pRenderer->DEBUGDrawSphere( ShadowLight.m_Location, ShadowLight.m_Radius, ARGB_TO_COLOR( 255, 255, 255, 255 ), false );
		pRenderer->DEBUGDrawCross( ShadowLight.m_Location, 1.0f, ARGB_TO_COLOR( 255, 255, 255, 255 ), false );
		pRenderer->DEBUGPrint(
			SimpleString::PrintF( "%d", ShadowLight.m_NumMeshes ),
			ShadowLight.m_Location,
			pView,
			pDisplay,
			DEFAULT_FONT_TAG,
			ARGB_TO_COLOR( 255, 255, 255, 0 ),
			ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	}

	const uint NumPathSteps = m_CAMHACKPathData.m_Path.Size();
	for( uint PathIndex = 1; PathIndex < NumPathSteps; ++PathIndex )
	{
		const Vector& PrevStep = m_CAMHACKPathData.m_Path[ PathIndex - 1 ];
		const Vector& NextStep = m_CAMHACKPathData.m_Path[ PathIndex ];

		pRenderer->DEBUGDrawLine( PrevStep, NextStep, ARGB_TO_COLOR( 255, 255, 0, 0 ) );
	}

	uint NavNodeIndex;
	if( GetWorld()->FindNavNodeUnder( Location, NavNodeIndex ) )
	{
		static const Vector	skNavNodeOffset = Vector( 0.0f, 0.0f, 0.01f );
		const SNavNode& NavNode = GetWorld()->GetNavNode( NavNodeIndex );

		// Draw the tri raised slightly and with each vert moved slightly toward centroid so it doesn't z-fight geo
		pRenderer->DEBUGDrawTriangle(
			NavNode.m_Tri.m_Vec1 + skNavNodeOffset + 0.01f * ( NavNode.m_Centroid - NavNode.m_Tri.m_Vec1 ).GetFastNormalized(),
			NavNode.m_Tri.m_Vec2 + skNavNodeOffset + 0.01f * ( NavNode.m_Centroid - NavNode.m_Tri.m_Vec2 ).GetFastNormalized(),
			NavNode.m_Tri.m_Vec3 + skNavNodeOffset + 0.01f * ( NavNode.m_Centroid - NavNode.m_Tri.m_Vec3 ).GetFastNormalized(),
			ARGB_TO_COLOR( 255, 255, 128, 0 ) );

		if( NavNode.m_Props != ENP_None )
		{
			pRenderer->DEBUGPrint( SimpleString::PrintF( "%sNav node props: 0x%08X", DebugRenderLineFeed().CStr(), NavNode.m_Props ), Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 128, 0 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
		}
	}

	for( float Radius = m_CACHED_LastAINoiseRadius; Radius > 0.0f; Radius -= 1.0f )
	{
		pRenderer->DEBUGDrawCircleXY( m_CACHED_LastAINoiseSourceLocation, Radius, ARGB_TO_COLOR( 255, 255, 255, 0 ) );
	}
}
#endif	// BUILD_DEV

#if BUILD_DEV
void WBCompRosaPlayer::DEVHACKInput()
{
	WBEntity* const				pEntity		= GetEntity();
	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	WBCompRosaCamera* const		pCamera		= WB_GETCOMP( pEntity, RosaCamera );
	WBCompRosaKeyRing* const	pKeyRing	= WB_GETCOMP( pEntity, RosaKeyRing );
	WBCompRosaVisible* const	pVisible	= WB_GETCOMP( pEntity, RosaVisible );
	WBCompRosaFootsteps* const	pFootsteps	= WB_GETCOMP( pEntity, RosaFootsteps );
	RosaWorld* const			pWorld		= GetWorld();
	RosaFramework* const		pFramework	= GetFramework();
	Keyboard* const				pKeyboard	= pFramework->GetKeyboard();

	// G (no Alt): Hide hands and HUD, Shift + G: re-show
	if( pKeyboard->IsLow( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_G ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			WB_MAKE_EVENT( ForceShowHands, pEntity );
			WB_DISPATCH_EVENT( GetEventManager(), ForceShowHands, pEntity );

			STATIC_HASHED_STRING( HUD );
			WB_MAKE_EVENT( RepushUIScreen, NULL );
			WB_SET_AUTO( RepushUIScreen, Hash, Screen, sHUD );
			WB_DISPATCH_EVENT( GetEventManager(), RepushUIScreen, NULL );
		}
		else
		{
			WB_MAKE_EVENT( HideHands, pEntity );
			WB_DISPATCH_EVENT( GetEventManager(), HideHands, pEntity );

			STATIC_HASHED_STRING( HUD );
			WB_MAKE_EVENT( RemoveUIScreen, NULL );
			WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sHUD );
			WB_DISPATCH_EVENT( GetEventManager(), RemoveUIScreen, NULL );
		}
	}

#if BUILD_WINDOWS
	if( pKeyboard->OnRise( Keyboard::EB_Grave ) )
	{
		Console::Toggle();
	}
#endif

	if( pKeyboard->OnRise( Keyboard::EB_LeftBrace ) )
	{
		m_DebugSpawnerIndex = ( m_DebugSpawnerIndex + m_DebugSpawners.Size() - 1 ) % m_DebugSpawners.Size();
		PRINTF( "Next entity: %s\n", m_DebugSpawners[ m_DebugSpawnerIndex ].m_Entity.CStr() );
	}

	if( pKeyboard->OnRise( Keyboard::EB_RightBrace ) )
	{
		m_DebugSpawnerIndex = ( m_DebugSpawnerIndex + 1 ) % m_DebugSpawners.Size();
		PRINTF( "Next entity: %s\n", m_DebugSpawners[ m_DebugSpawnerIndex ].m_Entity.CStr() );
	}

	// Numpad +/-: time dilation (use Ctrl to test clock multiplier instead of fixed frame time hack; ideal for slowing the game instead of speeding it up)
	if( pKeyboard->OnRise( Keyboard::EB_NumAdd ) || pKeyboard->OnRise( Keyboard::EB_NumSubtract ) )
	{
		const float Scalar			= pKeyboard->OnRise( Keyboard::EB_NumAdd ) ? 2.0f : 0.5f;
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			const float NewMultiplier	= m_DEVHACKClockMultiplier ? ( Scalar * m_DEVHACKClockMultiplier->m_Multiplier ) : Scalar;
			if( m_DEVHACKClockMultiplier )
			{
				pFramework->GetClock()->RemoveMultiplierRequest( &m_DEVHACKClockMultiplier );
			}
			if( NewMultiplier != 1.0f )
			{
				m_DEVHACKClockMultiplier = pFramework->GetClock()->AddMultiplierRequest( 0.0f, NewMultiplier );
			}
		}
		else
		{
			pFramework->DEV_SetFixedTimeScalar( Scalar * pFramework->DEV_GetFixedTimeScalar() );
		}
	}

	// Right shift: debug render targeted entity
	// (This is pretty basic, but it works to debug enemies, which is the main purpose)
	if( pKeyboard->OnRise( Keyboard::EB_RightShift ) )
	{
		WBEntityRef		OldTarget	= m_DEVHACKDebugTarget;
		m_DEVHACKDebugTarget		= NULL;

		// DLP 5 Dec 2021: First try the frob and aim targets, then trace if needed
		WBCompRosaFrobber* const	pFrobber	= WB_GETCOMP( pEntity, RosaFrobber );
		DEVASSERT( pFrobber );

		m_DEVHACKDebugTarget		= pFrobber->GetFrobTarget();

		if( !m_DEVHACKDebugTarget )
		{
			m_DEVHACKDebugTarget	= pFrobber->GetAimTarget();
		}

		if( !m_DEVHACKDebugTarget )
		{
			const Vector	EyeLoc		= pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pTransform->GetLocation() );
			const Vector	EyeDir		= pCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, pTransform->GetOrientation() ).ToVector();
			const Ray		TraceRay	= Ray( EyeLoc, EyeDir );

			CollisionInfo Info;
			Info.m_In_CollideWorld		= true;
			Info.m_In_CollideEntities	= true;
			Info.m_In_CollidingEntity	= GetEntity();
			Info.m_In_UserFlags			= EECF_Trace | EECF_CollideBones;

			if( pWorld->Trace( TraceRay, Info ) )
			{
				WBEntity* const pHitEntity = static_cast<WBEntity*>( Info.m_Out_HitEntity );
				m_DEVHACKDebugTarget = pHitEntity;
			}
		}

		// Targeting nothing; focus on self, then unfocus
		if( !m_DEVHACKDebugTarget )
		{
			if( OldTarget == GetEntity() )
			{
				// Do nothing (unfocus)
			}
			else
			{
				m_DEVHACKDebugTarget = GetEntity();
			}
		}

		if( OldTarget != m_DEVHACKDebugTarget )
		{
			if( OldTarget.Get() )
			{
				OldTarget.Get()->SetShouldDebugRender( false );
			}

			if( m_DEVHACKDebugTarget )
			{
				m_DEVHACKDebugTarget.Get()->SetShouldDebugRender( true );
			}
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_PageDown ) )
	{
		if( m_DEVHACKDebugTarget )
		{
			m_DEVHACKDebugTarget.Get()->GoToNextDebugRenderComponent();
		}
		else
		{
			m_DEVHACKDebugTarget = GetEntity();
			m_DEVHACKDebugTarget.Get()->SetShouldDebugRender( true );
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_PageUp ) )
	{
		if( m_DEVHACKDebugTarget )
		{
			m_DEVHACKDebugTarget.Get()->GoToPrevDebugRenderComponent();
		}
		else
		{
			m_DEVHACKDebugTarget = GetEntity();
			m_DEVHACKDebugTarget.Get()->SetShouldDebugRender( true );
			m_DEVHACKDebugTarget.Get()->GoToPrevDebugRenderComponent();
		}
	}

	// X: Teleport to trace destination
	if( pKeyboard->OnRise( Keyboard::EB_X ) )
	{
		const Vector	EyeLoc		= pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pTransform->GetLocation() );
		const Vector	EyeDir		= pCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, pTransform->GetOrientation() ).ToVector();
		const Ray		TraceRay	= Ray( EyeLoc, EyeDir );

		CollisionInfo Info;
		Info.m_In_CollideWorld		= true;
		Info.m_In_CollideEntities	= true;
		Info.m_In_CollidingEntity	= GetEntity();
		Info.m_In_UserFlags			= EECF_BlockerCollision;

		if( pWorld->Trace( TraceRay, Info ) )
		{
			Vector Destination = Info.m_Out_Intersection + Info.m_Out_Plane.m_Normal * 0.1f;
			if( pWorld->FindSpot( Destination, pCollision->GetExtents(), Info ) )
			{
				pTransform->SetLocation( Destination );
			}
		}
	}

	// Alt + K: Kill all enemies
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) &&
		pKeyboard->OnRise( Keyboard::EB_K ) &&
		!pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) &&
		!pKeyboard->IsHigh( Keyboard::EB_V ) )
	{
		Array<WBEntity*> AllEntities;
		WBScene::GetDefault()->GetAllEntities( AllEntities );
		FOR_EACH_ARRAY( EntityIter, AllEntities, WBEntity* )
		{
			WBEntity* const pIterEntity = EntityIter.GetValue();

			if( pIterEntity == pEntity )
			{
				continue;
			}

			if( WBCompRosaFaction::GetCon( pEntity, pIterEntity ) != RosaFactions::EFR_Hostile )
			{
				continue;
			}

			WB_MAKE_EVENT( Kill, pIterEntity );
			WB_SET_AUTO( Kill, Entity, Damager, pEntity );
			WB_DISPATCH_EVENT( GetEventManager(), Kill, pIterEntity );
		}
	}

	// Alt + L: Print player location and heading to log
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_L ) && !pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
	{
		PRINTF( "Player location:  %s\n", pTransform->GetLocation().GetString().CStr() );
		PRINTF( "Player direction: %s\n", pTransform->GetOrientation().ToVector().GetString().CStr() );
	}

	// Alt + M: Give master key
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_M ) )
	{
		STATIC_HASHED_STRING( Keycard_MASTER );
		pKeyRing->AddKeycard( sKeycard_MASTER, true );
	}

	// Alt + O: Toggle invisible, silent, noclip
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_O ) )
	{
		const bool CheatOn = pVisible->IsVisible();

		pVisible->SetVisible( !CheatOn );

		if( pFootsteps )
		{
			pFootsteps->SetFootstepsDisabled( CheatOn );
		}
	}

	// V: Spawn entity
	if( !pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_V ) )
	{
		const Vector EyeLoc = pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pTransform->GetLocation() );
		const Vector EyeDir = pCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, pTransform->GetOrientation() ).ToVector();
		const Ray TraceRay( EyeLoc, EyeDir );

		CollisionInfo TraceInfo;
		TraceInfo.m_In_CollideWorld = true;
		TraceInfo.m_In_UserFlags	= EECF_CollideAsEntity;

		if( pWorld->Trace( TraceRay, TraceInfo ) )
		{
			const SDebugSpawner&	Spawner				= m_DebugSpawners[ m_DebugSpawnerIndex ];
			WBEntity*				pSpawnedEntity		= WBWorld::GetInstance()->CreateEntity( Spawner.m_Entity );
			WBCompRosaTransform*	pSpawnedTransform	= pSpawnedEntity->GetTransformComponent<WBCompRosaTransform>();
			if( pSpawnedTransform )
			{
				Vector					SpawnLocation		= TraceInfo.m_Out_Intersection + TraceInfo.m_Out_Plane.m_Normal * Spawner.m_NormalDistance * pSpawnedTransform->GetScale() + Spawner.m_Offset;
				WBCompRosaCollision*	pSpawnedCollision	= WB_GETCOMP( pSpawnedEntity, RosaCollision );
				if( pSpawnedCollision )
				{
					CollisionInfo FindSpotInfo;
					FindSpotInfo.m_In_CollideWorld		= true;
					FindSpotInfo.m_In_CollideEntities	= true;
					FindSpotInfo.m_In_CollidingEntity	= pSpawnedEntity;
					FindSpotInfo.m_In_UserFlags			= EECF_EntityCollision;
					pWorld->FindSpot( SpawnLocation, pSpawnedCollision->GetExtents(), FindSpotInfo );
				}
				pSpawnedTransform->SetInitialTransform( SpawnLocation, Angles() );
			}
		}
	}

	// Alt+LMB: Test pathfinding from current loc to wherever we're aiming
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
	{
		const Vector EyeLoc = pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pTransform->GetLocation() );
		const Vector EyeDir = pCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, pTransform->GetOrientation() ).ToVector();
		const Ray TraceRay( EyeLoc, EyeDir );

		CollisionInfo Info;
		Info.m_In_CollideWorld	= true;
		Info.m_In_UserFlags		= EECF_CollideAsEntity;

		if( pWorld->Trace( TraceRay, Info ) )
		{
			const Vector HitLoc = Info.m_Out_Intersection + Info.m_Out_Plane.m_Normal * 0.1f;
			m_CAMHACKPathData.m_Params.m_PathMode		= RosaNav::EPM_Search;
			m_CAMHACKPathData.m_Params.m_Start			= pTransform->GetLocation();
			m_CAMHACKPathData.m_Params.m_Destination	= HitLoc;
			m_CAMHACKPathData.m_Params.m_AgentHeight	= pCollision->GetExtents().z * 2.0f;
			m_CAMHACKPathData.m_Params.m_AgentRadius	= pCollision->GetExtents().x * 1.414f;
			m_CAMHACKPathData.m_Params.m_MotionType		= RosaNav::EMT_Walking;
			m_CAMHACKPathData.m_Params.m_CanOpenDoors	= true;
			m_CAMHACKPathData.m_Params.m_MaxSteps		= 1000;
			m_CAMHACKPathData.m_Params.m_UsePartialPath	= true;
#if BUILD_DEBUG
			m_CAMHACKPathData.m_Params.m_Verbose		= true;
#endif

			RosaNav::GetInstance()->FindPath( m_CAMHACKPathData );
		}
	}

	// Alt + G: ghost (no collision, no gravity), Shift + Alt + G: disable ghost
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_G ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			pCollision->ResetCollisionFlags();

			WB_MAKE_EVENT( StopFlying, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), StopFlying, GetEntity() );
		}
		else
		{
			pCollision->SetCollisionFlags( 0 );

			WB_MAKE_EVENT( StartFlying, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), StartFlying, GetEntity() );
		}
	}

	// Camera hacks: Alt + V + ... (Used to be Shift + C + ...)
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->IsHigh( Keyboard::EB_V ) )
	{
		//WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		//WBCompRosaCollision* const	pCollision	= WB_GETCOMP( GetEntity(), RosaCollision );
		//WBCompRosaHealth* const		pHealth		= WB_GETCOMP( GetEntity(), RosaHealth );
		//WBCompRosaWallet* const		pWallet		= WB_GETCOMP( GetEntity(), RosaWallet );
		//WBCompRosaKeyRing* const		pKeyRing	= WB_GETCOMP( GetEntity(), RosaKeyRing );

		if( pKeyboard->OnRise( Keyboard::EB_U ) )
		{
			m_CAMHACKCamVelocity *= 0.8f;
			PRINTF( "m_CAMHACKCamVelocity: %f\n", m_CAMHACKCamVelocity );
		}

		if( pKeyboard->OnRise( Keyboard::EB_P ) )
		{
			m_CAMHACKCamVelocity *= 1.25f;
			PRINTF( "m_CAMHACKCamVelocity: %f\n", m_CAMHACKCamVelocity );
		}

		if( pKeyboard->OnRise( Keyboard::EB_J ) )
		{
			m_CAMHACKCamStartLocation		= pTransform->GetLocation();
			m_CAMHACKCamStartOrientation	= pTransform->GetOrientation();
			PRINTF( "Start point set\n" );
		}

		if( pKeyboard->OnRise( Keyboard::EB_K ) )
		{
			m_CAMHACKCamEndLocation		= pTransform->GetLocation();
			m_CAMHACKCamEndOrientation	= pTransform->GetOrientation();
			PRINTF( "End point set\n" );
		}

		if( pKeyboard->OnRise( Keyboard::EB_L ) )
		{
			PRINTF( "Toggled\n" );
			m_CAMHACKCamActive = !m_CAMHACKCamActive;

			if( m_CAMHACKCamActive )
			{
				if( m_CAMHACKCamStartLocation.IsZero() || m_CAMHACKCamEndLocation.IsZero() )
				{
					// Don't interpolate, it will cause problems.
					m_CAMHACKCamActive = false;
				}
				else
				{
					pCollision->SetCollisionFlags( 0 );
					pTransform->SetGravity( 0.0f );

					if( pFootsteps )
					{
						pFootsteps->SetFootstepsDisabled( true );
					}

					WB_MAKE_EVENT( HideHands, GetEntity() );
					WB_DISPATCH_EVENT( GetEventManager(), HideHands, GetEntity() );

					const float Distance = ( m_CAMHACKCamStartLocation - m_CAMHACKCamEndLocation ).Length();
					const float Duration = Distance / m_CAMHACKCamVelocity;
					m_CAMHACKCamLocation.Reset( Interpolator<Vector>::EIT_Linear, m_CAMHACKCamStartLocation, m_CAMHACKCamEndLocation, Duration );
					m_CAMHACKCamOrientation.Reset( Interpolator<Angles>::EIT_Linear, m_CAMHACKCamStartOrientation, m_CAMHACKCamEndOrientation, Duration );

					STATIC_HASHED_STRING( HUD );
					WB_MAKE_EVENT( RemoveUIScreen, NULL );
					WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sHUD );
					WB_DISPATCH_EVENT( GetEventManager(), RemoveUIScreen, NULL );
				}
			}
			else
			{
				pCollision->ResetCollisionFlags();
				pTransform->SetDefaultGravity();

				if( pFootsteps )
				{
					pFootsteps->SetFootstepsDisabled( false );
				}

				WB_MAKE_EVENT( ShowHands, GetEntity() );
				WB_DISPATCH_EVENT( GetEventManager(), ShowHands, GetEntity() );

				STATIC_HASHED_STRING( HUD );
				WB_MAKE_EVENT( RepushUIScreen, NULL );
				WB_SET_AUTO( RepushUIScreen, Hash, Screen, sHUD );
				WB_DISPATCH_EVENT( GetEventManager(), RepushUIScreen, NULL );
			}
		}
	}
}
#endif // BUILD_DEV

/*virtual*/ void WBCompRosaPlayer::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	// Forward all events the player receives to the game and world
	GetGame()->HandleEvent( Event );
	GetWorld()->HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( OnLoaded );
	STATIC_HASHED_STRING( CycleMenuDifficulty );
	STATIC_HASHED_STRING( SetMenuDifficulty );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( OnAutosave );
	STATIC_HASHED_STRING( AddAutosaveSuppression );
	STATIC_HASHED_STRING( RemoveAutosaveSuppression );
	STATIC_HASHED_STRING( StartDrag );
	STATIC_HASHED_STRING( StopDrag );
	STATIC_HASHED_STRING( OnTouchedClimbable );
	STATIC_HASHED_STRING( OnUntouchedClimbable );
	STATIC_HASHED_STRING( OnJumped );
	STATIC_HASHED_STRING( OnLanded );
	STATIC_HASHED_STRING( OnCollided );
	STATIC_HASHED_STRING( OnSteppedUp );
	STATIC_HASHED_STRING( OnKickImpulse );
	STATIC_HASHED_STRING( PreLevelTransition );
	STATIC_HASHED_STRING( PostLevelTransition );
	STATIC_HASHED_STRING( PushInputContext );
	STATIC_HASHED_STRING( PopInputContext );
	STATIC_HASHED_STRING( DisablePause );
	STATIC_HASHED_STRING( EnablePause );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );
	STATIC_HASHED_STRING( OnAINoise );
	STATIC_HASHED_STRING( OnFOVChanged );
	STATIC_HASHED_STRING( SlamFOVScale );
	STATIC_HASHED_STRING( SetInitialMusicTrackBits );
	STATIC_HASHED_STRING( StartFlying );
	STATIC_HASHED_STRING( StopFlying );
	STATIC_HASHED_STRING( OnDied );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawned )
	{
		SetCrouchOverlayHidden( true );
		QueueAutosave( m_RollingAutosaveDelay );

		// Update from difficulty system
		RosaDifficulty::PushMenuToGame();
	}
	else if( EventName == sOnLoaded )
	{
		RestoreCrouch();
		RestorePowerSlide();
		RestoreDrag();
		RestoreClimb();

		// Notify difficulty system (if we're traveling with persistence, we pull that later)
		RosaDifficulty::PushGameToMenu();
	}
	else if( EventName == sCycleMenuDifficulty || EventName == sSetMenuDifficulty )
	{
		// Update from difficulty system
		RosaDifficulty::PushMenuToGame();
	}
	else if( EventName == sOnInitialized )
	{
		// In case user switches away from game and we end up paused
		// before playing new music, stop the old music (and ambience!) immediately.
		WB_MAKE_EVENT( StopMusicAndAmbience, NULL );
		WB_DISPATCH_EVENT( GetEventManager(), StopMusicAndAmbience, GetGame() );

		// HACKHACK: The grossest hack. Since I'm (currently!) only using this to defer
		// music in RosaIntro, ignore it on subsequent initializations so returning to
		// the title will play music the usual way. Gross!
		static bool CanDeferMusic = true;
		if( m_DeferMusic && CanDeferMusic )
		{
			CanDeferMusic = false;
			// Do nothing, something else will play music later
		}
		else
		{
			WB_MAKE_EVENT( PlayMusicAndAmbience, NULL );
			WB_SET_AUTO( PlayMusicAndAmbience, Int, TrackBits, m_InitialMusicTrackBits );
			WB_DISPATCH_EVENT( GetEventManager(), PlayMusicAndAmbience, GetGame() );	// ROSANOTE: Since Eldritch, this had always been delayed by one tick. Can't remember why.
		}
	}
	else if( EventName == sOnAutosave )
	{
		if( CanAutosave() )
		{
			// Queue the next one *before* autosaving so the queued event gets saved.
			QueueAutosave( m_RollingAutosaveDelay );
			GetGame()->Autosave();
		}
		else
		{
			QueueAutosave( m_RetryAutosaveDelay );
		}
	}
	else if( EventName == sAddAutosaveSuppression )
	{
		STATIC_HASHED_STRING( Serialize );
		const bool Serialize = Event.GetBool( sSerialize );

		AddAutosaveSuppression( Serialize );
	}
	else if( EventName == sRemoveAutosaveSuppression )
	{
		STATIC_HASHED_STRING( Serialize );
		const bool Serialize = Event.GetBool( sSerialize );

		RemoveAutosaveSuppression( Serialize );
	}
	else if( EventName == sStartDrag )
	{
		STATIC_HASHED_STRING( DraggedEntity );
		WBEntity* const pDraggedEntity = Event.GetEntity( sDraggedEntity );
		DEVASSERT( pDraggedEntity );

		BeginDrag( pDraggedEntity );
	}
	else if( EventName == sStopDrag )
	{
#if BUILD_DEV
		// Validate that we're stopping the actual dragged entity
		STATIC_HASHED_STRING( DraggedEntity );
		WBEntity* const pDraggedEntity = Event.GetEntity( sDraggedEntity );
		DEVASSERT( pDraggedEntity );
		DEVASSERT( pDraggedEntity == m_DraggedEntity.Get() );
#endif

		EndDrag();
	}
	else if( EventName == sOnTouchedClimbable )
	{
		if( ShouldAttachToClimbable( Event ) )
		{
			IncrementClimbRefs( Event );
		}
	}
	else if( EventName == sOnUntouchedClimbable )
	{
		const bool AddClimbOffImpulse = true;
		DecrementClimbRefs( AddClimbOffImpulse );
	}
	else if( EventName == sOnLanded )
	{
		if( !m_HasSetSpawnPoint )
		{
			SetSpawnPoint();
		}

		m_HasDoubleJumped = false;

		m_CanMantle = true;
		ZeroClimbRefs();

		// Force a footstep on landed, with a timeout to prevent relanded events from making more footsteps
		WBEntity* const				pEntity			= GetEntity();
		WBCompRosaCollision* const	pCollision		= WB_GETCOMP( pEntity, RosaCollision );
		if( !pCollision->IsRecentlyLanded( m_UnlandedForceFootstepWindow ) )
		{
			// Pass landed magnitude through as additional speed
			STATIC_HASHED_STRING( LandedMagnitude );
			const float				LandedMagnitude	= Event.GetFloat( sLandedMagnitude );

			WB_MAKE_EVENT( ForceFootstep, GetEntity() );
			WB_SET_AUTO( ForceFootstep, Float, AdditionalSpeed, LandedMagnitude );
			WB_DISPATCH_EVENT( GetEventManager(), ForceFootstep, GetEntity() );
		}

		if( m_IsForceCrouched )
		{
			BeginUncrouch();
		}
	}
	else if( EventName == sOnCollided )
	{
		if( m_IsPowerSliding )
		{
			EndPowerSlide();
		}

		STATIC_HASHED_STRING( CollidedEntity );
		WBEntity* const				pCollidedEntity		= Event.GetEntity( sCollidedEntity );
		WBCompRosaCollision* const	pCollidedCollision	= WB_GETCOMP_SAFE( pCollidedEntity, RosaCollision );
		if( pCollidedCollision && pCollidedCollision->IsDynamicBlocker() )
		{
			// Collided entity is a blocker (a dynamic entity that blocks other entities). Do not attempt to mantle onto it.
		}
		else
		{
			STATIC_HASHED_STRING( CollisionNormal );
			const Vector CollisionNormal = Event.GetVector( sCollisionNormal );
			TryBeginMantle( CollisionNormal );
		}
	}
	else if( EventName == sOnSteppedUp )
	{
		STATIC_HASHED_STRING( StepHeight );
		const float StepHeight		= Event.GetFloat( sStepHeight );

		OnSteppedUp( StepHeight );
	}
	else if( EventName == sOnKickImpulse )
	{
		STATIC_HASHED_STRING( KickImpulse );
		const Angles KickImpulse = Event.GetAngles( sKickImpulse );

		m_KickVelocity += KickImpulse;
	}
	else if( EventName == sPreLevelTransition )
	{
		WB_MAKE_EVENT( PushPersistence, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), PushPersistence, GetEntity() );
	}
	else if( EventName == sPostLevelTransition )
	{
		const TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();
		if( Persistence.Size() )
		{
			WB_MAKE_EVENT( PullPersistence, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), PullPersistence, GetEntity() );
		}

		STATIC_HASHED_STRING( RestoreSpawnPoint );
		const bool ShouldRestoreSpawnPoint = Event.GetBool( sRestoreSpawnPoint );

		STATIC_HASHED_STRING( TeleportLabel );
		const HashedString TeleportLabel = Event.GetHash( sTeleportLabel );

		if( TeleportLabel != HashedString::NullString )
		{
			TeleportTo( TeleportLabel );
		}
		else if( ShouldRestoreSpawnPoint && m_HasSetSpawnPoint )
		{
			RestoreSpawnPoint();
		}
	}
	else if( EventName == sPushInputContext )
	{
		STATIC_HASHED_STRING( InputContext );
		const HashedString InputContext = Event.GetHash( sInputContext );
		GetFramework()->GetInputSystem()->PushContext( InputContext );
	}
	else if( EventName == sPopInputContext )
	{
		STATIC_HASHED_STRING( InputContext );
		const HashedString InputContext = Event.GetHash( sInputContext );
		GetFramework()->GetInputSystem()->PopContext( InputContext );
	}
	else if( EventName == sDisablePause )
	{
		m_IsDisablingPause = true;
	}
	else if( EventName == sEnablePause )
	{
		m_IsDisablingPause = false;
	}
	else if( EventName == sPushPersistence )
	{
		PushPersistence();
	}
	else if( EventName == sPullPersistence )
	{
		PullPersistence();
	}
	else if( EventName == sOnAINoise )
	{
#if BUILD_DEV
		STATIC_HASHED_STRING( EventOwner );
		WBEntity* const pEventOwner = Event.GetEntity( sEventOwner );
		ASSERT( pEventOwner );

		STATIC_HASHED_STRING( NoiseEntity );
		WBEntity* const pNoiseEntity	= Event.GetEntity( sNoiseEntity, pEventOwner );

		if( pNoiseEntity == GetEntity() )
		{
			STATIC_HASHED_STRING( NoiseLocation );
			const Vector NoiseEntityLocation	= pEventOwner->GetTransformComponent<WBCompRosaTransform>()->GetLocation();
			const Vector NoiseLocation			= Event.GetVector( sNoiseLocation, NoiseEntityLocation );
			DEVASSERT( !NoiseLocation.IsZero() );

			STATIC_HASHED_STRING( NoiseSourceLocation );
			const Vector NoiseSourceLocation	= Event.GetVector( sNoiseSourceLocation );
			m_CACHED_LastAINoiseSourceLocation	= NoiseSourceLocation.IsZero() ? NoiseLocation : NoiseSourceLocation;

			STATIC_HASHED_STRING( NoiseRadius );
			m_CACHED_LastAINoiseRadius = Event.GetFloat( sNoiseRadius );
		}
#endif
	}
	else if( EventName == sOnFOVChanged )
	{
		STATICHASH( FOV );
		m_CurrentFOV.Reset( ConfigManager::GetFloat( sFOV ) );
	}
	else if( EventName == sSlamFOVScale )
	{
		// HACKHACK for Intro hub wake-up moment, mainly

		STATICHASH( FOVScale );
		const float FOVScale = Event.GetFloat( sFOVScale );

		STATICHASH( FOV );
		STATICHASH( ForegroundFOV );
		TickScaleFOV( m_CurrentFOV,		ConfigManager::GetFloat( sFOV ),		FOVScale, 0.0f, 0.0f );
		TickScaleFOV( m_CurrentFGFOV,	ConfigManager::GetFloat( sForegroundFOV ),	FOVScale, 0.0f, 0.0f );
		GetFramework()->SetFOV(		m_CurrentFOV.GetValue() );
		GetFramework()->SetFGFOV(	m_CurrentFGFOV.GetValue() );
	}
	else if( EventName == sSetInitialMusicTrackBits )
	{
		STATIC_HASHED_STRING( InitialMusicTrackBits );
		const uint InitialMusicTrackBits = Event.GetInt( sInitialMusicTrackBits );

		m_InitialMusicTrackBits = InitialMusicTrackBits;
	}
	else if( EventName == sStartFlying )
	{
		// Adapted from Acid flying scripts
		m_IsFlying = true;

		WBEntity* const				pEntity		= GetEntity();
		WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();

		pTransform->SetGravity( 0.0f );

		WB_MAKE_EVENT( EnableAirFriction, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), EnableAirFriction, pEntity );

		WB_MAKE_EVENT( DisableFootsteps, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), DisableFootsteps, pEntity );
	}
	else if( EventName == sStopFlying )
	{
		// Adapted from Acid flying scripts
		m_IsFlying = false;

		WBEntity* const				pEntity		= GetEntity();
		WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();

		pTransform->SetDefaultGravity();

		WB_MAKE_EVENT( DisableAirFraction, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), DisableAirFraction, pEntity );

		WB_MAKE_EVENT( EnableFootsteps, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), EnableFootsteps, pEntity );
	}
	else if( EventName == sOnDied )
	{
		if( m_IsCrouched )
		{
			BeginUncrouch();
		}
	}
}

/*virtual*/ void WBCompRosaPlayer::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WBCompRosaWeapon* const pWeapon = GetWeapon();
	const bool IsAiming = pWeapon && pWeapon->IsAiming();

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT( CanUnlockDoors, 0.0f, pStatMod );
	const bool CanUnlockDoors = ( WB_MODDED( CanUnlockDoors ) != 0.0f );

	{
		WB_SET_CONTEXT( Event, Bool,	CanOpenDoors,	true );
		WB_SET_CONTEXT( Event, Bool,	CanUnlockDoors,	CanUnlockDoors );
		WB_SET_CONTEXT( Event, Bool,	IsSprinting,	m_IsSprinting && !m_IsCrouched );
		WB_SET_CONTEXT( Event, Float,	SprintTime,		GetTime() - m_SprintStartTime );
		WB_SET_CONTEXT( Event, Bool,	IsPowerSliding,	m_IsPowerSliding );
		WB_SET_CONTEXT( Event, Bool,	IsMantling,		m_IsMantling );
		WB_SET_CONTEXT( Event, Bool,	IsAiming,		IsAiming );
	}

	// OLDVAMP: Replace with an equivalent for Zeta campaign?
	//// Add some campaign context so we can easily query this stuff without special PEs
	//// (as long as we're querying it *after* the player has spawned!)
	//RosaCampaign* const	pCampaign	= RosaCampaign::GetCampaign();
	//if( pCampaign )
	//{
	//	WB_SET_CONTEXT( Event, Int,	Campaign_Legacy,	pCampaign->GetLegacy() );
	//	WB_SET_CONTEXT( Event, Int,	Campaign_Season,	pCampaign->GetSeason() );
	//	WB_SET_CONTEXT( Event, Int,	Campaign_Episode,	pCampaign->GetEpisode() );
	//}
}

// Manage difficulty persistence here, since there's no difficulty component
void WBCompRosaPlayer::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	const uint GameDifficulty = RosaDifficulty::GetGameDifficulty();

	STATIC_HASHED_STRING( GameDifficulty );
	Persistence.SetInt( sGameDifficulty, GameDifficulty );
}

void WBCompRosaPlayer::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( GameDifficulty );
	const uint GameDifficulty = Persistence.GetInt( sGameDifficulty );

	RosaDifficulty::SetGameDifficulty( GameDifficulty );
	RosaDifficulty::PushGameToMenu();
}

void WBCompRosaPlayer::AddAutosaveSuppression( const bool Serialize )
{
	if( Serialize )
	{
		++m_AutosaveSuppRefsSerialized;
	}
	else
	{
		++m_AutosaveSuppRefsTransient;
	}
}

void WBCompRosaPlayer::RemoveAutosaveSuppression( const bool Serialize )
{
	if( Serialize )
	{
		if( m_AutosaveSuppRefsSerialized > 0 )
		{
			--m_AutosaveSuppRefsSerialized;
		}
#if BUILD_DEV
		else
		{
			WARNDESC( "Autosave: Suppression refcount mismatch, trying to decrement past zero." );
		}
#endif
	}
	else
	{
		if( m_AutosaveSuppRefsTransient > 0 )
		{
			--m_AutosaveSuppRefsTransient;
		}
#if BUILD_DEV
		else
		{
			WARNDESC( "Autosave: Transient suppression refcount mismatch, trying to decrement past zero." );
		}
#endif
	}
}

bool WBCompRosaPlayer::CanAutosave()
{
	if( m_AutosaveSuppRefsSerialized > 0 || m_AutosaveSuppRefsTransient > 0 )
	{
		DEVPRINTF( "CanAutosave: false because suppression refcount is %d/%d\n", m_AutosaveSuppRefsSerialized, m_AutosaveSuppRefsTransient );
		return false;
	}

	WBCompRosaCollision* const pCollision = WB_GETCOMP( GetEntity(), RosaCollision );
	if( !pCollision->IsLanded() )
	{
		DEVPRINTF( "CanAutosave: false because player is unlanded\n" );
		return false;
	}

	DEVPRINTF( "CanAutosave: true\n" );
	return true;
}

void WBCompRosaPlayer::QueueAutosave( const float AutosaveDelay )
{
	WB_MAKE_EVENT( OnAutosave, NULL );
	WB_QUEUE_EVENT_DELAY( GetEventManager(), OnAutosave, GetEntity(), AutosaveDelay );
}

/*virtual*/ void WBCompRosaPlayer::OnInputContextsChanged()
{
	ConditionalApplyRunningStatMods();
}

void WBCompRosaPlayer::ConditionalApplyRunningStatMods()
{
	WBEntity* const				pEntity			= GetEntity();
	WBCompStatMod* const		pStatMod		= WB_GETCOMP( pEntity, StatMod );

	STATIC_HASHED_STRING( Running );
	if( m_IsSprinting && !m_IsCrouched )
	{
		pStatMod->SetEventActive( sRunning, true );
	}
	else
	{
		pStatMod->SetEventActive( sRunning, false );
	}
}

#define VERSION_EMPTY					0
#define VERSION_CROUCHING				1
#define VERSION_CLIMBING				2
#define VERSION_UNCLIMBINGGRAVITY		3
#define VERSION_SPAWNPOINT				4
#define VERSION_POWERSLIDE				5
#define VERSION_POWERSLIDEY				6
#define VERSION_ISDISABLINGPAUSE		7
#define VERSION_DRAG					8
#define VERSION_AUTOSAVESUPPRESSION		9
#define VERSION_HASDOUBLEJUMPED			10
#define VERSION_INITIALMUSICTRACKBITS	11
#define VERSION_ISFORCEDCROUCHED		12
#define VERSION_UNCLIMBINGGRAVITY_DEPR	13
#define VERSION_ISFLYING				14
#define VERSION_CURRENT					14

uint WBCompRosaPlayer::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += 1;						// m_IsCrouched
	Size += 1;						// m_IsUncrouching
	Size += 1;						// m_IsForceCrouched
	Size += 4;						// m_ClimbRefs
	Size += 1;						// m_HasSetSpawnPoint
	Size += sizeof( Vector );		// m_SpawnLocation
	Size += sizeof( Angles );		// m_SpawnOrientation
	Size += 1;						// m_HasDoubleJumped
	Size += 1;						// m_IsPowerSliding
	Size += 4;						// Power slide time remaining
	Size += sizeof( Vector );		// m_PowerSlideY
	Size += 1;						// m_IsDisablingPause

	Size += 1;						// m_IsDragging
	Size += sizeof( WBEntityRef );	// m_DraggedEntity

	Size += 4;						// m_AutosaveSuppRefsSerialized

	Size += 4;						// m_InitialMusicTrackBits

	Size += 1;						// m_IsFlying

	return Size;
}

void WBCompRosaPlayer::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_IsCrouched );
	Stream.WriteBool( m_IsUncrouching );
	Stream.WriteBool( m_IsForceCrouched );

	Stream.WriteInt32( m_ClimbRefs );

	Stream.WriteBool( m_HasSetSpawnPoint );
	Stream.Write( sizeof( Vector ), &m_SpawnLocation );
	Stream.Write( sizeof( Angles ), &m_SpawnOrientation );

	Stream.WriteBool( m_HasDoubleJumped );

	Stream.WriteBool( m_IsPowerSliding );
	Stream.WriteFloat( m_PowerSlideEndTime - GetTime() );
	Stream.Write( sizeof( Vector ), &m_PowerSlideY );

	Stream.WriteBool( m_IsDisablingPause );

	Stream.WriteBool( m_IsDragging );
	Stream.Write( sizeof( WBEntityRef ), &m_DraggedEntity );

	Stream.WriteUInt32( m_AutosaveSuppRefsSerialized );

	Stream.WriteUInt32( m_InitialMusicTrackBits );

	Stream.WriteBool( m_IsFlying );
}

void WBCompRosaPlayer::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_CROUCHING )
	{
		m_IsCrouched = Stream.ReadBool();
		m_IsUncrouching = Stream.ReadBool();
	}

	if( Version >= VERSION_ISFORCEDCROUCHED )
	{
		m_IsForceCrouched = Stream.ReadBool();
	}

	if( Version >= VERSION_CLIMBING )
	{
		m_ClimbRefs = Stream.ReadInt32();
	}

	if( Version >= VERSION_UNCLIMBINGGRAVITY && Version < VERSION_UNCLIMBINGGRAVITY_DEPR )
	{
		Stream.ReadFloat();
	}

	if( Version >= VERSION_SPAWNPOINT )
	{
		m_HasSetSpawnPoint = Stream.ReadBool();
		Stream.Read( sizeof( Vector ), &m_SpawnLocation );
		Stream.Read( sizeof( Angles ), &m_SpawnOrientation );
	}

	if( Version >= VERSION_HASDOUBLEJUMPED )
	{
		m_HasDoubleJumped = Stream.ReadBool();
	}

	if( Version >= VERSION_POWERSLIDE )
	{
		m_IsPowerSliding = Stream.ReadBool();
		m_PowerSlideEndTime = GetTime() + Stream.ReadFloat();
	}

	if( Version >= VERSION_POWERSLIDEY )
	{
		Stream.Read( sizeof( Vector ), &m_PowerSlideY );
	}

	if( Version >= VERSION_ISDISABLINGPAUSE )
	{
		m_IsDisablingPause = Stream.ReadBool();
	}

	if( Version >= VERSION_DRAG )
	{
		m_IsDragging = Stream.ReadBool();
		Stream.Read( sizeof( WBEntityRef ), &m_DraggedEntity );
	}

	if( Version >= VERSION_AUTOSAVESUPPRESSION )
	{
		m_AutosaveSuppRefsSerialized = Stream.ReadUInt32();
	}

	if( Version >= VERSION_INITIALMUSICTRACKBITS )
	{
		m_InitialMusicTrackBits = Stream.ReadUInt32();
	}

	if( Version >= VERSION_ISFLYING )
	{
		m_IsFlying = Stream.ReadBool();
	}
}
