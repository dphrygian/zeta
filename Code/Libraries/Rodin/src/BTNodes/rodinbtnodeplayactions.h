#ifndef RODINBTNODEPLAYACTIONS_H
#define RODINBTNODEPLAYACTIONS_H

#include "rodinbtnode.h"
#include "array.h"

class WBAction;

class RodinBTNodePlayActions : public RodinBTNode
{
public:
	RodinBTNodePlayActions();
	virtual ~RodinBTNodePlayActions();

	DEFINE_RODINBTNODE( PlayActions, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );

private:
	Array<WBAction*>	m_Actions;	// Config
};

#endif // RODINBTNODEPLAYACTIONS_H
