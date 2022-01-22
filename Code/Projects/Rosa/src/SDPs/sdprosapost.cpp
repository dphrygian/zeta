#include "core.h"
#include "sdprosapost.h"
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

SDPRosaPost::SDPRosaPost()
{
}

SDPRosaPost::~SDPRosaPost()
{
}

/*virtual*/ void SDPRosaPost::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const		pFramework			= RosaFramework::GetInstance();
	RosaGame* const				pGame				= pFramework->GetGame();
	const Vector4				Gamma				= Vector4( pGame->GetGamma(), 0.0f, 0.0f, 0.0f );

	// Dirty lens adjustment: scale from 16:9 to appropriate aspect ratio
	const float					Aspect				= CurrentView.GetAspectRatio();
	const float					RelativeAspect		= Aspect / kAspect_16_9;	// ROSATODO: Cache this in view, even though that's a bit of a hack.
	const float					LensUMultiply		= RelativeAspect;
	const float					LensUAdd			= ( 1.0f - RelativeAspect ) * 0.5f;
	const Vector4				LensUVAdjustment	= Vector4( LensUMultiply, 1.0f, LensUAdd, 0.0f );

	// Normalization and control of bloom and other effects
	// ROSATODO: Expose these as graphics sliders if desired (and disable the respective sampler toggles),
	// or use them internally to control post effects by ambient region or whatever.
	static const float			skBloomNormalize	= 1.0f / static_cast<float>( ROSA_BLOOM_PASSES );
	const float					BloomLevel			= 1.0f;
	const float					DirtyLensLevel		= 1.0f;
	const float					FilmGrainLevel		= 1.0f;
	const float					HalosLevel			= pGame->GetHalosEnabled() ? 1.0f : 0.0f;
	const Vector4				PostLevels			= Vector4( BloomLevel * skBloomNormalize, DirtyLensLevel, FilmGrainLevel, HalosLevel );

	// Create a matrix to transform the noise UVs.
	// Add another scale at the front for aspect ratio.
	static const Vector2		NoiseBaseLo			= Vector2( 0.0f, 0.0f );
	static const Vector2		NoiseBaseHi			= Vector2( 1.0f, 1.0f );
	const Vector2				NoiseBase			= Math::Random( NoiseBaseLo, NoiseBaseHi );
	const Matrix				TranslationMatrix	= Matrix::CreateTranslation( NoiseBase );

	const float					NoiseRotation		= Math::Random( 0.0f, TWOPI );
	const Matrix				RotationMatrix		= Matrix::CreateRotationAboutZ( NoiseRotation );

	const Vector2				NoiseScaleRange		= pGame->GetNoiseScaleRange();
	const float					NoiseScale			= Math::Random( NoiseScaleRange.x, NoiseScaleRange.y );
	const Vector				ScaleVector			= Vector( NoiseScale, NoiseScale, NoiseScale );
	const Matrix				ScaleMatrix			= Matrix::CreateScale( ScaleVector );

	const float					AspectRatio			= CurrentView.GetAspectRatio();
	const Vector				AspectVector		= Vector( AspectRatio, 1.0f, 0.0f );
	const Matrix				AspectMatrix		= Matrix::CreateScale( AspectVector );

	const Matrix				NoiseMatrix			= AspectMatrix * ScaleMatrix * RotationMatrix * TranslationMatrix;

	const float					NoiseRange			= pGame->GetNoiseRange();
	const Vector4				NoiseParams			= Vector4( NoiseRange, 0.0f, 0.0f, 0.0f );

	STATIC_HASHED_STRING( Gamma );
	pRenderer->SetPixelShaderUniform( sGamma, Gamma );

	STATIC_HASHED_STRING( LensUVAdjustment );
	pRenderer->SetPixelShaderUniform( sLensUVAdjustment, LensUVAdjustment );

	STATIC_HASHED_STRING( PostLevels );
	pRenderer->SetPixelShaderUniform( sPostLevels, PostLevels );

	STATIC_HASHED_STRING( NoiseMatrix );
	pRenderer->SetPixelShaderUniform( sNoiseMatrix, NoiseMatrix );

	STATIC_HASHED_STRING( NoiseParams );
	pRenderer->SetPixelShaderUniform( sNoiseParams, NoiseParams );
}
