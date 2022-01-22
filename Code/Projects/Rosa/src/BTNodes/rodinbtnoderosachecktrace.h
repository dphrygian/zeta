#ifndef RODINBTNODEROSACHECKTRACE_H
#define RODINBTNODEROSACHECKTRACE_H

#include "rodinbtnode.h"
#include "wbparamevaluator.h"

class RodinBTNodeRosaCheckTrace : public RodinBTNode
{
public:
	RodinBTNodeRosaCheckTrace();
	virtual ~RodinBTNodeRosaCheckTrace();

	DEFINE_RODINBTNODE( RosaCheckTrace, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );

private:
	WBParamEvaluator	m_StartPE;
	WBParamEvaluator	m_EndPE;
	WBParamEvaluator	m_ExtentsPE;
	WBParamEvaluator	m_TargetPE;
};

#endif // RODINBTNODEROSACHECKTRACE_H
