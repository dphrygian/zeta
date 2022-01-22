#include "core.h"
#include "wbactionrosaprogressconversation.h"
#include "rosaframework.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaProgressConversation::WBActionRosaProgressConversation()
{
}

WBActionRosaProgressConversation::~WBActionRosaProgressConversation()
{
}

/*virtual*/ void WBActionRosaProgressConversation::Execute()
{
	WBAction::Execute();

	RosaGame* const		pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( ProgressConversation, NULL );
	WB_DISPATCH_EVENT( pEventManager, ProgressConversation, pGame );
}
