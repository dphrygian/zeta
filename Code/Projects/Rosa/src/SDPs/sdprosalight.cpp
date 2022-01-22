#include "core.h"
#include "sdprosalight.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosamesh.h"
#include "vector4.h"
#include "Components/wbcomprosalight.h"

SDPRosaLight::SDPRosaLight()
{
}

SDPRosaLight::~SDPRosaLight()
{
}

/*virtual*/ void SDPRosaLight::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	// ROSANOTE: For lights, we *could* use CurrentView; but for fullscreen quads, we need
	// to grab the main view instead, so just make that the common pattern.
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	View* const				pMainView		= pFramework->GetMainView();
	const Vector4			ViewPosition	= pMainView->GetLocation();
	const Matrix			VPMatrix		= pMainView->GetViewProjectionMatrix();
	const Matrix			InvVPMatrix		= VPMatrix.GetInverse();

	// Different than the VS ViewPosition (in SDPBase); this is the in-world camera pos, even for FS quads
	STATIC_HASHED_STRING( EyePosition );
	pRenderer->SetPixelShaderUniform( sEyePosition,	ViewPosition );

	STATIC_HASHED_STRING( InvVPMatrix );
	pRenderer->SetPixelShaderUniform( sInvVPMatrix,	InvVPMatrix );

	// Set: light color, pos, falloff, eye dir, etc.
	RosaMesh* const				pRosaMesh	= static_cast<RosaMesh*>( pMesh );
	WBEntity* const				pEntity		= pRosaMesh->GetEntity();
	WBCompRosaLight* const		pLight		= WB_GETCOMP( pEntity, RosaLight );
	DEBUGASSERT( pLight );
	const Vector4&				LightColor	= pLight->GetColor();
	DEVASSERTDESC( LightColor.a > 0.0f, "Zero radius lights are not allowed!" );

	const float					LightScalar	= pLight->GetImportanceScalar( pMesh, &CurrentView );
	DEVASSERT( LightScalar > 0.0f );	// This shouldn't be rendered at all if it's <= 0.0
	Vector						ScaledLight	= Vector( LightColor ) * LightScalar;
	Vector4						FinalLight	= Vector4( ScaledLight, LightColor.w );	// Keep the radius, only scale down the color

	// So shadowing shader knows the range of rendered shadow depth
	// (see also RendererCommon::RenderShadowBucket)
	const Vector	LightExtents	= pMesh->m_AABB.GetExtents();
	const float		LightFar		= Max( LightExtents.x, Max( LightExtents.y, LightExtents.z ) );
	const Vector4	ShadowFarClip	= Vector4( LightFar, 0.0f, 0.0f, 0.0f );

	STATIC_HASHED_STRING( LightPosition );
	const Vector4 LightPosition = pMesh->m_Location;
	pRenderer->SetPixelShaderUniform( sLightPosition, LightPosition );

	STATIC_HASHED_STRING( LightColor );
	pRenderer->SetPixelShaderUniform( sLightColor, FinalLight );

	STATIC_HASHED_STRING( ShadowFarClip );
	pRenderer->SetPixelShaderUniform( sShadowFarClip, ShadowFarClip );
}
