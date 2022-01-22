#include "core.h"
#include "sdprosabloomclip.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "vector4.h"

SDPRosaBloomClip::SDPRosaBloomClip()
{
}

SDPRosaBloomClip::~SDPRosaBloomClip()
{
}

/*virtual*/ void SDPRosaBloomClip::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	RosaGame* const			pGame		= pFramework->GetGame();
	const Vector4&			BloomParams	= pGame->GetBloomParams();

	STATIC_HASHED_STRING( BloomParams );
	pRenderer->SetPixelShaderUniform( sBloomParams, BloomParams );
}
