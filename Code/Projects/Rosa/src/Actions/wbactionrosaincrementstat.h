#ifndef WBACTIONROSAINCREMENTSTAT_H
#define WBACTIONROSAINCREMENTSTAT_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaIncrementStat : public WBAction
{
public:
	WBActionRosaIncrementStat();
	virtual ~WBActionRosaIncrementStat();

	DEFINE_WBACTION_FACTORY( RosaIncrementStat );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString		m_StatTag;
	uint				m_Amount;
	WBParamEvaluator	m_AmountPE;
};

#endif // WBACTIONROSAINCREMENTSTAT_H
