#ifndef SDPROSACHARACTER_H
#define SDPROSACHARACTER_H

#include "SDPs/sdpbase.h"

class SDPRosaCharacter : public SDPBase
{
public:
	SDPRosaCharacter();
	virtual ~SDPRosaCharacter();

	DEFINE_SDP_FACTORY( RosaCharacter );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSACHARACTER_H
