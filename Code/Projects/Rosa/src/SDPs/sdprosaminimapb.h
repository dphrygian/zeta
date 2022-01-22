#ifndef SDPROSAMINIMAPB_H
#define SDPROSAMINIMAPB_H

#include "SDPs/sdpbase.h"

class SDPRosaMinimapB : public SDPBase
{
public:
	SDPRosaMinimapB();
	virtual ~SDPRosaMinimapB();

	DEFINE_SDP_FACTORY( RosaMinimapB );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAMINIMAPB_H
