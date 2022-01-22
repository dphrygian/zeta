#ifndef WBACTIONSETSTATE_H
#define WBACTIONSETSTATE_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class WBActionSetState : public WBAction
{
public:
	WBActionSetState();
	virtual ~WBActionSetState();

	DEFINE_WBACTION_FACTORY( SetState );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	WBParamEvaluator	m_EntityPE;
	HashedString		m_State;
};

#endif // WBACTIONSETSTATE_H
