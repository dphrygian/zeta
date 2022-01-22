#ifndef WBACTIONROSASETLANGUAGE_H
#define WBACTIONROSASETLANGUAGE_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaSetLanguage : public WBAction
{
public:
	WBActionRosaSetLanguage();
	virtual ~WBActionRosaSetLanguage();

	DEFINE_WBACTION_FACTORY( RosaSetLanguage );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString	m_Language;
};

#endif // WBACTIONROSASETLANGUAGE_H
