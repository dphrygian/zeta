#ifndef WBACTIONLOG_H
#define WBACTIONLOG_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionLog : public WBAction
{
public:
	WBActionLog();
	virtual ~WBActionLog();

	DEFINE_WBACTION_FACTORY( Log );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	SimpleString		m_Text;
	WBParamEvaluator	m_TextPE;

	bool				m_DevOnly;
};

#endif // WBACTIONLOG_H
