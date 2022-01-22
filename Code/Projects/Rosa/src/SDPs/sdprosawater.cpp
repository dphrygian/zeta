#include "core.h"
#include "sdprosawater.h"
#include "irenderer.h"
#include "mesh.h"
#include "rosamesh.h"
#include "vector4.h"
#include "matrix.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "clock.h"
#include "view.h"

SDPRosaWater::SDPRosaWater()
{
}

SDPRosaWater::~SDPRosaWater()
{
}

/*virtual*/ void SDPRosaWater::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPRosaGeo::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	const Vector4&			WindWaterVector	= pGame->GetWindWaterVector();

	const float				WaterPhaseTime	= pFramework->GetClock()->GetGameCurrentTime();
	const Vector4			WaterPhase		= WindWaterVector * WaterPhaseTime;

	STATIC_HASHED_STRING( WaterPhase );
	pRenderer->SetPixelShaderUniform( sWaterPhase, WaterPhase );
}
