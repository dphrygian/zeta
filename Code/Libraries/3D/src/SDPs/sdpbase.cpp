#include "core.h"
#include "sdpbase.h"
#include "irenderer.h"
#include "mesh.h"
#include "view.h"
#include "bonearray.h"
#include "reversehash.h"
#include "simplestring.h"

SDPBase::SDPBase()
{
}

SDPBase::~SDPBase()
{
}

/*virtual*/ void SDPBase::SetShaderParameters( IRenderer* const pRenderer, Mesh* const pMesh, const View& CurrentView ) const
{
	const Matrix&	WorldMatrix		= pRenderer->GetWorldMatrix();
	const Matrix&	VPMatrix		= pRenderer->GetViewProjectionMatrix();
	const Matrix	WVP				= WorldMatrix * VPMatrix;
	const Vector4	ViewPosition	= CurrentView.GetLocation();

	// Set RT dimensions for vertex shader VPos stuff
	const Vector2	ViewportDims		= pRenderer->GetRenderTargetOrViewportDimensions();
	const Vector4	RTDims				= Vector4( 0.5f * ViewportDims.x, 0.5f * ViewportDims.y, 0.5f / ViewportDims.x, 0.5f / ViewportDims.y );
	const Vector4	HalfPixelOffsetFix	= Vector4( -1.0f / ViewportDims.x, 1.0f / ViewportDims.y, 0.0f, 0.0f );	// Legacy D3D9 half-pixel offset fix

	// Create a reflection vector for fixing normals/tangents in lefty mode; w component is whether to use this
	const Vector4	ReflectVector		= Vector4( CurrentView.GetRotation().GetX(), CurrentView.GetMirrorX() ? 1.0f : 0.0f );

	// Clip planes for pixel shader world space depth normalization
	const Vector4	ClipPlanes			= Vector4( CurrentView.GetNearClip(), CurrentView.GetFarClip(), 1.0f / CurrentView.GetNearClip(), 1.0f / CurrentView.GetFarClip() );

	STATIC_HASHED_STRING( WorldMatrix );
	pRenderer->SetVertexShaderUniform( sWorldMatrix,		WorldMatrix );

	STATIC_HASHED_STRING( VP );
	pRenderer->SetVertexShaderUniform( sVP,					VPMatrix );

	// HACKHACK: Clear this first, because for some reason it fixes rendering glitch. Driver issue?
	STATIC_HASHED_STRING( WVP );
	pRenderer->SetVertexShaderUniform( sWVP,				Matrix() );
	pRenderer->SetVertexShaderUniform( sWVP,				WVP );

	STATIC_HASHED_STRING( ViewPosition );
	pRenderer->SetVertexShaderUniform( sViewPosition,		ViewPosition );
	pRenderer->SetPixelShaderUniform( sViewPosition,		ViewPosition );

	STATIC_HASHED_STRING( ReflectVector );
	pRenderer->SetVertexShaderUniform( sReflectVector,		ReflectVector );

	STATIC_HASHED_STRING( RTDims );
	pRenderer->SetVertexShaderUniform( sRTDims,				RTDims );

	STATIC_HASHED_STRING( HalfPixelOffsetFix );
	pRenderer->SetVertexShaderUniform( sHalfPixelOffsetFix,	HalfPixelOffsetFix );

	STATIC_HASHED_STRING( ClipPlanes );
	pRenderer->SetPixelShaderUniform( sClipPlanes,			ClipPlanes );

	if( pMesh->IsAnimated() )
	{
		DEVASSERT( pMesh->GetAnimationState()->AreBonesUpdated() );

		const float* const	pBoneMatrixFloats	= pMesh->GetAnimationState()->GetBoneMatrixFloats();
		const uint			NumBones			= pMesh->GetBones()->GetNumBones();

		STATIC_HASHED_STRING( BoneMatrices );
		pRenderer->SetVertexShaderMatrices( sBoneMatrices, pBoneMatrixFloats, NumBones );
	}
}
