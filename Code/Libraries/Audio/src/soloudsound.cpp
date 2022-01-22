#include "core.h"
#include "soloudsound.h"
#include "packstream.h"
#include "soloudsoundinstance.h"
#include "soloudaudiosystem.h"

PackageFileReader::PackageFileReader( const char* const Filename, const uint Offset, const uint Length )
:	m_File( NULL )
,	m_Offset( Offset )
,	m_Length( Length )
{
	FOPEN( m_File, Filename, "rb" );
	fseek( m_File, m_Offset, SEEK_SET );
}

PackageFileReader::~PackageFileReader()
{
	fclose( m_File );
}

SoLoudSound::SoLoudSound(
	IAudioSystem* const pSystem,
	const SSoundInit& SoundInit )
:	m_SoLoudWav( NULL )
,	m_SoLoudWavStream( NULL )
,	m_PackageFileReader( NULL )
{
	ASSERT( pSystem );
	SetAudioSystem( pSystem );

	ASSERT( SoundInit.m_Filename != "" );

	if( SoundInit.m_IsStream )
	{
		// DLP 7 Dec 2021: This is currently my one and only use of UnpackFileIntoMemory=false.
		// Instead of loading the packaged file into a permanent buffer in the heap, just get
		// the file entry and use it with PackageFileReader for a FILE*.
		CreateStream( PackStream( SoundInit.m_Filename.CStr(), false /*UnpackFileIntoMemory*/ ), SoundInit );
	}
	else
	{
		CreateSample( PackStream( SoundInit.m_Filename.CStr() ), SoundInit );
	}
}

SoLoudSound::~SoLoudSound()
{
	SafeDelete( m_SoLoudWav );
	SafeDelete( m_SoLoudWavStream );
	SafeDelete( m_PackageFileReader );
}

SoLoud::AudioSource& SoLoudSound::GetSoLoudAudioSource() const
{
	ASSERT( m_SoLoudWav || m_SoLoudWavStream );
	if( m_SoLoudWav )
	{
		return *m_SoLoudWav;
	}
	else
	{
		return *m_SoLoudWavStream;
	}
}

/*virtual*/ ISoundInstance* SoLoudSound::CreateInstance()
{
	// If this is a stream, we have to start decoding the Vorbis file on each instance.
	// This is expensive and is why I've switched all AI barks to being loaded into memory.
	PROFILE_FUNCTION;
	FRAME_PROFILE_FUNCTION;

	SoLoudSoundInstance* const	pInstance			= new SoLoudSoundInstance( this );

	SoLoudAudioSystem* const	pSoLoudAudioSystem	= static_cast<SoLoudAudioSystem*>( GetAudioSystem() );
	if( pSoLoudAudioSystem->IsReverbCategory( GetCategory() ) )
	{
		pInstance->m_SoLoudHandle					= pSoLoudAudioSystem->GetSoLoudEchoBus()->play(
														GetSoLoudAudioSource(),
														0.0f /*volume*/,
														0.0f /*pan*/,
														true /*paused*/ );
	}
	else
	{
		pInstance->m_SoLoudHandle					= pSoLoudAudioSystem->GetSoLoudEngine()->play(
														GetSoLoudAudioSource(),
														0.0f /*volume*/,
														0.0f /*pan*/,
														true /*paused*/,
														0 /*bus*/ );
	}
	DEVASSERT( pInstance->m_SoLoudHandle );

	pInstance->SetPriority( GetPriority() );

	return pInstance;
}

void SoLoudSound::CreateSample( const IDataStream& Stream, const SSoundInit& SoundInit )
{
	// Decoding Vorbis files may be expensive, watch for it.
	PROFILE_FUNCTION;
	FRAME_PROFILE_FUNCTION;

	const int	Length	= Stream.Size();
	byte*		pBuffer	= new byte[ Length ];
	Stream.Read( Length, pBuffer );

	m_SoLoudWav = new SoLoud::Wav;
	const SoLoud::result Result = m_SoLoudWav->loadMem( pBuffer, Length, true /*copy memory*/, true /*take ownership of memory*/ );
	DEVASSERT( Result == SoLoud::SO_NO_ERROR );
	Unused( Result );

	m_SoLoudWav->setLooping( SoundInit.m_IsLooping );

	const bool TickWhenInaudible = ( SoundInit.m_Priority == ESP_High );
	const bool KillWhenInaudible = !( TickWhenInaudible || SoundInit.m_IsLooping );
	m_SoLoudWav->setInaudibleBehavior( TickWhenInaudible, KillWhenInaudible );

	SafeDeleteArray( pBuffer );
}

void SoLoudSound::CreateStream( const PackStream& Stream, const SSoundInit& SoundInit )
{
	SoLoudAudioSystem* const		pSoLoudAudioSystem	= static_cast<SoLoudAudioSystem*>( GetAudioSystem() );
	const HashedString				FilenameHash		= Stream.GetVirtualFilename();	// Don't use GetPhysicalFilename, it won't work with packed files!
	const SoLoudPropCache* const	pPropCache			= pSoLoudAudioSystem->GetPropCache( FilenameHash );

	m_SoLoudWavStream									= new SoLoud::WavStream;
	if( pPropCache )
	{
		m_SoLoudWavStream->mCachedChannels				= pPropCache->m_Channels;
		m_SoLoudWavStream->mCachedSampleRate			= pPropCache->m_SampleRate;
		m_SoLoudWavStream->mCachedSampleCount			= pPropCache->m_SampleCount;
	}
	m_PackageFileReader									= new PackageFileReader( Stream.GetPhysicalFilename(), Stream.GetSubfileOffset(), Stream.GetSubfileLength() );
	const SoLoud::result			Result				= m_SoLoudWavStream->loadFile( m_PackageFileReader );
	DEVASSERT( Result == SoLoud::SO_NO_ERROR );
	Unused( Result );

	if( !pPropCache )
	{
		SoLoudPropCache				PropCache;
		PropCache.m_Channels							= m_SoLoudWavStream->mChannels;
		PropCache.m_SampleRate							= m_SoLoudWavStream->mBaseSamplerate;
		PropCache.m_SampleCount							= m_SoLoudWavStream->mSampleCount;
		pSoLoudAudioSystem->SetPropCache( FilenameHash, PropCache );
	}

	m_SoLoudWavStream->setLooping( SoundInit.m_IsLooping );

	const bool TickWhenInaudible = ( SoundInit.m_Priority == ESP_High );
	const bool KillWhenInaudible = !( TickWhenInaudible || SoundInit.m_IsLooping );
	m_SoLoudWavStream->setInaudibleBehavior( TickWhenInaudible, KillWhenInaudible );
}

float SoLoudSound::GetLength() const
{
	if( m_SoLoudWav )
	{
		return static_cast<float>( m_SoLoudWav->getLength() );
	}
	else if( m_SoLoudWavStream )
	{
		return static_cast<float>( m_SoLoudWavStream->getLength() );
	}
	else
	{
		return 0.0f;
	}
}
