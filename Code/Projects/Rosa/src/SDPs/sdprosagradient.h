#ifndef SDPROSAGRADIENT_H
#define SDPROSAGRADIENT_H

#include "SDPs/sdpbase.h"

class SDPRosaGradient : public SDPBase
{
public:
	SDPRosaGradient();
	virtual ~SDPRosaGradient();

	DEFINE_SDP_FACTORY( RosaGradient );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAGRADIENT_H
