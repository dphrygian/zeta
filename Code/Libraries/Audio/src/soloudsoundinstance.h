#ifndef SOLOUDSOUNDINSTANCE_H
#define SOLOUDSOUNDINSTANCE_H

#include "soundinstancecommon.h"
#include "iaudiosystem.h"
#include "vector.h"

#include "soloud.h"

class SoLoudSoundInstance : public SoundInstanceCommon
{
public:
	SoLoudSoundInstance( ISound* const pSound );
	virtual ~SoLoudSoundInstance();

	virtual void				Play() { SetPaused( false ); }
	virtual void				Stop();
	virtual void				SetPaused( bool Paused );
	virtual void				SetVolume( float Volume );
	virtual void				SetPan( float Pan );
	virtual void				SetPriority( ESoundPriority Priority ) const;
	virtual void				SetPitch( const float Pitch );

	virtual uint				GetPosition() const;
	virtual void				SetPosition( const uint Position );

	virtual bool				IsPlaying() const;
	virtual bool				IsFinished() const;
	virtual float				GetTimeElapsed() const;

	SoLoud::Soloud*				GetSoLoudEngine() const;

	SoLoud::handle				m_SoLoudHandle;

	// Store values to prevent redundant audio mutex locks
	bool						m_Paused;
	float						m_Volume;
	float						m_Pan;
	float						m_Pitch;
};

#endif // SOLOUDSOUNDINSTANCE_H
