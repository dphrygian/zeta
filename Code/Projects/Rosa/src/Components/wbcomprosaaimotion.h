#ifndef WBCOMPROSAAIMOTION_H
#define WBCOMPROSAAIMOTION_H

#include "wbrosacomponent.h"
#include "rosaworld.h"
#include "wbentityref.h"
#include "vector.h"
#include "aabb.h"
#include "irodinresourceuser.h"
#include "map.h"
#include "rosanav.h"
#include "interpolator.h"

class WBCompRosaAIMotion : public WBRosaComponent, public IRodinResourceUser
{
public:
	WBCompRosaAIMotion();
	virtual ~WBCompRosaAIMotion();

	DEFINE_WBCOMP( RosaAIMotion, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickFirst; }	// Should tick before motion is integrated so input is applied ASAP.

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

#if BUILD_DEV
	virtual void	Report() const;
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	void			StartMove( const Vector& Destination, const float ApproachDistance );
	void			StartFollow( const WBEntity* const pDestinationEntity, const float ApproachDistance, const bool UseActualTargetLocation );
	void			StartWander( const float WanderTargetDistance );
	void			StartFlee( const WBEntity* const pFleeEntity, const float FleeTargetDistance );
	void			StartTurn( const Vector& TurnTarget );
	void			StopMove();

#if BUILD_DEV
	void			SetWarnMaxSteps( const bool WarnMaxSteps ) { m_WarnMaxSteps = WarnMaxSteps; }
#endif

	void			SetStance( const HashedString& Stance ) { m_MotionStance = Stance; }
	void			SetCautious( const bool Cautious ) { m_Cautious = Cautious; }
	void			SetTurnTarget( const bool UseTurnTarget, const Angles& TurnTarget );
	void			SetReachedThreshold( const float ReachedThresholdMin, const float ReachedThresholdMax );
	void			UnsetTether();
	void			SetTether( const Vector& TetherLocation, const float TetherDistance, const float TetherDistanceZ );
	void			SetDeflectionRadius( const float DeflectionRadius );
	void			SetPostDeflectionEndTime( const float PostDeflectionEndTime ) { m_PostDeflectionEndTime = PostDeflectionEndTime; }
	void			SetFlyingDestinationOffsetZ( const float FlyingDestinationOffsetZ ) { m_FlyingDestinationOffsetZ = FlyingDestinationOffsetZ; }

	bool			IsMoving() const				{ return ( m_MotionStatus == EMS_Moving || m_MotionStatus == EMS_Following || m_MotionStatus == EMS_TurningToFace ); }
	bool			IsLocomoting() const			{ return ( m_MotionStatus == EMS_Moving || m_MotionStatus == EMS_Following ); }
	bool			DidMoveSucceed() const			{ return m_MotionStatus == EMS_MoveSucceeded; }
	bool			HasAnimationResource() const	{ return m_HasAnimationResource; }

	// Return true if teleport completes (destination is unblocked, etc.)
	bool			TeleportFinishMove();

	void			OnSteppedUp( const float StepHeight );

	// IRodinResourceUser
	virtual bool	OnResourceStolen( const HashedString& Resource );
	virtual void	OnResourceReturned( const HashedString& Resource );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			TickMove();
	void			TickWalking();
	void			TickFlying();
	void			TickTurn();

	void			DoLocalAvoidance( const Vector& CurrentLocation, Vector& InOutMovementDirection );

	void			ValidateFollow();
	void			RepathFollow();
	void			Repath();

	bool			GetNextStep( Vector& OutStep );

	float			GetLandAcceleration() const;
	float			GetTurnSpeed() const;
	float			GetIdleAnimationPlayRate() const { return m_IdleAnimRateBase; }
	float			GetMotionAnimationPlayRate() const;

	void			QueueIdleAnimation();

	// Common code for StartMove, StartFollow, and RepathFollow.
	void			InternalStartMove( const Vector& Destination, const WBEntity* const pDestinationEntity );
	void			PlayAnimation( const HashedString& AnimationName, const bool Loop, const bool IgnoreIfAlreadyPlaying, const float PlayRate, const bool NoBlend );

	bool			GetLocationFor( const WBEntity* const pFollowEntity, Vector& OutLocation ) const;
	bool			GetLastKnownLocationFor( const WBEntity* const pFollowEntity, Vector& OutLocation ) const;
	bool			GetActualLocationFor( const WBEntity* const pFollowEntity, Vector& OutLocation ) const;

	void				PlayIdleAnimation( const bool NoBlend );
	void				PlayMotionAnimation();
	const HashedString&	GetMotionStanceAnimationName() const;

	enum EMotionStatus
	{
		EMS_Still,
		EMS_Moving,
		EMS_Following,
		EMS_TurningToFace,
		EMS_MoveSucceeded,
		EMS_MoveFailed,
	};

#if BUILD_DEV
	static SimpleString	GetStatusFromEnum( const EMotionStatus Status );
#endif

	bool			m_CanMove;					// Config, ROSAHACK for training dummies
	float			m_LandAcceleration;			// Config
	float			m_AirAcceleration;			// Config
	float			m_TurnSpeed;				// Config
	float			m_FollowValidateTime;		// Config
	float			m_NextFollowValidateTime;	// Transient
	float			m_StepReachedThresholdSq;	// Config
	float			m_TurnReachedThreshold;		// Config
	bool			m_RepathOnNextTick;			// Transient
	bool			m_Paused;					// Serialized
	bool			m_IsFlying;					// Config
	bool			m_CanOpenDoors;				// Config
	bool			m_CanUnlockDoors;			// Config
	bool			m_CanBreakDoors;			// Config
	uint			m_MaxPathSteps;				// Config
	HashedString	m_IdleAnimationName;		// Config/serialized
	HashedString	m_WalkAnimationName;		// Config
	Map<HashedString, HashedString>	m_StanceAnimationNames;	// Config
	HashedString	m_AnimationResource;		// Config
	bool			m_HasAnimationResource;		// Transient
	float			m_IdleAnimRateBase;			// Config/Serialized
	float			m_WalkAnimRateBase;			// Config/Serialized
	RosaNav::SPathData	m_PathData;				// Transient
	uint			m_PathIndex;				// Transient
	AABB			m_PathBound;				// Transient
	EMotionStatus	m_MotionStatus;				// Transient
	HashedString	m_MotionStance;				// Serialized
	bool			m_Cautious;					// Transient
	bool			m_Wander;					// Transient
	float			m_WanderTargetDistance;		// Transient
	bool			m_Flee;						// Transient
	float			m_FleeTargetDistance;		// Transient
	bool			m_UseTether;				// Transient
	Vector			m_TetherLocation;			// Transient
	float			m_TetherDistance;			// Transient
	float			m_TetherDistanceZ;			// Transient
	float			m_ApproachDistance;			// Transient
	bool			m_UseActualTargetLocation;	// Transient
	float			m_ReachedThresholdMinSq;	// Transient
	float			m_ReachedThresholdMaxSq;	// Transient
	float			m_FlyingDeflectionRadiusSq;	// Transient
	float			m_PostDeflectionEndTime;	// Transient
	float			m_DeflectionEndTime;		// Transient
	float			m_FlyingDestinationOffsetZ;	// Transient
	Vector			m_LastDestination;			// Transient
	uint			m_LastDestinationIndex;		// Transient
	WBEntityRef		m_LastDestinationEntity;	// Transient
	bool			m_WaitingToPlayIdleAnim;	// Transient
	float			m_PlayIdleAnimTime;			// Transient
	bool			m_UseTurnTargetDirection;	// Transient
	Vector			m_TurnTargetDirection;		// Transient
	WBEntityRef		m_CurrentRepulsor;			// Transient
	Vector			m_StuckWatchdogLocation;	// Transient

	float				m_StepUpTime;			// Config
	Interpolator<float>	m_StepUpZInterpolator;	// Transient

#if BUILD_DEV
	bool			m_WarnMaxSteps;				// Transient
#endif
};

#endif // WBCOMPROSAAIMOTION_H
