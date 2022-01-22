#ifndef RODINBTNODELOOP_H
#define RODINBTNODELOOP_H

#include "rodinbtnodedecorator.h"

class RodinBTNodeLoop : public RodinBTNodeDecorator
{
public:
	RodinBTNodeLoop();
	virtual ~RodinBTNodeLoop();

	DEFINE_RODINBTNODE( Loop, RodinBTNodeDecorator );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus Tick( const float DeltaTime );

protected:
	bool				m_CanFail;		// Config
	bool				m_CanSucceed;	// Config
	float				m_LastTickTime;	// Transient (would be serialized)
};

#endif // RODINBTNODELOOP_H
