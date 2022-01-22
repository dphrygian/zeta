#ifndef SDPROSAANTILIGHT_H
#define SDPROSAANTILIGHT_H

#include "SDPs/sdpbase.h"

class SDPRosaAntiLight : public SDPBase
{
public:
	SDPRosaAntiLight();
	virtual ~SDPRosaAntiLight();

	DEFINE_SDP_FACTORY( RosaAntiLight );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAANTILIGHT_H
