#ifndef SDPROSAWATER_H
#define SDPROSAWATER_H

#include "sdprosageo.h"

class SDPRosaWater : public SDPRosaGeo
{
public:
	SDPRosaWater();
	virtual ~SDPRosaWater();

	DEFINE_SDP_FACTORY( RosaWater );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAWATER_H
