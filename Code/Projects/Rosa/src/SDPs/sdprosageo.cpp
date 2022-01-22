#include "core.h"
#include "sdprosageo.h"
#include "irenderer.h"
#include "mesh.h"
#include "rosamesh.h"
#include "vector4.h"
#include "rosaframework.h"
#include "view.h"

SDPRosaGeo::SDPRosaGeo()
{
}

SDPRosaGeo::~SDPRosaGeo()
{
}

/*virtual*/ void SDPRosaGeo::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	// Pass a null highlight to shader for geo, so I don't need separate gbuffer shader for geo and entities.
	static const Vector4 NullHighlight = Vector4( 1.0f, 1.0f, 1.0f, 0.0f );

	STATIC_HASHED_STRING( Highlight );
	pRenderer->SetPixelShaderUniform( sHighlight,		NullHighlight );

	// View position for transparent Fresnel test
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	View* const				pMainView		= pFramework->GetMainView();
	const Vector4			ViewPosition	= pMainView->GetLocation();

	STATIC_HASHED_STRING( EyePosition );
	pRenderer->SetPixelShaderUniform( sEyePosition,		ViewPosition );
}
