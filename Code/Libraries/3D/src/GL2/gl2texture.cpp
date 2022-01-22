#include "core.h"
#include "gl2texture.h"
#include "idatastream.h"
#include "configmanager.h"
#include "mathcore.h"

GL2Texture::GL2Texture()
:	m_Texture( 0 )
{
}

GL2Texture::GL2Texture( GLuint Texture )
:	m_Texture( Texture )
{
}

GL2Texture::~GL2Texture()
{
	if( m_Texture != 0 )
	{
		glDeleteTextures( 1, &m_Texture );
	}
}

/*virtual*/ void* GL2Texture::GetHandle()
{
	return &m_Texture;
}

static GLenum GLImageFormat[] =
{
	0,
	GL_RGBA8,
	GL_RGBA32F,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
};

/*virtual*/ void GL2Texture::CreateTexture( const STextureData& TextureData )
{
	XTRACE_FUNCTION;

	GLGUARD_ACTIVETEXTURE;
	GLGUARD_BINDTEXTURE;

	const uint		MipLevels	= TextureData.m_MipChain.Size();
	const GLenum	Format		= GLImageFormat[ TextureData.m_Format ];
	const GLint		Border		= 0;
	const bool		Compressed	= TextureData.m_Format == EIF_DXT1 ||
								  TextureData.m_Format == EIF_DXT3 ||
								  TextureData.m_Format == EIF_DXT5;

	glGenTextures( 1, &m_Texture );
	ASSERT( m_Texture != 0 );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_Texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MipLevels - 1 );

	// Now fill the texture (and mipmaps)
	for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
	{
		const TTextureMip&	Mip		= TextureData.m_MipChain[ MipLevel ];
		const uint			MinSize	= 1;
		const uint			Width	= Max( MinSize, TextureData.m_Width >> MipLevel );
		const uint			Height	= Max( MinSize, TextureData.m_Height >> MipLevel );

		if( Compressed )
		{
			glCompressedTexImage2D( GL_TEXTURE_2D, MipLevel, Format, Width, Height, Border, Mip.Size(), Mip.GetData() );
		}
		else
		{
			if( Format == GL_RGBA8 )
			{
				glTexImage2D( GL_TEXTURE_2D, MipLevel, Format, Width, Height, Border, GL_BGRA, GL_UNSIGNED_BYTE, Mip.GetData() );
			}
			else if( Format == GL_RGBA32F)
			{
				glTexImage2D( GL_TEXTURE_2D, MipLevel, Format, Width, Height, Border, GL_RGBA, GL_FLOAT, Mip.GetData() );
			}
			else
			{
				WARN;
			}
		}
	}

	GLERRORCHECK;
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

#define DDS_TAG		0x20534444	// 'DDS '
#define DXT1_TAG	0x31545844	// 'DXT1'
#define DXT3_TAG	0x33545844	// 'DXT3'
#define DXT5_TAG	0x35545844	// 'DXT5'

// NOTE: This is functionally identical to D3D9Texture::StaticLoadDDS, just without using DX headers and with GL-specific asserts.
/*static*/ void GL2Texture::StaticLoadDDS( const IDataStream& Stream, STextureData& OutTextureData )
{
	XTRACE_FUNCTION;

	ASSERT( GLEW_EXT_texture_compression_s3tc );

	const uint DDSTag = Stream.ReadUInt32();
	DEVASSERT( DDSTag == DDS_TAG );
	Unused( DDSTag );

	SDDSurfaceFormat DDSFormat;
	Stream.Read( sizeof( SDDSurfaceFormat ), &DDSFormat );
	DEVASSERT( DDSFormat.m_Size == sizeof( SDDSurfaceFormat ) );

	OutTextureData.m_Width	= DDSFormat.m_Width;
	OutTextureData.m_Height	= DDSFormat.m_Height;

	// Resize doesn't construct! So PushBack instead
	const uint MipLevels = Max( static_cast<uint>( 1 ), DDSFormat.m_NumMipMaps );
	for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
	{
		OutTextureData.m_MipChain.PushBack();
	}

	// GL doesn't support DXT2 or DXT4 (premultiplied alpha) formats.
	if( DDSFormat.m_PixelFormat.m_ID == DXT1_TAG )
	{
		OutTextureData.m_Format = EIF_DXT1;
	}
	else if( DDSFormat.m_PixelFormat.m_ID == DXT3_TAG )
	{
		OutTextureData.m_Format = EIF_DXT3;
	}
	else if( DDSFormat.m_PixelFormat.m_ID == DXT5_TAG )
	{
		OutTextureData.m_Format = EIF_DXT5;
	}
	DEVASSERT( OutTextureData.m_Format != EIF_Unknown );

	const uint	FormatBytes	= ( OutTextureData.m_Format == EIF_DXT1 ? 8 : 16 );
	const uint	MinSize		= 1;
	const uint	BlocksWide	= Max( MinSize, DDSFormat.m_Width >> 2 );
	const uint	BlocksHigh	= Max( MinSize, DDSFormat.m_Height >> 2 );

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
