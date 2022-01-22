#ifndef ROSAMUSIC_H
#define ROSAMUSIC_H

#include "interpolator.h"
#include "array.h"

class ISoundInstance;

class RosaMusic
{
public:
	RosaMusic();
	~RosaMusic();

	void	PlayMusic( const SimpleString& MusicDef, const uint TrackBits, const uint TrackMask );
	void	StopMusic();

	// TrackBits is the on/off signal for the tracks
	// TrackMask defines which tracks we're actually touching
	void	SetMusicLevels( const uint TrackBits, const uint TrackMask, const float Duration );

	void	PlayAmbience( const SimpleString& AmbienceSoundDef );
	void	FadeOutAmbience( const float Duration );
	void	StopAmbience();

	void	Tick( const float DeltaTime );

	static void		InstanceDeleteCallback( void* pVoid, ISoundInstance* pInstance );

private:
	void			OnInstanceDeleted( ISoundInstance* const pInstance );

	struct STrack
	{
		STrack()
		:	m_VolumeInterpolator()
		,	m_SoundInstance( NULL )
		{
		}

		Interpolator<float>	m_VolumeInterpolator;
		ISoundInstance*		m_SoundInstance;
	};

	Array<STrack>	m_Tracks;
	Array<STrack>	m_Ambiences;

	float			m_AmbienceFadeDuration;
};

#endif // ROSAMUSIC_H
