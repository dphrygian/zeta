#include "core.h"
#include "wbactionrosastopmotion.h"
#include "wbactionstack.h"
#include "Components/wbcomprosatransform.h"
#include "wbevent.h"
#include "angles.h"

WBActionRosaStopMotion::WBActionRosaStopMotion()
{
}

WBActionRosaStopMotion::~WBActionRosaStopMotion()
{
}

/*virtual*/ void WBActionRosaStopMotion::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();

	if( pEntity )
	{
		WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
		if( pTransform )
		{
			pTransform->SetGravity( 0.0f );
			pTransform->SetVelocity( Vector() );
			pTransform->SetAcceleration( Vector() );
			pTransform->SetRotationalVelocity( Angles() );
			pTransform->SetCanMove( false );
		}
	}
}
