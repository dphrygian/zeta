#ifndef RODINBTNODECONDITIONTIMEOUT_H
#define RODINBTNODECONDITIONTIMEOUT_H

#include "rodinbtnode.h"
#include "wbparamevaluator.h"

class RodinBTNodeConditionTimeout : public RodinBTNode
{
public:
	RodinBTNodeConditionTimeout();
	virtual ~RodinBTNodeConditionTimeout();

	DEFINE_RODINBTNODE( ConditionTimeout, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );

protected:
	WBParamEvaluator	m_TimeoutPE;			// Config
	HashedString		m_BlackboardKey;		// Config, optional, for shared timeouts across tree
	float				m_NextCanExecuteTime;	// Transient (would be serialized)
};

#endif // RODINBTNODETIMEOUT_H
