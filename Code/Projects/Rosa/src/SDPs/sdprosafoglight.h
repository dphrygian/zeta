#ifndef SDPROSAFOGLIGHT_H
#define SDPROSAFOGLIGHT_H

#include "SDPs/sdpbase.h"

class SDPRosaFogLight : public SDPBase
{
public:
	SDPRosaFogLight();
	virtual ~SDPRosaFogLight();

	DEFINE_SDP_FACTORY( RosaFogLight );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAFOGLIGHT_H
