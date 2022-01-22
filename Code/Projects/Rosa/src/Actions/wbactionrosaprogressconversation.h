#ifndef WBACTIONROSAPROGRESSCONVERSATION_H
#define WBACTIONROSAPROGRESSCONVERSATION_H

#include "wbaction.h"

class WBActionRosaProgressConversation : public WBAction
{
public:
	WBActionRosaProgressConversation();
	virtual ~WBActionRosaProgressConversation();

	DEFINE_WBACTION_FACTORY( RosaProgressConversation );

	virtual void	Execute();
};

#endif // WBACTIONROSAPROGRESSCONVERSATION_H
