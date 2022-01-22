#ifndef WBACTIONROSAREMOVEKEYCARD_H
#define WBACTIONROSAREMOVEKEYCARD_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaRemoveKeycard : public WBAction
{
public:
	WBActionRosaRemoveKeycard();
	virtual ~WBActionRosaRemoveKeycard();

	DEFINE_WBACTION_FACTORY( RosaRemoveKeycard );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	HashedString		m_Keycard;
	WBParamEvaluator	m_KeycardPE;
};

#endif // WBACTIONROSAREMOVEKEYCARD_H
