#ifndef RODINBTNODENULL_H
#define RODINBTNODENULL_H

#include "rodinbtnode.h"

class RodinBTNodeNull : public RodinBTNode
{
public:
	RodinBTNodeNull();
	virtual ~RodinBTNodeNull();

	DEFINE_RODINBTNODE( Null, RodinBTNode );

	virtual ETickStatus Tick( const float DeltaTime );
};

#endif // RODINBTNODENULL_H
