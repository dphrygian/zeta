#include "core.h"
#include "d3d9cubemap.h"
#include "configmanager.h"
#include "idatastream.h"
#include "mathcore.h"

#include <d3d9.h>

D3D9Cubemap::D3D9Cubemap( IDirect3DDevice9* D3DDevice )
:	m_CubeTexture( NULL )
,	m_D3DDevice( D3DDevice )
{
}

D3D9Cubemap::D3D9Cubemap( IDirect3DDevice9* D3DDevice, IDirect3DCubeTexture9* Texture )
:	m_CubeTexture( Texture )
,	m_D3DDevice( D3DDevice )
{
}

D3D9Cubemap::~D3D9Cubemap()
{
	SafeRelease( m_CubeTexture );
}

void* D3D9Cubemap::GetHandle()
{
	return m_CubeTexture;
}

// Maps to EImageFormat
static D3DFORMAT D3DImageFormat[] =
{
	D3DFMT_UNKNOWN,
	D3DFMT_A8R8G8B8,
	D3DFMT_A32B32G32R32F,
	D3DFMT_DXT1,
	D3DFMT_DXT3,
	D3DFMT_DXT5,
};

// Ordered X+, X-, Y+, Y-, Z+, Z-
// (or right, left, front, back, up, down)
static D3DCUBEMAP_FACES D3DCubemapFaceTypes[] =
{
	D3DCUBEMAP_FACE_POSITIVE_X,
	D3DCUBEMAP_FACE_NEGATIVE_X,
	D3DCUBEMAP_FACE_POSITIVE_Z,	// Swizzled
	D3DCUBEMAP_FACE_NEGATIVE_Z,	// Swizzled
	D3DCUBEMAP_FACE_POSITIVE_Y,	// Swizzled
	D3DCUBEMAP_FACE_NEGATIVE_Y,	// Swizzled
};

/*virtual*/ void D3D9Cubemap::CreateCubemap( const SCubemapData& CubemapData )
{
	XTRACE_FUNCTION;

	const STextureData&		FirstTextureData	= CubemapData.m_Textures[ 0 ];
	const uint				MipLevels			= FirstTextureData.m_MipChain.Size();
	const D3DFORMAT			Format				= D3DImageFormat[ FirstTextureData.m_Format ];
	const DWORD				Usage				= 0;
	HANDLE* const			pSharedHandle		= NULL;
	const RECT* const		pRect				= NULL;
	const DWORD				Flags				= 0;

	ASSERT( m_D3DDevice );
	ASSERT( FirstTextureData.m_Width == FirstTextureData.m_Height );
	HRESULT hr = m_D3DDevice->CreateCubeTexture( FirstTextureData.m_Width, MipLevels, Usage, Format, D3DPOOL_MANAGED, &m_CubeTexture, pSharedHandle );
	ASSERT( hr == D3D_OK );
	Unused( hr );
	ASSERT( m_CubeTexture );

	for( uint Side = 0; Side < 6; ++Side )
	{
		const STextureData&		TextureData	= CubemapData.m_Textures[ Side ];
		const D3DCUBEMAP_FACES	FaceType	= D3DCubemapFaceTypes[ Side ];

		// Now fill the texture (and mipmaps)
		D3DLOCKED_RECT LockedRect;
		for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
		{
			hr = m_CubeTexture->LockRect( FaceType, MipLevel, &LockedRect, pRect, Flags );
			ASSERT( hr == D3D_OK );

			const TTextureMip& Mip = TextureData.m_MipChain[ MipLevel ];
			memcpy( LockedRect.pBits, Mip.GetData(), Mip.Size() );

			hr = m_CubeTexture->UnlockRect( FaceType, MipLevel );
			ASSERT( hr == D3D_OK );
		}
	}
}
