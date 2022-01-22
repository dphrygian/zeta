#include "core.h"
#include "rodinbtnodenull.h"

RodinBTNodeNull::RodinBTNodeNull()
{
}

RodinBTNodeNull::~RodinBTNodeNull()
{
}

RodinBTNode::ETickStatus RodinBTNodeNull::Tick( const float DeltaTime )
{
	Unused( DeltaTime );
	return ETS_Success;
}
