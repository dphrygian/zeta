#ifndef WBACTIONROSAADDKEYCARD_H
#define WBACTIONROSAADDKEYCARD_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaAddKeycard : public WBAction
{
public:
	WBActionRosaAddKeycard();
	virtual ~WBActionRosaAddKeycard();

	DEFINE_WBACTION_FACTORY( RosaAddKeycard );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	HashedString		m_Keycard;
	WBParamEvaluator	m_KeycardPE;
	bool				m_SuppressLog;
};

#endif // WBACTIONROSAADDKEYCARD_H
