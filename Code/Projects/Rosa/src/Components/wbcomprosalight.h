#ifndef WBCOMPROSALIGHT_H
#define WBCOMPROSALIGHT_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "vector4.h"
#include "angles.h"
#include "interpolator.h"

class View;
class Mesh;

class WBCompRosaLight : public WBRosaComponent
{
public:
	WBCompRosaLight();
	virtual ~WBCompRosaLight();

	DEFINE_WBCOMP( RosaLight, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Lights used to only tick if they flickered, but that meant SetLightColorHSV wouldn't work

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	const Vector4&	GetColor() const			{ return m_CurrentColor; }
	const Vector&	GetTranslation() const		{ return m_CurrentTranslation; }
	float			GetFogRadius() const		{ return ( m_FogRadius > 0.0f ) ? m_FogRadius : m_CurrentColor.a; }
	float			GetFogValueScalar() const	{ return m_FogValueScalar; }

	float			GetImportanceScore( const Mesh* const pMesh, const View* const pView ) const;
	float			GetImportanceScalar( const Mesh* const pMesh, const View* const pView ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	Interpolator<Vector4>	m_BaseColor;			// Config/serialized; max light color/radius before flicker modulation
	Vector4					m_CurrentColor;			// Color/radius after modulation by flicker
	Vector					m_CurrentTranslation;	// Direction/distance
	float					m_FogRadius;			// Config; uses light's radius if <= 0
	float					m_FogValueScalar;		// Config; HACKHACK to adjust fog brightness relative to light brightness

	bool	m_DoColorFlicker;		// Config; set automatically if Octaves > 0
	float	m_ColorFlickerLow;		// Config; the lowest flicker color value as ratio of base color, before saturation
	float	m_ColorFlickerHigh;		// Config; the highest flicker color value as ratio of base color, before saturation
	float	m_ColorFlickerScalar;	// Config; time scalar for noise
	uint	m_ColorFlickerOctaves;	// Config; number of octaves in noise
	float	m_ColorFlickerOffset;	// Random time offset rolled at config time

	bool	m_DoRadiusFlicker;		// Config; set automatically if Octaves > 0
	float	m_RadiusFlickerLow;		// Config; the lowest flicker radius value as ratio of base radius, before saturation
	float	m_RadiusFlickerHigh;	// Config; the highest flicker radius value as ratio of base radius, before saturation
	float	m_RadiusFlickerScalar;	// Config; time scalar for noise
	uint	m_RadiusFlickerOctaves;	// Config; number of octaves in noise
	float	m_RadiusFlickerOffset;	// Random time offset rolled at config time

	bool	m_DoTranslationDrift;		// Config; set automatically if Octaves > 0
	float	m_TranslationDriftRadius;	// Config; the max spherical radius of drift
	float	m_TranslationDriftScalar;	// Config; time scalar for each axis of noise (w is distance control)
	uint	m_TranslationDriftOctaves;	// Config; number of octaves in noise
	Vector4	m_TranslationDriftOffsets;	// Random time offsets rolled at config time (XYZ/distance)

	float	m_ImportanceThresholdLo;	// Config; light meshes are only drawn above this threshold
	float	m_ImportanceThresholdHi;	// Config; light meshes are drawn fully bright above this threshold (interpolated for scores between lo and hi)
};

#endif // WBCOMPROSALIGHT_H
