#include "core.h"
#include "gl2cuberendertarget.h"
#include "gl2rendertarget.h"
#include "gl2cubemap.h"

GL2CubeRenderTarget::GL2CubeRenderTarget()
:	m_FrameBufferObject( 0 )
,	m_CubemapTextureObject( 0 )
,	m_CubemapTexture( NULL )
,	m_DepthStencilRenderBufferObject( 0 )
,	m_Size( 0 )
{
}

GL2CubeRenderTarget::~GL2CubeRenderTarget()
{
	// Don't clean up m_CubemapTextureObject; deleting the texture does that.
	SafeDelete( m_CubemapTexture );

	if( m_DepthStencilRenderBufferObject != 0 )
	{
		if( GLEW_ARB_framebuffer_object )
		{
			glDeleteRenderbuffers( 1, &m_DepthStencilRenderBufferObject );
			GLERRORCHECK;
		}
		else if( GLEW_EXT_framebuffer_object )
		{
			glDeleteRenderbuffersEXT( 1, &m_DepthStencilRenderBufferObject );
			GLERRORCHECK;
		}
	}

	if( m_FrameBufferObject != 0 )
	{
		if( GLEW_ARB_framebuffer_object )
		{
			glDeleteFramebuffers( 1, &m_FrameBufferObject );
			GLERRORCHECK;
		}
		else if( GLEW_EXT_framebuffer_object )
		{
			glDeleteFramebuffersEXT( 1, &m_FrameBufferObject );
			GLERRORCHECK;
		}
	}
}

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

/*virtual*/ void GL2CubeRenderTarget::Initialize( const SRenderTargetParams& Params )
{
	XTRACE_FUNCTION;

	m_Size	= Params.Width;

	const GLint		MipLevel	= 0;
	const GLint		Border		= 0;
	GLvoid* const	pNullPixels	= NULL;

	if( Params.ColorFormat != ERTF_None )
	{
		GLGUARD_ACTIVETEXTURE;
		GLGUARD_BINDCUBEMAP;

		glActiveTexture( GL_TEXTURE0 );

		glGenTextures( 1, &m_CubemapTextureObject );
		ASSERT( m_CubemapTextureObject != 0 );
		glBindTexture( GL_TEXTURE_CUBE_MAP, m_CubemapTextureObject );

		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0 );
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0 );

		const GLenum ColorFormat = GL2RenderTarget::GetGLFormat( Params.ColorFormat );
		for( uint Side = 0; Side < 6; ++Side )
		{
			const GLenum Target = GLCubemapTarget[ Side ];

			// The image format parameters don't necessarily match the color format,
			// but it doesn't matter because we're not providing image data here.
			glTexImage2D( Target, MipLevel, ColorFormat, Params.Width, Params.Width, Border, GL_BGRA, GL_UNSIGNED_BYTE, pNullPixels );
			GLERRORCHECK;
		}

		m_CubemapTexture = new GL2Cubemap( m_CubemapTextureObject );
	}

	if( Params.DepthStencilFormat != ERTF_None )
	{
		if( GLEW_ARB_framebuffer_object )
		{
			glGenRenderbuffers( 1, &m_DepthStencilRenderBufferObject );
			ASSERT( m_DepthStencilRenderBufferObject != 0 );
			glBindRenderbuffer( GL_RENDERBUFFER, m_DepthStencilRenderBufferObject );
			const GLenum DepthStencilFormat = GL2RenderTarget::GetGLFormat( Params.DepthStencilFormat );
			glRenderbufferStorage( GL_RENDERBUFFER, DepthStencilFormat, Params.Width, Params.Width );
		}
		else if( GLEW_EXT_framebuffer_object )
		{
			glGenRenderbuffersEXT( 1, &m_DepthStencilRenderBufferObject );
			ASSERT( m_DepthStencilRenderBufferObject != 0 );
			glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, m_DepthStencilRenderBufferObject );
			const GLenum DepthStencilFormat = GL2RenderTarget::GetGLFormat( Params.DepthStencilFormat );
			glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, DepthStencilFormat, Params.Width, Params.Width );
		}
	}

	CreateFBO();

	GLERRORCHECK;
}

void GL2CubeRenderTarget::CreateFBO()
{
	// FBOs were supported in GL 2.1 only by extension, but some newer drivers
	// don't still support that extension. Use whichever is available.
	ASSERT( GLEW_EXT_framebuffer_object || GLEW_ARB_framebuffer_object );

	if( GLEW_ARB_framebuffer_object )
	{
		glGenFramebuffers( 1, &m_FrameBufferObject );
		ASSERT( m_FrameBufferObject != 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, m_FrameBufferObject );
		GLERRORCHECK;

		// Just attach the first side now for completeness; we'll swap them later
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_CubemapTextureObject, 0 );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilRenderBufferObject );
		GLERRORCHECK;

		const GLenum FrameBufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER );
		ASSERT( FrameBufferStatus == GL_FRAMEBUFFER_COMPLETE );
		Unused( FrameBufferStatus );
	}
	else if( GLEW_EXT_framebuffer_object )
	{
		glGenFramebuffersEXT( 1, &m_FrameBufferObject );
		ASSERT( m_FrameBufferObject != 0 );
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, m_FrameBufferObject );
		GLERRORCHECK;

		// Just attach the first side now for completeness; we'll swap them later
		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT, m_CubemapTextureObject, 0 );
		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_DepthStencilRenderBufferObject );
		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_DepthStencilRenderBufferObject );
		GLERRORCHECK;

		const GLenum FrameBufferStatus = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
		ASSERT( FrameBufferStatus == GL_FRAMEBUFFER_COMPLETE_EXT );
		Unused( FrameBufferStatus );
	}
}
