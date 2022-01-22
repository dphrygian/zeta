#ifndef ANIMATIONSTATE_H
#define ANIMATIONSTATE_H

#include "array.h"
#include "list.h"
#include "matrix.h"

class Animation;
class BoneArray;
class Mesh;
class AnimEvent;
class Vector;
class Angles;
class IBoneModifier;

typedef void ( *NotifyAnimationFinishedFunc )( void*, Mesh*, Animation*, bool );
typedef void ( *NotifyAnimEventFunc )( void*, AnimEvent*, Mesh*, Animation* );
struct SAnimationListener
{
	SAnimationListener()
	:	m_NotifyFinishedFunc( NULL )
	,	m_NotifyAnimEventFunc( NULL )
	,	m_Void( NULL )
	{
	}

	NotifyAnimationFinishedFunc	m_NotifyFinishedFunc;
	NotifyAnimEventFunc			m_NotifyAnimEventFunc;
	void*						m_Void;

	bool operator==( const SAnimationListener& Other )
	{
		return (
			m_NotifyFinishedFunc == Other.m_NotifyFinishedFunc &&
			m_NotifyAnimEventFunc == Other.m_NotifyAnimEventFunc &&
			m_Void == Other.m_Void );
	}
};

class AnimationState
{
public:
	enum EAnimationEndBehavior
	{
		EAEB_Stop,	// Stop on last frame
		EAEB_Loop,	// Loop from last frame back to first
	};

	struct SPlayAnimationParams
	{
		SPlayAnimationParams()
		:	m_EndBehavior( EAEB_Stop )
		,	m_IgnoreIfAlreadyPlaying( false )
		,	m_SuppressImmediateAnimEvents( false )
		,	m_PlayRate( 1.0f )
		,	m_BlendTime( 0.0f )
		,	m_Layered( false )
		,	m_Additive( false )
		{
		}

		EAnimationEndBehavior	m_EndBehavior;
		bool					m_IgnoreIfAlreadyPlaying;
		bool					m_SuppressImmediateAnimEvents;
		float					m_PlayRate;
		float					m_BlendTime;
		bool					m_Layered;
		bool					m_Additive;
	};

	AnimationState();
	~AnimationState();

	int						AddReference();
	int						Release();

	void					Tick( const float DeltaTime );
	void					PlayAnimation( Animation* pAnimation, const SPlayAnimationParams& PlayParams );
	void					SetAnimationBlend( Animation* const pAnimationA, Animation* const pAnimationB, const float BlendAlpha );

	const Animation*		GetAnimation() const;
	float					GetAnimationTime() const;
	void					SetAnimationTime( const float AnimationTime );
	float					GetAnimationPlayRate() const;
	void					SetAnimationPlayRate( const float AnimationPlayRate );
	void					SetAnimationBonesTickRate( const float AnimationBonesTickRate );
	EAnimationEndBehavior	GetAnimationEndBehavior() const;

	void					GetAnimationVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity );

	void					UpdateBones( const BoneArray* pBones );

	void					SetOwnerMesh( Mesh* const pMesh ) { m_OwnerMesh = pMesh; }
	Mesh*					GetOwnerMesh() const { return m_OwnerMesh; }

	void					SuppressAnimEvents( const bool Suppress ) { m_SuppressAnimEvents = Suppress; }

	void					AddAnimationListener( const SAnimationListener& AnimationListener );
	void					RemoveAnimationListener( const SAnimationListener& AnimationListener );

	bool					AreBonesUpdated() const { return !m_DirtyBoneMatrices; }
	const Matrix*			GetBoneMatrices() const { return m_BoneMatrices; }
	const float*			GetBoneMatrixFloats() const { return m_BoneMatrices[0].GetArray(); }

	void					AddBoneModifier( IBoneModifier* pBoneModifier );

	static bool				StaticGetStylizedAnim();
	static void				StaticSetStylizedAnim( const bool StylizedAnim );

private:
	struct SAnimationLayer
	{
		SAnimationLayer()
		:	m_Animation( NULL )
		,	m_AnimationB( NULL )
		,	m_PlayRate( 1.0f )
		,	m_Time( 0.0f )
		,	m_BlendTime( 0.0f )
		,	m_BlendTimeLeft( 0.0f )
		,	m_BlendAlpha( 0.0f )
		,	m_EndBehavior( EAEB_Stop )
		,	m_Stopped( false )
		,	m_Layered( false )
		,	m_Additive( false )
		{
		}

		Animation*				m_Animation;
		Animation*				m_AnimationB;	// HACKHACK for blends (Vamp lockpicking hands)
		float					m_PlayRate;
		float					m_Time;
		float					m_BlendTime;
		float					m_BlendTimeLeft;
		float					m_BlendAlpha;
		EAnimationEndBehavior	m_EndBehavior;
		bool					m_Stopped;
		bool					m_Layered;
		bool					m_Additive;
	};

	void					TickLayer( SAnimationLayer& AnimLayer, const float DeltaTime, const bool IsTopLayer );
	void					TickAnimEvents( const SAnimationLayer& AnimLayer, const float OldTime );

	const SAnimationLayer*	GetTopAnimationLayer() const	{ return m_AnimLayers.Empty() ? NULL : &m_AnimLayers.Last(); }
	SAnimationLayer*		GetTopAnimationLayer()			{ return m_AnimLayers.Empty() ? NULL : &m_AnimLayers.Last(); }
	static uint				GetFrameFromTime( const Animation* pAnimation, const float AnimationTime );
	static uint				GetNextFrame( const SAnimationLayer& AnimLayer, uint Frame );
	static float			GetFrameDelta( const SAnimationLayer& AnimLayer );

	void					CallAnimEvent( AnimEvent* const pAnimEvent, const SAnimationLayer& AnimLayer );

	int							m_RefCount;

	Array<SAnimationLayer>		m_AnimLayers;
	Array<SAnimationLayer>		m_LayeredAnimLayers;	// Ugh, naming. I should probably rename "layers" to "tracks" or whatever.
	Array<SAnimationLayer>		m_AdditiveAnimLayers;
	Mesh*						m_OwnerMesh;			// Which singular mesh is the state's owner; chooses when to tick this state, and is the context for callbacks

	bool						m_SuppressAnimEvents;

	List<SAnimationListener>	m_AnimationListeners;

#if BUILD_DEBUG
	mutable int					m_IteratingLayersRefCount;
#endif

	// ROSANOTE: Migrated these from Mesh so they can be shared between meshes instead of recomputing matrices
	Matrix*						m_BoneMatrices;
	bool						m_DirtyBoneMatrices;
	Array<IBoneModifier*>		m_BoneModifiers;

	// HACKHACK to try animating on twos (12 fps).
	// NOTE: This also means bone modifiers won't be called every tick; in some cases,
	// I'm treating ModifyBone as a bit of a tick, mutating the modifier's state. That's
	// not a great pattern, but I shouldn't try to hack around it here; there's other
	// cases where they also aren't called every tick, like when they're not rendered.
	float						m_AnimationBonesTickRate;
	float						m_NextBonesRefreshTime;

	static bool					sm_StylizedAnim;
};

#endif // ANIMATIONSTATE_H
