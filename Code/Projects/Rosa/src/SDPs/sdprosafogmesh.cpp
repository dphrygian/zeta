#include "core.h"
#include "sdprosafogmesh.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosaworld.h"
#include "rosamesh.h"
#include "vector4.h"

SDPRosaFogMesh::SDPRosaFogMesh()
{
}

SDPRosaFogMesh::~SDPRosaFogMesh()
{
}

/*virtual*/ void SDPRosaFogMesh::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	RosaWorld* const		pWorld			= pFramework->GetWorld();
	const Vector4&			FogLightParams	= pGame->GetFogLightParams();	// Only using this for the exposure param
	View* const				pMainView		= pFramework->GetMainView();
	const Matrix			VPMatrix		= pMainView->GetViewProjectionMatrix();
	const Matrix			InvVPMatrix		= VPMatrix.GetInverse();

	STATIC_HASHED_STRING( InvVPMatrix );
	pRenderer->SetPixelShaderUniform( sInvVPMatrix,	InvVPMatrix );

	Vector4 FogMeshColor;
	Vector4 FogMeshParams;
	RosaMesh* const			pRosaMesh		= static_cast<RosaMesh*>( pMesh );
	pWorld->GetFogMeshValues( pRosaMesh->GetSection(), FogMeshColor, FogMeshParams );

	STATIC_HASHED_STRING( FogMeshColor );
	pRenderer->SetPixelShaderUniform( sFogMeshColor, FogMeshColor );

	STATIC_HASHED_STRING( FogMeshParams );
	pRenderer->SetPixelShaderUniform( sFogMeshParams, FogMeshParams );

	// HACKHACK: Only using this for the exposure param
	STATIC_HASHED_STRING( FogLightParams );
	pRenderer->SetPixelShaderUniform( sFogLightParams, FogLightParams );
}
