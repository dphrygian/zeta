#ifndef RODINBTNODECASTRESULT_H
#define RODINBTNODECASTRESULT_H

#include "rodinbtnodedecorator.h"
#include "wbparamevaluator.h"

class RodinBTNodeCastResult : public RodinBTNodeDecorator
{
public:
	RodinBTNodeCastResult();
	virtual ~RodinBTNodeCastResult();

	DEFINE_RODINBTNODE( CastResult, RodinBTNodeDecorator );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );

protected:
	WBParamEvaluator	m_ValuePE;	// Config
};

#endif // RODINBTNODECASTRESULT_H
