#ifndef RODINBTNODELOG_H
#define RODINBTNODELOG_H

#include "rodinbtnode.h"
#include "simplestring.h"

class RodinBTNodeLog : public RodinBTNode
{
public:
	RodinBTNodeLog();
	virtual ~RodinBTNodeLog();

	DEFINE_RODINBTNODE( Log, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus Tick( const float DeltaTime );

private:
	SimpleString	m_Text;			// Config
};

#endif // RODINBTNODELOG_H
