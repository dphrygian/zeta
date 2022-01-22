#include "core.h"
#include "sdprosahud.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"

SDPRosaHUD::SDPRosaHUD()
{
}

SDPRosaHUD::~SDPRosaHUD()
{
}

/*virtual*/ void SDPRosaHUD::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	STATIC_HASHED_STRING( MultiplyColor );
	pRenderer->SetPixelShaderUniform( sMultiplyColor,	pMesh->m_ConstantColor );

	STATIC_HASHED_STRING( ScreenColor );
	pRenderer->SetPixelShaderUniform( sScreenColor,		pMesh->GetShaderConstant( sScreenColor ) );

	STATIC_HASHED_STRING( HUDParams );
	pRenderer->SetPixelShaderUniform( sHUDParams,		pMesh->GetShaderConstant( sHUDParams ) );
}
