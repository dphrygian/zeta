#ifndef RODINBTNODESLEEP_H
#define RODINBTNODESLEEP_H

#include "rodinbtnode.h"

class RodinBTNodeSleep : public RodinBTNode
{
public:
	RodinBTNodeSleep();
	virtual ~RodinBTNodeSleep();

	DEFINE_RODINBTNODE( Sleep, RodinBTNode );

	virtual ETickStatus Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();
};

#endif // RODINBTNODESLEEP_H
