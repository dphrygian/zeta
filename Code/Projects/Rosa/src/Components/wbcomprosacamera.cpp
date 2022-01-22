#include "core.h"
#include "wbcomprosacamera.h"
#include "wbcomprosatransform.h"
#include "configmanager.h"
#include "wbentity.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "mathcore.h"
#include "matrix.h"
#include "quat.h"
#include "segment.h"
#include "wbeventmanager.h"

WBCompRosaCamera::WBCompRosaCamera()
:	m_HasTranslationOverride( false )
,	m_HasOrientationOverride( false )
,	m_TranslationOverride()
,	m_OrientationOverride()
,	m_TranslationOverrideStart( 0.0f )
,	m_TranslationOverrideEnd( 0.0f )
,	m_OrientationOverrideStart( 0.0f )
,	m_OrientationOverrideEnd( 0.0f )
,	m_ViewOffsetZ( 0.0f )
,	m_LastViewOffsetZ( 0.0f )
,	m_BaseViewOffsetZ( 0.0f )
,	m_ViewAngleOffsetRoll( 0.0f )
,	m_StepUpZ( 0.0f )
,	m_LeanRoll( 0.0f )
,	m_LeanPosition( 0.0f )
,	m_LeanVelocity( 0.0f )
,	m_LeanRollMax( 0.0f )
,	m_LeanRadius( 0.0f )
,	m_LeanExtent( 0.0f )
,	m_CachedLeanOffset()
,	m_SlideRoll( 0.0f )
,	m_StrafeRoll( 0.0f )
,	m_StrafeRollPosition( 0.0f )
,	m_StrafeRollVelocity( 0.0f )
,	m_StrafeRollMax( 0.0f )
,	m_ViewBobOffset()
,	m_ViewBobAngleOffset()
,	m_ViewSwayOffset()
,	m_ViewSwayAngleOffset()
,	m_ViewBobEnabled( false )
,	m_ViewSwayEnabled( false )
,	m_SlideRollEnabled( false )
,	m_StrafeRollEnabled( false )
,	m_HandsVelocityEnabled( false )
,	m_KickAngleOffset()
,	m_HandsVelocity()
,	m_HandsRotationalVelocity()
,	m_HandsFactor( 0.0f )
,	m_HandsLeanFactor( 0.0f )
,	m_HandsZFactor( 0.0f )
,	m_HandsVelocityFactor( 0.0f )
,	m_HandsRotationalVelocityFactor( 0.0f )
,	m_HandsVelocityLimit( 0.0f )
,	m_HandsRotationalVelocityLimit( 0.0f )
,	m_MinimapViewExtentNear( 0.0f )
,	m_MinimapViewExtentFar( 0.0f )
,	m_MinimapViewLerpTime( 0.0f )
,	m_MinimapViewExtent()
#if ROSA_USE_MAXIMAP
,	m_MaximapViewExtentNear( 0.0f )
,	m_MaximapViewExtentFar( 0.0f )
,	m_MaximapViewExtent()
#endif
{
}

WBCompRosaCamera::~WBCompRosaCamera()
{
}

/*virtual*/ void WBCompRosaCamera::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( ViewOffsetZ );
	m_ViewOffsetZ = ConfigManager::GetInheritedFloat( sViewOffsetZ, 0.0f, sDefinitionName );
	m_LastViewOffsetZ = m_ViewOffsetZ;
	m_BaseViewOffsetZ = m_ViewOffsetZ;

	STATICHASH( LeanVelocity );
	m_LeanVelocity = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sLeanVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( LeanRollMax );
	m_LeanRollMax = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sLeanRollMax, 0.0f, sDefinitionName ) );

	STATICHASH( LeanRadius );
	m_LeanRadius = ConfigManager::GetInheritedFloat( sLeanRadius, 0.0f, sDefinitionName );

	STATICHASH( LeanExtent );
	m_LeanExtent = ConfigManager::GetInheritedFloat( sLeanExtent, 0.0f, sDefinitionName );

	STATICHASH( StrafeRollVelocity );
	m_StrafeRollVelocity = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sStrafeRollVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( StrafeRollMax );
	m_StrafeRollMax = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sStrafeRollMax, 0.0f, sDefinitionName ) );

	// Not a config var of the component!
	STATICHASH( ViewBob );
	m_ViewBobEnabled = ConfigManager::GetBool( sViewBob );

	// Not a config var of the component!
	STATICHASH( ViewSway );
	m_ViewSwayEnabled = ConfigManager::GetBool( sViewSway );

	// Not a config var of the component!
	STATICHASH( SlideRoll );
	m_SlideRollEnabled = ConfigManager::GetBool( sSlideRoll );

	// Not a config var of the component!
	STATICHASH( StrafeRoll );
	m_StrafeRollEnabled = ConfigManager::GetBool( sStrafeRoll );

	// Not a config var of the component!
	STATICHASH( HandsVelocity );
	m_HandsVelocityEnabled = ConfigManager::GetBool( sHandsVelocity );

	STATICHASH( HandsFactor );
	m_HandsFactor = ConfigManager::GetInheritedFloat( sHandsFactor, 1.0f, sDefinitionName );

	STATICHASH( HandsLeanFactor );
	m_HandsLeanFactor = ConfigManager::GetInheritedFloat( sHandsLeanFactor, 1.0f, sDefinitionName );

	STATICHASH( HandsZFactor );
	m_HandsZFactor = ConfigManager::GetInheritedFloat( sHandsZFactor, 1.0f, sDefinitionName );

	STATICHASH( HandsVelocityFactor );
	m_HandsVelocityFactor = ConfigManager::GetInheritedFloat( sHandsVelocityFactor, 1.0f, sDefinitionName );

	STATICHASH( HandsRotationalVelocityFactor );
	m_HandsRotationalVelocityFactor = ConfigManager::GetInheritedFloat( sHandsRotationalVelocityFactor, 1.0f, sDefinitionName );

	STATICHASH( HandsVelocityLimit );
	m_HandsVelocityLimit = ConfigManager::GetInheritedFloat( sHandsVelocityLimit, 1.0f, sDefinitionName );

	STATICHASH( HandsRotationalVelocityLimit );
	m_HandsRotationalVelocityLimit = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sHandsRotationalVelocityLimit, 1.0f, sDefinitionName ) );

	// Configured in RosaMinimap section, *NOT* camera component;
	// this makes it a little easier to share config with framework.
	STATICHASH( RosaMinimap );
	STATICHASH( MinimapViewExtentNear );
	m_MinimapViewExtentNear = ConfigManager::GetFloat( sMinimapViewExtentNear, 0.0f, sRosaMinimap );
	m_MinimapViewExtent.Reset( Interpolator<float>::EIT_None, m_MinimapViewExtentNear, m_MinimapViewExtentNear, 0.0f );

	STATICHASH( MinimapViewExtentFar );
	m_MinimapViewExtentFar = ConfigManager::GetFloat( sMinimapViewExtentFar, 0.0f, sRosaMinimap );

	STATICHASH( MinimapViewLerpTime );
	m_MinimapViewLerpTime = ConfigManager::GetFloat( sMinimapViewLerpTime, 0.0f, sRosaMinimap );

#if ROSA_USE_MAXIMAP
	STATICHASH( MaximapViewExtentNear );
	m_MaximapViewExtentNear = ConfigManager::GetFloat( sMaximapViewExtentNear, 0.0f, sRosaMinimap );
	m_MaximapViewExtent.Reset( Interpolator<float>::EIT_None, m_MaximapViewExtentNear, m_MaximapViewExtentNear, 0.0f );

	STATICHASH( MaximapViewExtentFar );
	m_MaximapViewExtentFar = ConfigManager::GetFloat( sMaximapViewExtentFar, 0.0f, sRosaMinimap );
#endif
}

/*virtual*/ void WBCompRosaCamera::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetTranslationOverride );
	STATIC_HASHED_STRING( SetOrientationOverride );
	STATIC_HASHED_STRING( SetViewOffsetZ );
	STATIC_HASHED_STRING( ResetViewOffsetZ );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetTranslationOverride )
	{
		STATIC_HASHED_STRING( TranslationOverride );
		const Vector TranslationOverride = Event.GetVector( sTranslationOverride );

		STATIC_HASHED_STRING( Enable );
		const bool Enable = Event.GetBool( sEnable );

		STATIC_HASHED_STRING( LerpTime );
		const float LerpTime = Event.GetFloat( sLerpTime );

		SetTranslationOverride( TranslationOverride, Enable, LerpTime );
	}
	else if( EventName == sSetOrientationOverride )
	{
		STATIC_HASHED_STRING( OrientationOverride );
		const Angles OrientationOverride = Event.GetAngles( sOrientationOverride );

		STATIC_HASHED_STRING( Enable );
		const bool Enable = Event.GetBool( sEnable );

		STATIC_HASHED_STRING( LerpTime );
		const float LerpTime = Event.GetFloat( sLerpTime );

		SetOrientationOverride( OrientationOverride, Enable, LerpTime );
	}
	else if( EventName == sSetViewOffsetZ )
	{
		STATIC_HASHED_STRING( ViewOffsetZ );
		const float ViewOffsetZ = Event.GetFloat( sViewOffsetZ );
		SetViewOffsetZ( ViewOffsetZ );
	}
	else if( EventName == sResetViewOffsetZ )
	{
		// Hack for reviving player into correct view state
		SetViewOffsetZ( m_LastViewOffsetZ );
	}
}

/*virtual*/ void WBCompRosaCamera::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	RosaGame* const			pGame		= pFramework->GetGame();

	const float DesiredRoll = GetDesiredLean( m_LeanPosition );
	UpdateLean( DesiredRoll, DeltaTime );

	UpdateStrafeRoll( m_StrafeRollMax * m_StrafeRollPosition, DeltaTime );

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	m_MinimapViewExtent.Tick( DeltaTime );
	pFramework->SetMinimapViewExtent( m_MinimapViewExtent.GetValue() * pGame->GetMinimapScalar() );

#if ROSA_USE_MAXIMAP
	m_MaximapViewExtent.Tick( DeltaTime );
	pFramework->SetMaximapViewExtent( m_MaximapViewExtent.GetValue() * pGame->GetMinimapScalar() );
#endif

	const Vector ViewTranslation = GetModifiedTranslation( EVM_All, pTransform->GetLocation() );
	const Angles ViewOrientation = GetModifiedOrientation( EVM_All, pTransform->GetOrientation() );
	pFramework->SetMainViewTransform( ViewTranslation, ViewOrientation );

	WB_MAKE_EVENT( OnCameraTicked, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnCameraTicked, NULL );
}

void WBCompRosaCamera::ZoomMinimapOut()
{
	if( m_MinimapViewExtent.GetEndValue() == m_MinimapViewExtentFar )
	{
		// We're already zooming/zoomed out
		return;
	}

	const float	CurrentExtent	= m_MinimapViewExtent.GetValue();
	const float	RemainingT		= InvLerp( CurrentExtent, m_MinimapViewExtentFar, m_MinimapViewExtentNear );
	const float	Duration		= RemainingT * m_MinimapViewLerpTime;
	m_MinimapViewExtent.Reset( Interpolator<float>::EIT_EaseIn, m_MinimapViewExtent.GetValue(), m_MinimapViewExtentFar, Duration );
#if ROSA_USE_MAXIMAP
	m_MaximapViewExtent.Reset( Interpolator<float>::EIT_EaseIn, m_MaximapViewExtent.GetValue(), m_MaximapViewExtentFar, Duration );
#endif
}

void WBCompRosaCamera::ZoomMinimapIn()
{
	if( m_MinimapViewExtent.GetEndValue() == m_MinimapViewExtentNear )
	{
		// We're already zooming/zoomed in
		return;
	}

	const float	CurrentExtent	= m_MinimapViewExtent.GetValue();
	const float	RemainingT		= InvLerp( CurrentExtent, m_MinimapViewExtentNear, m_MinimapViewExtentFar );
	const float	Duration		= RemainingT * m_MinimapViewLerpTime;
	m_MinimapViewExtent.Reset( Interpolator<float>::EIT_EaseOut, m_MinimapViewExtent.GetValue(), m_MinimapViewExtentNear, Duration );
#if ROSA_USE_MAXIMAP
	m_MaximapViewExtent.Reset( Interpolator<float>::EIT_EaseOut, m_MaximapViewExtent.GetValue(), m_MaximapViewExtentNear, Duration );
#endif
}

void WBCompRosaCamera::SetTranslationOverride( const Vector& TranslationOverride, const bool Enable, const float LerpTime )
{
	m_HasTranslationOverride	= Enable;
	m_TranslationOverride		= TranslationOverride;
	m_TranslationOverrideStart	= GetTime();
	m_TranslationOverrideEnd	= m_TranslationOverrideStart + LerpTime;
}

void WBCompRosaCamera::SetOrientationOverride( const Angles& OrientationOverride, const bool Enable, const float LerpTime )
{
	m_HasOrientationOverride	= Enable;
	m_OrientationOverride		= OrientationOverride;
	m_OrientationOverrideStart	= GetTime();
	m_OrientationOverrideEnd	= m_OrientationOverrideStart + LerpTime;
}

void WBCompRosaCamera::SetViewOffsetZ( const float ViewOffsetZ )
{
	m_LastViewOffsetZ	= m_ViewOffsetZ;
	m_ViewOffsetZ		= ViewOffsetZ;
}

void WBCompRosaCamera::ModifyTranslation( const EViewModifiers Modifiers, Vector& InOutTranslation ) const
{
	if( m_HasTranslationOverride && ( Modifiers & EVM_Override ) && GetTime() >= m_TranslationOverrideEnd )
	{
		InOutTranslation = m_TranslationOverride;
		return;
	}

	if( Modifiers & EVM_OffsetZ )
	{
		if( Modifiers & EVM_Hands )
		{
			// Scale offset relative to base height
			const float Difference = m_BaseViewOffsetZ - m_ViewOffsetZ;
			const float ScaledDiff = Difference * m_HandsZFactor;
			const float NewOffsetZ = m_BaseViewOffsetZ - ScaledDiff;
			InOutTranslation.z += NewOffsetZ;
		}
		else
		{
			InOutTranslation.z += m_ViewOffsetZ;
		}
	}

	if( Modifiers & EVM_StepUpZ )
	{
		InOutTranslation.z += m_StepUpZ;
	}

	if( Modifiers & EVM_Lean )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsLeanFactor : 1.0f;
		InOutTranslation += m_CachedLeanOffset * Scalar;
	}

	if( m_ViewBobEnabled && ( Modifiers & EVM_Bob ) )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsFactor : 1.0f;
		InOutTranslation += m_ViewBobOffset * Scalar;
	}

	if( m_ViewSwayEnabled && ( Modifiers & EVM_Sway ) )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsFactor : 1.0f;
		InOutTranslation += m_ViewSwayOffset * Scalar;
	}

	if( m_HandsVelocityEnabled && ( Modifiers & EVM_HandsVelocity ) )
	{
		DEVASSERT( Modifiers & EVM_Hands );

		Vector LimitedHandsVelocity = m_HandsVelocity * m_HandsVelocityFactor;
		LimitedHandsVelocity.x = Clamp( LimitedHandsVelocity.x, -m_HandsVelocityLimit, m_HandsVelocityLimit );
		LimitedHandsVelocity.y = Clamp( LimitedHandsVelocity.y, -m_HandsVelocityLimit, m_HandsVelocityLimit );
		LimitedHandsVelocity.z = Clamp( LimitedHandsVelocity.z, -m_HandsVelocityLimit, m_HandsVelocityLimit );

		InOutTranslation -= LimitedHandsVelocity;
	}

	if( GetTime() < m_TranslationOverrideEnd )
	{
		const float	Alpha		= InvLerp( GetTime(), m_TranslationOverrideStart, m_TranslationOverrideEnd );
		// Fixed Hermite curve between 0 and 1 with tangents 0 and 0
		const float EaseAlpha	= Alpha * Alpha * ( 3.0f - 2.0f * Alpha );
		InOutTranslation		= InOutTranslation.LERP( m_HasTranslationOverride ? EaseAlpha : ( 1.0f - EaseAlpha ), m_TranslationOverride );
	}
}

void WBCompRosaCamera::ModifyOrientation( const EViewModifiers Modifiers, Angles& InOutOrientation ) const
{
	if( m_HasOrientationOverride && ( Modifiers & EVM_Override ) && GetTime() >= m_OrientationOverrideEnd )
	{
		InOutOrientation = m_OrientationOverride;
		return;
	}

	// DLP 13 Oct 2021: Not currently used by any systems.
	// No hands factor scaling applied here, hands would roll completely with this.
	if( Modifiers & EVM_Roll )
	{
		InOutOrientation.Roll += m_ViewAngleOffsetRoll;
	}

	if( Modifiers & EVM_Lean )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsLeanFactor : 1.0f;
		InOutOrientation.Roll += m_LeanRoll * Scalar;
	}

	if( m_SlideRollEnabled && ( Modifiers & EVM_SlideRoll ) )
	{
		// No hands factor scaling applied here, hands should roll completely when sliding.
		// (Also, I tested and it barely made any difference.)
		// But we do reverse this if we're in lefty mode!
		STATICHASH( LeftyMode );
		const bool	LeftyMode		= ConfigManager::GetBool( sLeftyMode );
		const float	UsingSlideRoll	= LeftyMode ? -m_SlideRoll : m_SlideRoll;

		InOutOrientation.Roll += UsingSlideRoll;
	}

	if( m_StrafeRollEnabled && ( Modifiers & EVM_StrafeRoll ) )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsFactor : 1.0f;
		InOutOrientation.Roll += m_StrafeRoll * Scalar;
	}

	if( m_ViewBobEnabled && ( Modifiers & EVM_Bob ) )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsFactor : 1.0f;
		InOutOrientation += m_ViewBobAngleOffset * Scalar;
	}

	if( m_ViewSwayEnabled && ( Modifiers & EVM_Sway ) )
	{
		const float Scalar = ( Modifiers & EVM_Hands ) ? m_HandsFactor : 1.0f;
		InOutOrientation += m_ViewSwayAngleOffset * Scalar;
	}

	if( Modifiers & EVM_Kick )
	{
		InOutOrientation += m_KickAngleOffset;
	}

	if( m_HandsVelocityEnabled && ( Modifiers & EVM_HandsVelocity ) )
	{
		DEVASSERT( Modifiers & EVM_Hands );

		Angles LimitedHandsRotationalVelocity = m_HandsRotationalVelocity * m_HandsRotationalVelocityFactor;
		LimitedHandsRotationalVelocity.Pitch = Clamp( LimitedHandsRotationalVelocity.Pitch, -m_HandsRotationalVelocityLimit, m_HandsRotationalVelocityLimit );
		LimitedHandsRotationalVelocity.Roll = Clamp( LimitedHandsRotationalVelocity.Roll, -m_HandsRotationalVelocityLimit, m_HandsRotationalVelocityLimit );
		LimitedHandsRotationalVelocity.Yaw = Clamp( LimitedHandsRotationalVelocity.Yaw, -m_HandsRotationalVelocityLimit, m_HandsRotationalVelocityLimit );

		InOutOrientation -= LimitedHandsRotationalVelocity;
	}

	if( GetTime() < m_OrientationOverrideEnd )
	{
		const float	Alpha			= InvLerp( GetTime(), m_TranslationOverrideStart, m_TranslationOverrideEnd );
		// Fixed Hermite curve between 0 and 1 with tangents 0 and 0
		const float EaseAlpha		= Alpha * Alpha * ( 3.0f - 2.0f * Alpha );
		const Quat	ActualQuat		= InOutOrientation.ToQuaternion();
		const Quat	OverrideQuat	= m_OrientationOverride.ToQuaternion();
		const Quat	LerpedQuat		= ActualQuat.SLERP( m_HasOrientationOverride ? EaseAlpha : ( 1.0f - EaseAlpha ), OverrideQuat );
		InOutOrientation			= LerpedQuat.ToAngles();
	}
}

Vector WBCompRosaCamera::GetModifiedTranslation( const EViewModifiers Modifiers, const Vector& InTranslation ) const
{
	Vector OutTranslation = InTranslation;
	ModifyTranslation( Modifiers, OutTranslation );
	return OutTranslation;
}

Angles WBCompRosaCamera::GetModifiedOrientation( const EViewModifiers Modifiers, const Angles& InOrientation ) const
{
	Angles OutOrientation = InOrientation;
	ModifyOrientation( Modifiers, OutOrientation );
	return OutOrientation;
}

Vector WBCompRosaCamera::GetLeanOffset( const float LeanRoll ) const
{
	if( LeanRoll == 0.0f )
	{
		// Small optimization.
		return Vector();
	}
	else
	{
		WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		const Angles	LeanAngle	= Angles( 0.0f, LeanRoll, pTransform->GetOrientation().Yaw );
		Vector			LeanVector	= Vector( 0.0f, 0.0f, m_LeanRadius );

		LeanVector		*= LeanAngle.ToMatrix();
		LeanVector.z	-= m_LeanRadius;

		return LeanVector;
	}
}

float WBCompRosaCamera::GetDesiredLean( const float LeanPosition ) const
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const float DesiredRoll = m_LeanRollMax * LeanPosition;

	const Vector	ViewBase	= GetModifiedTranslation( EVM_OffsetZ, pTransform->GetLocation() );
	const Vector	LeanOffset	= GetLeanOffset( DesiredRoll );
	const Segment	LeanSegment	= Segment( ViewBase, ViewBase + LeanOffset );
	const Vector	LeanExtents = Vector( m_LeanExtent, m_LeanExtent, 0.0f );

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= EECF_BlockerCollision;

	if( GetWorld()->Sweep( LeanSegment, LeanExtents, Info ) )
	{
		return m_LeanRollMax * Info.m_Out_HitT * Sign( LeanPosition );
	}
	else
	{
		return DesiredRoll;
	}
}

void WBCompRosaCamera::SetLeanRoll( const float LeanRoll )
{
	m_LeanRoll			= LeanRoll;
	m_CachedLeanOffset	= GetLeanOffset( LeanRoll );
}

void WBCompRosaCamera::UpdateLean( const float TargetRoll, const float DeltaTime )
{
	const float LeanDelta = m_LeanVelocity * DeltaTime;

	if( m_LeanRoll < TargetRoll )
	{
		SetLeanRoll( Min( m_LeanRoll + LeanDelta, TargetRoll ) );
	}
	else if( m_LeanRoll > TargetRoll )
	{
		SetLeanRoll( Max( m_LeanRoll - LeanDelta, TargetRoll ) );
	}
	else
	{
		SetLeanRoll( TargetRoll );
	}
}

void WBCompRosaCamera::UpdateStrafeRoll( const float TargetRoll, const float DeltaTime )
{
	const float StrafeRollDelta = m_StrafeRollVelocity * DeltaTime;

	if( m_StrafeRoll < TargetRoll )
	{
		m_StrafeRoll = Min( m_StrafeRoll + StrafeRollDelta, TargetRoll );
	}
	else if( m_StrafeRoll > TargetRoll )
	{
		m_StrafeRoll = Max( m_StrafeRoll - StrafeRollDelta, TargetRoll );
	}
	else
	{
		m_StrafeRoll = TargetRoll;
	}
}
