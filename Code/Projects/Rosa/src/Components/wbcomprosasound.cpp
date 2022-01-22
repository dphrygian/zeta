#include "core.h"
#include "wbcomprosasound.h"
#include "wbcomprosatransform.h"
#include "wbeventmanager.h"
#include "hashedstring.h"
#include "rosaframework.h"
#include "configmanager.h"
#include "isoundinstance.h"
#include "idatastream.h"
#include "bonearray.h"
#include "wbcomprosamesh.h"
#include "wbcomponentarrays.h"
#include "mathfunc.h"

WBCompRosaSound::WBCompRosaSound()
:	m_DefaultSound()
,	m_SoundInstances()
,	m_SendDeletedEvents( false )
,	m_AttachBone()
,	m_AttachBoneIndex( INVALID_INDEX )
{
	SInstanceDeleteCallback Callback;
	Callback.m_Callback	= InstanceDeleteCallback;
	Callback.m_Void		= this;
	GetFramework()->GetAudioSystem()->RegisterInstanceDeleteCallback( Callback );
}

WBCompRosaSound::~WBCompRosaSound()
{
	// Don't send events while shutting down! NOTE: This actually doesn't
	// matter anymore since I unregister the callback first.
	m_SendDeletedEvents = false;

	IAudioSystem* const pAudioSystem = GetFramework()->GetAudioSystem();
	ASSERT( pAudioSystem );

	// Unregister *before* removing sounds, otherwise we get callbacks
	// that modify m_SoundInstances during iteration.
	SInstanceDeleteCallback Callback;
	Callback.m_Callback	= InstanceDeleteCallback;
	Callback.m_Void		= this;
	pAudioSystem->UnregisterInstanceDeleteCallback( Callback );

	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		ASSERT( SoundInstance.m_SoundInstance );

		pAudioSystem->RemoveSoundInstance( SoundInstance.m_SoundInstance );
	}
}

/*virtual*/ void WBCompRosaSound::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Sound );
	m_DefaultSound = ConfigManager::GetInheritedString( sSound, "", sDefinitionName );

	STATICHASH( SendDeletedEvents );
	m_SendDeletedEvents = ConfigManager::GetInheritedBool( sSendDeletedEvents, false, sDefinitionName );

	STATICHASH( AttachBone );
	m_AttachBone = ConfigManager::GetInheritedHash( sAttachBone, HashedString::NullString, sDefinitionName );
}

void WBCompRosaSound::SetAttachBoneIndex( const BoneArray* const pBones )
{
	DEVASSERT( pBones );
	m_AttachBoneIndex = pBones->GetBoneIndex( m_AttachBone );
}

/*virtual*/ void WBCompRosaSound::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( OnMoved );
	STATIC_HASHED_STRING( OnLoaded );
	STATIC_HASHED_STRING( OnTeleported );
	STATIC_HASHED_STRING( ForceUpdateMesh );	// HACKHACK since the purpose is the same as OnTeleported
	STATIC_HASHED_STRING( OnAnimationTick );
	STATIC_HASHED_STRING( PlaySound );
	STATIC_HASHED_STRING( PlaySoundDef );	// HACKHACK: Declaring a "PlaySound" event in code doesn't work because of a name conflict with the Windows API.
	STATIC_HASHED_STRING( StopSound );
	STATIC_HASHED_STRING( StopCategory );
	STATIC_HASHED_STRING( MuteSound );
	STATIC_HASHED_STRING( UnmuteSound );
	STATIC_HASHED_STRING( SetSoundVolume );
	STATIC_HASHED_STRING( PlayBark );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawned )
	{
		SPlaySoundDefParams Params;

		Params.m_SoundDef = m_DefaultSound;
		Params.m_Attached = true;

		PlaySoundDef( Params );
	}
	else if( EventName == sOnMoved || EventName == sOnLoaded || EventName == sOnTeleported || EventName == sForceUpdateMesh )
	{
		STATIC_HASHED_STRING( Location );
		const Vector Location = Event.GetVector( sLocation );

		FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
		{
			SSoundInstance& SoundInstance = SoundInstIter.GetValue();
			ASSERT( SoundInstance.m_SoundInstance );

			if( SoundInstance.m_Attached )
			{
				SoundInstance.m_SoundInstance->SetLocation( Location );
			}

			if( !SoundInstance.m_SoundInstance->IsPlaying() )
			{
				SoundInstance.m_SoundInstance->Tick();	// Tick to commit location change
				SoundInstance.m_SoundInstance->Play();
			}
		}
	}
	else if( EventName == sOnAnimationTick )
	{
		DEVASSERT( m_AttachBoneIndex != INVALID_INDEX );

		WBCompRosaMesh* const pMesh = WB_GETCOMP( GetEntity(), RosaMesh );
		DEVASSERT( pMesh );

		FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
		{
			SSoundInstance& SoundInstance = SoundInstIter.GetValue();
			ASSERT( SoundInstance.m_SoundInstance );

			if( SoundInstance.m_Attached )
			{
				const Vector AttachBoneLocation = pMesh->GetBoneLocation( m_AttachBoneIndex );
				SoundInstance.m_SoundInstance->SetLocation( AttachBoneLocation );
			}
		}
	}
	else if( EventName == sPlaySound || EventName == sPlaySoundDef )
	{
		SPlaySoundDefParams Params;

		STATIC_HASHED_STRING( Sound );
		Params.m_SoundDef = Event.GetString( sSound );

		STATIC_HASHED_STRING( Location );
		Params.m_Location = Event.GetVector( sLocation );

		STATIC_HASHED_STRING( Volume );
		Params.m_VolumeOverride = Event.GetFloat( sVolume );

		STATIC_HASHED_STRING( Attached );
		Params.m_Attached = Event.GetBool( sAttached );

		STATIC_HASHED_STRING( Muted );
		Params.m_Muted = Event.GetBool( sMuted );

		PlaySoundDef( Params );
	}
	else if( EventName == sStopSound )
	{
		STATIC_HASHED_STRING( Sound );
		const SimpleString	Sound		= Event.GetString( sSound );
		const SimpleString&	UseSound	= ( Sound != "" ) ? Sound : m_DefaultSound;

		StopSound( UseSound );
	}
	else if( EventName == sStopCategory )
	{
		STATIC_HASHED_STRING( Category );
		const HashedString Category = Event.GetHash( sCategory );

		StopCategory( Category );
	}
	else if( EventName == sMuteSound )
	{
		STATIC_HASHED_STRING( Sound );
		const SimpleString	Sound		= Event.GetString( sSound );
		const SimpleString&	UseSound	= ( Sound != "" ) ? Sound : m_DefaultSound;

		MuteSound( UseSound );
	}
	else if( EventName == sUnmuteSound )
	{
		STATIC_HASHED_STRING( Sound );
		const SimpleString	Sound		= Event.GetString( sSound );
		const SimpleString&	UseSound	= ( Sound != "" ) ? Sound : m_DefaultSound;

		UnmuteSound( UseSound );
	}
	else if( EventName == sSetSoundVolume )
	{
		STATIC_HASHED_STRING( Sound );
		const SimpleString	Sound		= Event.GetString( sSound );
		const SimpleString&	UseSound	= ( Sound != "" ) ? Sound : m_DefaultSound;

		STATIC_HASHED_STRING( Volume );
		const float			Volume		= Event.GetFloat( sVolume );

		SetSoundVolume( UseSound, Volume );
	}
	else if( EventName == sPlayBark )
	{
		SPlaySoundDefParams Params;

		STATIC_HASHED_STRING( Category );
		const HashedString Category = Event.GetHash( sCategory );

		STATIC_HASHED_STRING( Sound );
		Params.m_SoundDef = Event.GetString( sSound );

		STATIC_HASHED_STRING( Location );
		Params.m_Location = Event.GetVector( sLocation );

		STATIC_HASHED_STRING( Volume );
		Params.m_VolumeOverride = Event.GetFloat( sVolume );

		Params.m_Attached = true;

		StopCategory( Category );
		PlaySoundDef( Params );
	}
}

void WBCompRosaSound::StopSound( const SimpleString& SoundDef )
{
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		DEVASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundDef == SoundDef )
		{
			// FIXME: This modifies m_SoundInstances during iteration.
			SoundInstance.m_SoundInstance->Stop();
		}
	}
}

void WBCompRosaSound::MuteSound( const SimpleString& SoundDef )
{
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		DEVASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundDef == SoundDef )
		{
			SoundInstance.m_SoundInstance->SetBaseVolume( 0.0f );
			SoundInstance.m_Muted = true;
		}
	}
}

void WBCompRosaSound::UnmuteSound( const SimpleString& SoundDef )
{
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		DEVASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundDef == SoundDef )
		{
			SoundInstance.m_SoundInstance->SetBaseVolume( SoundInstance.m_BaseVolume );
			SoundInstance.m_Muted = false;
		}
	}
}

void WBCompRosaSound::SetSoundVolume( const SimpleString& SoundDef, const float Volume )
{
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		DEVASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundDef == SoundDef )
		{
			SoundInstance.m_BaseVolume = Volume;
			SoundInstance.m_SoundInstance->SetBaseVolume( Volume );
		}
	}
}

void WBCompRosaSound::StopCategory( const HashedString& Category )
{
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		ASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundInstance->GetCategory() == Category )
		{
			// FIXME: This modifies m_SoundInstances during iteration.
			SoundInstance.m_SoundInstance->Stop();
		}
	}
}

bool WBCompRosaSound::ShouldSuppress( const SPlaySoundDefParams& Params )
{
	MAKEHASHFROM( SoundDef, Params.m_SoundDef );
	STATICHASH( SuppressGroup );
	const HashedString SuppressGroup = ConfigManager::GetInheritedHash( sSuppressGroup, HashedString::NullString, sSoundDef );
	if( !SuppressGroup )
	{
		return false;
	}

	STATICHASH( SuppressRadius );
	const float SuppressRadiusSq = Square( ConfigManager::GetInheritedFloat( sSuppressRadius, 0.0f, sSoundDef ) );
	DEVASSERT( SuppressRadiusSq > 0.0f );

	STATICHASH( SuppressLimit );
	const uint SuppressLimit = ConfigManager::GetInheritedInt( sSuppressLimit, 0, sSoundDef );
	DEVASSERT( SuppressLimit > 0 );

	const Array<WBCompRosaSound*>* const pSounds = WBComponentArrays::GetComponents<WBCompRosaSound>();
	DEVASSERT( pSounds );	// This one at least should exist!

	uint NumSuppressors = 0;
	FOR_EACH_ARRAY( SoundIter, *pSounds, WBCompRosaSound* )
	{
		const WBCompRosaSound* const pSound = SoundIter.GetValue();
		DEVASSERT( pSound );

		FOR_EACH_ARRAY( SoundInstanceIter, pSound->m_SoundInstances, SSoundInstance )
		{
			const SSoundInstance& SoundInstance = SoundInstanceIter.GetValue();
			if( SoundInstance.m_SuppressGroup != SuppressGroup )
			{
				continue;
			}

			const float DistanceSq = ( SoundInstance.m_SoundInstance->GetLocation() - Params.m_Location ).LengthSquared();
			if( DistanceSq > SuppressRadiusSq )
			{
				continue;
			}

			if( ++NumSuppressors < SuppressLimit )
			{
				continue;
			}

			// We've reached the limit, suppress this sound
			return true;
		}
	}

	return false;
}

ISoundInstance* WBCompRosaSound::PlaySoundDef( SPlaySoundDefParams& Params, const uint StartPosition /*= 0*/ )
{
	XTRACE_FUNCTION;

	if( Params.m_SoundDef == "" )
	{
		return NULL;
	}

	// Initialize attached sounds with the transform location so they don't start at the origin
	if( Params.m_Attached )
	{
		WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		Params.m_Location = pTransform->GetLocation();
	}

	if( ShouldSuppress( Params ) )
	{
		return NULL;
	}

	IAudioSystem* const		pAudioSystem	= GetFramework()->GetAudioSystem();
	ISoundInstance* const	pSoundInstance	= pAudioSystem->CreateSoundInstance( Params.m_SoundDef );
	DEVASSERT( pSoundInstance );

	const float OriginalBaseVolume	= ( Params.m_VolumeOverride > 0.0f ) ? Params.m_VolumeOverride : 1.0f;
	const float ActualBaseVolume	= Params.m_Muted ? 0.0f : OriginalBaseVolume;

	pSoundInstance->SetBaseVolume( ActualBaseVolume );
	pSoundInstance->SetLocation( Params.m_Location );
	pSoundInstance->SetPitch( Math::Random( pSoundInstance->GetPitchMin(), pSoundInstance->GetPitchMax() ) );
	if( StartPosition > 0 )
	{
		pSoundInstance->SetPosition( StartPosition );
	}
	pAudioSystem->ConditionalApplyReverb( pSoundInstance );
	pSoundInstance->Tick();
	pSoundInstance->Play();

	SSoundInstance& SoundInstance	= m_SoundInstances.PushBack();
	SoundInstance.m_SoundInstance	= pSoundInstance;
	SoundInstance.m_Attached		= Params.m_Attached;
	SoundInstance.m_Muted			= Params.m_Muted;
	SoundInstance.m_BaseVolume		= OriginalBaseVolume;
	SoundInstance.m_SoundDef		= Params.m_SoundDef;
	MAKEHASHFROM( SoundDef, Params.m_SoundDef );
	STATICHASH( SuppressGroup );
	SoundInstance.m_SuppressGroup	= ConfigManager::GetInheritedHash( sSuppressGroup, HashedString::NullString, sSoundDef );

	return pSoundInstance;
}

/*static*/ void WBCompRosaSound::InstanceDeleteCallback( void* pVoid, ISoundInstance* pInstance )
{
	WBCompRosaSound* pSound = static_cast<WBCompRosaSound*>( pVoid );
	DEVASSERT( pSound );

	pSound->OnInstanceDeleted( pInstance );
}

void WBCompRosaSound::OnInstanceDeleted( ISoundInstance* const pInstance )
{
	FOR_EACH_ARRAY_REVERSE( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		ASSERT( SoundInstance.m_SoundInstance );

		if( pInstance == SoundInstance.m_SoundInstance )
		{
			if( m_SendDeletedEvents )
			{
				WB_MAKE_EVENT( OnSoundDeleted, GetEntity() );
				WB_SET_AUTO( OnSoundDeleted, Hash, SoundDef, SoundInstance.m_SoundDef );
				WB_DISPATCH_EVENT( GetEventManager(), OnSoundDeleted, GetEntity() );
			}

			m_SoundInstances.FastRemove( SoundInstIter );
		}
	}
}

#define VERSION_EMPTY		0
#define VERSION_SOUNDS		1
#define VERSION_BASEVOLUME	2
#define VERSION_MUTED		3
#define VERSION_CURRENT		3

uint WBCompRosaSound::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// Number of persistent sounds
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		const SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		ASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundInstance->GetShouldSerialize() )
		{
			Size += IDataStream::SizeForWriteString( SoundInstance.m_SoundDef );
			Size += 4;					// Sound position (e.g. in ms)
			Size += sizeof( Vector );	// Location
			Size += 1;					// Attached
			Size += 1;					// Muted
			Size += 4;					// Base volume
		}
	}

	return Size;
}

void WBCompRosaSound::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	uint NumPersistentSounds = 0;
	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		const SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		ASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundInstance->GetShouldSerialize() )
		{
			++NumPersistentSounds;
		}
	}

	Stream.WriteUInt32( NumPersistentSounds );

	FOR_EACH_ARRAY( SoundInstIter, m_SoundInstances, SSoundInstance )
	{
		const SSoundInstance& SoundInstance = SoundInstIter.GetValue();
		ASSERT( SoundInstance.m_SoundInstance );

		if( SoundInstance.m_SoundInstance->GetShouldSerialize() )
		{
			const Vector Location = SoundInstance.m_SoundInstance->GetLocation();
			Stream.WriteString( SoundInstance.m_SoundDef );
			Stream.WriteUInt32( SoundInstance.m_SoundInstance->GetPosition() );
			Stream.Write( sizeof( Vector ), &Location );
			Stream.WriteBool( SoundInstance.m_Attached );
			Stream.WriteBool( SoundInstance.m_Muted );
			Stream.WriteFloat( SoundInstance.m_BaseVolume );
		}
	}
}

void WBCompRosaSound::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_SOUNDS )
	{
		const bool	SerializeBaseVolume	= ( Version >= VERSION_BASEVOLUME );
		const bool	SerializeMuted		= ( Version >= VERSION_MUTED );
		const uint	NumPersistentSounds = Stream.ReadUInt32();
		for( uint PersistentSoundIndex = 0; PersistentSoundIndex < NumPersistentSounds; ++PersistentSoundIndex )
		{
			SPlaySoundDefParams Params;
			Params.m_SoundDef		= Stream.ReadString();
			const uint Position		= Stream.ReadUInt32();
			Stream.Read( sizeof( Vector ), &Params.m_Location );
			Params.m_Attached		= Stream.ReadBool();
			Params.m_Muted			= SerializeMuted ? Stream.ReadBool() : false;
			Params.m_VolumeOverride	= SerializeBaseVolume ? Stream.ReadFloat() : 0.0f;

			PlaySoundDef( Params, Position );
		}
	}
}
