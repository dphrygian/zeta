#include "core.h"
#include "wbcomprosajukebox.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"

WBCompRosaJukebox::WBCompRosaJukebox()
:	m_TrackSoundDefs()
,	m_DeferPlay( false )
,	m_Loop( false )
,	m_TrackIndex( 0 )
,	m_Muted( false )
{
}

WBCompRosaJukebox::~WBCompRosaJukebox()
{
}

/*virtual*/ void WBCompRosaJukebox::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( DeferPlay );
	m_DeferPlay = ConfigManager::GetInheritedBool( sDeferPlay, false, sDefinitionName );

	STATICHASH( NumTracks );
	const uint NumTracks = ConfigManager::GetInheritedInt( sNumTracks, 0, sDefinitionName );
	for( uint TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex )
	{
		const SimpleString TrackSoundDef = ConfigManager::GetInheritedSequenceString( "Track%d", TrackIndex, "", sDefinitionName );
		m_TrackSoundDefs.PushBack( TrackSoundDef );
	}

	STATICHASH( Loop );
	m_Loop = ConfigManager::GetInheritedBool( sLoop, true, sDefinitionName );
}

/*virtual*/ void WBCompRosaJukebox::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( Play );
	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( OnSoundDeleted );
	STATIC_HASHED_STRING( MuteJukebox );
	STATIC_HASHED_STRING( UnmuteJukebox );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sPlay )
	{
		Play();
	}
	else if( EventName == sOnSpawned )
	{
		if( m_DeferPlay )
		{
			// Ignore
		}
		else
		{
			// Start first song playing
			Play();
		}
	}
	else if( EventName == sOnSoundDeleted )
	{
		STATIC_HASHED_STRING( SoundDef );
		const SimpleString SoundDef = Event.GetString( sSoundDef );

		HandleDeletedSound( SoundDef );
	}
	else if( EventName == sMuteJukebox )
	{
		Mute();
	}
	else if( EventName == sUnmuteJukebox )
	{
		Unmute();
	}
}

void WBCompRosaJukebox::HandleDeletedSound( const SimpleString& SoundDef )
{
	if( m_TrackIndex >= m_TrackSoundDefs.Size() )
	{
		return;
	}

	const SimpleString& CurrentTrack = m_TrackSoundDefs[ m_TrackIndex ];
	if( SoundDef != CurrentTrack )
	{
		return;
	}

	Next();

	if( !m_Loop && m_TrackIndex == 0 )
	{
		// We've run out of tracks to play and we're not looping
		return;
	}

	Play();
}

void WBCompRosaJukebox::Play()
{
	if( m_TrackIndex >= m_TrackSoundDefs.Size() )
	{
		return;
	}

	const SimpleString& CurrentTrack = m_TrackSoundDefs[ m_TrackIndex ];

	WB_MAKE_EVENT( PlaySoundDef, GetEntity() );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, CurrentTrack );
	WB_SET_AUTO( PlaySoundDef, Bool, Attached, true );
	WB_SET_AUTO( PlaySoundDef, Bool, Muted, m_Muted );
	WB_DISPATCH_EVENT( GetEventManager(), PlaySoundDef, GetEntity() );
}

void WBCompRosaJukebox::Next()
{
	// Advance to the next track
	m_TrackIndex = ( m_TrackIndex + 1 ) % m_TrackSoundDefs.Size();
}

void WBCompRosaJukebox::Mute()
{
	m_Muted = true;

	if( m_TrackIndex >= m_TrackSoundDefs.Size() )
	{
		return;
	}

	const SimpleString& CurrentTrack = m_TrackSoundDefs[ m_TrackIndex ];

	WB_MAKE_EVENT( MuteSound, GetEntity() );
	WB_SET_AUTO( MuteSound, Hash, Sound, CurrentTrack );
	WB_DISPATCH_EVENT( GetEventManager(), MuteSound, GetEntity() );
}

void WBCompRosaJukebox::Unmute()
{
	m_Muted = false;

	if( m_TrackIndex >= m_TrackSoundDefs.Size() )
	{
		return;
	}

	const SimpleString& CurrentTrack = m_TrackSoundDefs[ m_TrackIndex ];

	WB_MAKE_EVENT( UnmuteSound, GetEntity() );
	WB_SET_AUTO( UnmuteSound, Hash, Sound, CurrentTrack );
	WB_DISPATCH_EVENT( GetEventManager(), UnmuteSound, GetEntity() );
}

#define VERSION_EMPTY		0
#define VERSION_TRACKINDEX	1
#define VERSION_MUTED		2
#define VERSION_CURRENT		2

uint WBCompRosaJukebox::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_TrackIndex

	Size += 1;	// m_Muted

	return Size;
}

void WBCompRosaJukebox::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_TrackIndex );

	Stream.WriteBool( m_Muted );
}

void WBCompRosaJukebox::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_TRACKINDEX )
	{
		m_TrackIndex = Stream.ReadUInt32();
	}

	if( Version >= VERSION_MUTED )
	{
		m_Muted = Stream.ReadBool();
	}
}
