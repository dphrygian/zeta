#ifndef ROSALOCKPICKING_H
#define ROSALOCKPICKING_H

#include "iwbeventobserver.h"
#include "array.h"
#include "wbeventmanager.h"
#include "vector2.h"
#include "vector.h"
#include "angles.h"

class IRenderer;
class InputSystem;
class Mesh;
class ITexture;
class Segment2D;
class WBAction;
class WBCompStatMod;

class RosaLockpicking : public IWBEventObserver
{
public:
	RosaLockpicking();
	~RosaLockpicking();

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

	void			RegisterForEvents();

	void			InitializeFromDefinition( const SimpleString& DefinitionName );
	void			InitializeLockFromDefinition( const SimpleString& LockDef );

	void			Tick( const float DeltaTime );

	bool			IsLockpicking() const { return m_IsLockpicking; }
	void			ShutDown() { EndLockpicking(); }
	void			Reset() { EndLockpicking(); }

private:
	bool			CanAcceptInput() const { return m_IsActive && !m_IsInputTimedOut && !m_IsBinding && !m_IsForcing; }

	void			BeginLockpicking();
	void			EndLockpicking();

	void			ContinueLockpick();
	void			QueueSucceedLockpick();
	void			SucceedLockpick();

	void			SetCameraOverride( const bool Enable, const Vector& Translation, const Angles& Orientation, const float LerpTime );
	void			InitializeNextPin();
	void			SetLockpickVector( const Vector2& LockpickVector );
	void			PublishToHUD();

	InputSystem*	GetInputSystem() const;
	WBCompStatMod*	GetStatMod() const;
	void			PlaySoundDef( const SimpleString& SoundDef );
	void			StopSoundDef( const SimpleString& SoundDef );
	void			SetSoundVolume( const SimpleString& SoundDef, const float Volume );
	void			PlayAINoise( const float Radius, const float CertaintyScalar );

	bool				m_IsLockpicking;
	HashedString		m_InputContext;		// Config; input context to push when game is started
	float				m_GameEndDelay;		// Config; delay before lock is closed when succeeding or failing
	float				m_PinShakeLow;		// Config; random scalar applied when lockpick vector is opposite pin vector
	float				m_PinShakeHigh;		// Config; random scalar applied when lockpick vector is aligned with pin vector
	float				m_RattleVolumeLow;	// Config
	float				m_RattleVolumeHigh;	// Config
	float				m_LockpickVelocity;	// Config; how fast lockpick moves (scalar applied to input)
	float				m_InputTimeout;				// Config
	float				m_InputTimeoutRemaining;	// Transient
	float				m_ForceNoiseRadius;	// Config; AI noise radius when forcing pins
	float				m_ForceNoiseScalar;	// Config; AI noise certainty scalar
	SimpleString		m_RattleSound;		// Config
	SimpleString		m_PinMissSound;		// Config
	SimpleString		m_PinBindSound;		// Config
	SimpleString		m_PinResistSound;	// Config
	SimpleString		m_PinForceSound;	// Config
	SimpleString		m_OpenedSound;		// Config

	Array<WBAction*>	m_StartActions;
	Array<WBAction*>	m_SuccessActions;

	TEventUID			m_GameEndEventUID;
	Vector2				m_LockpickVector;
	Vector2				m_PinVector;
	float				m_CosPinAngle;		// Lock config; angle from pin vector accepted for binding pin
	uint				m_PinsRemaining;	// Lock config/transient; how many pins the lock has left
	uint				m_MaxPins;			// Lock config/transient; how many pins the lock has in total
	float				m_ForceChance;		// Lock config; chance [0,1] that a force will be successful
	uint				m_ForceFailPins;	// Lock config; how many pins get reset when force fails
	uint				m_ExtraForcePins;	// Lock config; extra pins that are bypassed by forcing (mostly for stat modding)
	bool				m_IsActive;
	bool				m_IsInputTimedOut;
	bool				m_IsBinding;
	bool				m_IsForcing;

	bool				m_UseCameraOverride;
	Vector				m_CameraTranslation;
	Angles				m_CameraOrientation;
	float				m_CameraLerpTime;
};

#endif // ROSALOCKPICKING_H
