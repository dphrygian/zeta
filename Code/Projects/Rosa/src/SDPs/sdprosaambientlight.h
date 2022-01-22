#ifndef SDPROSAAMBIENTLIGHT_H
#define SDPROSAAMBIENTLIGHT_H

#include "SDPs/sdpbase.h"

class SDPRosaAmbientLight : public SDPBase
{
public:
	SDPRosaAmbientLight();
	virtual ~SDPRosaAmbientLight();

	DEFINE_SDP_FACTORY( RosaAmbientLight );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAAMBIENTLIGHT_H
