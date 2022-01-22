#include "core.h"
#include "rodinbtnodeplayactions.h"
#include "wbaction.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "wbevent.h"

RodinBTNodePlayActions::RodinBTNodePlayActions()
:	m_Actions()
{
}

RodinBTNodePlayActions::~RodinBTNodePlayActions()
{
	WBActionFactory::ClearActionArray( m_Actions );
}

void RodinBTNodePlayActions::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBActionFactory::InitializeActionArray( DefinitionName, m_Actions );
}

RodinBTNode::ETickStatus RodinBTNodePlayActions::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	WBEntity* const pEntity = GetEntity();
	WB_MAKE_EVENT( BTNodePlayActionsEvent, pEntity );

	WBActionFactory::ExecuteActionArray( m_Actions, WB_AUTO_EVENT( BTNodePlayActionsEvent ), pEntity );

	return ETS_Success;
}
