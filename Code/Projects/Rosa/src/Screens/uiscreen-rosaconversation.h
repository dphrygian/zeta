#ifndef UISCREENROSACONVERSATION_H
#define UISCREENROSACONVERSATION_H

#include "uiscreen.h"

class UIScreenRosaConversation : public UIScreen
{
public:
	UIScreenRosaConversation();
	virtual ~UIScreenRosaConversation();

	DEFINE_UISCREEN_FACTORY( RosaConversation );

	virtual ETickReturn	TickInput();

private:
	void	SkipConversationLine();
	void	SelectConversationChoice( uint ChoiceIndex );
};

#endif // UISCREENROSACONVERSATION_H
