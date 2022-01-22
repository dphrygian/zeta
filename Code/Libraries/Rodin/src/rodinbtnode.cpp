#include "core.h"
#include "rodinbtnode.h"
#include "simplestring.h"
#include "Components/wbcomprodinbehaviortree.h"
#include "wbworld.h"
#include "idatastream.h"

RodinBTNode::RodinBTNode()
:	m_BehaviorTree( NULL )
,	m_IsSleeping( false )
#if BUILD_DEV
,	m_DEV_Name()
,	m_DEV_Depth( 0 )
,	m_DEV_CollapseDebug( false )
#endif
{
}

RodinBTNode::~RodinBTNode()
{
}

void RodinBTNode::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Unused( DefinitionName );
}

RodinBTNode::ETickStatus RodinBTNode::Tick( const float DeltaTime )
{
	Unused( DeltaTime );
	return ETS_None;
}

void RodinBTNode::OnStart()
{
}

void RodinBTNode::OnFinish()
{
}

void RodinBTNode::OnChildCompleted( RodinBTNode* pChildNode, ETickStatus TickStatus )
{
	Unused( pChildNode );
	Unused( TickStatus );
}

#if BUILD_DEV
void RodinBTNode::Report()
{
	PRINTF( WBPROPERTY_REPORT_PREFIX WB_REPORT_SPACER );
	for( uint i = 0; i < m_DEV_Depth; ++i )
	{
		PRINTF( WB_REPORT_SPACER );
	}
	PRINTF( "%s\n", GetDebugName().CStr() );
}
#endif

WBEntity* RodinBTNode::GetEntity() const
{
	DEVASSERT( m_BehaviorTree );
	return m_BehaviorTree->GetEntity();
}

float RodinBTNode::GetTime() const
{
	return WBWorld::GetInstance()->GetTime();
}

WBEventManager* RodinBTNode::GetEventManager() const
{
	return WBWorld::GetInstance()->GetEventManager();
}
