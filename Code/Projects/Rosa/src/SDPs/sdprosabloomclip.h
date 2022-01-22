#ifndef SDPROSABLOOMCLIP_H
#define SDPROSABLOOMCLIP_H

#include "SDPs/sdpbase.h"

class SDPRosaBloomClip : public SDPBase
{
public:
	SDPRosaBloomClip();
	virtual ~SDPRosaBloomClip();

	DEFINE_SDP_FACTORY( RosaBloomClip );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSABLOOMCLIP_H
