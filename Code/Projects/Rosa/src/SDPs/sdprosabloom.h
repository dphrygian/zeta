#ifndef SDPROSABLOOM_H
#define SDPROSABLOOM_H

#include "SDPs/sdpbase.h"

class SDPRosaBloom : public SDPBase
{
public:
	SDPRosaBloom();
	virtual ~SDPRosaBloom();

	DEFINE_SDP_FACTORY( RosaBloom );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSABLOOM_H
