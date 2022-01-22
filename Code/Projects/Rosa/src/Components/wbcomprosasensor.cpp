#include "core.h"
#include "wbcomprosasensor.h"
#include "wbevent.h"
#include "idatastream.h"

WBCompRosaSensor::WBCompRosaSensor()
:	m_Paused( false )
{
}

WBCompRosaSensor::~WBCompRosaSensor()
{

}

/*virtual*/ void WBCompRosaSensor::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	WBRosaComponent::HandleEvent( Event );

	STATIC_HASHED_STRING( TickSensors );
	STATIC_HASHED_STRING( PauseSensors );
	STATIC_HASHED_STRING( UnpauseSensors );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sTickSensors )
	{
		Tick( 0.0f );
	}
	else if( EventName == sPauseSensors )
	{
		m_Paused = true;
	}
	else if( EventName == sUnpauseSensors )
	{
		m_Paused = false;
	}
}

#define VERSION_EMPTY	0
#define VERSION_PAUSED	1
#define VERSION_CURRENT	1

uint WBCompRosaSensor::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_Paused

	return Size;
}

void WBCompRosaSensor::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Paused );
}

void WBCompRosaSensor::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_PAUSED )
	{
		m_Paused = Stream.ReadBool();
	}
}
