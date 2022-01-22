#ifndef WBCOMPROSACAMERA_H
#define WBCOMPROSACAMERA_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "angles.h"
#include "interpolator.h"
#include "rosagame.h"

class WBEvent;

class WBCompRosaCamera : public WBRosaComponent
{
public:
	WBCompRosaCamera();
	virtual ~WBCompRosaCamera();

	DEFINE_WBCOMP( RosaCamera, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Needs to tick after transform

	virtual void	HandleEvent( const WBEvent& Event );

	enum EViewModifiers
	{
		EVM_None			= 0x000,
		EVM_Hands			= 0x001,	// Modify other modifiers for hands
		EVM_OffsetZ			= 0x002,	// Translate view from transform root to eye height
		EVM_Lean			= 0x004,	// Translate and orient view to leaned position and roll.
		EVM_Roll			= 0x008,	// Orient view to arbitrary target roll (historically for power slide, but that's got its own enum now).
		EVM_Bob				= 0x010,	// Translate and orient view to bobbed position and roll.
		EVM_Sway			= 0x020,	// Translate and orient view to swayed position and roll.
		EVM_SlideRoll		= 0x040,	// Orient view when power sliding.
		EVM_StrafeRoll		= 0x080,	// Orient view when strafing.
		EVM_Kick			= 0x100,	// Orient view to weapon kicked pitch/yaw.
		EVM_Override		= 0x200,	// Override transform entirely, pre-empting all other modifiers.
		EVM_StepUpZ			= 0x400,	// Additional Z offset for step-up blend, which doesn't get adjusted for hands
		EVM_HandsVelocity	= 0x800,	// Add translation and rotation velocity to hands; this would probably always be true if EVM_Hands is true, I'm just being explicit

		// Everything, for reflecting the player's actual view
		EVM_All				= EVM_OffsetZ | EVM_Lean | EVM_Roll | EVM_Bob | EVM_Sway | EVM_SlideRoll | EVM_StrafeRoll | EVM_Kick | EVM_Override | EVM_StepUpZ,

		// Everything but the override, for things like where the player is visible to AIs
		EVM_All_NoOverride	= EVM_All & ~EVM_Override,

		EVM_All_Hands		= EVM_Hands | EVM_OffsetZ | EVM_Lean | EVM_Roll | EVM_Bob | EVM_Sway | EVM_SlideRoll | EVM_StrafeRoll | EVM_Kick | EVM_Override | EVM_StepUpZ | EVM_HandsVelocity,
	};

	void			SetTranslationOverride( const Vector& TranslationOverride, const bool Enable, const float LerpTime );
	void			SetOrientationOverride( const Angles& OrientationOverride, const bool Enable, const float LerpTime );

	void			SetViewOffsetZ( const float ViewOffsetZ );
	void			SetViewAngleOffsetRoll( const float ViewAngleOffsetRoll )	{ m_ViewAngleOffsetRoll	= ViewAngleOffsetRoll; }

	void			SetStepUpZ( const float StepUpZ )							{ m_StepUpZ				= StepUpZ; }

	void			SetViewBobOffset( const Vector& ViewBobOffset )				{ m_ViewBobOffset		= ViewBobOffset; }
	void			SetViewBobAngleOffset( const Angles& ViewBobAngleOffset )	{ m_ViewBobAngleOffset	= ViewBobAngleOffset; }
	void			SetViewSwayOffset( const Vector& ViewSwayOffset )			{ m_ViewSwayOffset		= ViewSwayOffset; }
	void			SetViewSwayAngleOffset( const Angles& ViewSwayAngleOffset )	{ m_ViewSwayAngleOffset	= ViewSwayAngleOffset; }

	void			SetKickAngleOffset( const Angles& KickAngleOffset )			{ m_KickAngleOffset		= KickAngleOffset; }

	void			SetHandsVelocity( const Vector& HandsVelocity, const Angles& HandsRotationalVelocity ) { m_HandsVelocity = HandsVelocity; m_HandsRotationalVelocity = HandsRotationalVelocity; }

	void			ModifyTranslation( const EViewModifiers Modifiers, Vector& InOutTranslation ) const;
	void			ModifyOrientation( const EViewModifiers Modifiers, Angles& InOutOrientation ) const;

	Vector			GetModifiedTranslation( const EViewModifiers Modifiers, const Vector& InTranslation ) const;
	Angles			GetModifiedOrientation( const EViewModifiers Modifiers, const Angles& InOrientation ) const;

	void			SetLeanPosition( const float LeanPosition )					{ m_LeanPosition			= LeanPosition; }
	void			SetSlideRoll( const float SlideRoll )						{ m_SlideRoll				= SlideRoll; }
	void			SetStrafeRollPosition( const float StrafeRollPosition )		{ m_StrafeRollPosition		= StrafeRollPosition; }

	void			SetViewBobEnabled( const bool ViewBobEnabled )				{ m_ViewBobEnabled			= ViewBobEnabled; }
	void			SetViewSwayEnabled( const bool ViewSwayEnabled )			{ m_ViewSwayEnabled			= ViewSwayEnabled; }
	void			SetSlideRollEnabled( const bool SlideRollEnabled )			{ m_SlideRollEnabled		= SlideRollEnabled; }
	void			SetStrafeRollEnabled( const bool StrafeRollEnabled )		{ m_StrafeRollEnabled		= StrafeRollEnabled; }
	void			SetHandsVelocityEnabled( const bool HandsVelocityEnabled )	{ m_HandsVelocityEnabled	= HandsVelocityEnabled; }

	void			ZoomMinimapOut();
	void			ZoomMinimapIn();

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	UpdateLean( const float TargetRoll, const float DeltaTime );
	float	GetDesiredLean( const float LeanPosition ) const;
	Vector	GetLeanOffset( const float LeanRoll ) const;
	void	SetLeanRoll( const float LeanRoll );

	void	UpdateStrafeRoll( const float TargetRoll, const float DeltaTime );

	bool	m_HasTranslationOverride;
	bool	m_HasOrientationOverride;
	Vector	m_TranslationOverride;
	Angles	m_OrientationOverride;
	float	m_TranslationOverrideStart;
	float	m_TranslationOverrideEnd;
	float	m_OrientationOverrideStart;
	float	m_OrientationOverrideEnd;

	float	m_ViewOffsetZ;			// Config/transient, should possibly be serialized
	float	m_LastViewOffsetZ;		// Transient, hack for reviving from death
	float	m_BaseViewOffsetZ;		// HACKHACK: reference height for HandsZFactor

	float	m_ViewAngleOffsetRoll;	// Transient

	float	m_StepUpZ;				// Transient

	// Seems a bit hacky to put leaning in the camera, but I'll give it a try.
	float	m_LeanRoll;				// Transient
	float	m_LeanPosition;			// Transient, maps [-1,1] into lean angle range.
	float	m_LeanVelocity;			// Config
	float	m_LeanRollMax;			// Config
	float	m_LeanRadius;			// Config
	float	m_LeanExtent;			// Config
	Vector	m_CachedLeanOffset;		// Transient, small optimization since lean offset is queried for every RosaMesh::Render

	// Slide roll's interp is managed by WBCompRosaPlayer code instead of the camera, so no config or -1/+1 "position" here.
	float	m_SlideRoll;			// Transient

	float	m_StrafeRoll;			// Transient
	float	m_StrafeRollPosition;	// Transient, [-1,1] which maps into +/- strafe roll max range.
	float	m_StrafeRollVelocity;	// Config
	float	m_StrafeRollMax;		// Config

	Vector	m_ViewBobOffset;		// Transient
	Angles	m_ViewBobAngleOffset;	// Transient
	Vector	m_ViewSwayOffset;		// Transient
	Angles	m_ViewSwayAngleOffset;	// Transient
	bool	m_ViewBobEnabled;		// Transient, driven by config option
	bool	m_ViewSwayEnabled;		// Transient, driven by config option
	bool	m_SlideRollEnabled;		// Transient, driven by config option
	bool	m_StrafeRollEnabled;	// Transient, driven by config option
	bool	m_HandsVelocityEnabled;	// Transient, driven by config option

	Angles	m_KickAngleOffset;		// Transient

	Vector	m_HandsVelocity;			// Transient
	Angles	m_HandsRotationalVelocity;	// Transient

	float	m_HandsFactor;			// Config
	float	m_HandsLeanFactor;		// Config
	float	m_HandsZFactor;			// Config
	float	m_HandsVelocityFactor;				// Config
	float	m_HandsRotationalVelocityFactor;	// Config
	float	m_HandsVelocityLimit;				// Config
	float	m_HandsRotationalVelocityLimit;		// Config

	float				m_MinimapViewExtentNear;	// Config
	float				m_MinimapViewExtentFar;		// Config
	float				m_MinimapViewLerpTime;		// Config
	Interpolator<float>	m_MinimapViewExtent;		// Transient

#if ROSA_USE_MAXIMAP
	float				m_MaximapViewExtentNear;	// Config
	float				m_MaximapViewExtentFar;		// Config
	Interpolator<float>	m_MaximapViewExtent;		// Transient
#endif
};

#endif // WBCOMPROSACAMERA_H
