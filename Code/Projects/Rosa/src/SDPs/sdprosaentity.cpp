#include "core.h"
#include "sdprosaentity.h"
#include "irenderer.h"
#include "mesh.h"
#include "rosamesh.h"
#include "vector4.h"
#include "Components/wbcomprosamesh.h"
#include "rosaframework.h"
#include "view.h"

SDPRosaEntity::SDPRosaEntity()
{
}

SDPRosaEntity::~SDPRosaEntity()
{
}

/*virtual*/ void SDPRosaEntity::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaMesh* const			pRosaMesh	= static_cast<RosaMesh*>( pMesh );
	WBEntity* const			pEntity		= pRosaMesh->GetEntity();
	DEVASSERT( pEntity );
	WBCompRosaMesh* const	pMeshComp	= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMeshComp );

	const Vector4			Highlight	= pMeshComp->GetExposureRelativeHighlight();

	STATIC_HASHED_STRING( Highlight );
	pRenderer->SetPixelShaderUniform( sHighlight,	Highlight );

	// View position for transparent Fresnel test
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	View* const				pMainView		= pFramework->GetMainView();
	const Vector4			ViewPosition	= pMainView->GetLocation();

	STATIC_HASHED_STRING( EyePosition );
	pRenderer->SetPixelShaderUniform( sEyePosition,		ViewPosition );
}
