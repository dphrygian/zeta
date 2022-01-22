#ifndef WBCOMPROSASENSORVISION_H
#define WBCOMPROSASENSORVISION_H

#include "wbcomprosasensorpoll.h"
#include "vector.h"

class WBCompRosaSensorVision : public WBCompRosaSensorPoll
{
public:
	WBCompRosaSensorVision();
	virtual ~WBCompRosaSensorVision();

	DEFINE_WBCOMP( RosaSensorVision, WBCompRosaSensorPoll );

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	PollTick( const float DeltaTime ) const;

private:
	void			GetEyesTransform( Vector& OutEyeLocation, Vector& OutEyeDirection ) const;

	HashedString	m_EyesBoneName;				// Config
	float			m_EyeOffsetZ;				// Config
	float			m_RadiusSq;					// Config
	float			m_ConeCos;					// Config
	float			m_ConeInvZScale;			// Config
	float			m_CertaintyFalloffRadius;	// Config
	float			m_DistanceCertaintyFactor;	// Config
	float			m_PeripheryCertaintyFactor;	// Config
	float			m_SpeedCertaintyScalar;		// Config
	float			m_SpeedCertaintyFactor;		// Config
	float			m_CrouchedCertaintyFactor;	// Config
	float			m_CertaintyVelocity;		// Config
	float			m_CertaintyDecay;			// Config
	float			m_CertaintyThreshold;		// Config: ignore sights below this amount
	float			m_IsAimingThresholdCos;		// Config; angle threshold to determine if the visible thing is aiming at us

	bool			m_OnlySeePlayer;			// Config, HACKHACK for security cameras to save otherwise wasted traces

#if BUILD_DEV
	// Cache the last certainty values for debug display
	mutable float	m_CACHED_DistanceCertaintyFactor;
	mutable float	m_CACHED_PeripheryCertaintyFactor;
	mutable float	m_CACHED_SpeedCertaintyFactor;
	mutable float	m_CACHED_CrouchedCertaintyFactor;
	mutable float	m_CACHED_ImmediateCertainty;
	mutable float	m_CACHED_Certainty;
#endif
};

#endif // WBCOMPROSASENSORVISION_H
