#include "core.h"
#include "soloudsoundinstance.h"
#include "isound.h"
#include "soloudaudiosystem.h"
#include "configmanager.h"
#include "mathcore.h"
#include "sound3dlistener.h"

// Avoid locking audio mutex for redundant changes
#define IGNORE_REDUNDANT_STATE 1

SoLoudSoundInstance::SoLoudSoundInstance( ISound* const pSound )
:	m_SoLoudHandle( 0 )
,	m_Paused( true )	// HACKHACK: Sounds are always started paused
,	m_Volume( 0.0f )
,	m_Pan( 0.0f )
,	m_Pitch( 1.0f )
{
	SetSound( pSound );
}

SoLoudSoundInstance::~SoLoudSoundInstance()
{
	if( !IsFinished() )
	{
		Stop();
	}

	// Delete the corresponding sound iff it's a stream (incidentally, stream sounds
	// only get automatically deleted if they're ever actually played).
	// Also delete if it is otherwise marked for deletion with instance.
	if( GetSound()->GetIsStream() )
	{
		SafeDelete( m_Sound );
	}

	m_SoLoudHandle	= 0;
}

// NOTE: I really don't want to be storing redundant pointers all the way down to sound instances
SoLoud::Soloud* SoLoudSoundInstance::GetSoLoudEngine() const
{
	return static_cast<SoLoudAudioSystem*>( m_Sound->GetAudioSystem() )->GetSoLoudEngine();
}

void SoLoudSoundInstance::Stop()
{
	GetSoLoudEngine()->stop( m_SoLoudHandle );
}

void SoLoudSoundInstance::SetPriority( ESoundPriority Priority ) const
{
	GetSoLoudEngine()->setProtectVoice( m_SoLoudHandle, ( Priority == ESP_High ) );
}

void SoLoudSoundInstance::SetPaused( bool Paused )
{
#if IGNORE_REDUNDANT_STATE
	if( Paused == m_Paused )
	{
		return;
	}
#endif
	m_Paused = Paused;

	GetSoLoudEngine()->setPause( m_SoLoudHandle, Paused );
}

void SoLoudSoundInstance::SetVolume( float Volume )
{
#if IGNORE_REDUNDANT_STATE
	if( Volume == m_Volume )
	{
		return;
	}
#endif
	m_Volume = Volume;

	GetSoLoudEngine()->setVolume( m_SoLoudHandle, Volume );
}

void SoLoudSoundInstance::SetPan( float Pan )
{
#if IGNORE_REDUNDANT_STATE
	if( Pan == m_Pan )
	{
		return;
	}
#endif
	m_Pan = Pan;

	GetSoLoudEngine()->setPan( m_SoLoudHandle, Pan );
}

/*virtual*/ void SoLoudSoundInstance::SetPitch( const float Pitch )
{
#if IGNORE_REDUNDANT_STATE
	if( Pitch == m_Pitch )
	{
		return;
	}
#endif
	m_Pitch = Pitch;

	GetSoLoudEngine()->setRelativePlaySpeed( m_SoLoudHandle, Pitch );
}

/*virtual*/ bool SoLoudSoundInstance::IsPlaying() const
{
	return !m_Paused;

	// NOTE: Given how IsPlaying is being used (only in sound component, to start
	// it playing if it wasn't already), I don't think I need to go all the way
	// to locking the audio mutex to get status.
	//return
	//	GetSoLoudEngine()->isValidVoiceHandle( m_SoLoudHandle ) &&
	//	!GetSoLoudEngine()->getPause( m_SoLoudHandle );
}

bool SoLoudSoundInstance::IsFinished() const
{
	return !GetSoLoudEngine()->isValidVoiceHandle( m_SoLoudHandle );
}

float SoLoudSoundInstance::GetTimeElapsed() const
{
	return static_cast<float>( GetSoLoudEngine()->getStreamTime( m_SoLoudHandle ) );
}

// NOTE: For compatibility with FMOD, position is time in ms
/*virtual*/ uint SoLoudSoundInstance::GetPosition() const
{
	// NOTE: getStreamTime is actual played time, which is not what I want for looping sounds;
	// I've added getLength so I can mod it.
	SoLoud::Soloud* const	pSoLoudEngine	= GetSoLoudEngine();
	const double			StreamTime		= pSoLoudEngine->getStreamTime( m_SoLoudHandle );
	const double			SoundLength		= pSoLoudEngine->getLength( m_SoLoudHandle );
	const double			Position		= fmod( StreamTime, SoundLength );
	return static_cast<uint>( 1000.0 * Position );
}

// NOTE: For compatibility with FMOD, position is time in ms
/*virtual*/ void SoLoudSoundInstance::SetPosition( const uint Position )
{
	GetSoLoudEngine()->seek( m_SoLoudHandle, static_cast<double>( Position ) / 1000.0 );
}
