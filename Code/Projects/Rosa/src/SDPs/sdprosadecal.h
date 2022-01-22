#ifndef SDPROSADECAL_H
#define SDPROSADECAL_H

#include "SDPs/sdpbase.h"

class SDPRosaDecal : public SDPBase
{
public:
	SDPRosaDecal();
	virtual ~SDPRosaDecal();

	DEFINE_SDP_FACTORY( RosaDecal );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSADECAL_H
