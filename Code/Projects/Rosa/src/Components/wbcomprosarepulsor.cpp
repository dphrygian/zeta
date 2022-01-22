#include "core.h"
#include "wbcomprosarepulsor.h"
#include "configmanager.h"
#include "wbevent.h"
#include "idatastream.h"

WBCompRosaRepulsor::WBCompRosaRepulsor()
:	m_Active( false )
,	m_Directed( false )
,	m_Radius( 0.0f )
{
}

WBCompRosaRepulsor::~WBCompRosaRepulsor()
{
}

/*virtual*/ void WBCompRosaRepulsor::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Active );
	m_Active = ConfigManager::GetInheritedBool( sActive, true, sDefinitionName );

	STATICHASH( Directed );
	m_Directed = ConfigManager::GetInheritedBool( sDirected, false, sDefinitionName );

	STATICHASH( Radius );
	m_Radius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaRepulsor::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( StopRepulsing );
	STATIC_HASHED_STRING( StartRepulsing );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sStopRepulsing )
	{
		m_Active = false;
	}
	else if( EventName == sStartRepulsing )
	{
		m_Active = true;
	}
}

#define VERSION_EMPTY	0
#define VERSION_ACTIVE	1
#define VERSION_CURRENT	1

uint WBCompRosaRepulsor::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;		// Version

	Size += 1;		// m_Active

	return Size;
}

void WBCompRosaRepulsor::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Active );
}

void WBCompRosaRepulsor::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ACTIVE )
	{
		m_Active = Stream.ReadBool();
	}
}
