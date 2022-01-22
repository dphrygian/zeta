#ifndef RODINBTNODEROSAMOVETO_H
#define RODINBTNODEROSAMOVETO_H

#include "rodinbtnode.h"
#include "wbparamevaluator.h"

class RodinBTNodeRosaMoveTo : public RodinBTNode
{
public:
	RodinBTNodeRosaMoveTo();
	virtual ~RodinBTNodeRosaMoveTo();

	DEFINE_RODINBTNODE( RosaMoveTo, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();

private:
	enum EMoveToState
	{
		EMTS_Begin,
		EMTS_StartedMove,
	};

	HashedString		m_MoveTargetBlackboardKey;	// Config
	HashedString		m_TurnTargetBlackboardKey;	// Config
	HashedString		m_Stance;					// Config
	bool				m_Cautious;					// Config
	bool				m_Wander;					// Config
	float				m_WanderTargetDistance;		// Config
	bool				m_Flee;						// Config
	float				m_FleeTargetDistance;		// Config
	WBParamEvaluator	m_TetherLocationPE;			// Config
	float				m_TetherDistance;			// Config
	float				m_TetherDistanceZ;			// Config
	float				m_ApproachDistance;			// Config
	WBParamEvaluator	m_ApproachDistancePE;		// Config
	bool				m_UseActualTargetLocation;	// Config; HACKHACK to override knowledge and move to target's real location instead of last known location
	float				m_ReachedThresholdMin;		// Config
	float				m_ReachedThresholdMax;		// Config
	float				m_FlyingDeflectionRadius;	// Config
	float				m_PostDeflectionEndTime;	// Config
	float				m_FlyingDestinationOffsetZ;	// Config
	bool				m_Teleport;					// Config
	float				m_TeleportDelay;			// Config
	float				m_TeleportDelayRemaining;	// Transient (would be serialized)
	EMoveToState		m_MoveState;				// Transient (would be serialized)

#if BUILD_DEV
	bool				m_WarnMaxSteps;				// Config
#endif
};

#endif // RODINBTNODEROSAMOVETO_H
