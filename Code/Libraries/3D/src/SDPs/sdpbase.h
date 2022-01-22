#ifndef SDPBASE_H
#define SDPBASE_H

#include "shaderdataprovider.h"
#include "3d.h"

class SDPBase : public ShaderDataProvider
{
public:
	SDPBase();
	virtual ~SDPBase();

	DEFINE_SDP_FACTORY( Base );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
	virtual void	Tick( const float DeltaTime ) { Unused( DeltaTime ); }
};

#endif // SDPBASE_H
