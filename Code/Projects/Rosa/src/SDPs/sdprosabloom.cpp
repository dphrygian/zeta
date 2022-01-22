#include "core.h"
#include "sdprosabloom.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "vector4.h"

SDPRosaBloom::SDPRosaBloom()
{
}

SDPRosaBloom::~SDPRosaBloom()
{
}

/*virtual*/ void SDPRosaBloom::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	Display* const			pDisplay		= pFramework->GetDisplay();
	IRenderTarget*			pRT				= pRenderer->GetCurrentRenderTarget();
	RosaGame* const			pGame			= pFramework->GetGame();

	const float				DisplayHeight	= static_cast<float>( pDisplay->m_Height );
	const float				RTHeight		= static_cast<float>( pRT->GetHeight() );
	const float				Ratio			= DisplayHeight / RTHeight;
	const Vector4			BloomRadius		= Ratio * pGame->GetBloomRadius();

	STATIC_HASHED_STRING( BloomStepRadius );
	pRenderer->SetPixelShaderUniform( sBloomStepRadius, BloomRadius );
}
