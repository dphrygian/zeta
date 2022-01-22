#include "core.h"
#include "sdprosapostwatercolor.h"
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
#include "configmanager.h"

SDPRosaPostWatercolor::SDPRosaPostWatercolor()
{
}

SDPRosaPostWatercolor::~SDPRosaPostWatercolor()
{
}

/*virtual*/ void SDPRosaPostWatercolor::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const		pFramework			= RosaFramework::GetInstance();
	RosaGame* const				pGame				= pFramework->GetGame();
	const Vector4				Gamma				= Vector4( pGame->GetGamma(), 0.0f, 0.0f, 0.0f );

	// Dirty lens adjustment: scale from 16:9 to appropriate aspect ratio
	const float					Aspect				= CurrentView.GetAspectRatio();
	const float					RelativeAspect		= Aspect / kAspect_16_9;	// ROSATODO: Cache this in view, even though that's a bit of a hack.
	const float					AspectUMultiply		= RelativeAspect;
	const float					AspectUAdd			= ( 1.0f - RelativeAspect ) * 0.5f;
	const Vector4				AspectUVAdjustmentA	= Vector4( AspectUMultiply, 1.0f, AspectUAdd, 0.0f );
	const Vector4				AspectUVAdjustmentB	= Vector4( 1.0f / AspectUMultiply, 1.0f, 0.0f, 0.0f );

	// Normalization and control of bloom and other effects
	static const float			skBlurNormalize		= 1.0f / static_cast<float>( ROSA_BLOOM_PASSES );
	STATICHASH( Blur );
	const bool					Blur				= ConfigManager::GetBool( sBlur );
	const float					BlurNormalize		= Blur ? skBlurNormalize : 1.0f;	// If we're using main scene, no need to normalize it
	const float					DisplacePct			= pGame->GetDisplacePct();
	const float					EdgeLuminanceMul	= pGame->GetEdgeLuminanceMul();
	const float					EdgeLuminanceAdd	= pGame->GetEdgeLuminanceAdd();
	const Vector4				PostLevels			= Vector4( BlurNormalize, DisplacePct, EdgeLuminanceMul, EdgeLuminanceAdd );
	const Vector4&				WatercolorParams	= pGame->GetWatercolorParams();

	View* const					pMainView			= pFramework->GetMainView();
	const Vector4				ClipValues			= Vector4( -pMainView->GetNearClip(), pMainView->GetFarClip(), pMainView->GetFarClip() - pMainView->GetNearClip(), 0.0f );
	const Vector4&				FogParams			= pGame->GetFogParams();

	STATIC_HASHED_STRING( Gamma );
	pRenderer->SetPixelShaderUniform( sGamma, Gamma );

	STATIC_HASHED_STRING( AspectUVAdjustmentA );
	pRenderer->SetPixelShaderUniform( sAspectUVAdjustmentA, AspectUVAdjustmentA );

	STATIC_HASHED_STRING( AspectUVAdjustmentB );
	pRenderer->SetPixelShaderUniform( sAspectUVAdjustmentB, AspectUVAdjustmentB );

	STATIC_HASHED_STRING( PostLevels );
	pRenderer->SetPixelShaderUniform( sPostLevels, PostLevels );

	STATIC_HASHED_STRING( WatercolorParams );
	pRenderer->SetPixelShaderUniform( sWatercolorParams, WatercolorParams );

	STATIC_HASHED_STRING( ClipValues );
	pRenderer->SetPixelShaderUniform( sClipValues, ClipValues );

	STATIC_HASHED_STRING( FogParams );
	pRenderer->SetPixelShaderUniform( sFogParams, FogParams );
}
