#include "core.h"
#include "wbactionrosaflushworldfiles.h"
#include "wbeventmanager.h"
#include "rosaframework.h"
#include "rosagame.h"

WBActionRosaFlushWorldFiles::WBActionRosaFlushWorldFiles()
{
}

WBActionRosaFlushWorldFiles::~WBActionRosaFlushWorldFiles()
{
}

/*virtual*/ void WBActionRosaFlushWorldFiles::Execute()
{
	WBAction::Execute();

	WB_MAKE_EVENT( FlushWorldFiles, NULL );
	WB_DISPATCH_EVENT( GetEventManager(), FlushWorldFiles, RosaFramework::GetInstance()->GetGame() );
}
