#include "core.h"
#include "wbcomprosaseccam.h"
#include "wbeventmanager.h"
#include "hashedstring.h"
#include "configmanager.h"
#include "wbcomprosatransform.h"
#include "quat.h"
#include "mathcore.h"
#include "idatastream.h"
#include "rosagame.h"
#include "Components/wbcomprosaheadtracker.h"
#include "hsv.h"

WBCompRosaSecCam::WBCompRosaSecCam()
:	m_CameraState( ECS_None )
,	m_OscillateState( EOS_Left )
,	m_OscillateStateTime( 0.0f )
,	m_FixedPitch( 0.0f )
,	m_MaxYaw( 0.0f )
,	m_MaxRotation( 0.0f )
,	m_OscillateVelocity( 0.0f )
,	m_TrackVelocity( 0.0f )
,	m_OscillateTurnTime( 0.0f )
,	m_OscillateWaitTime( 0.0f )
,	m_LightBlendDuration( 0.0f )
,	m_OscillateAlbedo()
,	m_OscillateSpec()
,	m_OscillateLightHSV()
,	m_TrackAlbedo()
,	m_TrackSpec()
,	m_TrackLightHSV()
,	m_AlarmAlbedo()
,	m_AlarmSpec()
,	m_AlarmLightHSV()
,	m_DisabledAlbedo()
,	m_DisabledSpec()
,	m_DisabledLightHSV()
,	m_LoopSound()
,	m_StopSound()
,	m_TrackSound()
{
}

WBCompRosaSecCam::~WBCompRosaSecCam()
{
}

/*virtual*/ void WBCompRosaSecCam::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( FixedPitch );
	m_FixedPitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sFixedPitch, 0.0f, sDefinitionName ) );

	STATICHASH( MaxYaw );
	m_MaxYaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxYaw, 0.0f, sDefinitionName ) );

	// Max rotation takes fixed pitch into account, to simplify math later
	const Angles	MaxRotationAngles	= Angles( m_FixedPitch, 0.0f, m_MaxYaw );
	const Quat		MaxRotationQuat		= MaxRotationAngles.ToQuaternion();
	m_MaxRotation						= MaxRotationQuat.GetAngle();

	STATICHASH( OscillateVelocity );
	m_OscillateVelocity = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOscillateVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( OscillateWaitTime );
	m_OscillateWaitTime = ConfigManager::GetInheritedFloat( sOscillateWaitTime, 0.0f, sDefinitionName );
	m_OscillateTurnTime = ( m_MaxYaw * 2.0f ) / m_OscillateVelocity;

	STATICHASH( TrackVelocity );
	m_TrackVelocity = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sTrackVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( LightBlendDuration );
	m_LightBlendDuration = ConfigManager::GetInheritedFloat( sLightBlendDuration, 0.0f, sDefinitionName );

	STATICHASH( OscillateAlbedo );
	m_OscillateAlbedo = ConfigManager::GetInheritedString( sOscillateAlbedo, "", sDefinitionName );

	STATICHASH( OscillateSpec );
	m_OscillateSpec = ConfigManager::GetInheritedString( sOscillateSpec, "", sDefinitionName );

	m_OscillateLightHSV = HSV::GetConfigHSV( "OscillateLight", sDefinitionName, Vector() );

	STATICHASH( TrackAlbedo );
	m_TrackAlbedo = ConfigManager::GetInheritedString( sTrackAlbedo, "", sDefinitionName );

	STATICHASH( TrackSpec );
	m_TrackSpec = ConfigManager::GetInheritedString( sTrackSpec, "", sDefinitionName );

	m_TrackLightHSV = HSV::GetConfigHSV( "TrackLight", sDefinitionName, Vector() );

	STATICHASH( AlarmAlbedo );
	m_AlarmAlbedo = ConfigManager::GetInheritedString( sAlarmAlbedo, "", sDefinitionName );

	STATICHASH( AlarmSpec );
	m_AlarmSpec = ConfigManager::GetInheritedString( sAlarmSpec, "", sDefinitionName );

	m_AlarmLightHSV = HSV::GetConfigHSV( "AlarmLight", sDefinitionName, Vector() );

	STATICHASH( DisabledAlbedo );
	m_DisabledAlbedo = ConfigManager::GetInheritedString( sDisabledAlbedo, "", sDefinitionName );

	STATICHASH( DisabledSpec );
	m_DisabledSpec = ConfigManager::GetInheritedString( sDisabledSpec, "", sDefinitionName );

	m_DisabledLightHSV = HSV::GetConfigHSV( "DisabledLight", sDefinitionName, Vector() );

	STATICHASH( LoopSound );
	m_LoopSound = ConfigManager::GetInheritedString( sLoopSound, "", sDefinitionName );

	STATICHASH( StopSound );
	m_StopSound = ConfigManager::GetInheritedString( sStopSound, "", sDefinitionName );

	STATICHASH( TrackSound );
	m_TrackSound = ConfigManager::GetInheritedString( sTrackSound, "", sDefinitionName );
}

/*virtual*/ void WBCompRosaSecCam::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawnedQueued );
	STATIC_HASHED_STRING( OnInitializedQueued );
	STATIC_HASHED_STRING( Oscillate );
	STATIC_HASHED_STRING( Track );
	STATIC_HASHED_STRING( TripAlarm );
	STATIC_HASHED_STRING( Disable );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawnedQueued )
	{
		Oscillate();
	}
	else if( EventName == sOnInitializedQueued )
	{
		// Set up headtracker
		WBEntity* const			pEntity			= GetEntity();
		WBEventManager* const	pEventManager	= GetEventManager();

		WB_MAKE_EVENT( SetMaxRotation, pEntity );
		WB_SET_AUTO( SetMaxRotation, Float, MaxRotation, m_MaxRotation );
		WB_DISPATCH_EVENT( pEventManager, SetMaxRotation, pEntity );

		WB_MAKE_EVENT( SetFixedPitch, pEntity );
		WB_SET_AUTO( SetFixedPitch, Bool, UseFixedPitch, true );
		WB_SET_AUTO( SetFixedPitch, Float, FixedPitch, m_FixedPitch );
		WB_DISPATCH_EVENT( pEventManager, SetFixedPitch, pEntity );
	}
	else if( EventName == sOscillate )
	{
		Oscillate();
	}
	else if( EventName == sTrack )
	{
		Track();
	}
	else if( EventName == sTripAlarm )
	{
		Alarm();
	}
	else if( EventName == sDisable )
	{
		Disable();
	}
}

void WBCompRosaSecCam::Oscillate()
{
	if( m_CameraState == ECS_Oscillate || m_CameraState == ECS_Disabled )
	{
		return;
	}

	const bool				WasActive		= ( m_CameraState == ECS_Track || m_CameraState == ECS_Alarm );
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	m_CameraState = ECS_Oscillate;

	if( WasActive )
	{
		WB_MAKE_EVENT( RemoveAutosaveSuppression, NULL );
		WB_SET_AUTO( RemoveAutosaveSuppression, Bool, Serialize, true );
		WB_DISPATCH_EVENT( pEventManager, RemoveAutosaveSuppression, RosaGame::GetPlayer() );
	}

	WB_MAKE_EVENT( SetLookVelocity, pEntity );
	WB_SET_AUTO( SetLookVelocity, Float, LookVelocity, m_OscillateVelocity );
	WB_DISPATCH_EVENT( pEventManager, SetLookVelocity, pEntity );

	{
		WB_MAKE_EVENT( SetTextures, pEntity );
		WB_SET_AUTO( SetTextures, Hash, AlbedoMap, m_OscillateAlbedo );
		WB_SET_AUTO( SetTextures, Hash, SpecMap, m_OscillateSpec );
		WB_DISPATCH_EVENT( pEventManager, SetTextures, pEntity );
	}

	{
		WB_MAKE_EVENT( SetLightColorHSV, pEntity );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorH, m_OscillateLightHSV.x );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorS, m_OscillateLightHSV.y );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorV, m_OscillateLightHSV.z );
		WB_SET_AUTO( SetLightColorHSV, Float, Duration, m_LightBlendDuration );
		WB_DISPATCH_EVENT( pEventManager, SetLightColorHSV, pEntity );
	}

	// Set m_OscillateStateTime properly so we resume oscillating for the proper duration in current direction.
	WBCompRosaHeadTracker* const	pTracker		= WB_GETCOMP( pEntity, RosaHeadTracker );
	DEVASSERT( pTracker );
	const float					CurrentYaw		= pTracker->GetLookAngles().Yaw;
	const float					CurrentTime		= GetTime();
	const bool					IsTurningLeft	= m_OscillateState == EOS_Left;
	const bool					IsTurningRight	= m_OscillateState == EOS_Right;

	if( IsTurningLeft )
	{
		m_OscillateStateTime = CurrentTime + ( ( m_MaxYaw - CurrentYaw ) / m_OscillateVelocity );
	}
	else if( IsTurningRight )
	{
		m_OscillateStateTime = CurrentTime + ( ( m_MaxYaw + CurrentYaw ) / m_OscillateVelocity );
	}
	else
	{
		m_OscillateStateTime = CurrentTime + m_OscillateWaitTime;
	}
	DEVASSERT( m_OscillateStateTime >= 0.0f );

	if( IsTurningLeft || IsTurningRight )
	{
		PlayLoopSound();
	}

	// ROSATODO: Fire OnOscillating event if needed? Most effects could be driven from BT, but that would centralize it to the entity.
}

void WBCompRosaSecCam::Track()
{
	if( m_CameraState == ECS_Track || m_CameraState == ECS_Disabled )
	{
		return;
	}

	const bool				WasIdle			= ( m_CameraState == ECS_Oscillate );
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	m_CameraState = ECS_Track;

	if( WasIdle )
	{
		WB_MAKE_EVENT( AddAutosaveSuppression, NULL );
		WB_SET_AUTO( AddAutosaveSuppression, Bool, Serialize, true );
		WB_DISPATCH_EVENT( pEventManager, AddAutosaveSuppression, RosaGame::GetPlayer() );
	}

	WB_MAKE_EVENT( SetLookVelocity, pEntity );
	WB_SET_AUTO( SetLookVelocity, Float, LookVelocity, m_TrackVelocity );
	WB_DISPATCH_EVENT( pEventManager, SetLookVelocity, pEntity );

	{
		WB_MAKE_EVENT( SetTextures, pEntity );
		WB_SET_AUTO( SetTextures, Hash, AlbedoMap, m_TrackAlbedo );
		WB_SET_AUTO( SetTextures, Hash, SpecMap, m_TrackSpec );
		WB_DISPATCH_EVENT( pEventManager, SetTextures, pEntity );
	}

	{
		WB_MAKE_EVENT( SetLightColorHSV, pEntity );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorH, m_TrackLightHSV.x );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorS, m_TrackLightHSV.y );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorV, m_TrackLightHSV.z );
		WB_SET_AUTO( SetLightColorHSV, Float, Duration, m_LightBlendDuration );
		WB_DISPATCH_EVENT( pEventManager, SetLightColorHSV, pEntity );
	}

	StopLoopSound();
	if( WasIdle )
	{
		PlayTrackSound();
	}

	// ROSATODO: Fire OnTracking event if needed? Most effects could be driven from BT, but that would centralize it to the entity.
}

void WBCompRosaSecCam::Alarm()
{
	if( m_CameraState == ECS_Alarm || m_CameraState == ECS_Disabled )
	{
		return;
	}

	const bool				WasIdle			= ( m_CameraState == ECS_Oscillate );	// It shouldn't be possible to skip ECS_Track, but just to be safe...
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	m_CameraState = ECS_Alarm;

	if( WasIdle )
	{
		WB_MAKE_EVENT( AddAutosaveSuppression, NULL );
		WB_SET_AUTO( AddAutosaveSuppression, Bool, Serialize, true );
		WB_DISPATCH_EVENT( pEventManager, AddAutosaveSuppression, RosaGame::GetPlayer() );
	}

	WB_MAKE_EVENT( SetLookVelocity, pEntity );
	WB_SET_AUTO( SetLookVelocity, Float, LookVelocity, m_TrackVelocity );
	WB_DISPATCH_EVENT( pEventManager, SetLookVelocity, pEntity );

	{
		WB_MAKE_EVENT( SetTextures, pEntity );
		WB_SET_AUTO( SetTextures, Hash, AlbedoMap, m_AlarmAlbedo );
		WB_SET_AUTO( SetTextures, Hash, SpecMap, m_AlarmSpec );
		WB_DISPATCH_EVENT( pEventManager, SetTextures, pEntity );
	}

	{
		WB_MAKE_EVENT( SetLightColorHSV, pEntity );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorH, m_AlarmLightHSV.x );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorS, m_AlarmLightHSV.y );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorV, m_AlarmLightHSV.z );
		WB_SET_AUTO( SetLightColorHSV, Float, Duration, m_LightBlendDuration );
		WB_DISPATCH_EVENT( pEventManager, SetLightColorHSV, pEntity );
	}

	StopLoopSound();

	// ROSATODO: Fire OnAlarm event if needed? Most effects could be driven from BT, but that would centralize it to the entity.
}

void WBCompRosaSecCam::Disable()
{
	if( m_CameraState == ECS_Disabled )
	{
		return;
	}

	const bool				WasActive		= ( m_CameraState == ECS_Track || m_CameraState == ECS_Alarm );
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	m_CameraState = ECS_Disabled;

	if( WasActive )
	{
		WB_MAKE_EVENT( RemoveAutosaveSuppression, NULL );
		WB_SET_AUTO( RemoveAutosaveSuppression, Bool, Serialize, true );
		WB_DISPATCH_EVENT( pEventManager, RemoveAutosaveSuppression, RosaGame::GetPlayer() );
	}

	{
		WB_MAKE_EVENT( SetTextures, pEntity );
		WB_SET_AUTO( SetTextures, Hash, AlbedoMap, m_DisabledAlbedo );
		WB_SET_AUTO( SetTextures, Hash, SpecMap, m_DisabledSpec );
		WB_DISPATCH_EVENT( pEventManager, SetTextures, pEntity );
	}

	{
		WB_MAKE_EVENT( SetLightColorHSV, pEntity );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorH, m_DisabledLightHSV.x );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorS, m_DisabledLightHSV.y );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorV, m_DisabledLightHSV.z );
		WB_SET_AUTO( SetLightColorHSV, Float, Duration, m_LightBlendDuration );
		WB_DISPATCH_EVENT( pEventManager, SetLightColorHSV, pEntity );
	}

	StopLoopSound();

	// ROSATODO: Fire OnDisabled event so we can change texture and play SFX. This is equivalent to death for AIs.
}

void WBCompRosaSecCam::PlayLoopSound()
{
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	WB_MAKE_EVENT( PlaySoundDef, pEntity );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_LoopSound );
	WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pEntity );
}

void WBCompRosaSecCam::StopLoopSound()
{
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	WB_MAKE_EVENT( StopSound, pEntity );
	WB_SET_AUTO( StopSound, Hash, Sound, m_LoopSound );
	WB_DISPATCH_EVENT( pEventManager, StopSound, pEntity );
}

void WBCompRosaSecCam::PlayStopSound()
{
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	WB_MAKE_EVENT( PlaySoundDef, pEntity );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_StopSound );
	WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pEntity );
}

void WBCompRosaSecCam::PlayTrackSound()
{
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	WB_MAKE_EVENT( PlaySoundDef, pEntity );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_TrackSound );
	WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pEntity );
}

/*virtual*/ void WBCompRosaSecCam::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	if( m_CameraState == ECS_Oscillate )
	{
		TickOscillate( DeltaTime );
	}
}

void WBCompRosaSecCam::TickOscillate( const float DeltaTime )
{
	XTRACE_FUNCTION;

	Unused( DeltaTime );

	// Advance oscillate state
	const float CurrentTime = GetTime();
	if( CurrentTime > m_OscillateStateTime )
	{
		switch( m_OscillateState )
		{
		case EOS_Left:		m_OscillateState = EOS_LeftWait;	StopLoopSound();	PlayStopSound();	m_OscillateStateTime = CurrentTime + m_OscillateWaitTime; break;
		case EOS_LeftWait:	m_OscillateState = EOS_Right;		PlayLoopSound();						m_OscillateStateTime = CurrentTime + m_OscillateTurnTime; break;
		case EOS_Right:		m_OscillateState = EOS_RightWait;	StopLoopSound();	PlayStopSound();	m_OscillateStateTime = CurrentTime + m_OscillateWaitTime; break;
		case EOS_RightWait:	m_OscillateState = EOS_Left;		PlayLoopSound();						m_OscillateStateTime = CurrentTime + m_OscillateTurnTime; break;
		}
	}

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();

	const bool		IsLeft					= ( m_OscillateState == EOS_Left || m_OscillateState == EOS_LeftWait );
	const float		OscillationTargetYaw	= pTransform->GetOrientation().Yaw + ( IsLeft ? m_MaxYaw : -m_MaxYaw );
	const Angles	OscillationLookTarget	= Angles( 0.0f, 0.0f, OscillationTargetYaw );

	// Update head tracker
	WB_MAKE_EVENT( LookAt, pEntity );
	WB_SET_AUTO( LookAt, Angles, LookAtAngles, OscillationLookTarget );
	WB_DISPATCH_EVENT( GetEventManager(), LookAt, pEntity );
}

#define VERSION_EMPTY			0
#define VERSION_CAMERASTATE		1
#define VERSION_OSCILLATESTATE	2
#define VERSION_CURRENT			2

uint WBCompRosaSecCam::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_CameraState

	Size += 4;	// m_OscillateState
	Size += 4;	// m_OscillateStateTime

	return Size;
}

void WBCompRosaSecCam::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_CameraState );

	Stream.WriteUInt32( m_OscillateState );
	Stream.WriteFloat( m_OscillateStateTime - GetTime() );
}

void WBCompRosaSecCam::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_CAMERASTATE )
	{
		m_CameraState = static_cast<ECameraState>( Stream.ReadUInt32() );
	}

	if( Version >= VERSION_OSCILLATESTATE )
	{
		m_OscillateState		= static_cast<EOscillateState>( Stream.ReadUInt32() );
		m_OscillateStateTime	= GetTime() + Stream.ReadFloat();
	}
}
