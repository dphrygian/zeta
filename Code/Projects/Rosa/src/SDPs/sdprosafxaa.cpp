#include "core.h"
#include "sdprosafxaa.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "vector4.h"
#include "vector2.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "display.h"
#include "matrix.h"

SDPRosaFXAA::SDPRosaFXAA()
{
}

SDPRosaFXAA::~SDPRosaFXAA()
{
}

/*virtual*/ void SDPRosaFXAA::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	const Vector2	ViewportDims	= pRenderer->GetRenderTargetOrViewportDimensions();

	// ROSANOTE: These are different than the usual vertex shader RTDims!
	const Vector4	RTDims			= Vector4( 1.0f / ViewportDims.x, 1.0f / ViewportDims.y, 0.5f / ViewportDims.x, 0.5f / ViewportDims.y );
	const Vector4	RTDims2			= Vector4( -1.0f / ViewportDims.x, -1.0f / ViewportDims.y, 1.0f / ViewportDims.x, 1.0f / ViewportDims.y );

	STATIC_HASHED_STRING( RTDims );
	pRenderer->SetPixelShaderUniform( sRTDims, RTDims );

	STATIC_HASHED_STRING( RTDims2 );
	pRenderer->SetPixelShaderUniform( sRTDims2, RTDims2 );
}
