#include "core.h"
#include "wbcomprosaalarmbox.h"
#include "hashedstring.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbcomprosaalarmtripper.h"
#include "wbcomponentarrays.h"
#include "wbcomprosatransform.h"
#include "rosagame.h"

WBCompRosaAlarmBox::WBCompRosaAlarmBox()
:	m_LinkedAlarmTrippers()
,	m_AlarmState( EAS_None )
,	m_AlarmTarget()
,	m_AlarmTargetLocation()
,	m_AlarmTargetTimestamp( 0.0f )
,	m_AlarmExpireEventUID( 0 )
,	m_AlarmRepeatEventUID( 0 )
,	m_AlarmDuration( 0.0f )
,	m_AlarmRadius( 0.0f )
,	m_AlarmNoiseRepeatTime( 0.0f )
,	m_AlarmNoiseCertaintyScalar( 0.0f )
,	m_IdleAlbedo()
,	m_IdleSpec()
,	m_ActiveAlbedo()
,	m_ActiveSpec()
,	m_DisabledAlbedo()
,	m_DisabledSpec()
,	m_AlarmSound()
,	m_IdleSound()
,	m_DisableSound()
{
}

WBCompRosaAlarmBox::~WBCompRosaAlarmBox()
{
}

/*virtual*/ void WBCompRosaAlarmBox::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AlarmDuration );
	m_AlarmDuration = ConfigManager::GetInheritedFloat( sAlarmDuration, 0.0f, sDefinitionName );

	STATICHASH( AlarmRadius );
	m_AlarmRadius = ConfigManager::GetInheritedFloat( sAlarmRadius, 0.0f, sDefinitionName );

	STATICHASH( AlarmNoiseRepeatTime );
	m_AlarmNoiseRepeatTime = ConfigManager::GetInheritedFloat( sAlarmNoiseRepeatTime, 0.0f, sDefinitionName );

	STATICHASH( AlarmNoiseCertaintyScalar );
	m_AlarmNoiseCertaintyScalar = ConfigManager::GetInheritedFloat( sAlarmNoiseCertaintyScalar, 0.0f, sDefinitionName );

	STATICHASH( IdleAlbedo );
	m_IdleAlbedo = ConfigManager::GetInheritedString( sIdleAlbedo, "", sDefinitionName );

	STATICHASH( IdleSpec );
	m_IdleSpec = ConfigManager::GetInheritedString( sIdleSpec, "", sDefinitionName );

	STATICHASH( ActiveAlbedo );
	m_ActiveAlbedo = ConfigManager::GetInheritedString( sActiveAlbedo, "", sDefinitionName );

	STATICHASH( ActiveSpec );
	m_ActiveSpec = ConfigManager::GetInheritedString( sActiveSpec, "", sDefinitionName );

	STATICHASH( DisabledAlbedo );
	m_DisabledAlbedo = ConfigManager::GetInheritedString( sDisabledAlbedo, "", sDefinitionName );

	STATICHASH( DisabledSpec );
	m_DisabledSpec = ConfigManager::GetInheritedString( sDisabledSpec, "", sDefinitionName );

	STATICHASH( AlarmSound );
	m_AlarmSound = ConfigManager::GetInheritedString( sAlarmSound, "", sDefinitionName );

	STATICHASH( IdleSound );
	m_IdleSound = ConfigManager::GetInheritedString( sIdleSound, "", sDefinitionName );

	STATICHASH( DisableSound );
	m_DisableSound = ConfigManager::GetInheritedString( sDisableSound, "", sDefinitionName );
}

/*virtual*/ void WBCompRosaAlarmBox::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetLinkedEntities );
	STATIC_HASHED_STRING( OnSpawnedQueued );
	STATIC_HASHED_STRING( TripAlarm );
	STATIC_HASHED_STRING( ExpireAlarm );
	STATIC_HASHED_STRING( DisableAlarm );
	STATIC_HASHED_STRING( ConditionalPlayAINoise );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetLinkedEntities )
	{
		STATIC_HASHED_STRING( LinkedEntities );
		void* pVoid = Event.GetPointer( sLinkedEntities );
		const Array<WBEntityRef>* const pLinkedEntities = static_cast<const Array<WBEntityRef>*>( pVoid );
		DEVASSERT( pLinkedEntities );

		SetLinkedAlarmTrippers( *pLinkedEntities );
	}
	else if( EventName == sOnSpawnedQueued )
	{
		IdleAlarm();
	}
	else if( EventName == sTripAlarm )
	{
		STATIC_HASHED_STRING( AlarmTarget );
		WBEntity* const	pAlarmTarget		= Event.GetEntity( sAlarmTarget );

		STATIC_HASHED_STRING( AlarmTargetLocation );
		const Vector	AlarmTargetLocation	= Event.GetVector( sAlarmTargetLocation );

		TripAlarm( pAlarmTarget, AlarmTargetLocation );
	}
	else if( EventName == sExpireAlarm )
	{
		IdleAlarm();
	}
	else if( EventName == sDisableAlarm )
	{
		DisableAlarm();
	}
	else if( EventName == sConditionalPlayAINoise )
	{
		ConditionalPlayAINoise();
	}
}

void WBCompRosaAlarmBox::SetLinkedAlarmTrippers( const Array<WBEntityRef>& LinkedEntities )
{
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	DEVASSERT( m_LinkedAlarmTrippers.Empty() );
	m_LinkedAlarmTrippers.Reserve( LinkedEntities.Size() );

	FOR_EACH_ARRAY( LinkedEntityIter, LinkedEntities, WBEntityRef )
	{
		const WBEntityRef&	LinkedEntityRef	= LinkedEntityIter.GetValue();
		WBEntity* const		pLinkedEntity	= LinkedEntityRef.Get();
		DEVASSERT( pLinkedEntity );

		WBCompRosaAlarmTripper* const pAlarmTripper = WB_GETCOMP( pLinkedEntity, RosaAlarmTripper );
		if( !pAlarmTripper )
		{
			continue;
		}

		m_LinkedAlarmTrippers.PushBack( pLinkedEntity );

		// Notify the entity that it is linked to this alarm box
		WB_MAKE_EVENT( LinkToAlarmBox, pLinkedEntity );
		WB_SET_AUTO( LinkToAlarmBox, Entity, AlarmBox, pEntity );
		WB_DISPATCH_EVENT( pEventManager, LinkToAlarmBox, pLinkedEntity );
	}
}

void WBCompRosaAlarmBox::IdleAlarm()
{
	if( m_AlarmState == EAS_Idle || m_AlarmState == EAS_Disabled )
	{
		return;
	}

	const bool				WasActive			= ( m_AlarmState == EAS_Active );
	WBEntity* const			pEntity				= GetEntity();
	WBEventManager* const	pEventManager		= GetEventManager();

	m_AlarmState = EAS_Idle;

	if( WasActive )
	{
		WB_MAKE_EVENT( RemoveAutosaveSuppression, NULL );
		WB_SET_AUTO( RemoveAutosaveSuppression, Bool, Serialize, true );
		WB_DISPATCH_EVENT( pEventManager, RemoveAutosaveSuppression, RosaGame::GetPlayer() );
	}

	{
		WB_MAKE_EVENT( SetTextures, pEntity );
		WB_SET_AUTO( SetTextures, Hash, AlbedoMap, m_IdleAlbedo );
		WB_SET_AUTO( SetTextures, Hash, SpecMap, m_IdleSpec );
		WB_DISPATCH_EVENT( GetEventManager(), SetTextures, pEntity );
	}

	{
		WB_MAKE_EVENT( StopSound, pEntity );
		WB_SET_AUTO( StopSound, Hash, Sound, m_AlarmSound );
		WB_DISPATCH_EVENT( pEventManager, StopSound, pEntity );
	}

	if( WasActive )
	{
		WB_MAKE_EVENT( PlaySoundDef, pEntity );
		WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_IdleSound );
		WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pEntity );
	}
}


void WBCompRosaAlarmBox::TripAlarm( WBEntity* const pAlarmTarget, const Vector& AlarmTargetLocation )
{
	// NOTE: *Do* repeat this if we're already active; this will extend the alarm duration as intended.
	if( m_AlarmState == EAS_Disabled )
	{
		return;
	}

	DEVASSERT( pAlarmTarget );
	DEVASSERT( !AlarmTargetLocation.IsZero() );

	const bool				WasAlreadyActive	= ( m_AlarmState == EAS_Active );
	WBEntity* const			pEntity				= GetEntity();
	WBEventManager* const	pEventManager		= GetEventManager();

	m_AlarmState = EAS_Active;

	if( !WasAlreadyActive )
	{
		WBEntity* const pPlayer = RosaGame::GetPlayer();

		WB_MAKE_EVENT( AddAutosaveSuppression, NULL );
		WB_SET_AUTO( AddAutosaveSuppression, Bool, Serialize, true );
		WB_DISPATCH_EVENT( pEventManager, AddAutosaveSuppression, pPlayer );

		// Hook for counting alarms for results screen mostly
		WB_MAKE_EVENT( OnAlarmTripped, NULL );
		WB_DISPATCH_EVENT( pEventManager, OnAlarmTripped, pPlayer );
	}

	m_AlarmTarget			= pAlarmTarget;
	m_AlarmTargetLocation	= AlarmTargetLocation;
	m_AlarmTargetTimestamp	= GetTime();

	{
		WB_MAKE_EVENT( SetTextures, pEntity );
		WB_SET_AUTO( SetTextures, Hash, AlbedoMap, m_ActiveAlbedo );
		WB_SET_AUTO( SetTextures, Hash, SpecMap, m_ActiveSpec );
		WB_DISPATCH_EVENT( GetEventManager(), SetTextures, pEntity );
	}

	if( !WasAlreadyActive )
	{
		// If we're already active, we'll just wait for our next queued noise event to notify AIs
		ConditionalPlayAINoise();

		{
			WB_MAKE_EVENT( PlaySoundDef, pEntity );
			WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_AlarmSound );
			WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pEntity );
		}
	}

	// Unqueue any ConditionalPlayAINoise event we're currently waiting for
	pEventManager->UnqueueEvent( m_AlarmExpireEventUID );

	WB_MAKE_EVENT( ExpireAlarm, pEntity );
	m_AlarmExpireEventUID = WB_QUEUE_EVENT_DELAY( pEventManager, ExpireAlarm, pEntity, m_AlarmDuration );
}

void WBCompRosaAlarmBox::DisableAlarm()
{
	if( m_AlarmState == EAS_Disabled )
	{
		return;
	}

	const bool				WasActive		= ( m_AlarmState == EAS_Active );
	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	m_AlarmState = EAS_Disabled;

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
		WB_DISPATCH_EVENT( GetEventManager(), SetTextures, pEntity );
	}

	{
		WB_MAKE_EVENT( StopSound, pEntity );
		WB_SET_AUTO( StopSound, Hash, Sound, m_AlarmSound );
		WB_DISPATCH_EVENT( pEventManager, StopSound, pEntity );
	}

	{
		WB_MAKE_EVENT( PlaySoundDef, pEntity );
		WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_DisableSound );
		WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pEntity );
	}

	FOR_EACH_ARRAY( LinkedEntityIter, m_LinkedAlarmTrippers, WBEntityRef )
	{
		const WBEntityRef&	LinkedEntityRef = LinkedEntityIter.GetValue();
		WBEntity* const		pLinkedEntity	= LinkedEntityRef.Get();

		if( !pLinkedEntity )
		{
			continue;
		}

		WB_MAKE_EVENT( OnAlarmBoxDisabled, pLinkedEntity );
		WB_DISPATCH_EVENT( pEventManager, OnAlarmBoxDisabled, pLinkedEntity );
	}
}

void WBCompRosaAlarmBox::ConditionalPlayAINoise()
{
	// Only send the AI noise if we're still in the alarmed mode
	if( m_AlarmState != EAS_Active )
	{
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	WBEventManager* const		pEventManager	= GetEventManager();

	WB_MAKE_EVENT( OnAINoise, pEntity );
	WB_SET_AUTO( OnAINoise, Entity,	NoiseEntity,			m_AlarmTarget );
	WB_SET_AUTO( OnAINoise, Vector,	NoiseLocation,			pTransform->GetLocation() );
	WB_SET_AUTO( OnAINoise, Vector,	NoiseSourceLocation,	m_AlarmTargetLocation );
	WB_SET_AUTO( OnAINoise, Float,	NoiseRadius,			m_AlarmRadius );
	WB_SET_AUTO( OnAINoise, Float,	NoiseCertaintyScalar,	m_AlarmNoiseCertaintyScalar );
	WB_SET_AUTO( OnAINoise, Float,	NoiseUpdateTime,		m_AlarmTargetTimestamp );
	WB_DISPATCH_EVENT( pEventManager, OnAINoise, NULL );

	// Unqueue any ConditionalPlayAINoise event we're currently waiting for
	pEventManager->UnqueueEvent( m_AlarmRepeatEventUID );

	WB_MAKE_EVENT( ConditionalPlayAINoise, pEntity );
	m_AlarmRepeatEventUID = WB_QUEUE_EVENT_DELAY( pEventManager, ConditionalPlayAINoise, pEntity, m_AlarmNoiseRepeatTime );
}

#define VERSION_EMPTY				0
#define VERSION_LINKEDALARMTRIPPERS	1
#define VERSION_ALARMSTATE			2
#define VERSION_ALARMTARGET			3
#define VERSION_ALARMEVENTUIDS		4
#define VERSION_CURRENT				4

uint WBCompRosaAlarmBox::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;														// m_LinkedAlarmTrippers.Size()
	Size += sizeof( WBEntityRef ) * m_LinkedAlarmTrippers.Size();	// m_LinkedAlarmTrippers

	Size += 4;	// m_AlarmState

	Size += sizeof( WBEntityRef );	// m_AlarmTarget
	Size += sizeof( Vector );		// m_AlarmTargetLocation
	Size += 4;						// m_AlarmTargetTimestamp

	Size += sizeof( TEventUID );	// m_AlarmRepeatEventUID

	return Size;
}

void WBCompRosaAlarmBox::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteArray( m_LinkedAlarmTrippers );

	Stream.WriteUInt32( m_AlarmState );

	Stream.Write( sizeof( WBEntityRef ), &m_AlarmTarget );
	Stream.Write( sizeof( Vector ), &m_AlarmTargetLocation );
	Stream.WriteFloat( m_AlarmTargetTimestamp );	// Don't try to fix this up relative to world time; knowledge system already handles that

	Stream.Write<TEventUID>( m_AlarmRepeatEventUID );
	Stream.Write<TEventUID>( m_AlarmExpireEventUID );
}

void WBCompRosaAlarmBox::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_LINKEDALARMTRIPPERS )
	{
		DEVASSERT( m_LinkedAlarmTrippers.Empty() );
		Stream.ReadArray( m_LinkedAlarmTrippers );
	}

	if( Version >= VERSION_ALARMSTATE )
	{
		m_AlarmState = static_cast<EAlarmState>( Stream.ReadUInt32() );
	}

	if( Version >= VERSION_ALARMTARGET )
	{
		Stream.Read( sizeof( WBEntityRef ), &m_AlarmTarget );
		Stream.Read( sizeof( Vector ), &m_AlarmTargetLocation );
		m_AlarmTargetTimestamp = Stream.ReadFloat();	// Don't try to fix this up relative to world time; knowledge system already handles that
	}

	if( Version >= VERSION_ALARMEVENTUIDS )
	{
		Stream.Read<TEventUID>( m_AlarmRepeatEventUID );
		Stream.Read<TEventUID>( m_AlarmExpireEventUID );
	}
}
