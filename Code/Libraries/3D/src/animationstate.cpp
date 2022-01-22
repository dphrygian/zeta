#include "core.h"
#include "animationstate.h"
#include "animation.h"
#include "bonearray.h"
#include "mathcore.h"
#include "matrix.h"
#include "bonearray.h"
#include "animevent.h"
#include "3d.h"
#include "ibonemodifier.h"

/*static*/ bool AnimationState::sm_StylizedAnim = false;

/*static*/ bool AnimationState::StaticGetStylizedAnim()
{
	return sm_StylizedAnim;
}

/*static*/ void AnimationState::StaticSetStylizedAnim( const bool StylizedAnim )
{
	sm_StylizedAnim = StylizedAnim;
}

#if BUILD_DEBUG
#define BEGIN_ITERATING_LAYERS	do{ ++m_IteratingLayersRefCount; } while(0)
#define END_ITERATING_LAYERS	do{ --m_IteratingLayersRefCount; } while(0)
#define CHECK_ITERATING_LAYERS	do{ DEVASSERT( 0 == m_IteratingLayersRefCount ); } while(0)
#else
#define BEGIN_ITERATING_LAYERS	DoNothing
#define END_ITERATING_LAYERS	DoNothing
#define CHECK_ITERATING_LAYERS	DoNothing
#endif

AnimationState::AnimationState()
:	m_RefCount( 0 )
,	m_AnimLayers()
,	m_LayeredAnimLayers()
,	m_AdditiveAnimLayers()
,	m_OwnerMesh( NULL )
,	m_SuppressAnimEvents( false )
,	m_AnimationListeners()
#if BUILD_DEBUG
,	m_IteratingLayersRefCount( 0 )
#endif
,	m_BoneMatrices( NULL )
,	m_DirtyBoneMatrices( false )
,	m_BoneModifiers()
,	m_AnimationBonesTickRate( 0.0f )
,	m_NextBonesRefreshTime( 0.0f )
{
	m_AnimLayers.SetDeflate( false );
	m_LayeredAnimLayers.SetDeflate( false );
	m_AdditiveAnimLayers.SetDeflate( false );
}

AnimationState::~AnimationState()
{
	SafeDeleteArray( m_BoneMatrices );
}

int AnimationState::AddReference()
{
	++m_RefCount;
	return m_RefCount;
}

int AnimationState::Release()
{
	--m_RefCount;
	if( m_RefCount <= 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCount;
}

void AnimationState::Tick( const float DeltaTime )
{
	if( !sm_StylizedAnim || m_AnimationBonesTickRate <= 0.0f )
	{
		m_NextBonesRefreshTime		= 0.0f;
		m_DirtyBoneMatrices			= true;
	}
	else
	{
		m_NextBonesRefreshTime		-= DeltaTime;
		while( m_NextBonesRefreshTime <= 0.0f )
		{
			m_NextBonesRefreshTime	+= m_AnimationBonesTickRate;
			m_DirtyBoneMatrices		= true;
		}
	}

	BEGIN_ITERATING_LAYERS;
	FOR_EACH_ARRAY_REVERSE( AnimLayerIter, m_AnimLayers, SAnimationLayer )
	{
		SAnimationLayer&	AnimLayer	= AnimLayerIter.GetValue();
		const bool			IsTopLayer	= AnimLayerIter.GetIndex() == ( m_AnimLayers.Size() - 1 );

		// Remove any layers that have blended out
		if( AnimLayer.m_BlendTimeLeft > 0.0f )
		{
			AnimLayer.m_BlendTimeLeft -= DeltaTime;
			if( AnimLayer.m_BlendTimeLeft <= 0.0f )
			{
				m_AnimLayers.Remove( AnimLayerIter );
				continue;
			}
		}

		// Tick layers, only doing notifications and events on top layer
		TickLayer( AnimLayer, DeltaTime, IsTopLayer );
	}
	END_ITERATING_LAYERS;

	BEGIN_ITERATING_LAYERS;
	FOR_EACH_ARRAY_REVERSE( LayeredAnimLayerIter, m_LayeredAnimLayers, SAnimationLayer )
	{
		SAnimationLayer&	LayeredAnimLayer	= LayeredAnimLayerIter.GetValue();
		const bool			IsTopLayer			= false;

		TickLayer( LayeredAnimLayer, DeltaTime, IsTopLayer );

		if( LayeredAnimLayer.m_Stopped )
		{
			m_LayeredAnimLayers.Remove( LayeredAnimLayerIter );
		}
	}
	END_ITERATING_LAYERS;

	BEGIN_ITERATING_LAYERS;
	FOR_EACH_ARRAY_REVERSE( AdditiveAnimLayerIter, m_AdditiveAnimLayers, SAnimationLayer )
	{
		SAnimationLayer&	AdditiveAnimLayer	= AdditiveAnimLayerIter.GetValue();
		const bool			IsTopLayer			= false;

		TickLayer( AdditiveAnimLayer, DeltaTime, IsTopLayer );

		if( AdditiveAnimLayer.m_Stopped )
		{
			m_AdditiveAnimLayers.Remove( AdditiveAnimLayerIter );
		}
	}
	END_ITERATING_LAYERS;
}

void AnimationState::TickLayer( SAnimationLayer& AnimLayer, const float DeltaTime, const bool IsTopLayer )
{
	DEVASSERT( IsTopLayer || AnimLayer.m_BlendTime > 0.0f || AnimLayer.m_Layered || AnimLayer.m_Additive );
	DEVASSERT( AnimLayer.m_Animation );

	const float OldTime			= AnimLayer.m_Time;
	const float LayerDeltaTime	= DeltaTime * AnimLayer.m_PlayRate * AnimLayer.m_Animation->m_PlayRate;

	if( AnimLayer.m_EndBehavior == EAEB_Stop && !AnimLayer.m_Stopped )
	{
		AnimLayer.m_Time += LayerDeltaTime;
		const float NonLoopingLength = AnimLayer.m_Animation->GetNonLoopingLengthSeconds();
		if( AnimLayer.m_Time >= NonLoopingLength )
		{
			AnimLayer.m_Stopped = true;
			AnimLayer.m_Time = NonLoopingLength;

			if( IsTopLayer || AnimLayer.m_Layered || AnimLayer.m_Additive )
			{
				// Notify any listeners that the top-layer animation has finished
				// Also notify for layered or additive layers because they don't interrupt (and the BT blocks on them)
				FOR_EACH_LIST( NotifyIter, m_AnimationListeners, SAnimationListener )
				{
					SAnimationListener& AnimationListener = *NotifyIter;
					if( AnimationListener.m_NotifyFinishedFunc )
					{
						AnimationListener.m_NotifyFinishedFunc( AnimationListener.m_Void, m_OwnerMesh, AnimLayer.m_Animation, false );
					}
				}
			}
		}
	}
	else if( AnimLayer.m_EndBehavior == EAEB_Loop )
	{
		AnimLayer.m_Time = Mod( AnimLayer.m_Time + LayerDeltaTime, AnimLayer.m_Animation->GetLengthSeconds() );
	}

	DEVASSERT( AnimLayer.m_Time >= 0.0f );
	DEVASSERT( AnimLayer.m_Time < AnimLayer.m_Animation->GetLengthSeconds() );
	DEVASSERT( static_cast<uint>( AnimLayer.m_Time * ANIM_FRAMERATE ) < AnimLayer.m_Animation->GetLengthFrames() );

	if( ( IsTopLayer || AnimLayer.m_Layered || AnimLayer.m_Additive ) && !m_SuppressAnimEvents )
	{
		TickAnimEvents( AnimLayer, OldTime );
	}
}

void AnimationState::TickAnimEvents( const SAnimationLayer& AnimLayer, const float OldTime )
{
	// NOTE: This logic precludes events on the last frame for non-looping
	// animations (which is okay, because we don't want to call it every
	// frame when the animation has ended, and we notify listeners about
	// animation finishing separately).
	for( uint AnimEventIndex = 0; AnimEventIndex < AnimLayer.m_Animation->m_AnimEvents.Size(); ++AnimEventIndex )
	{
		AnimEvent*	pAnimEvent			= AnimLayer.m_Animation->m_AnimEvents[ AnimEventIndex ];

		const bool	IsPostAnimEventTime	= AnimLayer.m_Time > pAnimEvent->m_Time;
		const bool	WasPreAnimEventTime	= OldTime <= pAnimEvent->m_Time && pAnimEvent->m_Time > 0.0f;	// Because 0-frame events are triggered immediately
		const bool	HasLooped			= OldTime > AnimLayer.m_Time;									// Catch anim events when animation loops (including 0-frame)

		const bool	ShouldCallAnimEvent	= IsPostAnimEventTime && ( WasPreAnimEventTime || HasLooped );
		if( ShouldCallAnimEvent )
		{
			CallAnimEvent( pAnimEvent, AnimLayer );
		}
	}
}

void AnimationState::PlayAnimation( Animation* pAnimation, const SPlayAnimationParams& PlayParams )
{
	if( !pAnimation )
	{
		return;
	}

	if( PlayParams.m_Layered )
	{
		DEVASSERTDESC( !PlayParams.m_Additive, "Animations cannot be both layered and additive." );
		DEVASSERTDESC( PlayParams.m_BlendTime == 0.0f, "Non-zero blend time in layered animation. Layered animations do not blend." );

		// Add a new layered animation layer; these do not interrupt each other or base anims
		CHECK_ITERATING_LAYERS;
		SAnimationLayer& LayeredAnimLayer	= m_LayeredAnimLayers.PushBack();
		LayeredAnimLayer.m_Animation		= pAnimation;
		LayeredAnimLayer.m_PlayRate			= PlayParams.m_PlayRate;
		//LayeredAnimLayer.m_Time			= 0.0f;
		LayeredAnimLayer.m_EndBehavior		= PlayParams.m_EndBehavior;
		//LayeredAnimLayer.m_Stopped		= false;
		LayeredAnimLayer.m_Additive			= true;

		return;
	}

	if( PlayParams.m_Additive )
	{
		DEVASSERTDESC( !PlayParams.m_Layered, "Animations cannot be both layered and additive." );
		DEVASSERTDESC( PlayParams.m_BlendTime == 0.0f, "Non-zero blend time in additive animation. Additive animations do not blend." );

		// Add a new additive animation layer; these do not interrupt each other or base anims
		CHECK_ITERATING_LAYERS;
		SAnimationLayer& AdditiveAnimLayer	= m_AdditiveAnimLayers.PushBack();
		AdditiveAnimLayer.m_Animation		= pAnimation;
		AdditiveAnimLayer.m_PlayRate		= PlayParams.m_PlayRate;
		//AdditiveAnimLayer.m_Time			= 0.0f;
		AdditiveAnimLayer.m_EndBehavior		= PlayParams.m_EndBehavior;
		//AdditiveAnimLayer.m_Stopped		= false;
		AdditiveAnimLayer.m_Additive		= true;

		return;
	}

	SAnimationLayer* const pTopLayer = GetTopAnimationLayer();
	if( pTopLayer )
	{
		DEVASSERT( pTopLayer->m_Animation );

		if( PlayParams.m_IgnoreIfAlreadyPlaying && pTopLayer->m_Animation == pAnimation )
		{
			// We're already playing this animation and don't want to restart.
			// But do update the play rate if it has changed. Poorly documented side effect!
			pTopLayer->m_PlayRate = PlayParams.m_PlayRate;
			return;
		}

		// Notify listeners that the current animation was interrupted
		if( !pTopLayer->m_Stopped )
		{
			FOR_EACH_LIST( NotifyIter, m_AnimationListeners, SAnimationListener )
			{
				SAnimationListener& AnimationListener = *NotifyIter;
				if( AnimationListener.m_NotifyFinishedFunc )
				{
					AnimationListener.m_NotifyFinishedFunc( AnimationListener.m_Void, m_OwnerMesh, pTopLayer->m_Animation, true );
				}
			}
		}
	}

	// Remove or blend out all lower layers
	if( PlayParams.m_BlendTime <= 0.0f )
	{
		CHECK_ITERATING_LAYERS;
		m_AnimLayers.Clear();
	}
	else
	{
		BEGIN_ITERATING_LAYERS;
		FOR_EACH_ARRAY( LayerIter, m_AnimLayers, SAnimationLayer )
		{
			SAnimationLayer&	Layer		= LayerIter.GetValue();
			const bool			WasBlending	= Layer.m_BlendTime > 0.0f;
			Layer.m_BlendTime				= WasBlending ? Min( Layer.m_BlendTime,		PlayParams.m_BlendTime ) : PlayParams.m_BlendTime;
			Layer.m_BlendTimeLeft			= WasBlending ? Min( Layer.m_BlendTimeLeft,	PlayParams.m_BlendTime ) : PlayParams.m_BlendTime;
		}
		END_ITERATING_LAYERS;
	}

	// Add a new animation layer
	CHECK_ITERATING_LAYERS;
	SAnimationLayer& AnimLayer	= m_AnimLayers.PushBack();
	AnimLayer.m_Animation		= pAnimation;
	AnimLayer.m_PlayRate		= PlayParams.m_PlayRate;
	//AnimLayer.m_Time			= 0.0f;
	AnimLayer.m_EndBehavior		= PlayParams.m_EndBehavior;
	//AnimLayer.m_Stopped		= false;
	//AnimLayer.m_Additive		= false;

	// Immediately play any anim events on the first frame
	if( !m_SuppressAnimEvents && !PlayParams.m_SuppressImmediateAnimEvents )
	{
		for( uint AnimEventIndex = 0; AnimEventIndex < AnimLayer.m_Animation->m_AnimEvents.Size(); ++AnimEventIndex )
		{
			AnimEvent* pAnimEvent = AnimLayer.m_Animation->m_AnimEvents[ AnimEventIndex ];
			if( pAnimEvent->m_Time == 0.0f )
			{
				CallAnimEvent( pAnimEvent, AnimLayer );
			}
		}
	}
}

void AnimationState::SetAnimationBlend( Animation* const pAnimationA, Animation* const pAnimationB, const float BlendAlpha )
{
	if( !pAnimationA || !pAnimationB )
	{
		return;
	}

	SAnimationLayer* const pTopLayer = GetTopAnimationLayer();
	if( pTopLayer )
	{
		DEVASSERT( pTopLayer->m_Animation );

		// Notify listeners that the current animation was interrupted
		if( !pTopLayer->m_Stopped )
		{
			FOR_EACH_LIST( NotifyIter, m_AnimationListeners, SAnimationListener )
			{
				SAnimationListener& AnimationListener = *NotifyIter;
				if( AnimationListener.m_NotifyFinishedFunc )
				{
					AnimationListener.m_NotifyFinishedFunc( AnimationListener.m_Void, m_OwnerMesh, pTopLayer->m_Animation, true );
				}
			}
		}
	}

	// ROSATODO: If needed, blend to this pose (my current use case doesn't require it)
	// ROSATODO: Also clear layered/additive anims? I don't currently have a use case,
	// this was only ever used for lockpicking hand position blends in Vamp.
	{
		CHECK_ITERATING_LAYERS;
		m_AnimLayers.Clear();
	}

	// Add a new animation layer
	CHECK_ITERATING_LAYERS;
	SAnimationLayer& AnimLayer	= m_AnimLayers.PushBack();
	AnimLayer.m_Animation		= pAnimationA;
	AnimLayer.m_AnimationB		= pAnimationB;
	AnimLayer.m_BlendAlpha		= BlendAlpha;
}

const Animation* AnimationState::GetAnimation() const
{
	const SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer )
	{
		return pLayer->m_Animation;
	}
	else
	{
		return NULL;
	}
}

float AnimationState::GetAnimationTime() const
{
	const SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer )
	{
		return pLayer->m_Time;
	}
	else
	{
		return 0.0f;
	}
}

void AnimationState::SetAnimationTime( const float AnimationTime )
{
	SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer )
	{
		pLayer->m_Time = AnimationTime;
	}
}

float AnimationState::GetAnimationPlayRate() const
{
	const SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer )
	{
		return pLayer->m_PlayRate;
	}
	else
	{
		return 1.0f;
	}
}

void AnimationState::SetAnimationPlayRate( const float AnimationPlayRate )
{
	SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer )
	{
		pLayer->m_PlayRate = AnimationPlayRate;
	}
}

void AnimationState::SetAnimationBonesTickRate( const float AnimationBonesTickRate )
{
	m_AnimationBonesTickRate = AnimationBonesTickRate;
}

AnimationState::EAnimationEndBehavior AnimationState::GetAnimationEndBehavior() const
{
	const SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer )
	{
		return pLayer->m_EndBehavior;
	}
	else
	{
		return EAEB_Stop;
	}
}

void AnimationState::GetAnimationVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity )
{
	const SAnimationLayer* const pLayer = GetTopAnimationLayer();
	if( pLayer && !pLayer->m_Stopped )
	{
		DEVASSERT( pLayer->m_Animation );
		pLayer->m_Animation->GetVelocity( OutVelocity, OutRotationalVelocity );
	}
}

/*static*/ uint AnimationState::GetFrameFromTime( const Animation* pAnimation, const float AnimationTime )
{
	DEVASSERT( pAnimation );

	uint Frame = 0;
	if( pAnimation->m_AnimData.m_Length > 0 )
	{
		// HACKHACK: Resolve double precision to float first, else we get weirdness with numbers very close to integers.
		const float FloatFrame = static_cast<float>( AnimationTime * ANIM_FRAMERATE );
		Frame = static_cast<uint>( FloatFrame );
		DEVASSERT( Frame < pAnimation->GetLengthFrames() );
	}
	Frame += pAnimation->m_AnimData.m_StartFrame;
	return Frame;
}

/*static*/ uint AnimationState::GetNextFrame( const SAnimationLayer& AnimLayer, uint Frame )
{
	DEVASSERT( AnimLayer.m_Animation );

	if( AnimLayer.m_Animation->m_AnimData.m_Length > 0 )
	{
		Frame -= AnimLayer.m_Animation->m_AnimData.m_StartFrame;
		Frame++;

		if( Frame >= AnimLayer.m_Animation->m_AnimData.m_Length )
		{
			if( AnimLayer.m_EndBehavior == EAEB_Stop )
			{
				Frame--;	// Keep interpolating into the same frame (see also ::Tick())
			}
			else if( AnimLayer.m_EndBehavior == EAEB_Loop )
			{
				Frame %= AnimLayer.m_Animation->m_AnimData.m_Length;
			}
		}

		Frame += AnimLayer.m_Animation->m_AnimData.m_StartFrame;
	}

	return Frame;
}

/*static*/ float AnimationState::GetFrameDelta( const SAnimationLayer& AnimLayer )
{
	return Mod( AnimLayer.m_Time * ANIM_FRAMERATE, 1.0f );
}

void AnimationState::UpdateBones( const BoneArray* const pBones )
{
	// Deferred creation of bone matrices
	if( !m_BoneMatrices )
	{
		ASSERT( pBones->GetNumBones() <= MAX_BONE_MATRICES );
		m_BoneMatrices		= new Matrix[ pBones->GetNumBones() ];
		m_DirtyBoneMatrices	= true;
	}

	if( !m_DirtyBoneMatrices )
	{
		return;
	}

	m_DirtyBoneMatrices = false;

	const int	NumBones	= pBones->GetNumBones();
	float		BlendAlpha	= 0.0f;

	Array<SBone> UpdatedBones;
	UpdatedBones.Resize( NumBones );

	// Use rest pose if we don't have any base animations playing
	// (we could theoretically play layered or additives on top!)
	if( m_AnimLayers.Empty() )
	{
		for( int BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex )
		{
			const SBone&	RestPoseBone	= pBones->GetBone( BoneIndex, FRAME_TPOSE );
			UpdatedBones[ BoneIndex ]		= RestPoseBone;
		}
	}

	BEGIN_ITERATING_LAYERS;
	FOR_EACH_ARRAY( AnimLayerIter, m_AnimLayers, SAnimationLayer )
	{
		const SAnimationLayer&	AnimLayer		= AnimLayerIter.GetValue();
		const bool				BlendedPose		= ( AnimLayer.m_AnimationB != NULL );
		const bool				IsBottomLayer	= AnimLayerIter.GetIndex() == 0;
		const bool				IsTopLayer		= AnimLayerIter.GetIndex() == ( m_AnimLayers.Size() - 1 );
		const float				NextBlendAlpha	= 1.0f - ( IsTopLayer ? 0.0f : ( AnimLayer.m_BlendTimeLeft / AnimLayer.m_BlendTime ) );
		const uint				CurrentFrame	= GetFrameFromTime( AnimLayer.m_Animation, AnimLayer.m_Time );
		const uint				NextFrame		= BlendedPose ? GetFrameFromTime( AnimLayer.m_AnimationB, AnimLayer.m_Time ) : GetNextFrame( AnimLayer, CurrentFrame );
		const float				FrameDelta		= BlendedPose ? AnimLayer.m_BlendAlpha : GetFrameDelta( AnimLayer );

		for( int BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex )
		{
			SBone& UpdatedBone = UpdatedBones[ BoneIndex ];
			if( IsBottomLayer )
			{
				pBones->GetInterpolatedBone( BoneIndex, CurrentFrame, NextFrame, FrameDelta, UpdatedBone );
			}
			else
			{
				SBone InterpolatedBone;
				pBones->GetInterpolatedBone( BoneIndex, CurrentFrame, NextFrame, FrameDelta, InterpolatedBone );

				SBone BlendedBone;
				BoneArray::GetInterpolatedBone( UpdatedBone, InterpolatedBone, BlendAlpha, BlendedBone );

				UpdatedBone = BlendedBone;
			}
		}

		BlendAlpha = NextBlendAlpha;
	}
	END_ITERATING_LAYERS;

	Array<Matrix> BoneSpaceMatrices;
	BoneSpaceMatrices.Reserve( NumBones );

	for( int BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex )
	{
		const SBoneInfo&	BoneInfo			= pBones->GetBoneInfo( BoneIndex );
		const SBone&		RestPoseBone		= pBones->GetBone( BoneIndex, FRAME_TPOSE );
		SBone&				UpdatedBone			= UpdatedBones[ BoneIndex ];
		Matrix&				BoneMatrix			= m_BoneMatrices[ BoneIndex ];
		const Matrix&		ParentBoneMatrix	= ( BoneInfo.m_ParentIndex >= 0 ) ? BoneSpaceMatrices[ BoneInfo.m_ParentIndex ] : Matrix();	// Use the transform *without* the inverse bind pose for children

		// Apply layered animations before bone modifiers
		BEGIN_ITERATING_LAYERS;
		FOR_EACH_ARRAY( LayeredAnimLayerIter, m_LayeredAnimLayers, SAnimationLayer )
		{
			const SAnimationLayer&	LayeredAnimLayer	= LayeredAnimLayerIter.GetValue();
			const uint				CurrentFrame		= GetFrameFromTime( LayeredAnimLayer.m_Animation, LayeredAnimLayer.m_Time );
			const uint				NextFrame			= GetNextFrame( LayeredAnimLayer, CurrentFrame );
			const float				FrameDelta			= GetFrameDelta( LayeredAnimLayer );

			SBone					LayeredBone;
			pBones->GetInterpolatedBone( BoneIndex, CurrentFrame, NextFrame, FrameDelta, LayeredBone );

			if( RestPoseBone.m_Quat.Equals( LayeredBone.m_Quat, EPSILON ) &&
				RestPoseBone.m_Vector.Equals( LayeredBone.m_Vector, EPSILON ) )
			{
				// The layered animation does not change this bone, leave it alone.
				continue;
			}
			else
			{
				// Override the bone with the layered animation
				UpdatedBone = LayeredBone;
			}
		}
		END_ITERATING_LAYERS;

		// Modify bones in order, since they stack
		// (Previously, I modified all bone *matrices* at the end.)
		FOR_EACH_ARRAY( BoneModifierIter, m_BoneModifiers, IBoneModifier* )
		{
			IBoneModifier* const pBoneModifier = BoneModifierIter.GetValue();
			pBoneModifier->ModifyBone( BoneInfo, BoneIndex, ParentBoneMatrix, UpdatedBone );
		}

		// Apply additive animations after bone modifiers (so they stack on top
		// of headtracking, and I can make ragdoll bodies twitch or whatever)
		BEGIN_ITERATING_LAYERS;
		FOR_EACH_ARRAY( AdditiveAnimLayerIter, m_AdditiveAnimLayers, SAnimationLayer )
		{
			const SAnimationLayer&	AdditiveAnimLayer	= AdditiveAnimLayerIter.GetValue();
			const uint				CurrentFrame		= GetFrameFromTime( AdditiveAnimLayer.m_Animation, AdditiveAnimLayer.m_Time );
			const uint				NextFrame			= GetNextFrame( AdditiveAnimLayer, CurrentFrame );
			const float				FrameDelta			= GetFrameDelta( AdditiveAnimLayer );

			SBone					AdditiveBone;
			pBones->GetInterpolatedBone( BoneIndex, CurrentFrame, NextFrame, FrameDelta, AdditiveBone );

			BoneArray::GetAdditiveBone( UpdatedBone, RestPoseBone, AdditiveBone, UpdatedBone );
		}
		END_ITERATING_LAYERS;

		// NOTE: Setting the translation elements produces the same results in
		// the simple case as multiplying by a translation matrix, but is faster.
		BoneMatrix = UpdatedBone.m_Quat.ToMatrix();
		BoneMatrix.SetTranslationElements( UpdatedBone.m_Vector );

		BoneMatrix = BoneMatrix * ParentBoneMatrix;

		BoneSpaceMatrices.PushBack( BoneMatrix );
		BoneMatrix = BoneInfo.m_InvBindPose * BoneMatrix;
	}
}

void AnimationState::AddAnimationListener( const SAnimationListener& AnimationListener )
{
	m_AnimationListeners.PushBack( AnimationListener );
}

void AnimationState::RemoveAnimationListener( const SAnimationListener& AnimationListener )
{
	m_AnimationListeners.Remove( AnimationListener );
}

void AnimationState::AddBoneModifier( IBoneModifier* pBoneModifier )
{
	m_BoneModifiers.PushBack( pBoneModifier );
}

void AnimationState::CallAnimEvent( AnimEvent* const pAnimEvent, const SAnimationLayer& AnimLayer )
{
	pAnimEvent->Call( m_OwnerMesh, AnimLayer.m_Animation );

	// Notify any listeners about this event
	FOR_EACH_LIST( NotifyIter, m_AnimationListeners, SAnimationListener )
	{
		SAnimationListener& AnimationListener = *NotifyIter;
		if( AnimationListener.m_NotifyAnimEventFunc )
		{
			AnimationListener.m_NotifyAnimEventFunc( AnimationListener.m_Void, pAnimEvent, m_OwnerMesh, AnimLayer.m_Animation );
		}
	}
}
