#ifndef RODINBTNODEROSALOOKAT_H
#define RODINBTNODEROSALOOKAT_H

#include "rodinbtnode.h"

class RodinBTNodeRosaLookAt : public RodinBTNode
{
public:
	RodinBTNodeRosaLookAt();
	virtual ~RodinBTNodeRosaLookAt();

	DEFINE_RODINBTNODE( RosaLookAt, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );

private:
	HashedString	m_LookTargetBlackboardKey;	// Config
};

#endif // RODINBTNODEROSALOOKAT_H
