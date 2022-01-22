#ifndef SDPROSALIGHT_H
#define SDPROSALIGHT_H

#include "SDPs/sdpbase.h"

class SDPRosaLight : public SDPBase
{
public:
	SDPRosaLight();
	virtual ~SDPRosaLight();

	DEFINE_SDP_FACTORY( RosaLight );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSALIGHT_H
