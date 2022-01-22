#ifndef WBACTIONROSAPLAYBARK_H
#define WBACTIONROSAPLAYBARK_H

#include "wbaction.h"
#include "wbparamevaluator.h"
#include "simplestring.h"

class WBActionRosaPlayBark : public WBAction
{
public:
	WBActionRosaPlayBark();
	virtual ~WBActionRosaPlayBark();

	DEFINE_WBACTION_FACTORY( RosaPlayBark );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString		m_SoundDef;
	WBParamEvaluator	m_SoundDefPE;
	HashedString		m_Category;
};

#endif // WBACTIONROSAPLAYBARK_H
