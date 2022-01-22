#include "core.h"
#include "sdprosaminimapa.h"
#include "irenderer.h"
#include "mathcore.h"
#include "mesh.h"
#include "rosagame.h"
#include "view.h"

SDPRosaMinimapA::SDPRosaMinimapA()
{
}

SDPRosaMinimapA::~SDPRosaMinimapA()
{
}

/*virtual*/ void SDPRosaMinimapA::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	static const float	skSkewSize	= 1.5f / 8.0f;	// TODO: Configurate as a property of the world? (This is 1.5m offset per 8m down)
	static const float	skSkewYaw	= DEGREES_TO_RADIANS( 45.0f );
	const Angles	SkewAngles		= CurrentView.GetRotation() + Angles( 0.0f, 0.0f, skSkewYaw );
	const Vector	SkewDirection	= skSkewSize * SkewAngles.ToVector2D();
	const Vector4	SkewParams		= Vector4( SkewDirection.x, SkewDirection.y, RosaGame::GetCachedPlayerFeetLocation().z, 0.0f );
	//const Vector4	SkewParams		= Vector4( -skSkewSize, skSkewSize, RosaGame::GetCachedPlayerFeetLocation().z, 0.0f );

	STATIC_HASHED_STRING( MinimapAParams );
	const Vector4&	MinimapAParams	= pMesh->GetShaderConstant( sMinimapAParams );

	STATIC_HASHED_STRING( SkewParams );
	pRenderer->SetVertexShaderUniform( sSkewParams, SkewParams );

	STATIC_HASHED_STRING( Params );
	pRenderer->SetPixelShaderUniform( sParams, MinimapAParams );
}
