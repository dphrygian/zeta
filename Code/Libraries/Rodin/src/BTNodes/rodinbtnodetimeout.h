#ifndef RODINBTNODETIMEOUT_H
#define RODINBTNODETIMEOUT_H

#include "rodinbtnodedecorator.h"
#include "wbparamevaluator.h"

class RodinBTNodeTimeout : public RodinBTNodeDecorator
{
public:
	RodinBTNodeTimeout();
	virtual ~RodinBTNodeTimeout();

	DEFINE_RODINBTNODE( Timeout, RodinBTNodeDecorator );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );
	virtual void		OnChildCompleted( RodinBTNode* pChildNode, ETickStatus TickStatus );

protected:
	WBParamEvaluator	m_TimeoutPE;				// Config
	HashedString		m_BlackboardKey;			// Config, optional, for shared timeouts across tree
	bool				m_IgnoreIfFailed;			// Config, HACKHACK so timeout doesn't apply if child behavior is interrupted
	float				m_NextCanExecuteTime;		// Transient (would be serialized)
	float				m_OldNextCanExecuteTime;	// Transient (would be serialized), to restore the old time if m_IgnoreIfFailed
};

#endif // RODINBTNODETIMEOUT_H
