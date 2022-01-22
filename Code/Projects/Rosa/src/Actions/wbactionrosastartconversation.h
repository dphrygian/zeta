#ifndef WBACTIONROSASTARTCONVERSATION_H
#define WBACTIONROSASTARTCONVERSATION_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaStartConversation : public WBAction
{
public:
	WBActionRosaStartConversation();
	virtual ~WBActionRosaStartConversation();

	DEFINE_WBACTION_FACTORY( RosaStartConversation );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString		m_Convo;
	WBParamEvaluator	m_ConvoPE;
};

#endif // WBACTIONROSASTARTCONVERSATION_H
