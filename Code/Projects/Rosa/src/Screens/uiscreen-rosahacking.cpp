#include "core.h"
#include "uiscreen-rosahacking.h"
#include "wbeventmanager.h"
#include "wbworld.h"

UIScreenRosaHacking::UIScreenRosaHacking()
{
}

UIScreenRosaHacking::~UIScreenRosaHacking()
{
}

// Whenever this screen is closed, end hacking! Usually, EndHacking would close
// this screen, but loading a game during a hack will do this, for example.
/*virtual*/ void UIScreenRosaHacking::Popped()
{
	UIScreen::Popped();

	WB_MAKE_EVENT( StopHack, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), StopHack, NULL );
}
