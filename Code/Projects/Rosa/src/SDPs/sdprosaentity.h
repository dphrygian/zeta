#ifndef SDPROSAENTITY_H
#define SDPROSAENTITY_H

#include "SDPs/sdpbase.h"

class SDPRosaEntity : public SDPBase
{
public:
	SDPRosaEntity();
	virtual ~SDPRosaEntity();

	DEFINE_SDP_FACTORY( RosaEntity );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAENTITY_H
