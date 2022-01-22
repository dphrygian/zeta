#ifndef RODINBTNODEDECORATOR_H
#define RODINBTNODEDECORATOR_H

#include "rodinbtnode.h"

class RodinBTNodeDecorator : public RodinBTNode
{
public:
	RodinBTNodeDecorator();
	virtual ~RodinBTNodeDecorator();

	DEFINE_RODINBTNODE( Decorator, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();
	virtual void		OnChildCompleted( RodinBTNode* pChildNode, ETickStatus TickStatus );

#if BUILD_DEV
	virtual void		Report();
#endif

protected:
	RodinBTNode*	m_Child;		// Config (would be serialized)
	ETickStatus		m_ChildStatus;	// Transient (would be serialized)
};

#endif // RODINBTNODEDECORATOR_H
