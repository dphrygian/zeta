#ifndef WBACTIONROSACOMPLETEOBJECTIVE_H
#define WBACTIONROSACOMPLETEOBJECTIVE_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaCompleteObjective : public WBAction
{
public:
	WBActionRosaCompleteObjective();
	virtual ~WBActionRosaCompleteObjective();

	DEFINE_WBACTION_FACTORY( RosaCompleteObjective );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString	m_ObjectiveTag;
	bool			m_Fail;		// Success is the default behavior
	bool			m_ForceAdd;	// If true, add this objective if it didn't exist (this was *always* the behavior in Neon)
};

#endif // WBACTIONROSACOMPLETEOBJECTIVE_H
