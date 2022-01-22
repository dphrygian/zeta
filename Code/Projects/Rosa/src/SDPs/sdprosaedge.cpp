#include "core.h"
#include "sdprosaedge.h"
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

SDPRosaEdge::SDPRosaEdge()
{
}

SDPRosaEdge::~SDPRosaEdge()
{
}

// ZETANOTE: This is now doing basically everything with fog that light-combine does, again.
// I wonder if I could merge those two somehow... but light-combine is already sampling a lot.
/*virtual*/ void SDPRosaEdge::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	// Set RT dimensions for vertex shader VPos stuff
	// Our default RT doesn't actually have dimensions in it >_<
	// So use display instead.
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	Display* const			pDisplay		= pFramework->GetDisplay();
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
	const Vector4			ViewPosition	= pMainView->GetLocation();
	const Matrix&			VPMatrix		= pMainView->GetViewProjectionMatrix();
	const Matrix			InvVPMatrix		= VPMatrix.GetInverse();
	const Vector4			ClipValues		= Vector4( -pMainView->GetNearClip(), pMainView->GetFarClip(), pMainView->GetFarClip() - pMainView->GetNearClip(), 0.0f );

	// ROSANOTE: These are different than the usual vertex shader RTDims!
	const float		RTWidth		= static_cast<float>( pDisplay->m_Width );
	const float		RTHeight	= static_cast<float>( pDisplay->m_Height );
	const Vector4	RTDims		= Vector4( 1.0f / RTWidth, 1.0f / RTHeight, 0.5f / RTWidth, 0.5f / RTHeight );
	const Vector4	RTDims2		= Vector4( -1.0f / RTWidth, -1.0f / RTHeight, 1.0f / RTWidth, 1.0f / RTHeight );

	const Vector4&	BackColor	= pGame->GetEdgeBackColor();
	const Vector4&	EdgeColor	= pGame->GetEdgeColor();

	STATIC_HASHED_STRING( RTDims );
	pRenderer->SetPixelShaderUniform( sRTDims,			RTDims );

	STATIC_HASHED_STRING( RTDims2 );
	pRenderer->SetPixelShaderUniform( sRTDims2,			RTDims2 );

	STATIC_HASHED_STRING( ClipValues );
	pRenderer->SetPixelShaderUniform( sClipValues,		ClipValues );

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

	STATIC_HASHED_STRING( BackColor );
	pRenderer->SetPixelShaderUniform( sBackColor,		BackColor );

	STATIC_HASHED_STRING( EdgeColor );
	pRenderer->SetPixelShaderUniform( sEdgeColor,		EdgeColor );
}
