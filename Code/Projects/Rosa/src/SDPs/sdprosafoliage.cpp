#include "core.h"
#include "sdprosafoliage.h"
#include "irenderer.h"
#include "mesh.h"
#include "rosamesh.h"
#include "vector4.h"
#include "matrix.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "clock.h"
#include "view.h"

SDPRosaFoliage::SDPRosaFoliage()
{
}

SDPRosaFoliage::~SDPRosaFoliage()
{
}

/*virtual*/ void SDPRosaFoliage::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPRosaGeo::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	const Matrix&			WindMatrix		= pGame->GetWindMatrix();

	Vector4					WindPhaseTime	= pGame->GetWindPhaseTime();
	WindPhaseTime.w							= pFramework->GetClock()->GetGameCurrentTime();

	const Vector4&			WindPhaseSpace	= pGame->GetWindPhaseSpace();

	View* const				pMainView		= pFramework->GetMainView();
	const Vector4			ViewPosition	= pMainView->GetLocation();

	STATIC_HASHED_STRING( WindMatrix );
	pRenderer->SetVertexShaderUniform( sWindMatrix,		WindMatrix );

	STATIC_HASHED_STRING( WindPhaseTime );
	pRenderer->SetVertexShaderUniform( sWindPhaseTime,	WindPhaseTime );

	STATIC_HASHED_STRING( WindPhaseSpace );
	pRenderer->SetVertexShaderUniform( sWindPhaseSpace,	WindPhaseSpace );

	STATIC_HASHED_STRING( EyePosition );
	pRenderer->SetVertexShaderUniform( sEyePosition,	ViewPosition );
}
