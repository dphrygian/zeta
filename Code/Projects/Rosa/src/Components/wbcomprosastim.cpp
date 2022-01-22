#include "core.h"
#include "wbcomprosastim.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcompowner.h"
#include "wbentity.h"
#include "configmanager.h"
#include "idatastream.h"
#include "Achievements/iachievementmanager.h"

WBCompRosaStim::WBCompRosaStim()
:	m_Duration( 0.0f )
,	m_IsActive( false )
,	m_DeactivateEventUID( 0 )
,	m_StatModEvent()
{
}

WBCompRosaStim::~WBCompRosaStim()
{
}

/*virtual*/ void WBCompRosaStim::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Duration );
	m_Duration = ConfigManager::GetInheritedFloat( sDuration, 0.0f, sDefinitionName );

	STATICHASH( StatModEvent );
	m_StatModEvent = ConfigManager::GetInheritedHash( sStatModEvent, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBCompRosaStim::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnUsed );
	STATIC_HASHED_STRING( DeactivateStim );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnUsed )
	{
		Activate();
	}
	else if( EventName == sDeactivateStim )
	{
		Deactivate();
	}
}

// NOTE: This does not trigger OnStimActivated again if the stim is already active.
// Associated SFX and VFX that need to be played on each reactivation should be hooked
// up in script to the OnUsed event.
void WBCompRosaStim::Activate()
{
	INCREMENT_STAT( "NumStimsUsed", 1 );

	if( !m_IsActive )
	{
		WB_MAKE_EVENT( OnStimActivated, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), OnStimActivated, GetEntity() );
	}

	// If we don't have a duration, this is just a transient/one-shot stim, like a health pack
	if( m_Duration <= 0.0f )
	{
		return;
	}

	m_IsActive = true;

	// Trigger stat mod only for time-limited stims; otherwise, they'll never be deactivated
	if( m_StatModEvent != HashedString::NullString )
	{
		WBEntity* const pOwnerEntity = WBCompOwner::GetTopmostOwner( GetEntity() );
		WB_MAKE_EVENT( TriggerStatMod, pOwnerEntity );
		WB_SET_AUTO( TriggerStatMod, Hash, StatModEvent, m_StatModEvent );
		WB_DISPATCH_EVENT( GetEventManager(), TriggerStatMod, pOwnerEntity );
	}

	// Unqueue any previous deactivation and renew the duration
	GetEventManager()->UnqueueEvent( m_DeactivateEventUID );
	WB_MAKE_EVENT( DeactivateStim, GetEntity() );
	m_DeactivateEventUID = WB_QUEUE_EVENT_DELAY( GetEventManager(), DeactivateStim, GetEntity(), m_Duration );
}

// NOTE: This is only invoked for stims with durations.
// Transient/one-shot stims should not depend on it.
void WBCompRosaStim::Deactivate()
{
	WB_MAKE_EVENT( OnStimDeactivated, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnStimDeactivated, GetEntity() );

	m_IsActive = false;

	if( m_StatModEvent != HashedString::NullString )
	{
		WBEntity* const pOwnerEntity = WBCompOwner::GetTopmostOwner( GetEntity() );
		WB_MAKE_EVENT( UnTriggerStatMod, pOwnerEntity );
		WB_SET_AUTO( UnTriggerStatMod, Hash, StatModEvent, m_StatModEvent );
		WB_DISPATCH_EVENT( GetEventManager(), UnTriggerStatMod, pOwnerEntity );
	}
}

#define VERSION_EMPTY				0
#define VERSION_DEACTIVATEEVENTUID	1
#define VERSION_ISACTIVE			2
#define VERSION_CURRENT				2

uint WBCompRosaStim::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version

	Size += sizeof( TEventUID );	// m_DeactivateEventUID

	Size += 1;						// m_IsActive

	return Size;
}

void WBCompRosaStim::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.Write<TEventUID>( m_DeactivateEventUID );

	Stream.WriteBool( m_IsActive );
}

void WBCompRosaStim::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_DEACTIVATEEVENTUID )
	{
		Stream.Read<TEventUID>( m_DeactivateEventUID );
	}

	if( Version >= VERSION_ISACTIVE )
	{
		m_IsActive = Stream.ReadBool();
	}
}
