#include "core.h"
#include "wbcomprosasensorvision.h"
#include "configmanager.h"
#include "mathcore.h"
#include "rosaframework.h"
#include "wbcomprosavisible.h"
#include "wbcomponentarrays.h"
#include "wbcomprosatransform.h"
#include "wbcomprosaplayer.h"
#include "Components/wbcomprodinknowledge.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcomprosaheadtracker.h"
#include "Components/wbcomprosamesh.h"
#include "collisioninfo.h"
#include "irenderer.h"
#include "rosaworld.h"
#include "fontmanager.h"
#include "rosagame.h"

WBCompRosaSensorVision::WBCompRosaSensorVision()
:	m_EyesBoneName()
,	m_EyeOffsetZ( 0.0f )
,	m_RadiusSq( 0.0f )
,	m_ConeCos( 0.0f )
,	m_ConeInvZScale( 0.0f )
,	m_CertaintyFalloffRadius( 0.0f )
,	m_DistanceCertaintyFactor( 0.0f )
,	m_PeripheryCertaintyFactor( 0.0f )
,	m_SpeedCertaintyScalar( 0.0f )
,	m_SpeedCertaintyFactor( 0.0f )
,	m_CrouchedCertaintyFactor( 0.0f )
,	m_CertaintyVelocity( 0.0f )
,	m_CertaintyDecay( 0.0f )
,	m_CertaintyThreshold( 0.0f )
,	m_IsAimingThresholdCos( 0.0f )
,	m_OnlySeePlayer( false )
#if BUILD_DEV
,	m_CACHED_DistanceCertaintyFactor( 0.0f )
,	m_CACHED_PeripheryCertaintyFactor( 0.0f )
,	m_CACHED_SpeedCertaintyFactor( 0.0f )
,	m_CACHED_CrouchedCertaintyFactor( 0.0f )
,	m_CACHED_ImmediateCertainty( 0.0f )
,	m_CACHED_Certainty( 0.0f )
#endif
{
}

WBCompRosaSensorVision::~WBCompRosaSensorVision()
{
}

/*virtual*/ void WBCompRosaSensorVision::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EyesBoneName );
	m_EyesBoneName = ConfigManager::GetInheritedHash( sEyesBoneName, HashedString::NullString, sDefinitionName );

	STATICHASH( EyeOffsetZ );
	m_EyeOffsetZ = ConfigManager::GetInheritedFloat( sEyeOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( Radius );
	m_RadiusSq = Square( ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName ) );

	STATICHASH( ConeAngle );
	m_ConeCos = Cos( DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sConeAngle, 0.0f, sDefinitionName ) ) );

	STATICHASH( ConeScaleZ );
	const float ConeScaleZ = ConfigManager::GetInheritedFloat( sConeScaleZ, 1.0f, sDefinitionName );
	m_ConeInvZScale = 1.0f / ConeScaleZ;

	STATICHASH( CertaintyFalloffRadius );
	m_CertaintyFalloffRadius = ConfigManager::GetInheritedFloat( sCertaintyFalloffRadius, 0.0f, sDefinitionName );

	STATICHASH( DistanceCertaintyFactor );
	m_DistanceCertaintyFactor = ConfigManager::GetInheritedFloat( sDistanceCertaintyFactor, 0.0f, sDefinitionName );

	STATICHASH( PeripheryCertaintyFactor );
	m_PeripheryCertaintyFactor = ConfigManager::GetInheritedFloat( sPeripheryCertaintyFactor, 0.0f, sDefinitionName );

	STATICHASH( SpeedCertaintyScalar );
	m_SpeedCertaintyScalar = ConfigManager::GetInheritedFloat( sSpeedCertaintyScalar, 0.0f, sDefinitionName );

	STATICHASH( SpeedCertaintyFactor );
	m_SpeedCertaintyFactor = ConfigManager::GetInheritedFloat( sSpeedCertaintyFactor, 0.0f, sDefinitionName );

	STATICHASH( CrouchedCertaintyFactor );
	m_CrouchedCertaintyFactor = ConfigManager::GetInheritedFloat( sCrouchedCertaintyFactor, 0.0f, sDefinitionName );

	STATICHASH( CertaintyVelocity );
	m_CertaintyVelocity = ConfigManager::GetInheritedFloat( sCertaintyVelocity, 0.0f, sDefinitionName );

	STATICHASH( CertaintyDecay );
	m_CertaintyDecay = ConfigManager::GetInheritedFloat( sCertaintyDecay, 0.0f, sDefinitionName );

	STATICHASH( CertaintyThreshold );
	m_CertaintyThreshold = ConfigManager::GetInheritedFloat( sCertaintyThreshold, 0.0f, sDefinitionName );

	STATICHASH( IsAimingThreshold );
	m_IsAimingThresholdCos = Cos( DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sIsAimingThreshold, 0.0f, sDefinitionName ) ) );

	STATICHASH( OnlySeePlayer );
	m_OnlySeePlayer = ConfigManager::GetInheritedBool( sOnlySeePlayer, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaSensorVision::PollTick( const float DeltaTime ) const
{
	const Array<WBCompRosaVisible*>* pVisibleComponents = WBComponentArrays::GetComponents<WBCompRosaVisible>();
	if( !pVisibleComponents )
	{
		return;
	}

	RosaWorld* const				pWorld			= GetWorld();
	ASSERT( pWorld );

	WBEntity* const					pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBEntity* const					pPlayer			= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRodinKnowledge* const		pKnowledge		= WB_GETCOMP( pEntity, RodinKnowledge );
	DEVASSERT( pKnowledge );

	WBCompStatMod* const			pStatMod		= WB_GETCOMP( pEntity, StatMod );

	Vector EyeLocation;
	Vector EyeDirection;
	GetEyesTransform( EyeLocation, EyeDirection );

	static const Vector				Up				= Vector( 0.0f, 0.0f, 1.0f );
	const Vector					EyeRight		= EyeDirection.Cross( Up ).GetNormalized();
	const Vector					EyeUp			= EyeRight.Cross( EyeDirection );

	WB_MODIFY_FLOAT_SAFE( CertaintyFalloffRadius,	m_CertaintyFalloffRadius,	pStatMod );
	WB_MODIFY_FLOAT_SAFE( DistanceCertaintyFactor,	m_DistanceCertaintyFactor,	pStatMod );
	WB_MODIFY_FLOAT_SAFE( PeripheryCertaintyFactor,	m_PeripheryCertaintyFactor,	pStatMod );
	WB_MODIFY_FLOAT_SAFE( SpeedCertaintyScalar,		m_SpeedCertaintyScalar,		pStatMod );
	WB_MODIFY_FLOAT_SAFE( SpeedCertaintyFactor,		m_SpeedCertaintyFactor,		pStatMod );
	WB_MODIFY_FLOAT_SAFE( CrouchedCertaintyFactor,	m_CrouchedCertaintyFactor,	pStatMod );
	WB_MODIFY_FLOAT_SAFE( CertaintyVelocity,		m_CertaintyVelocity,		pStatMod );
	WB_MODIFY_FLOAT_SAFE( CertaintyDecay,			m_CertaintyDecay,			pStatMod );

	const uint NumVisibles = pVisibleComponents->Size();
	for( uint VisibleIndex = 0; VisibleIndex < NumVisibles; ++VisibleIndex )
	{
		WBCompRosaVisible* const	pVisible		= ( *pVisibleComponents )[ VisibleIndex ];
		DEVASSERT( pVisible );

		WBEntity* const			pVisibleEntity	= pVisible->GetEntity();
		DEVASSERT( pVisibleEntity );

		// HACKHACK
		if( m_OnlySeePlayer && pVisibleEntity != pPlayer )
		{
			continue;
		}

		WBCompRodinKnowledge::TKnowledge* const pKnowledgeEntry = pKnowledge->GetKnowledge( pVisibleEntity );
		if( pKnowledgeEntry )
		{
			// Decay knowledge if there is any
			STATIC_HASHED_STRING( VisionCertainty );
			const float OldVisionCertainty = pKnowledgeEntry->GetFloat( sVisionCertainty );
			const float NewVisionCertainty = Max( 0.0f, OldVisionCertainty - WB_MODDED( CertaintyDecay ) * DeltaTime );
			pKnowledgeEntry->SetFloat( sVisionCertainty, NewVisionCertainty );

#if BUILD_DEV
			// Cache the last certainty values for the player for debug display
			WBCompRosaPlayer* const pVisiblePlayer = WB_GETCOMP( pVisibleEntity, RosaPlayer );
			if( pVisiblePlayer )
			{
				m_CACHED_DistanceCertaintyFactor	= 0.0f;
				m_CACHED_PeripheryCertaintyFactor	= 0.0f;
				m_CACHED_SpeedCertaintyFactor		= 0.0f;
				m_CACHED_CrouchedCertaintyFactor	= 0.0f;
				m_CACHED_ImmediateCertainty			= 0.0f;
				m_CACHED_Certainty					= NewVisionCertainty;
			}
#endif
		}

		if( !pVisible->IsVisible() )
		{
			continue;
		}

		// Don't check visibility of self
		if( pVisibleEntity == pEntity )
		{
			continue;
		}

		// Distance check
		static const bool IsAlreadyVisible = false;
		const Vector	VisibleLocation	= pVisible->GetVisibleLocation( IsAlreadyVisible );
		const Vector	VisibleOffset	= VisibleLocation - EyeLocation;
		const float		VisibleDistSq	= VisibleOffset.LengthSquared();
		if( VisibleDistSq > m_RadiusSq )
		{
			// Entity is beyond vision distance.
			continue;
		}

		// Cone check, with scaling on Z to flatten the cone
		const Vector	OffsetUpPart		= VisibleOffset.ProjectionOnto( EyeUp );
		const Vector	OffsetOtherPart		= VisibleOffset - OffsetUpPart;
		const Vector	EffectiveOffset		= OffsetOtherPart + OffsetUpPart * m_ConeInvZScale;
		const Vector	EffectiveDirection	= EffectiveOffset.GetFastNormalized();
		const float		EffectiveCos		= EyeDirection.Dot( EffectiveDirection );
		if( EffectiveCos < m_ConeCos )
		{
			// Entity is outside view cone.
			continue;
		}

		// World line check
		CollisionInfo Info;
		Info.m_In_CollideWorld			= true;
		Info.m_In_CollideEntities		= true;
		Info.m_In_UserFlags				= EECF_Occlusion;
		Info.m_In_StopAtAnyCollision	= true;
		if( pWorld->LineCheck( EyeLocation, VisibleLocation, Info ) )
		{
			// Entity is occluded.
			continue;
		}

		// Entity is visible.
		WBCompRosaTransform* const	pVisibleTransform		= pVisibleEntity->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pVisibleTransform );

		WBCompRosaPlayer* const		pVisiblePlayer			= WB_GETCOMP( pVisibleEntity, RosaPlayer );

		// Compute certainty of visibility based on many factors

		const float					VisibleCertainty			= pVisible->GetVisibleCertainty();	// Max certainty of this thing, typically 1.0

		const float					ViewingDistance				= VisibleOffset.Length();
		const float					DistanceCertainty			= Attenuate( ViewingDistance, WB_MODDED( CertaintyFalloffRadius ) );
		const float					DistanceCertaintyFactor		= Lerp( 1.0f - WB_MODDED( DistanceCertaintyFactor ), 1.0f, DistanceCertainty );

		const Vector				ViewingDirection			= VisibleOffset.GetFastNormalized();
		const float					PeripheryAngleCos			= EyeDirection.Dot( ViewingDirection );
		const float					PeripheryCertainty			= InvLerp( PeripheryAngleCos, m_ConeCos, 1.0f );
		const float					PeripheryCertaintyFactor	= Lerp( 1.0f - WB_MODDED( PeripheryCertaintyFactor ), 1.0f, PeripheryCertainty );

		const float					Speed						= pVisibleTransform->GetVelocity().Length();
		const float					SpeedCertainty				= Saturate( Speed * WB_MODDED( SpeedCertaintyScalar ) );
		const float					SpeedCertaintyFactor		= Lerp( 1.0f - WB_MODDED( SpeedCertaintyFactor ), 1.0f, SpeedCertainty );

		const bool					IsCrouched					= pVisiblePlayer ? pVisiblePlayer->IsCrouched() : false;
		const float					CrouchedCertaintyFactor		= IsCrouched ? ( 1.0f - WB_MODDED( CrouchedCertaintyFactor ) ) : 1.0f;

		const float					ImmediateCertainty			= Saturate( VisibleCertainty * DistanceCertaintyFactor * PeripheryCertaintyFactor * SpeedCertaintyFactor * CrouchedCertaintyFactor );

		WBCompRodinKnowledge::TKnowledge& Knowledge = pKnowledge->UpdateEntity( pVisibleEntity );

		// Don't set certainty lower than current; only let certainty fall via decay.
		STATIC_HASHED_STRING( VisionCertainty );
		const float OldVisionCertainty		= Knowledge.GetFloat( sVisionCertainty );
		if( ImmediateCertainty > OldVisionCertainty )
		{
			// Clamp to Certainty so we don't overshoot (this ticks infrequently, DeltaTime is large).
			const float NewVisionCertainty	= Min( ImmediateCertainty, OldVisionCertainty + WB_MODDED( CertaintyVelocity ) * DeltaTime );
			Knowledge.SetFloat( sVisionCertainty, NewVisionCertainty );
		}

#if BUILD_DEV
		// Cache the last certainty values for the player for debug display
		if( pVisiblePlayer )
		{
			m_CACHED_DistanceCertaintyFactor	= DistanceCertaintyFactor;
			m_CACHED_PeripheryCertaintyFactor	= PeripheryCertaintyFactor;
			m_CACHED_SpeedCertaintyFactor		= SpeedCertaintyFactor;
			m_CACHED_CrouchedCertaintyFactor	= CrouchedCertaintyFactor;
			m_CACHED_ImmediateCertainty			= ImmediateCertainty;
			m_CACHED_Certainty					= Knowledge.GetFloat( sVisionCertainty );
		}
#endif

		// ROSANOTE: Don't set LastSeenLocation/Time if we have zero certainty.
		// Else, AI ends up with knowledge it shouldn't actually have!
		if( ImmediateCertainty <= m_CertaintyThreshold )
		{
			continue;
		}

		// ********************************
		// It's visible!

		// Check if the thing we see is also aiming at us. This is just a cheap dot product test,
		// used to determine if we might want to dodge.
		const Vector			VisibleDirection	= pVisible->GetVisibleOrientation().ToVector();
		WBCompRosaTransform*	pTransform			= pEntity->GetTransformComponent<WBCompRosaTransform>();
		const Vector			VisibleToThis		= ( pTransform->GetLocation() - VisibleLocation ).GetFastNormalized();
		const float				VisibleAimCos		= VisibleToThis.Dot( VisibleDirection );
		const bool				VisibleIsAiming		= VisibleAimCos >= m_IsAimingThresholdCos;

		// ROSANOTE: Using SeenLocation instead of VisibleLocation here, because we don't want to
		// include the various offsets (camera Z, lean, etc.), we just want to know about the
		// seen entity's transform location. This should fix a bug.
		const Vector			SeenLocation		= pVisibleTransform->GetLocation();

		STATIC_HASHED_STRING( LastKnownLocation );
		Knowledge.SetVector( sLastKnownLocation, SeenLocation );
		ASSERT( !VisibleLocation.IsZero() );

		STATIC_HASHED_STRING( LastSeenLocation );
		Knowledge.SetVector( sLastSeenLocation, SeenLocation );

		STATIC_HASHED_STRING( LastSeenTime );
		Knowledge.SetFloat( sLastSeenTime, GetTime() );

		STATIC_HASHED_STRING( KnowledgeType );
		STATIC_HASHED_STRING( Target );
		Knowledge.SetHash( sKnowledgeType, sTarget );

		STATIC_HASHED_STRING( VisionPriority );
		Knowledge.SetInt( sVisionPriority, pVisible->GetVisionPriority() );

		STATIC_HASHED_STRING( IsAiming );
		Knowledge.SetBool( sIsAiming, VisibleIsAiming );
	}
}

void WBCompRosaSensorVision::GetEyesTransform( Vector& OutEyeLocation, Vector& OutEyeDirection ) const
{
	WBEntity* const						pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaMesh* const				pMesh			= WB_GETCOMP( GetEntity(), RosaMesh );
	DEVASSERT( pMesh );

	const BoneArray* const				pBones			= pMesh->GetBones();
	const int							EyesBoneIndex	= pBones ? pBones->GetBoneIndex( m_EyesBoneName ) : INVALID_INDEX;

	// If we have a valid eyes bone on an animated mesh, use its transform
	if( EyesBoneIndex != INVALID_INDEX )
	{
		Vector	BoneTranslation;
		Angles	BoneOrientation;
		pMesh->GetBoneTransform( EyesBoneIndex, BoneTranslation, BoneOrientation );

		OutEyeLocation									= BoneTranslation;
		OutEyeDirection									= BoneOrientation.ToVector();
	}
	else
	{
		// Else, use the transform (and headtracker if we have one)
		WBCompRosaTransform* const		pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		WBCompRosaHeadTracker* const	pHeadTracker	= WB_GETCOMP( pEntity, RosaHeadTracker );

		OutEyeLocation									= pTransform->GetLocation() + Vector( 0.0f, 0.0f, m_EyeOffsetZ );
		OutEyeDirection									= pHeadTracker ? pHeadTracker->GetLookDirection() : pTransform->GetOrientation().ToVector();
	}
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaSensorVision::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const		pFramework	= GetFramework();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();

	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	const Vector				Location	= pTransform->GetLocation();

	WBCompStatMod* const		pStatMod	= WB_GETCOMP( GetEntity(), StatMod );

	WB_MODIFY_FLOAT_SAFE( DistanceCertaintyFactor,	m_DistanceCertaintyFactor,	pStatMod );
	WB_MODIFY_FLOAT_SAFE( PeripheryCertaintyFactor,	m_PeripheryCertaintyFactor,	pStatMod );
	WB_MODIFY_FLOAT_SAFE( SpeedCertaintyFactor,		m_SpeedCertaintyFactor,		pStatMod );
	WB_MODIFY_FLOAT_SAFE( CrouchedCertaintyFactor,	m_CrouchedCertaintyFactor,	pStatMod );

	Vector EyeLocation;
	Vector EyeDirection;
	GetEyesTransform( EyeLocation, EyeDirection );

	pRenderer->DEBUGDrawCross( EyeLocation, 0.5f, ARGB_TO_COLOR( 255, 255, 0, 0 ) );
	pRenderer->DEBUGDrawArrow( EyeLocation, EyeDirection.ToAngles(), 1.0f, ARGB_TO_COLOR( 255, 255, 0, 0 ) );

	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s  Distance factor (%.2f-1.00): %.2f",	DebugRenderLineFeed().CStr(),	1.0f - WB_MODDED( DistanceCertaintyFactor ),	m_CACHED_DistanceCertaintyFactor ),		Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 192, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s  Periphery factor (%.2f-1.00): %.2f",	DebugRenderLineFeed().CStr(),	1.0f - WB_MODDED( PeripheryCertaintyFactor ),	m_CACHED_PeripheryCertaintyFactor ),	Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 192, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s* Speed factor (%.2f-1.00): %.2f",		DebugRenderLineFeed().CStr(),	1.0f - WB_MODDED( SpeedCertaintyFactor ),		m_CACHED_SpeedCertaintyFactor ),		Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 192, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s* Crouched factor (%.2f-1.00): %.2f",	DebugRenderLineFeed().CStr(),	1.0f - WB_MODDED( CrouchedCertaintyFactor ),	m_CACHED_CrouchedCertaintyFactor ),		Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 192, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s= Imm. certainty: %.2f",				DebugRenderLineFeed().CStr(),													m_CACHED_ImmediateCertainty ),			Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 192, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s> Vision Certainty: %.2f",				DebugRenderLineFeed().CStr(),													m_CACHED_Certainty ),					Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 192, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
}
#endif
