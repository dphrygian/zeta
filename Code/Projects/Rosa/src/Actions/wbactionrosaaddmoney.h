#ifndef WBACTIONROSAADDMONEY_H
#define WBACTIONROSAADDMONEY_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class WBActionRosaAddMoney : public WBAction
{
public:
	WBActionRosaAddMoney();
	virtual ~WBActionRosaAddMoney();

	DEFINE_WBACTION_FACTORY( RosaAddMoney );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	uint				m_Amount;
	WBParamEvaluator	m_AmountPE;
	bool				m_ShowLogMessage;
};

#endif // WBACTIONROSAADDMONEY_H
