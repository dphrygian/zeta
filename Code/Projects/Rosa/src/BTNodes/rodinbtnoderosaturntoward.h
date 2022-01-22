#ifndef RODINBTNODEROSATURNTOWARD_H
#define RODINBTNODEROSATURNTOWARD_H

#include "rodinbtnode.h"

class RodinBTNodeRosaTurnToward : public RodinBTNode
{
public:
	RodinBTNodeRosaTurnToward();
	virtual ~RodinBTNodeRosaTurnToward();

	DEFINE_RODINBTNODE( RosaTurnToward, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();

private:
	enum ETurnTowardState
	{
		ETTS_Begin,
		ETTS_StartedTurn,
	};

	HashedString		m_TurnTargetBlackboardKey;	// Config
	ETurnTowardState	m_TurnState;				// Transient (would be serialized)
};

#endif // RODINBTNODEROSATURNTOWARD_H
