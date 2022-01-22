#ifndef SDPROSAFOLIAGE_H
#define SDPROSAFOLIAGE_H

#include "sdprosageo.h"

class SDPRosaFoliage : public SDPRosaGeo
{
public:
	SDPRosaFoliage();
	virtual ~SDPRosaFoliage();

	DEFINE_SDP_FACTORY( RosaFoliage );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAFOLIAGE_H
