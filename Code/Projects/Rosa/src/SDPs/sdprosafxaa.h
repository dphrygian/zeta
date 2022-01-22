#ifndef SDPROSAFXAA_H
#define SDPROSAFXAA_H

#include "SDPs/sdpbase.h"

class SDPRosaFXAA : public SDPBase
{
public:
	SDPRosaFXAA();
	virtual ~SDPRosaFXAA();

	DEFINE_SDP_FACTORY( RosaFXAA );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAFXAA_H
