#ifndef WBPEROSAGETSURFACEPROPERTY_H
#define WBPEROSAGETSURFACEPROPERTY_H

#include "wbpe.h"

class WBPERosaGetSurfaceProperty : public WBPE
{
public:
	WBPERosaGetSurfaceProperty();
	virtual ~WBPERosaGetSurfaceProperty();

	DEFINE_WBPE_FACTORY( RosaGetSurfaceProperty );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	enum ESurfaceProperty
	{
		ESP_Sound,
		ESP_VolumeScalar,
		ESP_RadiusScalar,
	};

	ESurfaceProperty	GetSurfaceProperty( const HashedString& Property );

	HashedString		m_SurfaceParameter;
	ESurfaceProperty	m_SurfaceProperty;
};

#endif // WBPEROSAGETSURFACEPROPERTY_H
