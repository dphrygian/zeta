#ifndef IAUDIOSYSTEM_H
#define IAUDIOSYSTEM_H

#include "vector.h"
#include "hashedstring.h"
#include "simplestring.h"

class ISound;
class SoundManager;
class ISoundInstance;
class View;
class Sound3DListener;

enum ESoundPriority
{
	ESP_Default,
	ESP_High,
};

struct SSoundInit
{
	SSoundInit()
	:	m_Filename()
	,	m_IsStream( false )
	,	m_ShouldSerialize( false )
	,	m_IsLooping( false )
	,	m_Is3D( false )
	,	m_ShouldCalcOcclusion( false )
	,	m_Volume( 1.0f )
	,	m_FalloffDistance( 0.0f )
	,	m_PanBiasNear( 0.0f )
	,	m_PanBiasFar( 0.0f )
	,	m_PanPower( 1.0f )
	,	m_MinimumAttenuation( 0.0f )
	,	m_RearAttenuation( 0.0f )
	,	m_OcclusionDepthScalar( 0.0f )
	,	m_OccludedFalloffRadius( 0.0f )
	,	m_PitchMin( 1.0f )
	,	m_PitchMax( 1.0f )
	,	m_Category()
	,	m_Priority( ESP_Default )
	{
	}

	SimpleString	m_Filename;
	bool			m_IsStream;
	bool			m_ShouldSerialize;		// Should serialize position and restore itself
	bool			m_IsLooping;
	bool			m_Is3D;
	bool			m_ShouldCalcOcclusion;
	float			m_Volume;
	float			m_FalloffDistance;
	float			m_PanBiasNear;
	float			m_PanBiasFar;
	float			m_PanPower;
	float			m_MinimumAttenuation;
	float			m_RearAttenuation;
	float			m_OcclusionDepthScalar;
	float			m_OccludedFalloffRadius;
	float			m_PitchMin;
	float			m_PitchMax;
	HashedString	m_Category;
	ESoundPriority	m_Priority;
};

typedef void ( *InstanceDeleteCallback )( void*, ISoundInstance* );
struct SInstanceDeleteCallback
{
	bool operator==( const SInstanceDeleteCallback& Other ) const
	{
		return ( m_Void == Other.m_Void && m_Callback == Other.m_Callback );
	}
	InstanceDeleteCallback	m_Callback;
	void*					m_Void;
};

class IAudioSystem
{
public:
	virtual ~IAudioSystem() {}

	virtual bool					TryLockAudioSystem() = 0;
	virtual void					UnlockAudioSystem() = 0;

	virtual void					Tick( const float DeltaTime, bool GamePaused ) = 0;

	virtual SoundManager*			GetSoundManager() = 0;

	virtual ISound*					GetSound( const SimpleString& DefinitionName ) = 0;
	virtual ISound*					CreateSound( const SSoundInit& SoundInit ) = 0;

	virtual void					AddSoundInstance( ISoundInstance* pSoundInstance ) = 0;
	virtual void					RemoveSoundInstance( ISoundInstance* pSoundInstance ) = 0;
	virtual bool					IsValid( ISoundInstance* pSoundInstance ) const = 0;
	virtual void					FreeSoundInstances() = 0;

	virtual void					Set3DListener( const Sound3DListener* const pListener ) = 0;
	virtual const Sound3DListener*	Get3DListener() const = 0;

	virtual ISoundInstance*			CreateSoundInstance( ISound* const pSound ) = 0;
	virtual ISoundInstance*			CreateSoundInstance( const SimpleString& DefinitionName ) = 0;
	virtual ISoundInstance*			Play( const SimpleString& DefinitionName, const Vector& Location ) = 0;

	virtual void					SetReverbParams( const SimpleString& DefinitionName ) const = 0;
	virtual void					ConditionalApplyReverb( ISoundInstance* const pSoundInstance ) const = 0;

#if BUILD_DEV
	virtual void					ReverbTest_Toggle() = 0;
	virtual void					ReverbTest_Update() = 0;
	virtual bool					ReverbTest_IsActive() const = 0;
	virtual void					ReverbTest_Print() const = 0;
	virtual void					ReverbTest_Export() const = 0;
	virtual void					ReverbTest_PrevSetting() = 0;
	virtual void					ReverbTest_NextSetting() = 0;
	virtual void					ReverbTest_IncrementSetting( const float Scalar ) = 0;
	virtual void					ReverbTest_DecrementSetting( const float Scalar ) = 0;
#endif

	virtual bool					GetGlobalMute() const = 0;
	virtual void					SetGlobalMute( const bool Mute ) = 0;

	virtual float					GetMasterVolume() const = 0;
	virtual void					SetMasterVolume( const float Volume ) = 0;

	virtual float					GetCategoryVolume( const HashedString& Category ) const = 0;
	virtual void					SetCategoryVolume( const HashedString& Category, float Volume, float InterpolationTime ) = 0;

	virtual void					RegisterInstanceDeleteCallback( const SInstanceDeleteCallback& Callback ) = 0;
	virtual void					UnregisterInstanceDeleteCallback( const SInstanceDeleteCallback& Callback ) = 0;
};

#endif // IAUDIOSYSTEM_H
