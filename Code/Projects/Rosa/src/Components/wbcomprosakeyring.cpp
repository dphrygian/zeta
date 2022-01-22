#include "core.h"
#include "wbcomprosakeyring.h"
#include "configmanager.h"
#include "idatastream.h"
#include "mathcore.h"
#include "rosagame.h"
#include "reversehash.h"
#include "rosahudlog.h"
#include "stringmanager.h"

WBCompRosaKeyRing::WBCompRosaKeyRing()
:	m_Keys( 0 )
,	m_Keycards()
,	m_ShowKeycardUsedLog( false )
{
}

WBCompRosaKeyRing::~WBCompRosaKeyRing()
{
}

/*virtual*/ void WBCompRosaKeyRing::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( ShowKeycardUsedLog );
	m_ShowKeycardUsedLog = ConfigManager::GetInheritedBool( sShowKeycardUsedLog, false, sDefinitionName );

	STATICHASH( NumKeycards );
	const uint NumKeycards = ConfigManager::GetInheritedInt( sNumKeycards, 0, sDefinitionName );

	for( uint KeycardIndex = 0; KeycardIndex < NumKeycards; ++KeycardIndex )
	{
		const HashedString Keycard = ConfigManager::GetInheritedSequenceHash( "Keycard%d", KeycardIndex, HashedString::NullString, sDefinitionName );
		m_Keycards.Insert( Keycard );
	}
}

/*virtual*/ void WBCompRosaKeyRing::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( AddKey );
	STATIC_HASHED_STRING( RemoveKey );
	STATIC_HASHED_STRING( AddKeycard );
	STATIC_HASHED_STRING( RemoveKeycard );
	STATIC_HASHED_STRING( OnKeycardUsed );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		PublishToHUD();
	}
	else if( EventName == sAddKey )
	{
		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		AddKeys( 1, ShowLogMessage );
	}
	else if( EventName == sRemoveKey )
	{
		RemoveKey();
	}
	else if( EventName == sAddKeycard )
	{
		STATIC_HASHED_STRING( Keycard );
		const HashedString Keycard = Event.GetHash( sKeycard );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		AddKeycard( Keycard, ShowLogMessage );
	}
	else if( EventName == sRemoveKeycard )
	{
		STATIC_HASHED_STRING( Keycard );
		const HashedString Keycard = Event.GetHash( sKeycard );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		RemoveKeycard( Keycard, ShowLogMessage );
	}
	else if( EventName == sOnKeycardUsed )
	{
		if( m_ShowKeycardUsedLog )
		{
			STATIC_HASHED_STRING( Keycard );
			const SimpleString Keycard = Event.GetString( sKeycard );

			STATICHASH( KeycardUsed );
			ConfigManager::SetString( sKeycard, Keycard.CStr(), sKeycardUsed );

			RosaHUDLog::StaticAddDynamicMessage( sKeycardUsed );
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

/*virtual*/ void WBCompRosaKeyRing::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Int, Keys, m_Keys );
}

void WBCompRosaKeyRing::AddKeys( const uint Keys, const bool ShowLogMessage )
{
	m_Keys += Keys;

	PublishToHUD();

	if( ShowLogMessage && Keys > 0 )
	{
		STATICHASH( KeyPickup );
		STATICHASH( Keys );
		ConfigManager::SetInt( sKeys, Keys, sKeyPickup );

		RosaHUDLog::StaticAddDynamicMessage( sKeyPickup );
	}
}

void WBCompRosaKeyRing::RemoveKey()
{
	ASSERT( HasKeys() );

	m_Keys -= 1;

	PublishToHUD();
}

bool WBCompRosaKeyRing::HasKeycard( const Array<HashedString>& Keycards, HashedString* const pOutUsedKeycard /*= NULL*/ )
{
	FOR_EACH_ARRAY( KeycardIter, Keycards, HashedString )
	{
		const HashedString& Keycard = KeycardIter.GetValue();
		if( HasKeycard( Keycard ) )
		{
			if( pOutUsedKeycard )
			{
				*pOutUsedKeycard = Keycard;
			}

			return true;
		}
	}

	return false;
}

bool WBCompRosaKeyRing::HasKeycard( const HashedString& Keycard )
{
	return m_Keycards.Search( Keycard ).IsValid();
}

void WBCompRosaKeyRing::AddKeycard( const HashedString& Keycard, const bool ShowLogMessage )
{
	m_Keycards.Insert( Keycard );

	if( ShowLogMessage )
	{
		// For Rosa, I'm replacing pickup screens with log messages

		const SimpleString KeycardTag = ReverseHash::ReversedHash( Keycard );

		STATICHASH( KeycardPickup );
		STATICHASH( Keycard );
		ConfigManager::SetString( sKeycard, KeycardTag.CStr(), sKeycardPickup );

		RosaHUDLog::StaticAddDynamicMessage( sKeycardPickup );
	}
}

void WBCompRosaKeyRing::RemoveKeycard( const HashedString& Keycard, const bool ShowLogMessage )
{
	m_Keycards.Remove( Keycard );

	if( ShowLogMessage )
	{
		// For Rosa, I'm replacing pickup screens with log messages

		const SimpleString KeycardTag = ReverseHash::ReversedHash( Keycard );

		STATICHASH( KeycardLost );
		STATICHASH( Keycard );
		ConfigManager::SetString( sKeycard, KeycardTag.CStr(), sKeycardLost );

		RosaHUDLog::StaticAddDynamicMessage( sKeycardLost );
	}
}

void WBCompRosaKeyRing::PublishToHUD() const
{
	STATICHASH( HUD );
	STATICHASH( Keys );
	ConfigManager::SetInt( sKeys, m_Keys, sHUD );
}

void WBCompRosaKeyRing::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Keys );
	Persistence.SetInt( sKeys, m_Keys );

	const uint NumKeycards = m_Keycards.Size();
	STATIC_HASHED_STRING( NumKeycards );
	Persistence.SetInt( sNumKeycards, NumKeycards );

	uint KeycardIndex = 0;
	FOR_EACH_SET( KeycardIter, m_Keycards, HashedString )
	{
		Persistence.SetHash( SimpleString::PrintF( "Keycard%d", KeycardIndex ), KeycardIter.GetValue() );
		++KeycardIndex;
	}
}

void WBCompRosaKeyRing::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Keys );
	m_Keys = Persistence.GetInt( sKeys );

	m_Keycards.Clear();
	STATIC_HASHED_STRING( NumKeycards );
	const uint NumKeycards = Persistence.GetInt( sNumKeycards );

	for( uint KeycardIndex = 0; KeycardIndex < NumKeycards; ++KeycardIndex )
	{
		const HashedString Keycard = Persistence.GetHash( SimpleString::PrintF( "Keycard%d", KeycardIndex ) );
		m_Keycards.Insert( Keycard );
	}

	PublishToHUD();
}

#define VERSION_EMPTY		0
#define VERSION_KEYS		1
#define VERSION_KEYCARDS	2
#define VERSION_CURRENT		2

uint WBCompRosaKeyRing::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 4;	// m_Keys

	Size += 4;	// m_Keycards.Size();
	Size += sizeof( HashedString ) * m_Keycards.Size();	// m_Keycards

	return Size;
}

void WBCompRosaKeyRing::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.WriteUInt32( m_Keys );

	Stream.WriteUInt32( m_Keycards.Size() );
	FOR_EACH_SET( KeycardIter, m_Keycards, HashedString )
	{
		const HashedString& Keycard = KeycardIter.GetValue();
		Stream.WriteHashedString( Keycard );
	}
}

void WBCompRosaKeyRing::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_KEYS )
	{
		m_Keys = Stream.ReadUInt32();
	}

	if( Version >= VERSION_KEYCARDS )
	{
		const uint NumKeycards = Stream.ReadUInt32();
		for( uint KeycardIndex = 0; KeycardIndex < NumKeycards; ++KeycardIndex )
		{
			const HashedString Keycard = Stream.ReadHashedString();
			m_Keycards.Insert( Keycard );
		}
	}
}
