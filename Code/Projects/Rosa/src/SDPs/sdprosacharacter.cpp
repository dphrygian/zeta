#include "core.h"
#include "sdprosacharacter.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "rosamesh.h"
#include "vector4.h"
#include "matrix.h"
#include "Components/wbcomprosacharacterconfig.h"
#include "Components/wbcomprosamesh.h"
#include "Components/wbcompowner.h"

SDPRosaCharacter::SDPRosaCharacter()
{
}

SDPRosaCharacter::~SDPRosaCharacter()
{
}

/*virtual*/ void SDPRosaCharacter::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	SDPBase::SetShaderParameters( pRenderer, pMesh, CurrentView );

	RosaMesh* const						pRosaMesh			= static_cast<RosaMesh*>( pMesh );
	//const HashedString&					Section				= pRosaMesh->GetSection();
	WBEntity* const						pEntity				= pRosaMesh->GetEntity();
	DEVASSERT( pEntity );
	//WBEntity* const						pOwnerEntity		= WBCompOwner::GetTopmostOwner( pEntity );
	//DEVASSERT( pOwnerEntity );
	WBCompRosaMesh* const				pMeshComp			= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMeshComp );
	//WBCompRosaCharacterConfig* const	pCharacterConfig	= WB_GETCOMP( pOwnerEntity, RosaCharacterConfig );
	//DEVASSERT( pCharacterConfig );
	//const Matrix&						Colors				= pCharacterConfig->GetCharacterColors( Section );
	const Vector4						Highlight			= pMeshComp->GetExposureRelativeHighlight();

	STATIC_HASHED_STRING( Highlight );
	pRenderer->SetPixelShaderUniform( sHighlight,		Highlight );

	//STATIC_HASHED_STRING( CharacterColors );
	//pRenderer->SetPixelShaderUniform( sCharacterColors,	Colors );
}
