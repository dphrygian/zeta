#include "core.h"
#include "wbactionrosaopenuserdatapath.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "wbeventmanager.h"

WBActionRosaOpenUserDataPath::WBActionRosaOpenUserDataPath()
{
}

WBActionRosaOpenUserDataPath::~WBActionRosaOpenUserDataPath()
{
}

/*virtual*/ void WBActionRosaOpenUserDataPath::Execute()
{
	WBAction::Execute();

	RosaGame* const		pGame				= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( OpenUserDataPath, NULL );
	WB_LOG_EVENT( OpenUserDataPath );
	WB_DISPATCH_EVENT( pEventManager, OpenUserDataPath, pGame );
}
