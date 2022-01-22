#include "core.h"
#include "wbactionrosaclearobjectives.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaClearObjectives::WBActionRosaClearObjectives()
{
}

WBActionRosaClearObjectives::~WBActionRosaClearObjectives()
{
}

/*virtual*/ void WBActionRosaClearObjectives::Execute()
{
	WBAction::Execute();

	WB_MAKE_EVENT( ClearObjectives, NULL );
	WB_DISPATCH_EVENT( GetEventManager(), ClearObjectives, RosaGame::GetPlayer() );
}
