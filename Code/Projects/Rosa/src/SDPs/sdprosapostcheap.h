#ifndef SDPROSAPOSTCHEAP_H
#define SDPROSAPOSTCHEAP_H

#include "SDPs/sdpbase.h"

class SDPRosaPostCheap : public SDPBase
{
public:
	SDPRosaPostCheap();
	virtual ~SDPRosaPostCheap();

	DEFINE_SDP_FACTORY( RosaPostCheap );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAPOSTCHEAP_H
