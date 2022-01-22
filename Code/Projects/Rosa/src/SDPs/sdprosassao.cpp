#include "core.h"
#include "sdprosassao.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "vector4.h"
#include "vector2.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "display.h"
#include "matrix.h"

SDPRosaSSAO::SDPRosaSSAO()
{
}

SDPRosaSSAO::~SDPRosaSSAO()
{
}

/*virtual*/ void SDPRosaSSAO::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework			= RosaFramework::GetInstance();
	View* const				pMainView			= pFramework->GetMainView();
	const Vector4			ClipValues			= Vector4( -pMainView->GetNearClip(), pMainView->GetFarClip(), pMainView->GetFarClip() - pMainView->GetNearClip(), 0.0f );

	IRenderTarget*			pRT					= pRenderer->GetCurrentRenderTarget();
	const float				RTHeight			= static_cast<float>( pRT->GetHeight() );

	static const Vector2	skRandomBaseLo		= Vector2( 0.0f, 0.0f );
	static const Vector2	skRandomBaseHi		= Vector2( 1.0f, 1.0f );
	const Vector2			RandomBase			= Math::Random( skRandomBaseLo, skRandomBaseHi );
	const Matrix			TranslationMatrix	= Matrix::CreateTranslation( RandomBase );

	const float				NoiseRotation		= Math::Random( 0.0f, TWOPI );
	const Matrix			RotationMatrix		= Matrix::CreateRotationAboutZ( NoiseRotation );

	const float				RandomScale			= RTHeight / 64.0f;	// HACKHACK: Divide by 64 because the random kernel texture is 64x64.
	const Vector			ScaleVector			= Vector( RandomScale, RandomScale, RandomScale );
	const Matrix			ScaleMatrix			= Matrix::CreateScale( ScaleVector );

	const float				AspectRatio			= CurrentView.GetAspectRatio();
	const Vector			AspectVector		= Vector( AspectRatio, 1.0f, 0.0f );
	const Matrix			AspectMatrix		= Matrix::CreateScale( AspectVector );

	const Matrix			RandomMatrix		= AspectMatrix * ScaleMatrix * RotationMatrix * TranslationMatrix;

	STATIC_HASHED_STRING( RandomMatrix );
	pRenderer->SetPixelShaderUniform( sRandomMatrix,	RandomMatrix );

	const float				OffsetScalarVertical	= 0.04f;	// ZETATODO: Configurate? (I believe this is the max kernel radius relative to vertical resolution, so like 43.2 pixels at 1080p)
	const Vector4			OffsetScalar			= Vector4( AspectRatio * OffsetScalarVertical, OffsetScalarVertical, 0.0f, 0.0f );
	STATIC_HASHED_STRING( OffsetScalar );
	pRenderer->SetPixelShaderUniform( sOffsetScalar,	OffsetScalar );

	// For GetLinearDepth
	STATIC_HASHED_STRING( ClipValues );
	pRenderer->SetPixelShaderUniform( sClipValues,		ClipValues );
}
