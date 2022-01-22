#ifndef WBACTIONROSAADDAMMO_H
#define WBACTIONROSAADDAMMO_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaAddAmmo : public WBAction
{
public:
	WBActionRosaAddAmmo();
	virtual ~WBActionRosaAddAmmo();

	DEFINE_WBACTION_FACTORY( RosaAddAmmo );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	HashedString		m_Type;
	WBParamEvaluator	m_TypePE;
	uint				m_Count;
	WBParamEvaluator	m_CountPE;
	bool				m_SuppressLog;
};

#endif // WBACTIONROSAADDAMMO_H
