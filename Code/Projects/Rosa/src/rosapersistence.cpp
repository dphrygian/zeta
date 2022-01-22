#include "core.h"
#include "rosapersistence.h"
#include "idatastream.h"
#include "wbeventmanager.h"

RosaPersistence::RosaPersistence()
:	m_VariableMap()
{
	RegisterForEvents();
}

RosaPersistence::~RosaPersistence()
{
	// No need to unregister events; this object outlives the event manager.
}

void RosaPersistence::RegisterForEvents()
{
	STATIC_HASHED_STRING( SetPersistentVar );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sSetPersistentVar, this, NULL );
}

/*virtual*/ void RosaPersistence::HandleEvent( const WBEvent& Event )
{
	STATIC_HASHED_STRING( SetPersistentVar );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetPersistentVar )
	{
		STATIC_HASHED_STRING( Name );
		const HashedString Name = Event.GetHash( sName );

		STATIC_HASHED_STRING( Value );
		const WBEvent::SParameter* const pParameter = Event.GetParameter( sValue );
		m_VariableMap.SetFromParameter( Name, pParameter );
	}
}

void RosaPersistence::Reset()
{
	m_VariableMap.Clear();
}

void RosaPersistence::Report() const
{
	PRINTF( "RosaPersistence:\n" );
	m_VariableMap.Report();
}

#define VERSION_EMPTY		0
#define VERSION_VARIABLEMAP	1
#define VERSION_CURRENT		1

void RosaPersistence::Save( const IDataStream& Stream ) const
{
	Stream.WriteUInt32( VERSION_CURRENT );

	m_VariableMap.Save( Stream );
}

void RosaPersistence::Load( const IDataStream& Stream )
{
	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_VARIABLEMAP )
	{
		m_VariableMap.Load( Stream );
	}
}
