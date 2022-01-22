#ifndef WBACTIONROSAADDOBJECTIVE_H
#define WBACTIONROSAADDOBJECTIVE_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaAddObjective : public WBAction
{
public:
	WBActionRosaAddObjective();
	virtual ~WBActionRosaAddObjective();

	DEFINE_WBACTION_FACTORY( RosaAddObjective );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString	m_ObjectiveTag;
};

#endif // WBACTIONROSAADDOBJECTIVE_H
