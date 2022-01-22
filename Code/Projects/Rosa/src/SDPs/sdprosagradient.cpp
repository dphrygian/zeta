#include "core.h"
#include "sdprosagradient.h"
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

SDPRosaGradient::SDPRosaGradient()
{
}

SDPRosaGradient::~SDPRosaGradient()
{
}

/*virtual*/ void SDPRosaGradient::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	View* const				pMainView		= pFramework->GetMainView();
	const Vector4			ViewPosition	= pMainView->GetLocation();
	const Vector4			ViewDirection	= pMainView->GetRotation().ToVector();
	const Matrix			VPMatrix		= pMainView->GetViewMatrix() * pMainView->GetProjectionMatrix();
	const Matrix			InvVPMatrix		= VPMatrix.GetInverse();
	RosaGame* const			pGame			= pFramework->GetGame();
	const Vector4			FogParams		= pGame->GetFogParams();

	// Different than the VS ViewPosition (in SDPBase); this is the in-world camera pos, even for FS quads
	STATIC_HASHED_STRING( EyePosition );
	pRenderer->SetPixelShaderUniform( sEyePosition,		ViewPosition );

	STATIC_HASHED_STRING( EyeDirection );
	pRenderer->SetPixelShaderUniform( sEyeDirection,	ViewDirection );

	STATIC_HASHED_STRING( InvVPMatrix );
	pRenderer->SetPixelShaderUniform( sInvVPMatrix,		InvVPMatrix );

	STATIC_HASHED_STRING( FogParams );
	pRenderer->SetPixelShaderUniform( sFogParams,		FogParams );
}
