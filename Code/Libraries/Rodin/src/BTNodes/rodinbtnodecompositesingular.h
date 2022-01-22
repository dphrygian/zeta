#ifndef RODINBTNODECOMPOSITESINGULAR_H
#define RODINBTNODECOMPOSITESINGULAR_H

#include "rodinbtnodecomposite.h"

class RodinBTNodeCompositeSingular : public RodinBTNodeComposite
{
public:
	RodinBTNodeCompositeSingular();
	virtual ~RodinBTNodeCompositeSingular();

	DEFINE_RODINBTNODE( CompositeSingular, RodinBTNodeComposite );

	virtual void	OnStart();
	virtual void	OnFinish();
	virtual void	OnChildCompleted( RodinBTNode* pChildNode, ETickStatus TickStatus );

#if BUILD_DEV
	virtual void	Report();
#endif

protected:
	uint		m_ChildIndex;	// Transient (would be serialized)
	ETickStatus m_ChildStatus;	// Transient (would be serialized)
};

#endif // RODINBTNODECOMPOSITESINGULAR_H
