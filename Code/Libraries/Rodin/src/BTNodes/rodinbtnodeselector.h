#ifndef RODINBTNODESELECTOR_H
#define RODINBTNODESELECTOR_H

#include "rodinbtnodecompositesingular.h"

class RodinBTNodeSelector : public RodinBTNodeCompositeSingular
{
public:
	RodinBTNodeSelector();
	virtual ~RodinBTNodeSelector();

	DEFINE_RODINBTNODE( Selector, RodinBTNodeCompositeSingular );

	virtual ETickStatus Tick( const float DeltaTime );
};

#endif // RODINBTNODESELECTOR_H
