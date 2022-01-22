#ifndef RODINBTNODEROSASTOPMOVING_H
#define RODINBTNODEROSASTOPMOVING_H

#include "rodinbtnode.h"

class RodinBTNodeRosaStopMoving : public RodinBTNode
{
public:
	RodinBTNodeRosaStopMoving();
	virtual ~RodinBTNodeRosaStopMoving();

	DEFINE_RODINBTNODE( RosaStopMoving, RodinBTNode );

	virtual ETickStatus	Tick( const float DeltaTime );
};

#endif // RODINBTNODEROSASTOPMOVING_H
