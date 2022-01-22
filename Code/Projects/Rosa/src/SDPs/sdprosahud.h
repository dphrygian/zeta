#ifndef SDPROSAHUD_H
#define SDPROSAHUD_H

#include "SDPs/sdpbase.h"

class SDPRosaHUD : public SDPBase
{
public:
	SDPRosaHUD();
	virtual ~SDPRosaHUD();

	DEFINE_SDP_FACTORY( RosaHUD );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAHUD_H
