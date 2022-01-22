#include "core.h"
#include "wbcomprosasensorhearing.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "Components/wbcomprodinknowledge.h"
#include "wbcomprosaplayer.h"
#include "wbcomprosatransform.h"
#include "mathcore.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "fontmanager.h"

WBCompRosaSensorHearing::WBCompRosaSensorHearing()
:	m_Radius( 0.0f )
,	m_InvDistanceScaleZ( 0.0f )
,	m_CertaintyFalloffRadius( 0.0f )
,	m_DistanceCertaintyFactor( 0.0f )
,	m_OcclusionCertaintyFactor( 0.0f )
#if BUILD_DEV
,	m_CACHED_LastHeardLocation()
,	m_CACHED_DistanceCertaintyFactor( 0.0f )
,	m_CACHED_OcclusionCertaintyFactor( 0.0f )
,	m_CACHED_Certainty( 0.0f )
#endif
{
	STATIC_HASHED_STRING( OnAINoise );
	GetEventManager()->AddObserver( sOnAINoise, this );
}

WBCompRosaSensorHearing::~WBCompRosaSensorHearing()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnAINoise );
		pEventManager->RemoveObserver( sOnAINoise, this );
	}
}

/*virtual*/ void WBCompRosaSensorHearing::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Radius );
	m_Radius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName );

	STATICHASH( DistanceScaleZ );
	const float DistanceScaleZ = ConfigManager::GetInheritedFloat( sDistanceScaleZ, 1.0f, sDefinitionName );
	m_InvDistanceScaleZ = 1.0f / DistanceScaleZ;

	STATICHASH( CertaintyFalloffRadius );
	m_CertaintyFalloffRadius = ConfigManager::GetInheritedFloat( sCertaintyFalloffRadius, 0.0f, sDefinitionName );

	STATICHASH( DistanceCertaintyFactor );
	m_DistanceCertaintyFactor = ConfigManager::GetInheritedFloat( sDistanceCertaintyFactor, 0.0f, sDefinitionName );

	STATICHASH( OcclusionCertaintyFactor );
	m_OcclusionCertaintyFactor = ConfigManager::GetInheritedFloat( sOcclusionCertaintyFactor, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaSensorHearing::HandleEvent( const WBEvent& Event )
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

		if( pEventOwner == GetEntity() )
		{
			// Ignore AI noises from self (even if there's a separate noise entity).
			// This prevents endlessly making self aware of the same target.
			// (Though two AIs could ping-pong awareness... Maybe should decay
			// certainty over time? As a parameter of the AI noise event?)
		}
		else
		{
			// This way, if we've provided a NoiseEntity parameter and it is null, we *won't* play the noise from the event owner.
			STATIC_HASHED_STRING( NoiseEntity );
			WBEntity* const pNoiseEntity	= Event.GetEntity( sNoiseEntity, pEventOwner );
			if( pNoiseEntity == NULL )
			{
				return;
			}

			// NOTE: Using a parameter instead of the entity's actual transform;
			// fall back to *event owner's* transform if location is unspecified,
			// since that's almost always the default behavior in practice (like
			// a thrown object notifying about the thrower at its own impact
			// location instead of the thrower's location).
			STATIC_HASHED_STRING( NoiseLocation );
			const Vector NoiseEntityLocation	= pEventOwner->GetTransformComponent<WBCompRosaTransform>()->GetLocation();
			const Vector NoiseLocation			= Event.GetVector( sNoiseLocation, NoiseEntityLocation );
			DEVASSERT( !NoiseLocation.IsZero() );

			// If a source location is provided, this is where the AI will understand the noise
			// to have come from, and NoiseLocation will be used for the radius test only.
			// (E.g., when the player opens a door, the player's location is NoiseSourceLocation, and the door's location is NoiseLocation.)
			// (E.g., when an AI calls an alert, the target's location is NoiseSourceLocation, and the AI's location is NoiseLocation.)
			STATIC_HASHED_STRING( NoiseSourceLocation );
			const Vector NoiseSourceLocation	= Event.GetVector( sNoiseSourceLocation );
			const Vector UseNoiseSourceLocation	= NoiseSourceLocation.IsZero() ? NoiseLocation : NoiseSourceLocation;

			STATIC_HASHED_STRING( NoiseRadius );
			const float NoiseRadius = Event.GetFloat( sNoiseRadius );

			// ROSANOTE: Added this as a sort of "volume" on noises that can increase their certainty.
			// E.g., to make sure alarms are relevant at long distances and occluded.
			STATIC_HASHED_STRING( NoiseCertaintyScalar );
			const float NoiseCertaintyScalar	= Event.GetFloat( sNoiseCertaintyScalar );
			const float UseNoiseCertaintyScalar	= ( NoiseCertaintyScalar > 0.0f ) ? NoiseCertaintyScalar : 1.0f;

			STATIC_HASHED_STRING( NoiseUpdateTime );
			const float NoiseUpdateTime = Event.GetFloat( sNoiseUpdateTime );

			STATIC_HASHED_STRING( ExpireTimeBonus );
			const float ExpireTimeBonus = Event.GetFloat( sExpireTimeBonus );

			HandleNoise( pNoiseEntity, NoiseLocation, NoiseRadius, UseNoiseSourceLocation, UseNoiseCertaintyScalar, NoiseUpdateTime, ExpireTimeBonus );
		}
	}
}

bool WBCompRosaSensorHearing::GetNoiseCertainty( WBEntity* const pNoiseEntity, const Vector& NoiseLocation, const float NoiseRadius, const Vector& NoiseSourceLocation, const float NoiseCertaintyScalar, float& OutCertainty ) const
{
	Unused( pNoiseEntity );
	Unused( NoiseSourceLocation );

	RosaWorld* const			pWorld			= GetWorld();
	ASSERT( pWorld );

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector	EarsLocation	= pTransform->GetLocation();
	const float		EffectiveRadius	= Max( 0.0f, m_Radius + NoiseRadius );
	const float		RadiusSumSq		= Square( EffectiveRadius );
	const Vector	ScaleVector		= Vector( 1.0f, 1.0f, m_InvDistanceScaleZ );
	const Vector	NoiseOffset		= ( NoiseLocation - EarsLocation ) * ScaleVector;
	const float		DistanceSq		= NoiseOffset.LengthSquared();

	if( RadiusSumSq < DistanceSq )
	{
		return false;
	}

	CollisionInfo Info;
	Info.m_In_CollideWorld			= true;
	Info.m_In_CollideEntities		= true;
	Info.m_In_UserFlags				= EECF_Audio;
	Info.m_In_StopAtAnyCollision	= true;
	const bool	Occluded					= pWorld->LineCheck( EarsLocation, NoiseLocation, Info );

	const float	HeardDistance				= SqRt( DistanceSq );
	const float	DistanceCertainty			= AttenuateQuad( HeardDistance, m_CertaintyFalloffRadius );	// Changed from Attenuate to match change to audio falloff
	const float	DistanceCertaintyFactor		= Lerp( 1.0f - m_DistanceCertaintyFactor, 1.0f, DistanceCertainty );

	const float	OcclusionCertainty			= Occluded ? 0.0f : 1.0f;
	const float	OcclusionCertaintyFactor	= Lerp( 1.0f - m_OcclusionCertaintyFactor, 1.0f, OcclusionCertainty );

	OutCertainty							= DistanceCertaintyFactor * OcclusionCertaintyFactor * NoiseCertaintyScalar;

#if BUILD_DEV
	// Cache the last certainty values for the player for debug display
	WBCompRosaPlayer* const pNoisePlayer = WB_GETCOMP_SAFE( pNoiseEntity, RosaPlayer );
	if( pNoisePlayer )
	{
		m_CACHED_LastHeardLocation			= NoiseSourceLocation;
		m_CACHED_DistanceCertaintyFactor	= DistanceCertaintyFactor;
		m_CACHED_OcclusionCertaintyFactor	= OcclusionCertaintyFactor;
		m_CACHED_Certainty					= OutCertainty;
	}
#endif

	return true;
}

void WBCompRosaSensorHearing::HandleNoise( WBEntity* const pNoiseEntity, const Vector& NoiseLocation, const float NoiseRadius, const Vector& NoiseSourceLocation, const float NoiseCertaintyScalar, const float NoiseUpdateTime, const float ExpireTimeBonus ) const
{
	if( m_Paused )
	{
		return;
	}

	float Certainty = 0.0f;
	if( !GetNoiseCertainty( pNoiseEntity, NoiseLocation, NoiseRadius, NoiseSourceLocation, NoiseCertaintyScalar, Certainty ) )
	{
		return;
	}

	WBEntity* const						pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRodinKnowledge* const			pKnowledge	= WB_GETCOMP( pEntity, RodinKnowledge );
	if( !pKnowledge )
	{
		// ROSANOTE: I'm using hearing sensors for coffins, which don't have knowledge/thinkers/etc.
		return;
	}

	WBCompRodinKnowledge::TKnowledge&	Knowledge	= pKnowledge->UpdateEntity( pNoiseEntity, NoiseUpdateTime, ExpireTimeBonus );

	STATIC_HASHED_STRING( LastKnownLocation );
	Knowledge.SetVector( sLastKnownLocation, NoiseSourceLocation );
	ASSERT( !NoiseSourceLocation.IsZero() );

	STATIC_HASHED_STRING( LastHeardLocation );
	Knowledge.SetVector( sLastHeardLocation, NoiseSourceLocation );

	STATIC_HASHED_STRING( LastHeardTime );
	Knowledge.SetFloat( sLastHeardTime, GetTime() );

	STATIC_HASHED_STRING( HearingCertainty );
	Knowledge.SetFloat( sHearingCertainty, Certainty );

	STATIC_HASHED_STRING( KnowledgeType );
	STATIC_HASHED_STRING( Target );
	Knowledge.SetHash( sKnowledgeType, sTarget );
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaSensorHearing::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const	pFramework	= GetFramework();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();

	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector				Location	= pTransform->GetLocation();

	pRenderer->DEBUGDrawCross( m_CACHED_LastHeardLocation, 0.5f, ARGB_TO_COLOR( 255, 0, 255, 128 ) );

	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s  Distance factor (%.2f-1.00): %.2f",	DebugRenderLineFeed().CStr(),	1.0f - m_DistanceCertaintyFactor,	m_CACHED_DistanceCertaintyFactor ),		Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 128, 192, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s* Occlusion factor (%.2f-1.00): %.2f",	DebugRenderLineFeed().CStr(),	1.0f - m_OcclusionCertaintyFactor,	m_CACHED_OcclusionCertaintyFactor ),	Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 128, 192, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s= Hearing Certainty: %.2f",				DebugRenderLineFeed().CStr(),										m_CACHED_Certainty ),					Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 128, 192, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
}
#endif
