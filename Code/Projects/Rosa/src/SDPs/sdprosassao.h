#ifndef SDPROSASSAO_H
#define SDPROSASSAO_H

#include "SDPs/sdpbase.h"

class SDPRosaSSAO : public SDPBase
{
public:
	SDPRosaSSAO();
	virtual ~SDPRosaSSAO();

	DEFINE_SDP_FACTORY( RosaSSAO );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSASSAO_H
