#ifndef WBACTIONROSAREMOVEMONEY_H
#define WBACTIONROSAREMOVEMONEY_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class WBActionRosaRemoveMoney : public WBAction
{
public:
	WBActionRosaRemoveMoney();
	virtual ~WBActionRosaRemoveMoney();

	DEFINE_WBACTION_FACTORY( RosaRemoveMoney );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	uint				m_Amount;
	WBParamEvaluator	m_AmountPE;
};

#endif // WBACTIONROSAREMOVEMONEY_H
