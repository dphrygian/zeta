#include "core.h"
#include "wbactionrosafootstep.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "Components/wbcomprosatransform.h"
#include "rosaframework.h"
#include "rosaworld.h"

WBActionRosaFootstep::WBActionRosaFootstep()
{
}

WBActionRosaFootstep::~WBActionRosaFootstep()
{
}

/*virtual*/ void WBActionRosaFootstep::Execute()
{
	WBAction::Execute();

	STATIC_HASHED_STRING( Location );
	const Vector			Location		= WBActionStack::TopEvent().GetVector( sLocation );
	WBEventManager* const	pEventManager	= GetEventManager();
	RosaWorld* const		pWorld			= RosaFramework::GetInstance()->GetWorld();
	WBEntity* const			pEntity			= GetEntity();
	const HashedString		FootstepSurface	= pWorld->GetSurfaceBelowPoint( Location, pEntity );

	WB_MAKE_EVENT( OnFootstep, pEntity );
	WB_SET_AUTO( OnFootstep, Hash, FootstepSurface, FootstepSurface );
	WB_DISPATCH_EVENT( pEventManager, OnFootstep, pEntity );
}
