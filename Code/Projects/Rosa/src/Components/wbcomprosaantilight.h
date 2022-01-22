#ifndef WBCOMPROSAANTILIGHT_H
#define WBCOMPROSAANTILIGHT_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "vector4.h"
#include "angles.h"
#include "interpolator.h"

class View;
class Mesh;

class WBCompRosaAntiLight : public WBRosaComponent
{
public:
	WBCompRosaAntiLight();
	virtual ~WBCompRosaAntiLight();

	DEFINE_WBCOMP( RosaAntiLight, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	const Vector4&	GetColor() const { return m_Color; }

	float			GetImportanceScore( const Mesh* const pMesh, const View* const pView ) const;
	float			GetImportanceScalar( const Mesh* const pMesh, const View* const pView ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	Vector4	m_Color;					// Light intensity (r) and radius (a)

	float	m_ImportanceThresholdLo;	// Config; anti-light meshes are only drawn above this threshold
	float	m_ImportanceThresholdHi;	// Config; anti-light meshes are drawn fully above this threshold (interpolated for scores between lo and hi)
};

#endif // WBCOMPROSAANTILIGHT_H
