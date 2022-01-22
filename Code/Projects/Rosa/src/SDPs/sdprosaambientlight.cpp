#include "core.h"
#include "sdprosaambientlight.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "vector4.h"
#include "rosaworld.h"
#include "rosaworldcubemap.h"

SDPRosaAmbientLight::SDPRosaAmbientLight()
{
}

SDPRosaAmbientLight::~SDPRosaAmbientLight()
{
}

/*virtual*/ void SDPRosaAmbientLight::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

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

	const Material&			CubemapMaterial	= pMesh->HasMultiPassMaterials() ? pMesh->GetMultiPassMaterial( 1 ) : pMesh->GetMaterial();
	RosaWorldCubemap* const	pCubemap		= static_cast<RosaWorldCubemap*>( CubemapMaterial.GetTexture( 3 ) );
	const SVoxelIrradiance&	Irradiance		= pCubemap->GetIrradiance();

	STATIC_HASHED_STRING( LightCube );
	pRenderer->SetPixelShaderFloat4s( sLightCube,	Irradiance.m_Light[0].GetArray(),	6 );
}
