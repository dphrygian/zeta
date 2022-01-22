#ifndef SDPROSALIGHTCOMBINE_H
#define SDPROSALIGHTCOMBINE_H

#include "SDPs/sdpbase.h"

class SDPRosaLightCombine : public SDPBase
{
public:
	SDPRosaLightCombine();
	virtual ~SDPRosaLightCombine();

	DEFINE_SDP_FACTORY( RosaLightCombine );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSALIGHTCOMBINE_H
