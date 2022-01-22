#include "core.h"
#include "wbcomprosaalarmtripper.h"
#include "idatastream.h"
#include "wbeventmanager.h"

WBCompRosaAlarmTripper::WBCompRosaAlarmTripper()
:	m_LinkedAlarmBox()
{
}

WBCompRosaAlarmTripper::~WBCompRosaAlarmTripper()
{
}

/*virtual*/ void WBCompRosaAlarmTripper::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( TripAlarm );
	STATIC_HASHED_STRING( LinkToAlarmBox );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sTripAlarm )
	{
		// Forward event to linked alarm box
		WBEntity* const	pAlarmBox			= m_LinkedAlarmBox.Get();
		DEVASSERT( pAlarmBox );

		GetEventManager()->DispatchEvent( Event, pAlarmBox );
	}
	else if( EventName == sLinkToAlarmBox )
	{
		STATIC_HASHED_STRING( AlarmBox );
		m_LinkedAlarmBox = Event.GetEntity( sAlarmBox );
	}
}

#define VERSION_EMPTY			0
#define VERSION_LINKEDALARMBOX	1
#define VERSION_CURRENT			1

uint WBCompRosaAlarmTripper::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += sizeof( WBEntityRef );	// m_LinkedAlarmBox

	return Size;
}

void WBCompRosaAlarmTripper::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.Write( sizeof( WBEntityRef ), &m_LinkedAlarmBox );
}

void WBCompRosaAlarmTripper::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_LINKEDALARMBOX )
	{
		Stream.Read( sizeof( WBEntityRef ), &m_LinkedAlarmBox );
	}
}
