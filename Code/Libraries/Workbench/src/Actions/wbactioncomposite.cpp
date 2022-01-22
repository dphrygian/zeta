#include "core.h"
#include "wbactioncomposite.h"
#include "wbactionfactory.h"

WBActionComposite::WBActionComposite()
:	m_Actions()
{
}

WBActionComposite::~WBActionComposite()
{
	WBActionFactory::ClearActionArray( m_Actions );
}

/*virtual*/ void WBActionComposite::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	WBActionFactory::InitializeActionArray( DefinitionName, m_Actions );
}

/*virtual*/ void WBActionComposite::Execute()
{
	WBAction::Execute();

	const uint NumActions = m_Actions.Size();
	for( uint ActionIndex = 0; ActionIndex < NumActions; ++ActionIndex )
	{
		WBAction* const pAction = m_Actions[ ActionIndex ];
		ASSERT( pAction );

		pAction->Execute();
	}
}
