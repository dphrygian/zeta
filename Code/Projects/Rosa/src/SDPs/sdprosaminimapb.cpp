#include "core.h"
#include "sdprosaminimapb.h"
#include "irenderer.h"
#include "mathcore.h"
#include "mesh.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "vector4.h"
#include "view.h"

SDPRosaMinimapB::SDPRosaMinimapB()
{
}

SDPRosaMinimapB::~SDPRosaMinimapB()
{
}

/*virtual*/ void SDPRosaMinimapB::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
#if ROSA_USE_MAXIMAP
	// HACKHACK: Use Maximap view based on bucket. I was going to use sections,
	// but I didn't feel like converting fullscreen quads to RosaMeshes.
	STATIC_HASHED_STRING( MaximapB );
	const bool				IsMaximap		= ( pMesh->GetBucket() == sMaximapB );
	View* const				pMinimapView	= IsMaximap ? pFramework->GetMaximapView() : pFramework->GetMinimapView();
#else
	View* const				pMinimapView	= pFramework->GetMinimapView();
#endif

	// NOTE: Assuming the MinimapB RT is the same size as the MinimapA RT.
	IRenderTarget*	pRT				= pRenderer->GetCurrentRenderTarget();
	const float		RTWidth			= static_cast<float>( pRT->GetWidth() );
	const float		RTHeight		= static_cast<float>( pRT->GetHeight() );

	const Vector4	Params			= Vector4( 1.0f / RTWidth, 1.0f / RTHeight, 0.0f, pGame->GetMinimapRenderEdges() );
	const Vector4	HeightParams	= Vector4( pGame->GetPlayerLocation().z + pGame->GetMinimapHeightOffset(), pGame->GetMinimapHeightDiffScale(), pGame->GetMinimapHeightToneScale(), pGame->GetMinimapHeightThreshold() );

	// Create a matrix to transform the map texture UVs.
	// Add another scale at the front for aspect ratio.
	Vector2						CenterTranslation	= Vector2( -0.5f, -0.5f );
	const Matrix				CenterMatrix		= Matrix::CreateTranslation( CenterTranslation );

	const float					MinimapTileSize		= pGame->GetMinimapRcpTileSize();
	Vector2						MinimapTranslation	= MinimapTileSize * pMinimapView->GetLocation();
	MinimapTranslation.y							*= -1.0f;		// Fix for texture coordinates
	const Matrix				TranslationMatrix	= Matrix::CreateTranslation( MinimapTranslation );

	const float					MinimapRotation		= -pMinimapView->GetRotation().Yaw;
	const Matrix				RotationMatrix		= Matrix::CreateRotationAboutZ( MinimapRotation );

#if ROSA_USE_MAXIMAP
	const float					ScaledExtent		= MinimapTileSize * ( IsMaximap ? pFramework->GetMaximapViewExtent() : pFramework->GetMinimapViewExtent() );
#else
	const float					ScaledExtent		= MinimapTileSize * pFramework->GetMinimapViewExtent();
#endif
	const Vector				ScaleVector			= Vector( ScaledExtent, ScaledExtent, ScaledExtent );
	const Matrix				ScaleMatrix			= Matrix::CreateScale( ScaleVector );

	const Matrix				MapTexturesMatrix	= CenterMatrix * ScaleMatrix * RotationMatrix * TranslationMatrix;

	// Copied from MinimapA SDP to try to correct the floor texture sampling
	static const float	skSkewSize	= 1.5f / 8.0f;	// TODO: Configurate as a property of the world? (This is 1.5m offset per 8m down)
	static const float	skSkewYaw	= DEGREES_TO_RADIANS( 45.0f );
	const Angles	SkewAngles		= pMinimapView->GetRotation() + Angles( 0.0f, 0.0f, skSkewYaw );
	const Vector	SkewDirection	= MinimapTileSize * skSkewSize * SkewAngles.ToVector2D();
	const Vector4	SkewParams		= Vector4( SkewDirection.x, SkewDirection.y, RosaGame::GetCachedPlayerFeetLocation().z, 0.0f );

	STATIC_HASHED_STRING( Params );
	pRenderer->SetPixelShaderUniform( sParams, Params );

	STATIC_HASHED_STRING( HeightParams );
	pRenderer->SetPixelShaderUniform( sHeightParams, HeightParams );

	STATIC_HASHED_STRING( MapTexturesMatrix );
	pRenderer->SetPixelShaderUniform( sMapTexturesMatrix, MapTexturesMatrix );

	STATIC_HASHED_STRING( SkewParams );
	pRenderer->SetPixelShaderUniform( sSkewParams, SkewParams );
}
