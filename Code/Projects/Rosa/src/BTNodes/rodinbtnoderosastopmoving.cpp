#include "core.h"
#include "rodinbtnoderosastopmoving.h"
#include "Components/wbcomprosaaimotion.h"

RodinBTNodeRosaStopMoving::RodinBTNodeRosaStopMoving()
{
}

RodinBTNodeRosaStopMoving::~RodinBTNodeRosaStopMoving()
{
}

RodinBTNode::ETickStatus RodinBTNodeRosaStopMoving::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	WBEntity* const		pEntity		= GetEntity();
	WBCompRosaAIMotion*	pAIMotion	= WB_GETCOMP( pEntity, RosaAIMotion );
	ASSERT( pAIMotion );

	pAIMotion->StopMove();

	return ETS_Success;
}
