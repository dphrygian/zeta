#include "core.h"
#include "wbactionrosareturntotitle.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "Common/uimanagercommon.h"

WBActionRosaReturnToTitle::WBActionRosaReturnToTitle()
{
}

WBActionRosaReturnToTitle::~WBActionRosaReturnToTitle()
{
}

/*virtual*/ void WBActionRosaReturnToTitle::Execute()
{
	WBAction::Execute();

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();

	pGame->Autosave();
	pGame->RequestReturnToTitle();
}
