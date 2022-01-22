#include "core.h"
#include "wbactionrosapublishresults.h"

WBActionRosaPublishResults::WBActionRosaPublishResults()
{
}

WBActionRosaPublishResults::~WBActionRosaPublishResults()
{
}

/*virtual*/ void WBActionRosaPublishResults::Execute()
{
	WBAction::Execute();

	// ROSANOTE: This is now published through campaign!
}
