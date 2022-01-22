#ifndef SDPROSAFOGMESH_H
#define SDPROSAFOGMESH_H

#include "SDPs/sdpbase.h"

class SDPRosaFogMesh : public SDPBase
{
public:
	SDPRosaFogMesh();
	virtual ~SDPRosaFogMesh();

	DEFINE_SDP_FACTORY( RosaFogMesh );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAFOGMESH_H
