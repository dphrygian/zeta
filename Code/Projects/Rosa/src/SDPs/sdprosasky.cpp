#include "core.h"
#include "sdprosasky.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosamesh.h"
#include "vector4.h"

SDPRosaSky::SDPRosaSky()
{
}

SDPRosaSky::~SDPRosaSky()
{
}

/*virtual*/ void SDPRosaSky::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	const Vector4&			SunVector		= pGame->GetSunVector();
	const Vector4&			SkyColorHi		= pGame->GetSkyColorHi();
	const Vector4&			SkyColorLo		= pGame->GetSkyColorLo();

	STATIC_HASHED_STRING( SunVector );
	pRenderer->SetPixelShaderUniform( sSunVector,	SunVector );

	STATIC_HASHED_STRING( SkyColorHi );
	pRenderer->SetPixelShaderUniform( sSkyColorHi,	SkyColorHi );

	STATIC_HASHED_STRING( SkyColorLo );
	pRenderer->SetPixelShaderUniform( sSkyColorLo,	SkyColorLo );
}
