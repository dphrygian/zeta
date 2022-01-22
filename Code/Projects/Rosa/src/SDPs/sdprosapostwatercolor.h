#ifndef SDPROSAPOSTWATERCOLOR_H
#define SDPROSAPOSTWATERCOLOR_H

#include "SDPs/sdpbase.h"

class SDPRosaPostWatercolor : public SDPBase
{
public:
	SDPRosaPostWatercolor();
	virtual ~SDPRosaPostWatercolor();

	DEFINE_SDP_FACTORY( RosaPostWatercolor );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAPOSTWATERCOLOR_H
