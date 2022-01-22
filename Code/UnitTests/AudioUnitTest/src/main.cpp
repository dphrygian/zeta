#include "core.h"
#include "filestream.h"
#include "keyboard.h"
#include "audio.h"
#include "iaudiosystem.h"
#include "soundmanager.h"
#include "test.h"
#include "isound.h"
#include "isoundinstance.h"
#include "allocator.h"
#include "mathcore.h"

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <Windows.h>

// This is not really a unit test, per se.
// It does output test results, but it's also
// got an infinite loop for testing audio.

int main()
{
	Allocator::GetDefault().Initialize( 16 << 20 );

	STARTTESTS( audio );

	IAudioSystem* pAudioSystem = CreateSoLoudAudioSystem();
	SoundManager* pSoundManager = pAudioSystem->GetSoundManager();

	SSoundInit SoundInit;

	SoundInit.m_Filename = "test.wav";
	SoundInit.m_IsStream = false;
	SoundInit.m_IsLooping = false;
	SoundInit.m_Is3D = false;
	ISound* pSampleSound1 = pSoundManager->GetSound( SoundInit, "" );

	SoundInit.m_Filename = "test.wav";
	SoundInit.m_IsStream = false;
	SoundInit.m_IsLooping = false;
	SoundInit.m_Is3D = false;
	ISound* pSampleSound2 = pSoundManager->GetSound( SoundInit, "" );

	SoundInit.m_Filename = "test.ogg";
	SoundInit.m_IsStream = true;
	SoundInit.m_IsLooping = false;
	SoundInit.m_Is3D = false;
	ISound* pStreamSound3 = pSoundManager->GetSound( SoundInit, "" );

	SoundInit.m_Filename = "test.ogg";
	SoundInit.m_IsStream = true;
	SoundInit.m_IsLooping = false;
	SoundInit.m_Is3D = false;
	ISound* pStreamSound4 = pSoundManager->GetSound( SoundInit, "" );

	SoundInit.m_Filename = "testloop.wav";
	SoundInit.m_IsStream = false;
	SoundInit.m_IsLooping = true;
	SoundInit.m_Is3D = false;
	ISound* pSampleSound5 = pSoundManager->GetSound( SoundInit, "" );

	TEST( pSampleSound1 == pSampleSound2 );
	TEST( pStreamSound3 != pStreamSound4 );

	ISoundInstance* pSound1Instance1 = pAudioSystem->CreateSoundInstance( pSampleSound1 );
	pSound1Instance1->Play();
	ISoundInstance* pSound1Instance2 = pAudioSystem->CreateSoundInstance( pSampleSound1 );
	pSound1Instance2->Play();
	pAudioSystem->CreateSoundInstance( pSampleSound1 )->Play();
	ISoundInstance* pSound3Instance = pAudioSystem->CreateSoundInstance( pStreamSound3 );
	pSound3Instance->Play();
	ISoundInstance* pSound4Instance = pAudioSystem->CreateSoundInstance( pStreamSound4 );
	pSound4Instance->Play();
	ISoundInstance* pSound5Instance = pAudioSystem->CreateSoundInstance( pSampleSound5 );
	pSound5Instance->Play();

	// One-line way of playing the sound through audio system
	SoundInit.m_Filename = "test.wav";
	SoundInit.m_IsStream = false;
	SoundInit.m_IsLooping = false;
	SoundInit.m_Is3D = false;
	pAudioSystem->CreateSoundInstance( pAudioSystem->GetSoundManager()->GetSound( SoundInit, "" ) )->Play();

	pAudioSystem->SetMasterVolume( 0.2f );

	Keyboard keyboard;

	float Sound5Pan = 0.0f;
	float Sound5Pitch = 1.0f;

	while( !keyboard.IsHigh( Keyboard::EB_Escape ) )
	{
		keyboard.Tick( 0.0f );

		pAudioSystem->Tick( 0.0f, false );

		//if( pAudioSystem->IsValid( pSound1Instance1 ) ) PRINTF( "Sound 1 Instance 1 valid\n" );
		//if( pAudioSystem->IsValid( pSound1Instance2 ) ) PRINTF( "Sound 1 Instance 2 valid\n" );
		//if( pAudioSystem->IsValid( pSound3Instance ) ) PRINTF( "Sound 3 Instance valid\n" );
		//if( pAudioSystem->IsValid( pSound4Instance ) ) PRINTF( "Sound 4 Instance valid\n" );

		if( keyboard.OnRise( Keyboard::EB_LeftBrace ) )
		{
			if( keyboard.IsHigh( Keyboard::EB_Virtual_Shift ) )
			{
				Sound5Pitch = Max( 0.5f, Sound5Pitch - 0.1f );
				pSound5Instance->SetPitch( Sound5Pitch );
				PRINTF( "Pitch: %f\n", Sound5Pitch );
			}
			else
			{
				Sound5Pan = Max( -1.0f, Sound5Pan - 0.2f );
				pSound5Instance->SetPan( Sound5Pan );
				PRINTF( "Pan: %f\n", Sound5Pan );
			}
		}
		else if( keyboard.OnRise( Keyboard::EB_RightBrace ) )
		{
			if( keyboard.IsHigh( Keyboard::EB_Virtual_Shift ) )
			{
				Sound5Pitch = Min( 2.0f, Sound5Pitch + 0.1f );
				pSound5Instance->SetPitch( Sound5Pitch );
				PRINTF( "Pitch: %f\n", Sound5Pitch );
			}
			else
			{
				Sound5Pan = Min( 1.0f, Sound5Pan + 0.2f );
				pSound5Instance->SetPan( Sound5Pan );
				PRINTF( "Pan: %f\n", Sound5Pan );
			}
		}
	}

	//SafeDelete( pStreamSound3 );		// Need to delete streaming sounds manually because they're not added to the SoundManager's map
	//SafeDelete( pStreamSound4 );
	SafeDelete( pAudioSystem );

	int NumFailed = TESTSFAILED;
	printf( "%d tests failed\n", NumFailed );
	ENDTESTS;

	Allocator::GetDefault().Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Write ) );
	Allocator::GetDefault().ShutDown();

	return NumFailed;
}
