#ifndef WBACTIONROSALOGMESSAGE_H
#define WBACTIONROSALOGMESSAGE_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaLogMessage : public WBAction
{
public:
	WBActionRosaLogMessage();
	virtual ~WBActionRosaLogMessage();

	DEFINE_WBACTION_FACTORY( RosaLogMessage );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString		m_String;	// A string table tag, not the literal string
	WBParamEvaluator	m_StringPE;
	bool				m_IsDynamic;
};

#endif // WBACTIONROSASTARTCONVERSATION_H
