#ifndef SDPROSAWATERFLOW_H
#define SDPROSAWATERFLOW_H

#include "sdprosageo.h"

class SDPRosaWaterFlow : public SDPRosaGeo
{
public:
	SDPRosaWaterFlow();
	virtual ~SDPRosaWaterFlow();

	DEFINE_SDP_FACTORY( RosaWaterFlow );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAWATERFLOW_H
