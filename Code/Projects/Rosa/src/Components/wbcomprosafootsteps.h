#ifndef WBCOMPROSAFOOTSTEPS_H
#define WBCOMPROSAFOOTSTEPS_H

#include "wbrosacomponent.h"
#include "vector.h"

class WBCompRosaFootsteps : public WBRosaComponent
{
public:
	WBCompRosaFootsteps();
	virtual ~WBCompRosaFootsteps();

	DEFINE_WBCOMP( RosaFootsteps, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	float			GetStepPhaseSignedAlpha() const { return m_StepPhaseSignedAlpha; }

	void			SetFootstepsDisabled( const bool Disabled ) { m_FootstepsDisabled = Disabled; }

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	void			TeleportBy( const Vector& Offset ) { m_LastFootstepLocation += Offset; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			DoFootstep( const Vector& CurrentLocation, const float AdditionalSpeed );
	void			UpdateStepPhaseSignedAlpha();

	enum EStepPhase
	{
		ESP_RightFoot,	// Stepping from left foot to right
		ESP_LeftFoot,	// Stepping from right foot to left
	};

	float		m_FootstepStride;		// Config
	Vector		m_LastFootstepLocation;	// Serialized
	bool		m_HasTakenFirstStep;	// Serialized
	EStepPhase	m_StepPhase;			// Serialized
	float		m_StepPhaseAlpha;		// Serialized; [0.0, 1.0], how close we are to landing this step
	float		m_StepPhaseSignedAlpha;	// Serialized; [-1.0, 1.0], from left (-1) to right (1)
	bool		m_FootstepsDisabled;	// Serialized (mostly for debugging, but why not)
};

#endif // WBCOMPROSAFOOTSTEPS_H
