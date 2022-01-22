#include "core.h"
#include "sdprosafoglight.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosamesh.h"
#include "vector4.h"
#include "Components/wbcomprosalight.h"

SDPRosaFogLight::SDPRosaFogLight()
{
}

SDPRosaFogLight::~SDPRosaFogLight()
{
}

/*virtual*/ void SDPRosaFogLight::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	// ROSANOTE: For lights, we *could* use CurrentView; but for fullscreen quads, we need
	// to grab the main view instead, so just make that the common pattern.
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	const Vector4&			FogLightParams	= pGame->GetFogLightParams();
	View* const				pMainView		= pFramework->GetMainView();
	const Matrix			VPMatrix		= pMainView->GetViewProjectionMatrix();
	const Matrix			InvVPMatrix		= VPMatrix.GetInverse();

	STATIC_HASHED_STRING( InvVPMatrix );
	pRenderer->SetPixelShaderUniform( sInvVPMatrix,	InvVPMatrix );

	// Set: light color, pos, falloff, eye dir, etc.
	RosaMesh* const				pRosaMesh	= static_cast<RosaMesh*>( pMesh );
	WBEntity* const				pEntity		= pRosaMesh->GetEntity();
	WBCompRosaLight* const		pLight		= WB_GETCOMP( pEntity, RosaLight );
	DEBUGASSERT( pLight );
	Vector4						LightColor	= pLight->GetColor();
	const float					FogRadius	= pLight->GetFogRadius();
	DEVASSERT( FogRadius > 0.0f );
	const float					FogScalar	= pLight->GetFogValueScalar();
	DEVASSERT( FogScalar > 0.0f );
	const float					LightScalar	= pLight->GetImportanceScalar( pMesh, &CurrentView );
	DEVASSERT( LightScalar > 0.0f );	// This shouldn't be rendered at all if it's <= 0.0
	LightColor								= LightColor * LightScalar * FogScalar;
	LightColor.w							= FogRadius;

	STATIC_HASHED_STRING( LightPosition );
	const Vector4 LightPosition = pMesh->m_Location;
	pRenderer->SetPixelShaderUniform( sLightPosition, LightPosition );

	STATIC_HASHED_STRING( LightColor );
	pRenderer->SetPixelShaderUniform( sLightColor, LightColor );

	STATIC_HASHED_STRING( FogLightParams );
	pRenderer->SetPixelShaderUniform( sFogLightParams, FogLightParams );
}
