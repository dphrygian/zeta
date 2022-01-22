#include "core.h"
#include "rodinbtnoderosaplayanim.h"
#include "configmanager.h"
#include "Components/wbcomprodinbehaviortree.h"
#include "wbeventmanager.h"
#include "animation.h"
#include "idatastream.h"

RodinBTNodeRosaPlayAnim::RodinBTNodeRosaPlayAnim()
:	m_AnimationName( "" )
,	m_AnimationNamePE()
,	m_Loop( false )
,	m_PlayRate( 0.0f )
,	m_BlendTime( 0.0f )
,	m_Layered( false )
,	m_Additive( false )
{
}

RodinBTNodeRosaPlayAnim::~RodinBTNodeRosaPlayAnim()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnAnimationFinished );
		pEventManager->UncheckedRemoveObserver( sOnAnimationFinished, this, GetEntity() );
	}
}

void RodinBTNodeRosaPlayAnim::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Animation );
	m_AnimationName = ConfigManager::GetHash( sAnimation, HashedString::NullString, sDefinitionName );

	STATICHASH( AnimationPE );
	const SimpleString AnimationPE = ConfigManager::GetString( sAnimationPE, "", sDefinitionName );
	m_AnimationNamePE.InitializeFromDefinition( AnimationPE );

	STATICHASH( Loop );
	m_Loop = ConfigManager::GetBool( sLoop, false, sDefinitionName );

	STATICHASH( PlayRate );
	m_PlayRate = ConfigManager::GetFloat( sPlayRate, 0.0f, sDefinitionName );

	STATICHASH( BlendTime );
	m_BlendTime = ConfigManager::GetFloat( sBlendTime, 0.0f, sDefinitionName );

	STATICHASH( Layered );
	m_Layered = ConfigManager::GetBool( sLayered, false, sDefinitionName );

	STATICHASH( Additive );
	m_Additive = ConfigManager::GetBool( sAdditive, false, sDefinitionName );
}

RodinBTNode::ETickStatus RodinBTNodeRosaPlayAnim::Tick( const float DeltaTime )
{
	Unused( DeltaTime );
	return ETS_Success;
}

/*virtual*/ void RodinBTNodeRosaPlayAnim::OnStart()
{
	Super::OnStart();

	STATIC_HASHED_STRING( OnAnimationFinished );
	GetEventManager()->AddObserver( sOnAnimationFinished, this, GetEntity() );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_AnimationNamePE.Evaluate( PEContext );
	const HashedString AnimationName = ( m_AnimationNamePE.GetType() == WBParamEvaluator::EPT_String ) ? m_AnimationNamePE.GetString() : m_AnimationName;

	WB_MAKE_EVENT( PlayAnim, GetEntity() );
	WB_SET_AUTO( PlayAnim, Hash, AnimationName, AnimationName );
	WB_SET_AUTO( PlayAnim, Bool, Loop, m_Loop );
	WB_SET_AUTO( PlayAnim, Float, PlayRate, m_PlayRate );
	WB_SET_AUTO( PlayAnim, Float, BlendTime, m_BlendTime );
	WB_SET_AUTO( PlayAnim, Bool, Layered, m_Layered );
	WB_SET_AUTO( PlayAnim, Bool, Additive, m_Additive );
	WB_DISPATCH_EVENT( GetEventManager(), PlayAnim, GetEntity() );

	// NOTE: We just have to trust that the animation will actually start. No return value from event.
	// Go to sleep and wait for animation to complete.
	m_BehaviorTree->Sleep( this );
}

/*virtual*/ void RodinBTNodeRosaPlayAnim::OnFinish()
{
	Super::OnFinish();

	STATIC_HASHED_STRING( OnAnimationFinished );
	GetEventManager()->RemoveObserver( sOnAnimationFinished, this, GetEntity() );

	if( m_IsSleeping )
	{
		m_BehaviorTree->Wake( this );
	}
}

/*virtual*/ void RodinBTNodeRosaPlayAnim::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( OnAnimationFinished );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnAnimationFinished )
	{
		STATIC_HASHED_STRING( AnimationName );
		const HashedString AnimationName = Event.GetHash( sAnimationName );

		if( AnimationName == m_AnimationName )
		{
			if( m_IsSleeping )
			{
				m_BehaviorTree->Wake( this );
			}
		}
	}
}
