#include "core.h"
#include "sdprosalightcombine.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosamesh.h"
#include "vector4.h"

SDPRosaLightCombine::SDPRosaLightCombine()
{
}

SDPRosaLightCombine::~SDPRosaLightCombine()
{
}

/*virtual*/ void SDPRosaLightCombine::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	const bool				FogEnabled		= pGame->GetFogEnabled();
	const Matrix&			FogColors		= FogEnabled ? pGame->GetFogColors() : Matrix::Zero;
	const Vector4&			FogNearFarCurve	= pGame->GetFogNearFarCurve();
	const Vector4&			FogLoHiCurve	= pGame->GetFogLoHiCurve();
	const Vector4&			FogParams		= pGame->GetFogParams();
	const Vector4&			HeightFogParams	= pGame->GetHeightFogParams();
	const Vector4&			RegionFogScalar	= pGame->GetRegionFogScalar();
	const Vector4&			SunVector		= pGame->GetSunVector();
	View* const				pMainView		= pFramework->GetMainView();
	const Vector4&			ViewPosition	= pMainView->GetLocation();
	const Matrix			VPMatrix		= pMainView->GetViewProjectionMatrix();
	const Matrix			InvVPMatrix		= VPMatrix.GetInverse();

	// Different than the VS ViewPosition (in SDPBase); this is the in-world camera pos, even for FS quads
	STATIC_HASHED_STRING( EyePosition );
	pRenderer->SetPixelShaderUniform( sEyePosition,		ViewPosition );

	STATIC_HASHED_STRING( InvVPMatrix );
	pRenderer->SetPixelShaderUniform( sInvVPMatrix,		InvVPMatrix );

	STATIC_HASHED_STRING( FogColors );
	pRenderer->SetPixelShaderUniform( sFogColors,		FogColors );

	STATIC_HASHED_STRING( FogNearFarCurve );
	pRenderer->SetPixelShaderUniform( sFogNearFarCurve,	FogNearFarCurve );

	STATIC_HASHED_STRING( FogLoHiCurve );
	pRenderer->SetPixelShaderUniform( sFogLoHiCurve,	FogLoHiCurve );

	STATIC_HASHED_STRING( FogParams );
	pRenderer->SetPixelShaderUniform( sFogParams,		FogParams );

	STATIC_HASHED_STRING( HeightFogParams );
	pRenderer->SetPixelShaderUniform( sHeightFogParams,	HeightFogParams );

	STATIC_HASHED_STRING( RegionFogScalar );
	pRenderer->SetPixelShaderUniform( sRegionFogScalar,	RegionFogScalar );

	STATIC_HASHED_STRING( SunVector );
	pRenderer->SetPixelShaderUniform( sSunVector,		SunVector );
}
