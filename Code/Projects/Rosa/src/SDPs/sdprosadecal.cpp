#include "core.h"
#include "sdprosadecal.h"
#include "irenderer.h"
#include "matrix.h"
#include "vector4.h"
#include "Components/wbcomprosadecal.h"
#include "rosamesh.h"

SDPRosaDecal::SDPRosaDecal()
{
}

SDPRosaDecal::~SDPRosaDecal()
{
}

/*virtual*/ void SDPRosaDecal::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	const Matrix			WorldMatrix		= pRenderer->GetWorldMatrix();
	const Matrix			InvWorldMatrix	= WorldMatrix.GetInverse();
	const Matrix			InvVPMatrix		= pRenderer->GetViewProjectionMatrix().GetInverse();

	// Set: light color, pos, falloff, eye dir, etc.
	RosaMesh* const			pRosaMesh		= static_cast<RosaMesh*>( pMesh );
	WBEntity* const			pEntity			= pRosaMesh->GetEntity();
	WBCompRosaDecal* const	pDecal			= WB_GETCOMP( pEntity, RosaDecal );
	DEVASSERT( pDecal );

	const Matrix&			NormalBasis		= pDecal->GetNormalBasis();
	const Vector4			DecalAlpha		= Vector4( pDecal->GetAlpha(), 0.0f, 0.0f, 0.0f );

	STATIC_HASHED_STRING( NormalBasis );
	pRenderer->SetPixelShaderUniform(	sNormalBasis,		NormalBasis );

	STATIC_HASHED_STRING( DecalAlpha );
	pRenderer->SetPixelShaderUniform(	sDecalAlpha,		DecalAlpha );

	STATIC_HASHED_STRING( InvWorldMatrix );
	pRenderer->SetPixelShaderUniform(	sInvWorldMatrix,	InvWorldMatrix );

	STATIC_HASHED_STRING( InvVPMatrix );
	pRenderer->SetPixelShaderUniform(	sInvVPMatrix,		InvVPMatrix );
}
