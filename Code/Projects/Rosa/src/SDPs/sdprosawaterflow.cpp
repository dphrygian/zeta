#include "core.h"
#include "sdprosawaterflow.h"
#include "irenderer.h"
#include "mesh.h"
#include "rosamesh.h"
#include "vector4.h"
#include "matrix.h"
#include "rosaframework.h"
#include "clock.h"
#include "view.h"
#include "mathcore.h"

SDPRosaWaterFlow::SDPRosaWaterFlow()
{
}

SDPRosaWaterFlow::~SDPRosaWaterFlow()
{
}

/*virtual*/ void SDPRosaWaterFlow::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPRosaGeo::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework			= RosaFramework::GetInstance();

	const float				WaterPhaseTime		= pFramework->GetClock()->GetGameCurrentTime();
	// Since a flow map will eventually cause distortion, cycle each layer separately to restart
	static const float		skCycleTime			= 5.0f;	// Configurate if desired. Also configurate a scalar on phase time as a max flow speed?
	static const float		skHalfCycleTime		= 0.5f * skCycleTime;
	const float				PhaseA				= Mod( WaterPhaseTime, skCycleTime );
	const float				PhaseB				= Mod( WaterPhaseTime + skHalfCycleTime, skCycleTime );
	const float				PhaseBlendT			= Abs( PhaseA - skHalfCycleTime ) / skHalfCycleTime;
	const Vector4			WaterPhase			= Vector4( PhaseA, PhaseB, PhaseBlendT, 0.0f );

	STATIC_HASHED_STRING( WaterPhase );
	pRenderer->SetPixelShaderUniform( sWaterPhase, WaterPhase );
}
