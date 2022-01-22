#ifndef RODINBTNODESEQUENCE_H
#define RODINBTNODESEQUENCE_H

#include "rodinbtnodecompositesingular.h"

class RodinBTNodeSequence : public RodinBTNodeCompositeSingular
{
public:
	RodinBTNodeSequence();
	virtual ~RodinBTNodeSequence();

	DEFINE_RODINBTNODE( Sequence, RodinBTNodeCompositeSingular );

	virtual ETickStatus Tick( const float DeltaTime );
};

#endif // RODINBTNODESEQUENCE_H
