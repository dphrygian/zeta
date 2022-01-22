#include "core.h"
#include "sdprosaantilight.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosamesh.h"
#include "vector4.h"
#include "Components/wbcomprosaantilight.h"

SDPRosaAntiLight::SDPRosaAntiLight()
{
}

SDPRosaAntiLight::~SDPRosaAntiLight()
{
}

/*virtual*/ void SDPRosaAntiLight::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
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
	RosaMesh* const				pRosaMesh		= static_cast<RosaMesh*>( pMesh );
	WBEntity* const				pEntity			= pRosaMesh->GetEntity();
	WBCompRosaAntiLight* const	pAntiLight		= WB_GETCOMP( pEntity, RosaAntiLight );
	DEBUGASSERT( pAntiLight );
	const Vector4&				AntiLightColor	= pAntiLight->GetColor();
	DEVASSERTDESC( AntiLightColor.a > 0.0f, "Zero radius anti-lights are not allowed!" );

	const float					AntiLightScalar	= pAntiLight->GetImportanceScalar( pMesh, &CurrentView );
	DEVASSERT( AntiLightScalar > 0.0f );	// This shouldn't be rendered at all if it's <= 0.0
	Vector						ScaledAntiLight	= Vector( AntiLightColor ) * AntiLightScalar;
	Vector4						FinalAntiLight	= Vector4( ScaledAntiLight, AntiLightColor.w );	// Keep the radius, only scale down the color

	STATIC_HASHED_STRING( AntiLightPosition );
	const Vector4 AntiLightPosition = pMesh->m_Location;
	pRenderer->SetPixelShaderUniform( sAntiLightPosition, AntiLightPosition );

	STATIC_HASHED_STRING( AntiLightColor );
	pRenderer->SetPixelShaderUniform( sAntiLightColor, FinalAntiLight );
}
