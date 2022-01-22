#ifndef SDPROSAGEO_H
#define SDPROSAGEO_H

#include "SDPs/sdpbase.h"

class SDPRosaGeo : public SDPBase
{
public:
	SDPRosaGeo();
	virtual ~SDPRosaGeo();

	DEFINE_SDP_FACTORY( RosaGeo );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAGEO_H
