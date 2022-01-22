#include "core.h"
#include "rodinbtnodeactiondecorator.h"
#include "wbaction.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "wbevent.h"
#include "configmanager.h"

RodinBTNodeActionDecorator::RodinBTNodeActionDecorator()
:	m_StartActions()
,	m_FinishActions()
{
}

RodinBTNodeActionDecorator::~RodinBTNodeActionDecorator()
{
	WBActionFactory::ClearActionArray( m_StartActions );
	WBActionFactory::ClearActionArray( m_FinishActions );
}

void RodinBTNodeActionDecorator::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	RodinBTNodeDecorator::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	WBActionFactory::InitializeActionArray( sDefinitionName, "Start", m_StartActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "Finish", m_FinishActions );
}

/*virtual*/ void RodinBTNodeActionDecorator::OnStart()
{
	RodinBTNodeDecorator::OnStart();

	WBEntity* const pEntity = GetEntity();
	WB_MAKE_EVENT( BTNodeActionDecoratorStart, pEntity );

	WBActionFactory::ExecuteActionArray( m_StartActions, WB_AUTO_EVENT( BTNodeActionDecoratorStart ), pEntity );
}

/*virtual*/ void RodinBTNodeActionDecorator::OnFinish()
{
	RodinBTNodeDecorator::OnFinish();

	WBEntity* const pEntity = GetEntity();
	WB_MAKE_EVENT( BTNodeActionDecoratorFinish, pEntity );

	WBActionFactory::ExecuteActionArray( m_FinishActions, WB_AUTO_EVENT( BTNodeActionDecoratorFinish ), pEntity );
}
