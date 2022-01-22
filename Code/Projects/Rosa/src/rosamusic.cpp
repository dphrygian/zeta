#include "core.h"
#include "rosamusic.h"
#include "isoundinstance.h"
#include "rosaframework.h"
#include "configmanager.h"
#include "wbeventmanager.h"

RosaMusic::RosaMusic()
:	m_Tracks()
,	m_Ambiences()
,	m_AmbienceFadeDuration( 0.0f )
{
	m_Tracks.SetDeflate( false );

	IAudioSystem* const pAudioSystem = RosaFramework::GetInstance()->GetAudioSystem();
	ASSERT( pAudioSystem );

	SInstanceDeleteCallback Callback;
	Callback.m_Callback	= InstanceDeleteCallback;
	Callback.m_Void		= this;
	pAudioSystem->RegisterInstanceDeleteCallback( Callback );

	STATICHASH( RosaMusic );
	STATICHASH( AmbienceFadeDuration );
	m_AmbienceFadeDuration = ConfigManager::GetFloat( sAmbienceFadeDuration, 0.0f, sRosaMusic );
}

RosaMusic::~RosaMusic()
{
	IAudioSystem* const pAudioSystem = RosaFramework::GetInstance()->GetAudioSystem();
	ASSERT( pAudioSystem );

	SInstanceDeleteCallback Callback;
	Callback.m_Callback	= InstanceDeleteCallback;
	Callback.m_Void		= this;
	pAudioSystem->UnregisterInstanceDeleteCallback( Callback );

	FOR_EACH_ARRAY( TrackIter, m_Tracks, STrack )
	{
		const STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		pAudioSystem->RemoveSoundInstance( Track.m_SoundInstance );
	}

	FOR_EACH_ARRAY( TrackIter, m_Ambiences, STrack )
	{
		const STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		pAudioSystem->RemoveSoundInstance( Track.m_SoundInstance );
	}
}

void RosaMusic::Tick( const float DeltaTime )
{
	FOR_EACH_ARRAY( TrackIter, m_Tracks, STrack )
	{
		STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		Track.m_VolumeInterpolator.Tick( DeltaTime );
		Track.m_SoundInstance->SetBaseVolume( Track.m_VolumeInterpolator.GetValue() );
	}

	FOR_EACH_ARRAY_REVERSE( TrackIter, m_Ambiences, STrack )
	{
		STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		Track.m_VolumeInterpolator.Tick( DeltaTime );
		Track.m_SoundInstance->SetBaseVolume( Track.m_VolumeInterpolator.GetValue() );

		if( Track.m_VolumeInterpolator.IsFinished() &&
			Track.m_VolumeInterpolator.GetValue() == 0.0f )
		{
			// Remove tracks when they're done fading out
			Track.m_SoundInstance->Stop();
			m_Ambiences.Remove( TrackIter );
		}
	}
}

void RosaMusic::SetMusicLevels( const uint TrackBits, const uint TrackMask, const float Duration )
{
	FOR_EACH_ARRAY( TrackIter, m_Tracks, STrack )
	{
		const uint	TrackIndex	= TrackIter.GetIndex();
		const uint	TrackBit	= 1 << TrackIndex;

		// If the bitmask doesn't let us modify this track, leave it alone
		const bool	TrackMasked	= 0 == ( TrackMask & TrackBit );
		if( TrackMasked )
		{
			continue;
		}

		const bool										TrackOff			= 0 == ( TrackBits & TrackBit );
		const float										TrackLevel			= TrackOff ? 0.0f : 1.0f;
		const Interpolator<float>::EInterpolationType	InterpolationType	= TrackOff ? Interpolator<float>::EIT_EaseOut : Interpolator<float>::EIT_EaseIn;

		STrack&		Track		= TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		Track.m_VolumeInterpolator.Reset( InterpolationType, Track.m_VolumeInterpolator.GetValue(), TrackLevel, Duration );
		Track.m_SoundInstance->SetBaseVolume( Track.m_VolumeInterpolator.GetValue() );
	}
}

void RosaMusic::PlayMusic( const SimpleString& MusicDef, const uint TrackBits, const uint TrackMask )
{
	StopMusic();
	DEVASSERT( m_Tracks.Empty() );

	if( MusicDef == "" )
	{
		return;
	}

	IAudioSystem* const pAudioSystem = RosaFramework::GetInstance()->GetAudioSystem();
	DEVASSERT( pAudioSystem );

	MAKEHASH( MusicDef );

	STATICHASH( NumTracks );
	const uint NumTracks = ConfigManager::GetInheritedInt( sNumTracks, 0, sMusicDef );
	m_Tracks.Reserve( NumTracks );

	for( uint TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex )
	{
		const SimpleString	TrackSoundDef	= ConfigManager::GetInheritedSequenceString( "Track%d", TrackIndex, "", sMusicDef );
		STrack&				Track			= m_Tracks.PushBack();

		Track.m_SoundInstance				= pAudioSystem->CreateSoundInstance( TrackSoundDef );
		DEVASSERT( Track.m_SoundInstance );
	}

	static const float sDuration = 0.0f;
	SetMusicLevels( TrackBits, TrackMask, sDuration );

	// Now start all sounds together for best possible sync
	// TODO: I could use SoLoud voice groups to do this atomically.
	FOR_EACH_ARRAY( TrackIter, m_Tracks, STrack )
	{
		STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		Track.m_SoundInstance->Tick();
		Track.m_SoundInstance->Play();
	}
}

void RosaMusic::StopMusic()
{
	FOR_EACH_ARRAY_REVERSE( TrackIter, m_Tracks, STrack )
	{
		const STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		Track.m_SoundInstance->Stop();
		m_Tracks.Remove( TrackIter );
	}
}

void RosaMusic::PlayAmbience( const SimpleString& AmbienceSoundDef )
{
	FadeOutAmbience( m_AmbienceFadeDuration );

	if( AmbienceSoundDef == "" )
	{
		return;
	}

	IAudioSystem* const	pAudioSystem	= RosaFramework::GetInstance()->GetAudioSystem();
	DEVASSERT( pAudioSystem );

	STrack&				Track			= m_Ambiences.PushBack();
	Track.m_SoundInstance				= pAudioSystem->CreateSoundInstance( AmbienceSoundDef );
	DEVASSERT( Track.m_SoundInstance );

	Track.m_VolumeInterpolator.Reset( Interpolator<float>::EIT_EaseIn, 0.0f, 1.0f, m_AmbienceFadeDuration );
	Track.m_SoundInstance->SetBaseVolume( 0.0f );
	Track.m_SoundInstance->Tick();
	Track.m_SoundInstance->Play();
}

void RosaMusic::FadeOutAmbience( const float Duration )
{
	if( m_Ambiences.Empty() )
	{
		return;
	}

	STrack& Track = m_Ambiences.Last();
	Track.m_VolumeInterpolator.Reset( Interpolator<float>::EIT_EaseOut, Track.m_VolumeInterpolator.GetValue(), 0.0f, Duration );
}

void RosaMusic::StopAmbience()
{
	FOR_EACH_ARRAY_REVERSE( TrackIter, m_Ambiences, STrack )
	{
		const STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		Track.m_SoundInstance->Stop();
		m_Ambiences.Remove( TrackIter );
	}
}

/*static*/ void RosaMusic::InstanceDeleteCallback( void* pVoid, ISoundInstance* pInstance )
{
	RosaMusic* pMusic = static_cast<RosaMusic*>( pVoid );
	ASSERT( pMusic );

	pMusic->OnInstanceDeleted( pInstance );
}

void RosaMusic::OnInstanceDeleted( ISoundInstance* const pInstance )
{
	FOR_EACH_ARRAY_REVERSE( TrackIter, m_Tracks, STrack )
	{
		const STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		if( pInstance == Track.m_SoundInstance )
		{
			m_Tracks.Remove( TrackIter );

			// HACKHACK: Notify observers when music is done
			if( m_Tracks.Empty() )
			{
				WB_MAKE_EVENT( OnMusicFinished, NULL );
				WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnMusicFinished, NULL );
			}
		}
	}

	FOR_EACH_ARRAY_REVERSE( TrackIter, m_Ambiences, STrack )
	{
		const STrack& Track = TrackIter.GetValue();
		DEVASSERT( Track.m_SoundInstance );

		if( pInstance == Track.m_SoundInstance )
		{
			m_Ambiences.Remove( TrackIter );
		}
	}
}
