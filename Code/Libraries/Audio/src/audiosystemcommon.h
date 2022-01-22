#ifndef AUDIOSYSTEMCOMMON_H
#define AUDIOSYSTEMCOMMON_H

#include "iaudiosystem.h"
#include "set.h"
#include "array.h"
#include "hashedstring.h"
#include "interpolator.h"
#include "map.h"

class SoundManager;

class AudioSystemCommon : public IAudioSystem
{
public:
	AudioSystemCommon();
	virtual ~AudioSystemCommon();

	virtual void					Tick( const float DeltaTime, bool GamePaused );

	virtual SoundManager*			GetSoundManager();

	virtual ISound*					GetSound( const SimpleString& DefinitionName );

	virtual void					AddSoundInstance( ISoundInstance* pSoundInstance );
	virtual void					RemoveSoundInstance( ISoundInstance* pSoundInstance );
	virtual bool					IsValid( ISoundInstance* pSoundInstance ) const;
	virtual void					FreeSoundInstances();

	virtual void					Set3DListener( const Sound3DListener* const pListener ) { m_Sound3DListener = pListener; }
	virtual const Sound3DListener*	Get3DListener() const { return m_Sound3DListener; }

	virtual ISoundInstance*			CreateSoundInstance( ISound* const pSound );
	virtual ISoundInstance*			CreateSoundInstance( const SimpleString& DefinitionName );

	virtual bool					GetGlobalMute() const { return m_GlobalMute; }
	virtual void					SetGlobalMute( const bool Mute ) { m_GlobalMute = Mute; }

	virtual float					GetMasterVolume() const { return GetGlobalMute() ? 0.0f : m_MasterVolume; }
	virtual void					SetMasterVolume( const float Volume ) { m_MasterVolume = Volume; }

	virtual float					GetCategoryVolume( const HashedString& Category ) const;
	virtual void					SetCategoryVolume( const HashedString& Category, float Volume, float InterpolationTime );

	virtual void					RegisterInstanceDeleteCallback( const SInstanceDeleteCallback& Callback );
	virtual void					UnregisterInstanceDeleteCallback( const SInstanceDeleteCallback& Callback );

protected:
	SimpleString					GetRandomSource( const SimpleString& DefinitionName );
	void							FreeSoundInstance( ISoundInstance* pSoundInstance );

	SoundManager*							m_SoundManager;
	const Sound3DListener*					m_Sound3DListener;

	Set<ISoundInstance*>					m_SoundInstances;
	Array<SInstanceDeleteCallback>			m_InstanceDeleteCallbacks;
	float									m_MasterVolume;
	bool									m_GlobalMute;
	Map<HashedString, Interpolator<float> >	m_CategoryVolumes;
	Array<HashedString>						m_PauseCategories;		// Categories that pause/unpause with the game
	Map<HashedString, uint>					m_LastRandomSources;	// Prevent repeats from random sources by tracking the last one
};

#endif // AUDIOSYSTEMCOMMON_H
