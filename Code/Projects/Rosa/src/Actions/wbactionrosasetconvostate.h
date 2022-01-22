#ifndef WBACTIONROSASETCONVOSTATE_H
#define WBACTIONROSASETCONVOSTATE_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class WBActionRosaSetConvoState : public WBAction
{
public:
	WBActionRosaSetConvoState();
	virtual ~WBActionRosaSetConvoState();

	DEFINE_WBACTION_FACTORY( RosaSetConvoState );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	WBParamEvaluator	m_EntityPE;
	HashedString		m_State;
};

#endif // WBACTIONROSASETCONVOSTATE_H
