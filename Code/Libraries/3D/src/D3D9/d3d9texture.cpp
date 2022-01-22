#include "core.h"
#include "d3d9texture.h"
#include "configmanager.h"
#include "idatastream.h"
#include "mathcore.h"

#include <d3d9.h>
#include <ddraw.h>	// For DDSURFACEDESC2

D3D9Texture::D3D9Texture( IDirect3DDevice9* D3DDevice )
:	m_Texture( NULL )
,	m_D3DDevice( D3DDevice )
{
}

D3D9Texture::D3D9Texture( IDirect3DDevice9* D3DDevice, IDirect3DTexture9* Texture )
:	m_D3DDevice( D3DDevice )
,	m_Texture( Texture )
{
}

D3D9Texture::~D3D9Texture()
{
	SafeRelease( m_Texture );
}

void* D3D9Texture::GetHandle()
{
	return m_Texture;
}

static D3DFORMAT D3DImageFormat[] =
{
	D3DFMT_UNKNOWN,
	D3DFMT_A8R8G8B8,
	D3DFMT_A32B32G32R32F,
	D3DFMT_DXT1,
	D3DFMT_DXT3,
	D3DFMT_DXT5,
};

/*virtual*/ void D3D9Texture::CreateTexture( const STextureData& TextureData )
{
	XTRACE_FUNCTION;

	const uint		MipLevels		= TextureData.m_MipChain.Size();
	const D3DFORMAT	Format			= D3DImageFormat[ TextureData.m_Format ];
	const DWORD		Usage			= 0;
	HANDLE* const	pSharedHandle	= NULL;
	const RECT*		pRect			= NULL;
	const DWORD		Flags			= 0;

	ASSERT( m_D3DDevice );
	HRESULT hr = m_D3DDevice->CreateTexture( TextureData.m_Width, TextureData.m_Height, MipLevels, Usage, Format, D3DPOOL_MANAGED, &m_Texture, pSharedHandle );
	ASSERT( hr == D3D_OK );
	Unused( hr );
	ASSERT( m_Texture );

	// Now fill the texture (and mipmaps)
	D3DLOCKED_RECT LockedRect;
	for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
	{
		hr = m_Texture->LockRect( MipLevel, &LockedRect, pRect, Flags );
		ASSERT( hr == D3D_OK );

		const TTextureMip& Mip = TextureData.m_MipChain[ MipLevel ];
		memcpy( LockedRect.pBits, Mip.GetData(), Mip.Size() );

		hr = m_Texture->UnlockRect( MipLevel );
		ASSERT( hr == D3D_OK );
	}
}

// Mirrors DDCOLORKEY
struct SDDColorKey
{
	uint	m_ColorSpaceLow;
	uint	m_ColorSpaceHigh;
};

// Mirrors DDSCAPS2
struct SDDSurfaceCaps
{
	uint	m_Caps[4];
};

// Mirrors DDPIXELFORMAT
struct SDDPixelFormat
{
	uint	m_Size;
	uint	m_Flags;
	uint	m_ID;
	uint	m_BitCount;
	uint	m_BitMasks[4];
};

// Mirrors DDSURFACEDESC2
struct SDDSurfaceFormat
{
	uint			m_Size;
	uint			m_Flags;
	uint			m_Height;
	uint			m_Width;
	int				m_Pitch;
	uint			m_NumBackBuffers;
	uint			m_NumMipMaps;
	uint			m_AlphaBitDepth;
	uint			m_Reserved;
	uint			m_Surface;	// This is a 32-bit pointer in the original format
	SDDColorKey		m_DestOverlayColorKey;
	SDDColorKey		m_DestBlitColorKey;
	SDDColorKey		m_SrcOverlayColorKey;
	SDDColorKey		m_SrcBlitColorKey;
	SDDPixelFormat	m_PixelFormat;
	SDDSurfaceCaps	m_Caps;
	uint			m_TextureStage;
};

/*static*/ void D3D9Texture::StaticLoadDDS( const IDataStream& Stream, STextureData& OutTextureData )
{
	XTRACE_FUNCTION;

	uint			Magic = Stream.ReadUInt32();
	DEVASSERT( Magic == ' SDD' );
	Unused( Magic );

	SDDSurfaceFormat	SurfaceDesc;
	Stream.Read( sizeof( SDDSurfaceFormat ), &SurfaceDesc );
	DEVASSERT( SurfaceDesc.m_Size == sizeof( SDDSurfaceFormat ) );

	OutTextureData.m_Width	= SurfaceDesc.m_Width;
	OutTextureData.m_Height	= SurfaceDesc.m_Height;

	// Resize doesn't construct! So PushBack instead
	const uint MipLevels = ( SurfaceDesc.m_Flags & DDSD_MIPMAPCOUNT ) ? SurfaceDesc.m_NumMipMaps : 1;
	for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
	{
		OutTextureData.m_MipChain.PushBack();
	}

	DEVASSERT( SurfaceDesc.m_Flags & ( DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT ) );
	DEVASSERT( SurfaceDesc.m_Caps.m_Caps[0] & DDSCAPS_TEXTURE );

	// Assume compressed texture
	DEVASSERT( SurfaceDesc.m_PixelFormat.m_Flags & DDPF_FOURCC );

	// GL doesn't support DXT2 or DXT4, so neither do I.
	if( SurfaceDesc.m_PixelFormat.m_ID == MAKEFOURCC( 'D', 'X', 'T', '1' ) )
	{
		OutTextureData.m_Format = EIF_DXT1;
	}
	else if( SurfaceDesc.m_PixelFormat.m_ID == MAKEFOURCC( 'D', 'X', 'T', '3' ) )
	{
		OutTextureData.m_Format = EIF_DXT3;
	}
	else if( SurfaceDesc.m_PixelFormat.m_ID == MAKEFOURCC( 'D', 'X', 'T', '5' ) )
	{
		OutTextureData.m_Format = EIF_DXT5;
	}
	DEVASSERT( OutTextureData.m_Format != EIF_Unknown );

	// For a compressed texture, the size of each mipmap level image is one-fourth
	// the size of the previous, with a minimum of 8 (DXT1) or 16 (DXT2-5) bytes
	// (for square textures). Use the following formula to calculate the size of
	// each level for a non-square texture:
	// max(1, width x 4) x max(1, height x 4) x 8(DXT1) or 16(DXT2-5)
	// DLP NOTE: That's from the DX docs, but it's definitely supposed to be width and height *divided by* 4

	// Still assuming compressed textures
	const uint	FormatBytes	= ( OutTextureData.m_Format == EIF_DXT1 ? 8 : 16 );
	const uint	MinSize		= 1;
	const uint	BlocksWide	= Max( MinSize, SurfaceDesc.m_Width >> 2 );
	const uint	BlocksHigh	= Max( MinSize, SurfaceDesc.m_Height >> 2 );

	for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
	{
		const uint	MipBlocksWide	= Max( MinSize, BlocksWide >> MipLevel );
		const uint	MipBlocksHigh	= Max( MinSize, BlocksHigh >> MipLevel );
		const uint	ReadBytes		= MipBlocksWide * MipBlocksHigh * FormatBytes;

		TTextureMip& Mip = OutTextureData.m_MipChain[ MipLevel ];
		Mip.Resize( ReadBytes );
		Stream.Read( ReadBytes, Mip.GetData() );
	}
}
