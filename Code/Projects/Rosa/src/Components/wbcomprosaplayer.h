#ifndef WBCOMPROSAPLAYER_H
#define WBCOMPROSAPLAYER_H

#include "wbrosacomponent.h"
#include "wbevent.h"
#include "vector.h"
#include "iinputsystemobserver.h"
#include "interpolator.h"
#include "wbentityref.h"

#if BUILD_DEV
#include "rosanav.h"
#include "clock.h"
#endif

class WBEvent;
class WBCompRosaWeapon;

class WBCompRosaPlayer : public WBRosaComponent, public IInputSystemObserver
{
public:
	WBCompRosaPlayer();
	virtual ~WBCompRosaPlayer();

	DEFINE_WBCOMP( RosaPlayer, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickFirst; }	// Should tick before motion is integrated so input is applied ASAP.

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

#if BUILD_DEV
	virtual void	Report() const;
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	// IInputSystemObserver
	virtual void	OnInputContextsChanged();

	bool			IsCrouched() const			{ return m_IsCrouched; }
	bool			IsDisablingPause() const	{ return m_IsDisablingPause; }

	void			SetAutoAimEnabled( const bool AutoAimEnabled )		{ m_AutoAimEnabled		= AutoAimEnabled; }
	void			SetSprintFOVEnabled( const bool SprintFOVEnabled )	{ m_SprintFOVEnabled	= SprintFOVEnabled; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PushPersistence() const;
	void			PullPersistence();

#if BUILD_DEV
	void			DEVHACKInput();
#endif

	void			ConditionalApplyRunningStatMods();

	bool			CanCrouch();
	void			Crouch();
	void			BeginUncrouch();
	void			CancelUncrouch();
	void			TryUncrouch();
	bool			CanUncrouch();
	void			Uncrouch();
	void			RestoreCrouch();
	void			SetCrouchOverlayHidden( const bool Hidden );

	void			OnSteppedUp( const float StepHeight );

	bool			CanDoubleJump() const;

	bool			CanPowerJump() const;

	bool			CanPowerSlide() const;
	void			BeginPowerSlide( const Vector& PowerSlideY );
	void			EndPowerSlide();
	void			RestorePowerSlide();

	void			BeginDrag( WBEntity* const pDraggedEntity );
	void			DropDraggedEntity();
	void			EndDrag();
	void			RestoreDrag();

	bool			ShouldAttachToClimbable( const WBEvent& ClimbEvent );
	void			IncrementClimbRefs( const WBEvent& ClimbEvent );
	void			DecrementClimbRefs( const bool AddClimbOffImpulse );
	void			ZeroClimbRefs();		// Immediately end climb
	void			BeginClimb( const WBEvent& ClimbEvent );
	void			EndClimb( const bool AddClimbOffImpulse );
	void			RestoreClimb();

	Vector			GetMantleDestination( const Vector& CollisionNormal, const Vector& Location, const Vector& Extents ) const;
	void			TryBeginMantle( const Vector& CollisionNormal );
	void			BeginMantle( const Vector& MantleDestination );
	void			EndMantle( const bool AllowMantle );

	void			TickScaleFOV( Interpolator<float>& FOVInterpolator, const float BaseFOVDegrees, const float Scale, const float Time, const float DeltaTime );

	void			SetSpawnPoint();
	void			RestoreSpawnPoint();

	void			TeleportTo( const HashedString& TeleportLabel );

	void			TickViewBobAndSway();
	void			TickKick( const float DeltaTime );
	void			TickHandsVelocity( const float DeltaTime, const Vector& TargetVelocity, const Angles& TargetRotationalVelocity );
	void			TickAutoAim( const float DeltaTime );

	// Rolling autosave; suppression does not prevent manual saves, scripted checkpoints, saving when closing the game, etc.
	void			AddAutosaveSuppression( const bool Serialize );
	void			RemoveAutosaveSuppression( const bool Serialize );
	bool			CanAutosave();
	void			QueueAutosave( const float AutosaveDelay );

	// Gross helper function
	WBCompRosaWeapon*	GetWeapon() const;

	bool	m_DeferMusic;					// Config; HACKHACK to defer playing music on launch before title screen
	uint	m_InitialMusicTrackBits;		// Config/serialized; which tracks are played by default when music is started (0 means music starts silently)

	float	m_RollingAutosaveDelay;			// Config
	float	m_RetryAutosaveDelay;			// Config
	uint	m_AutosaveSuppRefsSerialized;	// Serialized
	uint	m_AutosaveSuppRefsTransient;	// Transient; HACKHACK to prevent AI BT autosave suppression from doubling up, because BTs don't serialize their state

	float	m_UnlandedForceFootstepWindow;
	float	m_UnlandedJumpWindow;
	float	m_UnlandedLeanWindow;
	float	m_LandAcceleration;
	float	m_AirAcceleration;
	float	m_TurnSpeed;
	float	m_JumpHeight;
	float	m_BackpedalScalar;

	bool	m_UncrouchOnSprint;					// Config

	bool	m_IsCrouched;
	bool	m_IsUncrouching;
	bool	m_IsForceCrouched;	// For mantle, maybe for other things later
	float	m_StandExtentsZ;
	float	m_CrouchExtentsZ;
	float	m_StandViewOffsetZ;
	float	m_CrouchViewOffsetZ;
	float	m_CrouchViewInterpTime;								// Config
	Interpolator<float>		m_ViewOffsetZInterpolator;			// Transient
	Interpolator<float>		m_PowerSlideRollInterpolator;		// Transient
	Interpolator<float>		m_StepUpZInterpolator;				// Transient
	Interpolator<Vector>	m_ViewBobOffsetInterpolator;		// Transient
	Interpolator<Angles>	m_ViewBobAngleOffsetInterpolator;	// Transient
	Interpolator<Vector>	m_ViewSwayOffsetInterpolator;		// Transient
	Interpolator<Angles>	m_ViewSwayAngleOffsetInterpolator;	// Transient
	Vector					m_ViewHandsAcceleration;			// Transient
	Vector					m_ViewHandsVelocity;				// Transient
	Angles					m_ViewHandsRotationalAcceleration;	// Transient
	Angles					m_ViewHandsRotationalVelocity;		// Transient

	float			m_KickSpringK;				// Config
	float			m_KickDamperC;				// Config
	Angles			m_KickVelocity;				// Transient
	Angles			m_KickPosition;				// Transient

	float			m_ViewHandsSpringK;			// Config
	float			m_ViewHandsDamperC;			// Config

	bool			m_PushToSprint;				// Config
	bool			m_IsSprinting;				// Transient (for push-to-sprint; means "is the sprint input active" even if player is standing still)
	bool			m_IsSprintingWithMovement;	// Transient (means m_IsSprinting AND there is movement input)
	float			m_SprintStartTime;			// Transient (for sprint VO delay)

	float			m_SprintFOVScale;			// Config
	float			m_SprintFOVTime;			// Config

	float			m_DoubleJumpHeight;			// Config
	bool			m_HasDoubleJumped;			// Serialized

	float			m_PowerJumpRatio;			// Config, ratio of forward impulse to up impulse (normalized before applied)

	bool			m_IsPowerSliding;			// Serialized
	float			m_PowerSlideDuration;		// Config
	float			m_PowerSlideEndTime;		// Serialized (as time remaining)
	Vector			m_PowerSlideY;				// Serialized
	HashedString	m_PowerSlideInputContext;	// Config
	float			m_PowerSlideReqVelocitySq;	// Config
	float			m_PowerSlideRoll;			// Config
	float			m_PowerSlideRollInterpTime;	// Config

	bool			m_IsDragging;				// Serialized
	HashedString	m_DragInputContext;			// Config
	Vector			m_DragDropOffset;			// Config
	Angles			m_DragDropOrientation;		// Config
	float			m_DragDropSpawnImpulse;		// Config
	float			m_DragDropSpawnImpulseZ;	// Config
	WBEntityRef		m_DraggedEntity;			// Serialized

	int				m_ClimbRefs;				// Serialized; refcount climbing so we can transfer climbables without issue
	bool			m_IsClimbing;				// Transient
	HashedString	m_ClimbInputContext;
	float			m_ClimbOffImpulse;			// Config
	float			m_ClimbFacingBiasAngle;		// Config
	float			m_ClimbFacingBiasScale;		// Config

	bool			m_IsMantling;				// Transient
	HashedString	m_MantleInputContext;		// Config
	float			m_MantleVelocity;			// Config
	Vector			m_MantleVector;				// Transient
	Vector			m_MantleDestination;		// Transient
	bool			m_CanMantle;				// Transient

	bool			m_AutoAimEnabled;			// Transient, driven by config option
	float			m_AutoAimMaxTurn;			// Config (as degrees per second, transformed into radians per second)

	bool			m_SprintFOVEnabled;			// Transient, driven by config option

	Interpolator<float>	m_CurrentFOV;			// Transient
	Interpolator<float>	m_CurrentFGFOV;			// Transient

	bool			m_IsDisablingPause;			// Serialized

	bool			m_HasSetSpawnPoint;			// Serialized
	Vector			m_SpawnLocation;			// Serialized
	Angles			m_SpawnOrientation;			// Serialized

	// Originally implemented for Acid
	bool			m_IsFlying;					// Serialized

	// View bob and sway
	Vector			m_MaxViewBobOffset;
	Angles			m_MaxViewBobAngleOffset;
	float			m_UnlandedViewBobWindow;
	float			m_ViewBobAngleExponent;
	float			m_ViewBobInterpolateTime;
	Vector			m_MaxViewSwayOffset;
	Angles			m_MaxViewSwayAngleOffset;
	float			m_ViewSwayInterpolateTime;
	uint			m_ViewSwayNoiseOctaves;
	Vector			m_ViewSwayNoiseScalars;
	Vector			m_ViewSwayNoiseAngleScalars;

#if BUILD_DEV
	WBEntityRef					m_DEVHACKDebugTarget;
	Clock::MultiplierRequest*	m_DEVHACKClockMultiplier;

	RosaNav::SPathData		m_CAMHACKPathData;

	bool					m_CAMHACKCamActive;
	float					m_CAMHACKCamVelocity;
	Interpolator<Vector>	m_CAMHACKCamLocation;
	Interpolator<Angles>	m_CAMHACKCamOrientation;
	Vector					m_CAMHACKCamStartLocation;
	Vector					m_CAMHACKCamEndLocation;
	Angles					m_CAMHACKCamStartOrientation;
	Angles					m_CAMHACKCamEndOrientation;

	struct SDebugSpawner
	{
		SimpleString	m_Entity;
		Vector			m_Offset;
		float			m_NormalDistance;
	};
	Array<SDebugSpawner>	m_DebugSpawners;
	uint					m_DebugSpawnerIndex;

	Vector					m_CACHED_LastAINoiseSourceLocation;
	float					m_CACHED_LastAINoiseRadius;
#endif
};

#endif // WBCOMPROSAPLAYER_H
