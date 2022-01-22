#include "core.h"
#include "gl2cubemap.h"
#include "idatastream.h"
#include "configmanager.h"
#include "mathcore.h"

GL2Cubemap::GL2Cubemap()
:	m_CubeTexture( 0 )
{
}

GL2Cubemap::GL2Cubemap( GLuint CubeTexture )
:	m_CubeTexture( CubeTexture )
{
}

GL2Cubemap::~GL2Cubemap()
{
	if( m_CubeTexture != 0 )
	{
		glDeleteTextures( 1, &m_CubeTexture );
	}
}

/*virtual*/ void* GL2Cubemap::GetHandle()
{
	return &m_CubeTexture;
}

// Maps to EImageFormat
static GLenum GLImageFormat[] =
{
	0,
	GL_RGBA8,
	GL_RGBA32F,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
};

// Ordered X+, X-, Y+, Y-, Z+, Z-
// (or right, left, front, back, up, down)
static GLenum GLCubemapTarget[] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,	// Swizzled
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,	// Swizzled
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,	// Swizzled
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,	// Swizzled
};

/*virtual*/ void GL2Cubemap::CreateCubemap( const SCubemapData& CubemapData )
{
	XTRACE_FUNCTION;

	GLGUARD_ACTIVETEXTURE;
	GLGUARD_BINDCUBEMAP;

	const STextureData&	FirstTextureData	= CubemapData.m_Textures[ 0 ];
	const uint			MipLevels			= FirstTextureData.m_MipChain.Size();
	const GLenum		Format				= GLImageFormat[ FirstTextureData.m_Format ];
	const GLint			Border				= 0;
	const bool			Compressed			= FirstTextureData.m_Format == EIF_DXT1 ||
											  FirstTextureData.m_Format == EIF_DXT3 ||
											  FirstTextureData.m_Format == EIF_DXT5;

	glGenTextures( 1, &m_CubeTexture );
	ASSERT( m_CubeTexture != 0 );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, m_CubeTexture );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, MipLevels - 1 );

	for( uint Side = 0; Side < 6; ++Side )
	{
		const STextureData&	TextureData	= CubemapData.m_Textures[ Side ];
		const GLenum		Target		= GLCubemapTarget[ Side ];

		// Now fill the texture (and mipmaps)
		for( uint MipLevel = 0; MipLevel < MipLevels; ++MipLevel )
		{
			const TTextureMip&	Mip		= TextureData.m_MipChain[ MipLevel ];
			const uint			Width	= Max( static_cast<uint>( 1 ), TextureData.m_Width >> MipLevel );
			const uint			Height	= Max( static_cast<uint>( 1 ), TextureData.m_Height >> MipLevel );

			if( Compressed )
			{
				glCompressedTexImage2D( Target, MipLevel, Format, Width, Height, Border, Mip.Size(), Mip.GetData() );
			}
			else
			{
				if( Format == GL_RGBA8 )
				{
					glTexImage2D( Target, MipLevel, Format, Width, Height, Border, GL_BGRA, GL_UNSIGNED_BYTE, Mip.GetData() );
				}
				else if( Format == GL_RGBA32F)
				{
					glTexImage2D( Target, MipLevel, Format, Width, Height, Border, GL_RGBA, GL_FLOAT, Mip.GetData() );
				}
				else
				{
					WARN;
				}
			}
		}
	}
}
