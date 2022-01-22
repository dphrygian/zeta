#ifndef SDPROSAEDGE_H
#define SDPROSAEDGE_H

#include "SDPs/sdpbase.h"

class SDPRosaEdge : public SDPBase
{
public:
	SDPRosaEdge();
	virtual ~SDPRosaEdge();

	DEFINE_SDP_FACTORY( RosaEdge );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAEDGE_H
