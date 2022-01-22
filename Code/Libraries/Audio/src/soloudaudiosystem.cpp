#include "core.h"
#include "soloudaudiosystem.h"
#include "soundmanager.h"
#include "soloudsound.h"
#include "soloudsoundinstance.h"
#include "configmanager.h"
#include "mathfunc.h"
#include "mathcore.h"
#include "interpolator.h"
#include "filestream.h"

#define ECHO_FILTER_ID		0
#define WETDRY_ATTRIBUTE_ID	0

#if BUILD_DEV
static const uint			skReverbTest_NumSettings	= 4;
static const char* const	skReverbTest_ConfigNames[] 	=
{
	"EchoTime",
	"DecayTime",
	"LowPassFilter",
	"WetDryMix",
};
static const char* const	skReverbTest_PrettyNames[] 	=
{
	"Echo Time:       ",
	"Decay Time:      ",
	"Low-Pass Filter: ",
	"Wet/Dry Mix:     ",
};
static const char* const	skReverbTest_Units[] 		= { "s",	"s",	"a",	"a", };
static const float			skReverbTest_Lows[]			= { 0.02f,	0.05f,	0.0f,	0.0f, };
static const float			skReverbTest_Highs[]		= { 0.50f,	2.0f,	1.0f,	1.0f, };
static const float			skReverbTest_Steps[]		= { 0.02f,	0.05f,	0.05f,	0.25f, };
static float				sReverbTest_Settings[]		= { 0.2f,	1.0f,	0.0f,	1.0f, };
#endif	// BUILD_DEV

#define SOLOUD_ERROR_CHECK( func )							\
	ASSERT( Result == SoLoud::SO_NO_ERROR );				\
	if( Result != SoLoud::SO_NO_ERROR )						\
	{														\
		PRINTF( "SoLoud: " #func ": 0x%08X\n", Result );	\
	}

SoLoudAudioSystem::SoLoudAudioSystem()
:	m_SoLoudEngine( NULL )
,	m_SoLoudEchoBus( NULL )
,	m_SoLoudEchoFilter( NULL )
,	m_SoLoudEchoBusHandle( 0 )
,	m_DefaultReverb()
,	m_ReverbCategories()
,	m_PropCache()
#if BUILD_DEV
,	m_ReverbTest_IsActive( false )
,	m_ReverbTest_SettingIndex( 0 )
#endif
{
	DEBUGPRINTF( "Initializing SoLoud audio system...\n" );

	STATICHASH( AudioSystem );

	// Create SoLoud engine
	SoLoud::result Result;
	m_SoLoudEngine = new SoLoud::Soloud;
	Result = m_SoLoudEngine->init(
				SoLoud::Soloud::CLIP_ROUNDOFF /*flags*/,
				SoLoud::Soloud::AUTO /*backend*/,
				SoLoud::Soloud::AUTO /*sample rate*/,
				2048 /*buffer size; using SoLoud::Soloud::AUTO here defaults to 4096 for Windows APIs, which introduces noticeable latency*/,
				2 /*channels*/ );
	SOLOUD_ERROR_CHECK( init );

	PRINTF( "SoLoud backend: %s\n", m_SoLoudEngine->getBackendString() );

	// TODO: Set max virtual voices higher than 1024 if desired by editing VOICE_COUNT in header (up to 4095) and recompiling
	// TODO: Set max active voice count if desired; defaults to 16
	//m_SoLoudEngine->setMaxActiveVoiceCount( 16 );

#define USE_REVERB 1
#if USE_REVERB
	m_SoLoudEchoFilter = new SoLoud::EchoFilter;

	m_SoLoudEchoBus = new SoLoud::Bus;
	m_SoLoudEchoBus->setFilter( ECHO_FILTER_ID, m_SoLoudEchoFilter );

	m_SoLoudEchoBusHandle = m_SoLoudEngine->play(
		*m_SoLoudEchoBus /*source*/,
		-1.0f /*volume*/,
		0.0f /*pan*/,
		false /*paused*/,
		0 /*bus*/ );

	STATICHASH( DefaultReverb );
	m_DefaultReverb = ConfigManager::GetString( sDefaultReverb, "", sAudioSystem );
	SetReverbParams( m_DefaultReverb );

	STATICHASH( NumReverbCategories );
	const uint NumReverbCategories = ConfigManager::GetInt( sNumReverbCategories, 0, sAudioSystem );
	for( uint ReverbCategoryIndex = 0; ReverbCategoryIndex < NumReverbCategories; ++ReverbCategoryIndex )
	{
		const HashedString ReverbCategory = ConfigManager::GetSequenceHash( "ReverbCategory%d", ReverbCategoryIndex, "", sAudioSystem );
		m_ReverbCategories.PushBack( ReverbCategory );
	}
#endif
}

SoLoudAudioSystem::~SoLoudAudioSystem()
{
	// Free sound instances before deleting sound manager, else
	// querying if sounds are streams when deleting instances will fail
	FreeSoundInstances();

	// Parent class owns this, but order of deletion matters...
	SafeDelete( m_SoundManager );

	SafeDelete( m_SoLoudEchoFilter );
	SafeDelete( m_SoLoudEchoBus );

	m_SoLoudEngine->deinit();
	SafeDelete( m_SoLoudEngine );
}

bool SoLoudAudioSystem::TryLockAudioSystem()
{
	return m_SoLoudEngine->tryLockMixerMutex();
}

void SoLoudAudioSystem::UnlockAudioSystem()
{
	m_SoLoudEngine->unlockMixerMutex();
}

void SoLoudAudioSystem::Tick( const float DeltaTime, bool GamePaused )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	AudioSystemCommon::Tick( DeltaTime, GamePaused );

	//m_FMODSystem->update();
}

ISound* SoLoudAudioSystem::CreateSound( const SSoundInit& SoundInit )
{
	SoLoudSound* const pSound = new SoLoudSound( this, SoundInit );
	pSound->Initialize( SoundInit );
	return pSound;
}

ISoundInstance* SoLoudAudioSystem::Play( const SimpleString& DefinitionName, const Vector& Location )
{
	ISoundInstance* const pInstance = CreateSoundInstance( DefinitionName );
	ASSERT( pInstance );

	// Set variables, then unpause
	pInstance->SetLocation( Location );

	pInstance->Tick();	// To make sure all changes are applied
	pInstance->SetPaused( false );

	return pInstance;
}

void SoLoudAudioSystem::SetReverbParams( const SimpleString& DefinitionName ) const
{
	const SimpleString& ReverbDef = ( DefinitionName != "" ) ? DefinitionName : m_DefaultReverb;

	STATICHASH( SoLoudEchoTime );
	STATICHASH( SoLoudDecayTime );
	STATICHASH( SoLoudFilter );
	STATICHASH( SoLoudWetDryMix );
	MAKEHASH( ReverbDef );

	const float EchoTime		= ConfigManager::GetInheritedFloat( sSoLoudEchoTime, 0.2f, sReverbDef );
	const float DecayTime		= ConfigManager::GetInheritedFloat( sSoLoudDecayTime, 1.0f, sReverbDef );
	const float LowPassFilter	= ConfigManager::GetInheritedFloat( sSoLoudFilter, 0.0f, sReverbDef );
	const float WetDryMix		= ConfigManager::GetInheritedFloat( sSoLoudWetDryMix, 1.0f, sReverbDef );

	ApplyReverb( EchoTime, DecayTime, LowPassFilter, WetDryMix );
}

void SoLoudAudioSystem::ApplyReverb( const float EchoTime, const float DecayTime, const float LowPassFilter, const float WetDryMix ) const
{
	if( !m_SoLoudEchoFilter )
	{
		return;
	}

	const float NumEchoesOverDecayTime	= DecayTime / EchoTime;
	const float	TargetDecay				= 0.01f;
	const float Decay					= Pow( TargetDecay, 1.0f / NumEchoesOverDecayTime );	// Reach target over time

	m_SoLoudEchoFilter->setParams( EchoTime, Decay, LowPassFilter );
	m_SoLoudEchoBus->setFilter( ECHO_FILTER_ID, m_SoLoudEchoFilter );	// Re-add the filter to get a new instance (resizes delay buffer)
	m_SoLoudEngine->setFilterParameter( m_SoLoudEchoBusHandle, ECHO_FILTER_ID, WETDRY_ATTRIBUTE_ID, WetDryMix );

#if BUILD_DEV
	sReverbTest_Settings[0]	= EchoTime;
	sReverbTest_Settings[1]	= DecayTime;
	sReverbTest_Settings[2]	= LowPassFilter;
	sReverbTest_Settings[3]	= WetDryMix;
#endif
}

const SoLoudPropCache* SoLoudAudioSystem::GetPropCache( const HashedString& Filename ) const
{
	Map<HashedString, SoLoudPropCache>::Iterator SampleCountIter = m_PropCache.Search( Filename );
	return SampleCountIter.IsValid() ? &SampleCountIter.GetValue() : NULL;
}

void SoLoudAudioSystem::SetPropCache( const HashedString& Filename, const SoLoudPropCache& PropCache )
{
	m_PropCache.Insert( Filename, PropCache );
}

#if BUILD_DEV

/*virtual*/ void SoLoudAudioSystem::ReverbTest_Toggle()
{
	m_ReverbTest_IsActive = !m_ReverbTest_IsActive;
	ReverbTest_Update();
	ReverbTest_Print();
	ReverbTest_Export();
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_Update()
{
	ApplyReverb( sReverbTest_Settings[0], sReverbTest_Settings[1], sReverbTest_Settings[2], sReverbTest_Settings[3] );
}

/*virtual*/ bool SoLoudAudioSystem::ReverbTest_IsActive() const
{
	return m_ReverbTest_IsActive;
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_Print() const
{
	if( !m_ReverbTest_IsActive )
	{
		return;
	}

	PRINTF( "================================\n" );
	FOR_EACH_INDEX( ReverbSettingIndex, skReverbTest_NumSettings )
	{
		PRINTF( ReverbSettingIndex == m_ReverbTest_SettingIndex ? "| -> " : "|    " );
		PRINTF( "%s: %.2f %s\n", skReverbTest_PrettyNames[ ReverbSettingIndex ], sReverbTest_Settings[ ReverbSettingIndex ], skReverbTest_Units[ ReverbSettingIndex ] );
	}
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_Export() const
{
	FileStream Stream = FileStream( "reverb-settings.txt", FileStream::EFM_Write );

	Stream.PrintF( "[Reverb]\n" );
	FOR_EACH_INDEX( ReverbSettingIndex, skReverbTest_NumSettings )
	{
		Stream.PrintF( "%s = %.2f\n", skReverbTest_ConfigNames[ ReverbSettingIndex ], sReverbTest_Settings[ ReverbSettingIndex ] );
	}
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_PrevSetting()
{
	m_ReverbTest_SettingIndex = ( m_ReverbTest_SettingIndex + ( skReverbTest_NumSettings - 1 ) ) % skReverbTest_NumSettings;
	ReverbTest_Print();
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_NextSetting()
{
	m_ReverbTest_SettingIndex = ( m_ReverbTest_SettingIndex + 1 ) % skReverbTest_NumSettings;
	ReverbTest_Print();
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_IncrementSetting( const float Scalar )
{
	float& Setting = sReverbTest_Settings[ m_ReverbTest_SettingIndex ];
	Setting = Min( Setting + Scalar * skReverbTest_Steps[ m_ReverbTest_SettingIndex ], skReverbTest_Highs[ m_ReverbTest_SettingIndex ] );
	ReverbTest_Update();
	ReverbTest_Print();
	ReverbTest_Export();
}

/*virtual*/ void SoLoudAudioSystem::ReverbTest_DecrementSetting( const float Scalar )
{
	float& Setting = sReverbTest_Settings[ m_ReverbTest_SettingIndex ];
	Setting = Max( Setting - Scalar * skReverbTest_Steps[ m_ReverbTest_SettingIndex ], skReverbTest_Lows[ m_ReverbTest_SettingIndex ] );
	ReverbTest_Update();
	ReverbTest_Print();
	ReverbTest_Export();
}

#endif	// BUILD_DEV
