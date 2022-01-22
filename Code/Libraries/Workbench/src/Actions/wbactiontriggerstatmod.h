#ifndef WBACTIONTRIGGERSTATMOD_H
#define WBACTIONTRIGGERSTATMOD_H

#include "wbaction.h"

class WBActionTriggerStatMod : public WBAction
{
public:
	WBActionTriggerStatMod();
	virtual ~WBActionTriggerStatMod();

	DEFINE_WBACTION_FACTORY( TriggerStatMod );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	HashedString	m_StatModEvent;
	bool			m_Trigger;	// If false, untrigger (defaults to true)
	bool			m_RefCount;	// If true, use refcounting instead of forcing trigger/untrigger
};

#endif // WBACTIONTRIGGERSTATMOD_H
