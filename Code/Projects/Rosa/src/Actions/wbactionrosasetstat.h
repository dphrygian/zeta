#ifndef WBACTIONROSASETSTAT_H
#define WBACTIONROSASETSTAT_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaSetStat : public WBAction
{
public:
	WBActionRosaSetStat();
	virtual ~WBActionRosaSetStat();

	DEFINE_WBACTION_FACTORY( RosaSetStat );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString		m_StatTag;
	uint				m_Value;
	WBParamEvaluator	m_ValuePE;
};

#endif // WBACTIONROSASETSTAT_H
