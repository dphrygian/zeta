#include "core.h"
#include "rosalockpicking.h"
#include "wbworld.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "inputsystem.h"
#include "keyboard.h"
#include "mouse.h"
#include "xinputcontroller.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "rosagame.h"
#include "wbentity.h"
#include "mesh.h"
#include "irenderer.h"
#include "ivertexdeclaration.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "texturemanager.h"
#include "mathfunc.h"
#include "segment2d.h"
#include "collisioninfo2d.h"
#include "wbaction.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "Achievements/iachievementmanager.h"
#include "Components/wbcompstatmod.h"
#include "hsv.h"
#include "rosadifficulty.h"

#if BUILD_DEV
#include "keyboard.h"
#endif

RosaLockpicking::RosaLockpicking()
:	m_IsLockpicking( false )
,	m_InputContext()
,	m_GameEndDelay( 0.0f )
,	m_PinShakeLow( 0.0f )
,	m_PinShakeHigh( 0.0f )
,	m_RattleVolumeLow( 0.0f )
,	m_RattleVolumeHigh( 0.0f )
,	m_LockpickVelocity( 0.0f )
,	m_InputTimeout( 0.0f )
,	m_InputTimeoutRemaining( 0.0f )
,	m_ForceNoiseRadius( 0.0f )
,	m_ForceNoiseScalar( 0.0f )
,	m_RattleSound()
,	m_PinMissSound()
,	m_PinBindSound()
,	m_PinResistSound()
,	m_PinForceSound()
,	m_OpenedSound()
,	m_StartActions()
,	m_SuccessActions()
,	m_GameEndEventUID( 0 )
,	m_LockpickVector()
,	m_PinVector()
,	m_CosPinAngle( 0.0f )
,	m_PinsRemaining( 0 )
,	m_MaxPins( 0 )
,	m_ForceChance( 0.0f )
,	m_ForceFailPins( 0 )
,	m_ExtraForcePins( 0 )
,	m_IsActive( false )
,	m_IsInputTimedOut( false )
,	m_IsBinding( false )
,	m_IsForcing( false )
,	m_UseCameraOverride( false )
,	m_CameraTranslation()
,	m_CameraOrientation()
,	m_CameraLerpTime( 0.0f )
{
	InitializeFromDefinition( "RosaLockpicking" );

	RegisterForEvents();
}

RosaLockpicking::~RosaLockpicking()
{
	// I don't unregister for events here because world has already been destroyed. Assumptions!

	WBActionFactory::ClearActionArray( m_StartActions );
	WBActionFactory::ClearActionArray( m_SuccessActions );
}

void RosaLockpicking::RegisterForEvents()
{
	STATIC_HASHED_STRING( BeginLockpicking );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sBeginLockpicking, this, NULL );

	STATIC_HASHED_STRING( EndLockpicking );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sEndLockpicking, this, NULL );

	STATIC_HASHED_STRING( SucceedLockpick );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sSucceedLockpick, this, NULL );

	STATIC_HASHED_STRING( Lockpicking_EndBind );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sLockpicking_EndBind, this, NULL );

	STATIC_HASHED_STRING( Lockpicking_EndForce );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sLockpicking_EndForce, this, NULL );
}

void RosaLockpicking::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( InputContext );
	m_InputContext = ConfigManager::GetHash( sInputContext, HashedString::NullString, sDefinitionName );

	STATICHASH( GameEndDelay );
	m_GameEndDelay = ConfigManager::GetInheritedFloat( sGameEndDelay, 0.0f, sDefinitionName );

	STATICHASH( PinShakeLow );
	m_PinShakeLow = ConfigManager::GetInheritedFloat( sPinShakeLow, 0.0f, sDefinitionName );

	STATICHASH( PinShakeHigh );
	m_PinShakeHigh = ConfigManager::GetInheritedFloat( sPinShakeHigh, 0.0f, sDefinitionName );

	STATICHASH( RattleVolumeLow );
	m_RattleVolumeLow = ConfigManager::GetInheritedFloat( sRattleVolumeLow, 0.0f, sDefinitionName );

	STATICHASH( RattleVolumeHigh );
	m_RattleVolumeHigh = ConfigManager::GetInheritedFloat( sRattleVolumeHigh, 0.0f, sDefinitionName );

	STATICHASH( LockpickVelocity );
	m_LockpickVelocity = ConfigManager::GetInheritedFloat( sLockpickVelocity, 0.0f, sDefinitionName );

	STATICHASH( InputTimeout );
	m_InputTimeout = ConfigManager::GetInheritedFloat( sInputTimeout, 0.0f, sDefinitionName );

	STATICHASH( ForceNoiseRadius );
	m_ForceNoiseRadius = ConfigManager::GetInheritedFloat( sForceNoiseRadius, 0.0f, sDefinitionName );

	STATICHASH( ForceNoiseScalar );
	m_ForceNoiseScalar = ConfigManager::GetInheritedFloat( sForceNoiseScalar, 0.0f, sDefinitionName );

	STATICHASH( RattleSound );
	m_RattleSound = ConfigManager::GetString( sRattleSound, "", sDefinitionName );

	STATICHASH( PinMissSound );
	m_PinMissSound = ConfigManager::GetString( sPinMissSound, "", sDefinitionName );

	STATICHASH( PinBindSound );
	m_PinBindSound = ConfigManager::GetString( sPinBindSound, "", sDefinitionName );

	STATICHASH( PinForceSound );
	m_PinForceSound = ConfigManager::GetString( sPinForceSound, "", sDefinitionName );

	STATICHASH( PinResistSound );
	m_PinResistSound = ConfigManager::GetString( sPinResistSound, "", sDefinitionName );

	STATICHASH( OpenedSound );
	m_OpenedSound = ConfigManager::GetString( sOpenedSound, "", sDefinitionName );

	STATICHASH( CameraLerpTime );
	m_CameraLerpTime = ConfigManager::GetFloat( sCameraLerpTime, 0.0f, sDefinitionName );

	WBActionFactory::InitializeActionArray( sDefinitionName, "Start",	m_StartActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "Success",	m_SuccessActions );
}

void RosaLockpicking::InitializeLockFromDefinition( const SimpleString& LockDef )
{
	WBCompStatMod* const pStatMod = GetStatMod();

	MAKEHASH( LockDef );

	STATICHASH( NumPins );
	WB_MODIFY_FLOAT( Lock_NumPins, ConfigManager::GetInheritedFloat( sNumPins, 0.0f, sLockDef ), pStatMod );
	m_PinsRemaining = m_MaxPins = Max( 1, static_cast<int>( Round( WB_MODDED( Lock_NumPins ) ) ) );

	STATICHASH( PinAngle );
	WB_MODIFY_FLOAT( Lock_PinAngle, ConfigManager::GetInheritedFloat( sPinAngle, 0.0f, sLockDef ), pStatMod );
	m_CosPinAngle = Cos( DEGREES_TO_RADIANS( WB_MODDED( Lock_PinAngle ) ) );

	STATICHASH( ForceChance );
	WB_MODIFY_FLOAT( Lock_ForceChance, ConfigManager::GetInheritedFloat( sForceChance, 0.0f, sLockDef ), pStatMod );
	m_ForceChance = WB_MODDED( Lock_ForceChance );

	STATICHASH( ForceFailPins );
	WB_MODIFY_FLOAT( Lock_ForceFailPins, ConfigManager::GetInheritedFloat( sForceFailPins, 0.0f, sLockDef ), pStatMod );
	m_ForceFailPins = Max( 0, static_cast<int>( Round( WB_MODDED( Lock_ForceFailPins ) ) ) );

	STATICHASH( ExtraForcePins );
	WB_MODIFY_FLOAT( Lock_ExtraForcePins, ConfigManager::GetInheritedFloat( sExtraForcePins, 0.0f, sLockDef ), pStatMod );
	m_ExtraForcePins = Max( 0, static_cast<int>( Round( WB_MODDED( Lock_ExtraForcePins ) ) ) );
}

InputSystem* RosaLockpicking::GetInputSystem() const
{
	return RosaFramework::GetInstance()->GetInputSystem();
}

WBCompStatMod* RosaLockpicking::GetStatMod() const
{
	WBEntity* const			pPlayer		= RosaGame::GetPlayer();
	WBCompStatMod* const	pStatMod	= WB_GETCOMP( pPlayer, StatMod );
	return pStatMod;
}

void RosaLockpicking::PlaySoundDef( const SimpleString& SoundDef )
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	WB_MAKE_EVENT( PlaySoundDef, pPlayer );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, SoundDef );
	WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pPlayer );
}

void RosaLockpicking::StopSoundDef( const SimpleString& SoundDef )
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	WB_MAKE_EVENT( StopSound, pPlayer );
	WB_SET_AUTO( StopSound, Hash, Sound, SoundDef );
	WB_DISPATCH_EVENT( pEventManager, StopSound, pPlayer );
}

void RosaLockpicking::SetSoundVolume( const SimpleString& SoundDef, const float Volume )
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	WB_MAKE_EVENT( SetSoundVolume, pPlayer );
	WB_SET_AUTO( SetSoundVolume, Hash, Sound, SoundDef );
	WB_SET_AUTO( SetSoundVolume, Float, Volume, Volume );
	WB_DISPATCH_EVENT( pEventManager, SetSoundVolume, pPlayer );
}

void RosaLockpicking::PlayAINoise( const float Radius, const float CertaintyScalar )
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();
	const Vector			PlayerLocation	= RosaGame::GetPlayerLocation();

	WB_MAKE_EVENT(	OnAINoise, pPlayer );
	WB_SET_AUTO(	OnAINoise, Entity,	NoiseEntity,			pPlayer );
	WB_SET_AUTO(	OnAINoise, Vector,	NoiseLocation,			PlayerLocation );
	WB_SET_AUTO(	OnAINoise, Float,	NoiseRadius,			Radius );
	WB_SET_AUTO(	OnAINoise, Float,	NoiseCertaintyScalar,	CertaintyScalar );
	WB_DISPATCH_EVENT( pEventManager, OnAINoise, NULL );
}

/*virtual*/ void RosaLockpicking::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( BeginLockpicking );
	STATIC_HASHED_STRING( EndLockpicking );
	STATIC_HASHED_STRING( SucceedLockpick );
	STATIC_HASHED_STRING( Lockpicking_EndBind );
	STATIC_HASHED_STRING( Lockpicking_EndForce );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sBeginLockpicking )
	{
		STATIC_HASHED_STRING( LockDef );
		const SimpleString LockDef = Event.GetString( sLockDef );

		STATIC_HASHED_STRING( UseCameraOverride );
		m_UseCameraOverride = Event.GetBool( sUseCameraOverride );

		STATIC_HASHED_STRING( CameraTranslation );
		m_CameraTranslation = Event.GetVector( sCameraTranslation );

		STATIC_HASHED_STRING( CameraOrientation );
		m_CameraOrientation = Event.GetAngles( sCameraOrientation );

		InitializeLockFromDefinition( LockDef );
		BeginLockpicking();
	}
	else if( EventName == sEndLockpicking )
	{
		EndLockpicking();
	}
	else if( EventName == sSucceedLockpick )
	{
		SucceedLockpick();
	}
	else if( EventName == sLockpicking_EndBind )
	{
		m_IsBinding = false;
	}
	else if( EventName == sLockpicking_EndForce )
	{
		m_IsForcing = false;
	}
}

void RosaLockpicking::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;

	if( !m_IsLockpicking )
	{
		return;
	}

	if( m_IsInputTimedOut )
	{
		m_InputTimeoutRemaining -= DeltaTime;
		if( m_InputTimeoutRemaining <= 0.0f )
		{
			m_IsInputTimedOut = false;
		}
	}

	WBEntity* const			pPlayer			= RosaGame::GetPlayer();
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	InputSystem* const		pInputSystem	= GetInputSystem();
	Mouse* const			pMouse			= pInputSystem->GetMouse();
	XInputController* const	pController		= pInputSystem->GetController();

	// HACKHACK: Assumptions! Zeta currently only has Tourist, Easy, and Normal.
	//const bool				NormalOrEasier	= RosaDifficulty::GetGameDifficulty() <= 2;
	const bool				NormalOrEasier	= true;
	const float				LockpickDotPin	= m_LockpickVector.Dot( m_PinVector );

	if( LockpickDotPin > m_CosPinAngle && NormalOrEasier )
	{
		STATIC_HASHED_STRING( LockpickPinAngle );
		GetStatMod()->TriggerEvent( sLockpickPinAngle );
	}
	else
	{
		STATIC_HASHED_STRING( LockpickPinAngle );
		GetStatMod()->UnTriggerEvent( sLockpickPinAngle );
	}

	if( m_IsBinding || m_IsForcing )
	{
		// Silence rattle and do nothing with input or pin shake until force animation completes
		SetSoundVolume( m_RattleSound, 0.0f );
	}
	else
	{
		Vector2				NewLockpickVector		= m_LockpickVector;

		if( m_IsActive )
		{
			// Adjust rattle volume relative to how close lockpick is to pin center
			const float		ProximityAlpha			= Pow( Saturate( InvLerp( LockpickDotPin, m_CosPinAngle, 1.0f ) ), 0.5f );
			const float		kRattleVolume			= Lerp( m_RattleVolumeLow,	m_RattleVolumeHigh,	ProximityAlpha );
			SetSoundVolume( m_RattleSound, kRattleVolume );

			// Add some random noise to lockpick vector relative to how close lockpick is to pin center
			const float		kPinShakeScalar			= Lerp( m_PinShakeLow,		m_PinShakeHigh,		ProximityAlpha );
			const Matrix	RandomRotation			= Matrix::CreateRotationAboutZ( Math::Random( 0.0f, TWOPI ) );
			const Vector2	RandomVector			= ( Vector2( 1.0f, 0.0f ) * RandomRotation ).GetNormalized();
			NewLockpickVector						+= ( RandomVector * kPinShakeScalar ) * DeltaTime;
		}

		{
			// ROSANOTE: Radial lockpick control copied from UIScreenRosaRadial
			static const float		skMouseScalar	= 0.0625f;
			const float X =
				skMouseScalar * pMouse->GetPosition( Mouse::EA_X ) +
				pController->GetPosition( XInputController::EA_LeftThumbX ) +
				pController->GetPosition( XInputController::EA_RightThumbX );
			const float Y =
				-1.0f * skMouseScalar * pMouse->GetPosition( Mouse::EA_Y ) +
				pController->GetPosition( XInputController::EA_LeftThumbY ) +
				pController->GetPosition( XInputController::EA_RightThumbY );

			static const float	skMagSqThreshold	= Square( 0.1f );	// Lower threshold than radial uses, but thresholded nonetheless
			const Vector2		InputOffset			= Vector2( X, Y );
			const float			InputMagSq			= InputOffset.LengthSquared();
			if( InputMagSq >= skMagSqThreshold )
			{
				const Vector2	InputVector			= InputOffset.GetNormalized();
				NewLockpickVector					+= ( InputVector * m_LockpickVelocity ) * DeltaTime;
			}
		}

		SetLockpickVector( NewLockpickVector.GetNormalized() );
	}

	STATIC_HASHED_STRING( LockpickTap );
	STATIC_HASHED_STRING( LockpickForce );
	STATIC_HASHED_STRING( LockpickExit );
	STATIC_HASHED_STRING( LockpickExitAlt );

	if( CanAcceptInput() && pInputSystem->OnRise( sLockpickTap ) )
	{
		m_IsInputTimedOut = true;
		m_InputTimeoutRemaining = m_InputTimeout;

		// Tap the current pin
		if( LockpickDotPin > m_CosPinAngle )
		{
			// Bind pin
			m_IsBinding = true;

			// Play Bind hand animation (only when we actually bind, not when we tap and miss)
			{
				STATIC_HASHED_STRING( Bind );
				WB_MAKE_EVENT( PlayHandAnim, pPlayer );
				WB_SET_AUTO( PlayHandAnim, Hash,	AnimationName,	sBind );
				WB_SET_AUTO( PlayHandAnim, Bool,	Loop,			false );
				WB_SET_AUTO( PlayHandAnim, Float,	PlayRate,		1.0f );
				WB_SET_AUTO( PlayHandAnim, Float,	BlendTime,		0.05f );
				WB_DISPATCH_EVENT( pEventManager, PlayHandAnim, pPlayer );
			}

			PlaySoundDef( m_PinBindSound );
			ContinueLockpick();
		}
		else
		{
			// Missed binding pin
			PlaySoundDef( m_PinMissSound );
		}
	}
	else if( CanAcceptInput() && pInputSystem->OnRise( sLockpickForce ) )
	{
		// Force the current pin(s)
		m_IsForcing = true;
		m_IsInputTimedOut = true;
		m_InputTimeoutRemaining = m_InputTimeout;

		// Play Force hand animation
		{
			STATIC_HASHED_STRING( Force );
			WB_MAKE_EVENT( PlayHandAnim, pPlayer );
			WB_SET_AUTO( PlayHandAnim, Hash,	AnimationName,	sForce );
			WB_SET_AUTO( PlayHandAnim, Bool,	Loop,			false );
			WB_SET_AUTO( PlayHandAnim, Float,	PlayRate,		1.0f );
			WB_SET_AUTO( PlayHandAnim, Float,	BlendTime,		0.05f );
			WB_DISPATCH_EVENT( pEventManager, PlayHandAnim, pPlayer );
		}

		if( Math::RandomF( m_ForceChance ) )
		{
			// Force multiple pins if we're stat modded to do that
			m_PinsRemaining -= Min( m_ExtraForcePins, m_PinsRemaining );

			PlaySoundDef( m_PinForceSound );
			PlayAINoise( m_ForceNoiseRadius, m_ForceNoiseScalar );
			ContinueLockpick();
		}
		else
		{
			// Add some pins, up to the max (add an extra one because ContinueLockpick subtracts 1)
			m_PinsRemaining = Min( m_MaxPins, m_PinsRemaining + m_ForceFailPins + 1 );

			PlaySoundDef( m_PinResistSound );
			PlayAINoise( m_ForceNoiseRadius, m_ForceNoiseScalar );
			ContinueLockpick();
		}
	}
	else if( pInputSystem->OnRise( sLockpickExit ) || pInputSystem->OnRise( sLockpickExitAlt ) )
	{
		EndLockpicking();
	}

#if BUILD_DEV
	// Alt + H succeeds lockpicking instantly
	Keyboard* const pKeyboard = RosaFramework::GetInstance()->GetKeyboard();
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_H ) )
	{
		SucceedLockpick();
	}
#endif // BUILD_DEV
}

void RosaLockpicking::ContinueLockpick()
{
	if( m_PinsRemaining == 0 )
	{
		m_IsActive =  false;
		PlaySoundDef( m_OpenedSound );
		StopSoundDef( m_RattleSound );
		QueueSucceedLockpick();
	}
	else
	{
		InitializeNextPin();
	}

	PublishToHUD();
}

void RosaLockpicking::QueueSucceedLockpick()
{
	WB_MAKE_EVENT( SucceedLockpick, NULL );
	m_GameEndEventUID = WB_QUEUE_EVENT_DELAY( WBWorld::GetInstance()->GetEventManager(), SucceedLockpick, NULL, m_GameEndDelay );
}

void RosaLockpicking::SucceedLockpick()
{
	// ROSATODO: Re-enable if desired
	//INCREMENT_STAT( "NumLockpicksCompleted", 1 );

	WBActionFactory::ExecuteActionArray( m_SuccessActions, WBEvent(), NULL );
	EndLockpicking();
}

void RosaLockpicking::SetCameraOverride( const bool Enable, const Vector& Translation, const Angles& Orientation, const float LerpTime )
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	{
		WB_MAKE_EVENT(		SetTranslationOverride, NULL );
		WB_SET_AUTO(		SetTranslationOverride, Bool,	Enable,					Enable );
		WB_SET_AUTO(		SetTranslationOverride, Vector,	TranslationOverride,	Translation );
		WB_SET_AUTO(		SetTranslationOverride, Float,	LerpTime,				LerpTime );
		WB_DISPATCH_EVENT(	pEventManager, SetTranslationOverride, pPlayer );
	}

	{
		WB_MAKE_EVENT(		SetOrientationOverride, NULL );
		WB_SET_AUTO(		SetOrientationOverride, Bool,	Enable,					Enable );
		WB_SET_AUTO(		SetOrientationOverride, Angles,	OrientationOverride,	Orientation );
		WB_SET_AUTO(		SetOrientationOverride, Float,	LerpTime,				LerpTime );
		WB_DISPATCH_EVENT(	pEventManager, SetOrientationOverride, pPlayer );
	}
}

void RosaLockpicking::InitializeNextPin()
{
	DEVASSERT( m_PinsRemaining > 0 );

	m_PinsRemaining--;

	const Matrix	RandomRotation	= Matrix::CreateRotationAboutZ( Math::Random( 0.0f, TWOPI ) );
	m_PinVector						= ( Vector2( 1.0f, 0.0f ) * RandomRotation ).GetNormalized();
	DEBUGPRINTF( "Pin vector: %f, %f\n", m_PinVector.x, m_PinVector.y );
}

void RosaLockpicking::SetLockpickVector( const Vector2& LockpickVector )
{
	// LockpickVector should be normalized
	DEVASSERT( Equal( LockpickVector.LengthSquared(), 1.0f, EPSILON ) );
	m_LockpickVector = LockpickVector;

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	STATIC_HASHED_STRING( Down_Right );
	STATIC_HASHED_STRING( Down_Left );
	STATIC_HASHED_STRING( Up_Left );
	STATIC_HASHED_STRING( Up_Right );

	const bool			HorizontalBlend	= Abs( m_LockpickVector.x ) < Abs( m_LockpickVector.y );
	const float			BlendTerm		= HorizontalBlend ? m_LockpickVector.x : m_LockpickVector.y;
	static const float	skCos45			= 0.707107f;
	const float			BlendAlpha		= Saturate( ( BlendTerm + skCos45 ) * skCos45 );	// Since lockpick vector is normalized, scale [~-.707,~.707] to [0,1]

	HashedString	AnimationA;
	HashedString	AnimationB;
	if( HorizontalBlend )
	{
		const bool Down	= m_LockpickVector.y < 0.0f;
		AnimationA		= Down ? sDown_Left		: sUp_Left;
		AnimationB		= Down ? sDown_Right	: sUp_Right;
	}
	else
	{
		const bool Left	= m_LockpickVector.x < 0.0f;
		AnimationA		= Left ? sDown_Left	: sDown_Right;
		AnimationB		= Left ? sUp_Left	: sUp_Right;
	}

	WB_MAKE_EVENT( SetHandAnimBlend, pPlayer );
	WB_SET_AUTO( SetHandAnimBlend, Hash,	AnimationNameA,	AnimationA );
	WB_SET_AUTO( SetHandAnimBlend, Hash,	AnimationNameB,	AnimationB );
	WB_SET_AUTO( SetHandAnimBlend, Float,	BlendAlpha,		BlendAlpha );
	WB_DISPATCH_EVENT( pEventManager, SetHandAnimBlend, pPlayer );
}

void RosaLockpicking::PublishToHUD()
{
	STATICHASH( RosaLockpicking );

	const uint ShowPinsRemaining = m_IsActive ? ( m_PinsRemaining + 1 ) : 0;
	STATICHASH( PinsRemaining );
	ConfigManager::SetInt( sPinsRemaining, ShowPinsRemaining, sRosaLockpicking );

	const uint PinsCompleted = m_MaxPins - ShowPinsRemaining;
	STATICHASH( PinsCompleted );
	ConfigManager::SetInt( sPinsCompleted, PinsCompleted, sRosaLockpicking );

	STATICHASH( MaxPins );
	ConfigManager::SetInt( sMaxPins, m_MaxPins, sRosaLockpicking );
}

void RosaLockpicking::BeginLockpicking()
{
	if( m_IsLockpicking )
	{
		return;
	}

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	// ROSATODO: Re-enable if desired
	//INCREMENT_STAT( "NumLockpicksAttempted", 1 );

	// If we're autolockpicking or playing in tourist mode, immediately succeed.
	// We still want to do all the lockpick screen setup so we don't need a
	// separate code path for EndLockpicking.
	WB_MODIFY_FLOAT( AutoLockpick, 0.0f, GetStatMod() );
	const bool AutoLockpick		= ( WB_MODDED( AutoLockpick ) != 0.0f );
	const bool Tourist			= RosaDifficulty::GetGameDifficulty() == 0;
	const bool ImmediateSuccess	= AutoLockpick || Tourist;

	// Initialize game state
	m_IsLockpicking = true;
	SetLockpickVector( Vector2( 1.0f, 0.0f ) );	// Initialize to a default position, why not. Always to the right.
	InitializeNextPin();
	m_IsActive			= true;
	m_IsInputTimedOut	= false;
	m_IsBinding			= false;
	m_IsForcing			= false;

	PublishToHUD();

	// Start appropriate sound
	if( ImmediateSuccess )
	{
		PlaySoundDef( m_OpenedSound );
	}
	else
	{
		PlaySoundDef( m_RattleSound );
		SetSoundVolume( m_RattleSound, 0.0f );
	}

	// Unqueue any delayed game end events
	// (We don't do this when ending lockpicking, because the player may manually close the
	// lockpick screen while waiting for the SucceedLockpick event to be dispatched.)
	pEventManager->UnqueueEvent( m_GameEndEventUID );

	// Push input context
	GetInputSystem()->PushContext( m_InputContext );

	// Put weapon down and raise lockpick
	{
		STATIC_HASHED_STRING( Lockpick );
		WB_MAKE_EVENT( BeginModalHands, pPlayer );
		WB_SET_AUTO( BeginModalHands, Hash, Item, sLockpick );
		WB_DISPATCH_EVENT( pEventManager, BeginModalHands, pPlayer );
	}

	// Set camera
	SetCameraOverride( m_UseCameraOverride, m_CameraTranslation, m_CameraOrientation, ImmediateSuccess ? 0.0f : m_CameraLerpTime );

	// Disable frobbing to hide the prompt
	{
		WB_MAKE_EVENT( DisableFrob, NULL );
		WB_DISPATCH_EVENT( pEventManager, DisableFrob, pPlayer );
	}

	// Disable player movement
	{
		WB_MAKE_EVENT( SetCanMove, NULL );
		WB_SET_AUTO( SetCanMove, Bool, CanMove, false );
		WB_DISPATCH_EVENT( pEventManager, SetCanMove, pPlayer );
	}

	// Push the lockpick UI screen
	{
		STATIC_HASHED_STRING( LockpickScreen );
		WB_MAKE_EVENT( PushUIScreen, NULL );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sLockpickScreen );
		WB_DISPATCH_EVENT( pEventManager, PushUIScreen, NULL );
	}

	// Hide the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
	}

	// Trigger lockpicking statmod
	STATIC_HASHED_STRING( Lockpicking );
	GetStatMod()->TriggerEvent( sLockpicking );

	// Suppress autosave (the only form of saving that works during lockpicking
	// is hard quitting the game, which forcibly ends lockpicking first).
	{
		WB_MAKE_EVENT( AddAutosaveSuppression, NULL );
		WB_DISPATCH_EVENT( pEventManager, AddAutosaveSuppression, pPlayer );
	}

	WBActionFactory::ExecuteActionArray( m_StartActions, WBEvent(), NULL );

	if( ImmediateSuccess )
	{
		SucceedLockpick();
	}
}

void RosaLockpicking::EndLockpicking()
{
	if( !m_IsLockpicking )
	{
		return;
	}

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	// We also need to know about autolockpick/tourist for exiting the camera mode now
	WB_MODIFY_FLOAT( AutoLockpick, 0.0f, GetStatMod() );
	const bool AutoLockpick		= ( WB_MODDED( AutoLockpick ) != 0.0f );
	const bool Tourist			= RosaDifficulty::GetGameDifficulty() == 0;
	const bool ImmediateSuccess	= AutoLockpick || Tourist;

	m_IsLockpicking = false;

	StopSoundDef( m_RattleSound );

	// Pop input context
	GetInputSystem()->PopContext( m_InputContext );

	// Tick input system once more to debounce e.g. frob and crouch inputs
	GetInputSystem()->Tick();

	// Put lockpick down and raise weapon
	{
		WB_MAKE_EVENT( EndModalHands, pPlayer );
		WB_DISPATCH_EVENT( pEventManager, EndModalHands, pPlayer );
	}

	// Reset camera
	SetCameraOverride( false, m_CameraTranslation, m_CameraOrientation, ImmediateSuccess ? 0.0f : m_CameraLerpTime );

	// Enable frobbing again
	{
		WB_MAKE_EVENT( EnableFrob, NULL );
		WB_DISPATCH_EVENT( pEventManager, EnableFrob, pPlayer );
	}

	// Enable player movement again
	{
		WB_MAKE_EVENT( SetCanMove, NULL );
		WB_SET_AUTO( SetCanMove, Bool, CanMove, true );
		WB_DISPATCH_EVENT( pEventManager, SetCanMove, pPlayer );
	}

	// Pop the lockpick UI screen
	{
		STATIC_HASHED_STRING( LockpickScreen );
		WB_MAKE_EVENT( RemoveUIScreen, NULL );
		WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sLockpickScreen );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), RemoveUIScreen, NULL );
	}

	// Show the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, false );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
	}

	// Untrigger lockpicking statmod(s)
	STATIC_HASHED_STRING( Lockpicking );
	GetStatMod()->UnTriggerEvent( sLockpicking );

	STATIC_HASHED_STRING( LockpickPinAngle );
	GetStatMod()->UnTriggerEvent( sLockpickPinAngle );

	// Unsuppress autosave (the only form of saving that works during lockpicking
	// is hard quitting the game, which forcibly ends lockpicking first).
	{
		WB_MAKE_EVENT( RemoveAutosaveSuppression, NULL );
		WB_DISPATCH_EVENT( pEventManager, RemoveAutosaveSuppression, pPlayer );
	}
}
