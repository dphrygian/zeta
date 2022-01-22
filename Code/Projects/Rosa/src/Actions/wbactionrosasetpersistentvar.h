#ifndef WBACTIONROSASETPERSISTENTVAR_H
#define WBACTIONROSASETPERSISTENTVAR_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaSetPersistentVar : public WBAction
{
public:
	WBActionRosaSetPersistentVar();
	virtual ~WBActionRosaSetPersistentVar();

	DEFINE_WBACTION_FACTORY( RosaSetPersistentVar );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString		m_Key;
	WBParamEvaluator	m_ValuePE;
};

#endif // WBACTIONROSASETPERSISTENTVAR_H
