#include "core.h"
#include "sdprosapostcheap.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "vector4.h"
#include "vector2.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "display.h"
#include "matrix.h"
#include "configmanager.h"

SDPRosaPostCheap::SDPRosaPostCheap()
{
}

SDPRosaPostCheap::~SDPRosaPostCheap()
{
}

/*virtual*/ void SDPRosaPostCheap::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const		pFramework			= RosaFramework::GetInstance();
	RosaGame* const				pGame				= pFramework->GetGame();
	const Vector4				Gamma				= Vector4( pGame->GetGamma(), 0.0f, 0.0f, 0.0f );

	STATIC_HASHED_STRING( Gamma );
	pRenderer->SetPixelShaderUniform( sGamma, Gamma );
}
