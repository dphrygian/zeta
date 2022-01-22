#ifndef SDPROSAHUDCALIBRATION_H
#define SDPROSAHUDCALIBRATION_H

#include "SDPs/sdprosahud.h"

class SDPRosaHUDCalibration : public SDPRosaHUD
{
public:
	SDPRosaHUDCalibration();
	virtual ~SDPRosaHUDCalibration();

	DEFINE_SDP_FACTORY( RosaHUDCalibration );

	virtual void	SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const;
};

#endif // SDPROSAHUDCALIBRATION_H
