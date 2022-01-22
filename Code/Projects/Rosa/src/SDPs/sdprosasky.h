#ifndef SDPROSASKY_H
#define SDPROSASKY_H

#include "SDPs/sdpbase.h"

class SDPRosaSky : public SDPBase
{
public:
	SDPRosaSky();
	virtual ~SDPRosaSky();

	DEFINE_SDP_FACTORY( RosaSky );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSASKY_H
