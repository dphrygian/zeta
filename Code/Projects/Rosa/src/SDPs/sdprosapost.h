#ifndef SDPROSAPOST_H
#define SDPROSAPOST_H

#include "SDPs/sdpbase.h"

class SDPRosaPost : public SDPBase
{
public:
	SDPRosaPost();
	virtual ~SDPRosaPost();

	DEFINE_SDP_FACTORY( RosaPost );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAPOST_H
