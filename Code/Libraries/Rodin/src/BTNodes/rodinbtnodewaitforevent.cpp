#include "core.h"
#include "rodinbtnodewaitforevent.h"
#include "wbpatternmatching.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "Components/wbcomprodinbehaviortree.h"

RodinBTNodeWaitForEvent::RodinBTNodeWaitForEvent()
:	m_Rule()
{
}

RodinBTNodeWaitForEvent::~RodinBTNodeWaitForEvent()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		pEventManager->UncheckedRemoveObserver( m_Rule.GetEvent(), this, GetEntity() );
	}
}

void RodinBTNodeWaitForEvent::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Rule );
	const SimpleString Rule = ConfigManager::GetString( sRule, "", sDefinitionName );

	m_Rule.InitializeFromDefinition( Rule );
}

RodinBTNode::ETickStatus RodinBTNodeWaitForEvent::Tick( const float DeltaTime )
{
	Unused( DeltaTime );
	return ETS_Success;
}

void RodinBTNodeWaitForEvent::OnStart()
{
	Super::OnStart();

	// Currently, I'm only listening for events on this entity.
	GetEventManager()->AddObserver( m_Rule.GetEvent(), this, GetEntity() );

	m_BehaviorTree->Sleep( this );
}

void RodinBTNodeWaitForEvent::OnFinish()
{
	Super::OnFinish();

	// Currently, I'm only listening for events on this entity.
	GetEventManager()->RemoveObserver( m_Rule.GetEvent(), this, GetEntity() );

	if( m_IsSleeping )
	{
		m_BehaviorTree->Wake( this );
	}
}

/*virtual*/ void RodinBTNodeWaitForEvent::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	// Create a context for pattern matching rules to evaluate their PEs.
	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	if( WBPatternMatching::Compare( m_Rule, Event, PEContext ) )
	{
		if( m_IsSleeping )
		{
			m_BehaviorTree->Wake( this );
		}
	}
}
