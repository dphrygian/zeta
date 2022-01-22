#ifndef RODINBTNODECOMPOSITE_H
#define RODINBTNODECOMPOSITE_H

#include "rodinbtnode.h"
#include "array.h"

class RodinBTNodeComposite : public RodinBTNode
{
public:
	RodinBTNodeComposite();
	virtual ~RodinBTNodeComposite();

	DEFINE_RODINBTNODE( Composite, RodinBTNode );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

protected:
	Array<RodinBTNode*>	m_Children;	// Config (would be serialized)
};

#endif // RODINBTNODECOMPOSITE_H
