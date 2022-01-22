#ifndef WBACTIONROSASELECTCONVERSATIONCHOICE_H
#define WBACTIONROSASELECTCONVERSATIONCHOICE_H

#include "wbaction.h"

class SimpleString;

class WBActionRosaSelectConversationChoice : public WBAction
{
public:
	WBActionRosaSelectConversationChoice();
	virtual ~WBActionRosaSelectConversationChoice();

	DEFINE_WBACTION_FACTORY( RosaSelectConversationChoice );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	uint	m_ChoiceIndex;
};

#endif // WBACTIONROSASELECTCONVERSATIONCHOICE_H
