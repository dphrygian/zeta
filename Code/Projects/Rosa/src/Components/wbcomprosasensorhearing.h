#ifndef WBCOMPROSASENSORHEARING_H
#define WBCOMPROSASENSORHEARING_H

#include "wbcomprosasensor.h"
#include "vector.h"

class WBCompRosaSensorHearing : public WBCompRosaSensor
{
public:
	WBCompRosaSensorHearing();
	virtual ~WBCompRosaSensorHearing();

	DEFINE_WBCOMP( RosaSensorHearing, WBCompRosaSensor );

	virtual void	HandleEvent( const WBEvent& Event );

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	bool	GetNoiseCertainty( WBEntity* const pNoiseEntity, const Vector& NoiseLocation, const float NoiseRadius, const Vector& NoiseSourceLocation, const float NoiseCertaintyScalar, float& OutCertainty ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	HandleNoise( WBEntity* const pNoiseEntity, const Vector& NoiseLocation, const float NoiseRadius, const Vector& NoiseSourceLocation, const float NoiseCertaintyScalar, const float NoiseUpdateTime, const float ExpireTimeBonus ) const;

	float	m_Radius;					// Config; adds to noise's radius, so some AIs can hear further than others. Can be negative.
	float	m_InvDistanceScaleZ;		// Config
	float	m_CertaintyFalloffRadius;	// Config
	float	m_DistanceCertaintyFactor;	// Config
	float	m_OcclusionCertaintyFactor;	// Config

#if BUILD_DEV
	// Cache the last certainty values for debug display
	mutable Vector	m_CACHED_LastHeardLocation;
	mutable float	m_CACHED_DistanceCertaintyFactor;
	mutable float	m_CACHED_OcclusionCertaintyFactor;
	mutable float	m_CACHED_Certainty;
#endif
};

#endif // WBCOMPROSASENSORVISION_H
