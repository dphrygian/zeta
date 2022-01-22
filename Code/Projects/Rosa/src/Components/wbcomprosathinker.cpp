#include "core.h"
#include "wbcomprosathinker.h"
#include "wbevent.h"
#include "idatastream.h"

WBCompRosaThinker::WBCompRosaThinker()
:	m_Paused( false )
{
}

WBCompRosaThinker::~WBCompRosaThinker()
{
}

/*virtual*/ void WBCompRosaThinker::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	WBRosaComponent::HandleEvent( Event );

	STATIC_HASHED_STRING( TickThinkers );
	STATIC_HASHED_STRING( PauseThinkers );
	STATIC_HASHED_STRING( UnpauseThinkers );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sTickThinkers )
	{
		Tick( 0.0f );
	}
	else if( EventName == sPauseThinkers )
	{
		m_Paused = true;
	}
	else if( EventName == sUnpauseThinkers )
	{
		m_Paused = false;
	}
}

#define VERSION_EMPTY	0
#define VERSION_PAUSED	1
#define VERSION_CURRENT	1

uint WBCompRosaThinker::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 1;	// m_Paused

	return Size;
}

void WBCompRosaThinker::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Paused );
}

void WBCompRosaThinker::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_PAUSED )
	{
		m_Paused = Stream.ReadBool();
	}
}
