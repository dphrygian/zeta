#include "core.h"
#include "wbcomprosafaction.h"
#include "configmanager.h"
#include "wbentity.h"
#include "wbeventmanager.h"
#include "idatastream.h"
#include "rosagame.h"
#include "reversehash.h"

WBCompRosaFaction::WBCompRosaFaction()
:	m_Faction()
,	m_Immutable( false )
{
	RosaFactions::AddRef();
}

WBCompRosaFaction::~WBCompRosaFaction()
{
	RosaFactions::Release();
}

/*virtual*/ void WBCompRosaFaction::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Faction );
	m_Faction = ConfigManager::GetInheritedHash( sFaction, HashedString::NullString, sDefinitionName );

	STATICHASH( Immutable );
	m_Immutable = ConfigManager::GetInheritedBool( sImmutable, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaFaction::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetFaction );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetFaction )
	{
		if( m_Immutable )
		{
			// Faction can't be changed, do nothing
		}
		else
		{
			STATIC_HASHED_STRING( Faction );
			const HashedString NewFaction = Event.GetHash( sFaction );
			if( NewFaction != m_Faction )
			{
				m_Faction = NewFaction;

				// The new faction will be sent via AddContextToEvent
				WB_MAKE_EVENT( OnFactionChanged, GetEntity() );
				WB_DISPATCH_EVENT( GetEventManager(), OnFactionChanged, GetEntity() );
			}
		}
	}
	else if( EventName == sPushPersistence )
	{
		PushPersistence();
	}
	else if( EventName == sPullPersistence )
	{
		PullPersistence();
	}
}

/*virtual*/ void WBCompRosaFaction::AddContextToEvent( WBEvent& Event ) const
{
	WB_SET_CONTEXT( Event, Hash, Faction, m_Faction );
}

void WBCompRosaFaction::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Faction );
	Persistence.SetHash( sFaction, m_Faction );
}

void WBCompRosaFaction::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Faction );
	m_Faction = Persistence.GetHash( sFaction );
}

RosaFactions::EFactionCon WBCompRosaFaction::GetCon( const WBEntity* const pEntityB )
{
	ASSERT( pEntityB );

	WBCompRosaFaction* const pFactionB = WB_GETCOMP( pEntityB, RosaFaction );
	ASSERT( pFactionB );

	return RosaFactions::GetCon( m_Faction, pFactionB->m_Faction );
}

RosaFactions::EFactionCon WBCompRosaFaction::GetCon( const HashedString& FactionB )
{
	return RosaFactions::GetCon( m_Faction, FactionB );
}

/*static*/ RosaFactions::EFactionCon WBCompRosaFaction::GetCon( const WBEntity* const pEntityA, const WBEntity* const pEntityB )
{
	ASSERT( pEntityA );
	ASSERT( pEntityB );

	WBCompRosaFaction* const pFactionA = WB_GETCOMP( pEntityA, RosaFaction );
	WBCompRosaFaction* const pFactionB = WB_GETCOMP( pEntityB, RosaFaction );

	// Null factions are always treated as neutral.
	if( pFactionA == NULL || pFactionB == NULL )
	{
		return RosaFactions::EFR_Neutral;
	}

	return RosaFactions::GetCon( pFactionA->m_Faction, pFactionB->m_Faction );
}

#if BUILD_DEV
void WBCompRosaFaction::Report() const
{
	Super::Report();

	PRINTF( WBPROPERTY_REPORT_PREFIX "Faction: %s\n", ReverseHash::ReversedHash( m_Faction ).CStr() );
}
#endif

#define VERSION_EMPTY	0
#define VERSION_FACTION	1
#define VERSION_CURRENT	1

uint WBCompRosaFaction::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += sizeof( HashedString );	// m_Faction

	return Size;
}

void WBCompRosaFaction::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.WriteHashedString( m_Faction );
}

void WBCompRosaFaction::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_FACTION )
	{
		m_Faction = Stream.ReadHashedString();
	}
}
