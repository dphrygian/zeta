#include "core.h"
#include "wbcomprosasleeper.h"
#include "wbcomprosasensorhearing.h"
#include "wbcomprosatransform.h"
#include "wbcomprosafaction.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "rosagame.h"
#include "rosadifficulty.h"

WBCompRosaSleeper::WBCompRosaSleeper()
:	m_IsAwake( false )
,	m_NoiseThreshold( 0.0f )
,	m_OnlyHearPlayer( false )
,	m_OnlyHearHostiles( false )
{
	STATIC_HASHED_STRING( OnAINoise );
	GetEventManager()->AddObserver( sOnAINoise, this );
}

WBCompRosaSleeper::~WBCompRosaSleeper()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnAINoise );
		pEventManager->RemoveObserver( sOnAINoise, this );
	}
}

/*virtual*/ void WBCompRosaSleeper::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Awake );
	m_IsAwake	= ConfigManager::GetInheritedBool( sAwake, false, sDefinitionName );

	STATICHASH( NoiseThreshold );
	m_NoiseThreshold = ConfigManager::GetInheritedFloat( sNoiseThreshold, 0.0f, sDefinitionName );

	STATICHASH( OnlyHearPlayer );
	m_OnlyHearPlayer = ConfigManager::GetInheritedBool( sOnlyHearPlayer, false, sDefinitionName );

	STATICHASH( OnlyHearHostiles );
	m_OnlyHearHostiles = ConfigManager::GetInheritedBool( sOnlyHearHostiles, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaSleeper::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnAINoise );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnAINoise )
	{
		STATIC_HASHED_STRING( EventOwner );
		WBEntity* const pEventOwner = Event.GetEntity( sEventOwner );
		DEVASSERT( pEventOwner );

		if( RosaDifficulty::GetGameDifficulty() == 0 )
		{
			// ROSAHACK: In tourist mode, ignore all noises
		}
		else if( pEventOwner == GetEntity() )
		{
			// Ignore AI noises from self (see SensorHearing for more details).
		}
		else if( m_OnlyHearPlayer && pEventOwner != RosaGame::GetPlayer() )
		{
			// Ignore noises from non-player entities
		}
		else if( m_OnlyHearHostiles && RosaFactions::EFR_Hostile != WBCompRosaFaction::GetCon( GetEntity(), pEventOwner ) )
		{
			// Ignore noises from entities we're not hostile to
		}
		else
		{
			STATIC_HASHED_STRING( NoiseLocation );
			const Vector NoiseEntityLocation	= pEventOwner->GetTransformComponent<WBCompRosaTransform>()->GetLocation();
			const Vector NoiseLocation			= Event.GetVector( sNoiseLocation, NoiseEntityLocation );
			DEVASSERT( !NoiseLocation.IsZero() );

			STATIC_HASHED_STRING( NoiseRadius );
			const float NoiseRadius = Event.GetFloat( sNoiseRadius );

			STATIC_HASHED_STRING( NoiseCertaintyScalar );
			const float NoiseCertaintyScalar	= Event.GetFloat( sNoiseCertaintyScalar );
			const float UseNoiseCertaintyScalar	= ( NoiseCertaintyScalar > 0.0f ) ? NoiseCertaintyScalar : 1.0f;

			HandleNoise( NoiseLocation, NoiseRadius, UseNoiseCertaintyScalar );
		}
	}
}

void WBCompRosaSleeper::Wake()
{
	if( m_IsAwake )
	{
		return;
	}

	m_IsAwake = true;

	WB_MAKE_EVENT( OnWoken, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnWoken, GetEntity() );
}

void WBCompRosaSleeper::HandleNoise( const Vector& NoiseLocation, const float NoiseRadius, const float NoiseCertaintyScalar )
{
	if( m_IsAwake )
	{
		return;
	}

	WBCompRosaSensorHearing* const pHearingSensor = WB_GETCOMP( GetEntity(), RosaSensorHearing );
	DEVASSERT( pHearingSensor );

	float Certainty = 0.0f;
	if( !pHearingSensor->GetNoiseCertainty( NULL, NoiseLocation, NoiseRadius, NoiseLocation, NoiseCertaintyScalar, Certainty ) )
	{
		return;
	}

	if( Certainty >= m_NoiseThreshold )
	{
		Wake();
	}
}

#define VERSION_EMPTY	0
#define VERSION_AWAKE	1
#define VERSION_CURRENT	1

uint WBCompRosaSleeper::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += 1;					// m_IsAwake

	return Size;
}

void WBCompRosaSleeper::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_IsAwake );
}

void WBCompRosaSleeper::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_AWAKE )
	{
		m_IsAwake = Stream.ReadBool();
	}
}
