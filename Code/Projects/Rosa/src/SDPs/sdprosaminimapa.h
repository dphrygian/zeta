#ifndef SDPROSAMINIMAPA_H
#define SDPROSAMINIMAPA_H

#include "SDPs/sdpbase.h"

class SDPRosaMinimapA : public SDPBase
{
public:
	SDPRosaMinimapA();
	virtual ~SDPRosaMinimapA();

	DEFINE_SDP_FACTORY( RosaMinimapA );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAMINIMAPA_H
