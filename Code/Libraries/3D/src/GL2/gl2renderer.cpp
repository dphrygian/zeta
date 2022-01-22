#include "core.h"
#include "gl2renderer.h"
#include "gl2vertexbuffer.h"
#include "gl2indexbuffer.h"
#include "gl2texture.h"
#include "gl2cubemap.h"
#include "gl2vertexdeclaration.h"
#include "gl2rendertarget.h"
#include "gl2cuberendertarget.h"
#include "gl2vertexshader.h"
#include "gl2pixelshader.h"
#include "gl2shaderprogram.h"
#include "simplestring.h"
#include "vector4.h"
#include "bucket.h"
#include "mesh.h"
#include "configmanager.h"
#include "packstream.h"
#include "windowwrapper.h"
#include "mathcore.h"

// I'm making this easy to compile out because GL, with its global state,
// might be causing problems if I create new objects after setting render state.
#define IGNORE_REDUNDANT_STATE			1

// This is supported separately, with state shadowed on each *texture*.
// (See GL2Texture::GetSamplerState and GL2Cubemap::GetSamplerState.)
#define IGNORE_REDUNDANT_SAMPLER_STATE	1

GL2Renderer::GL2Renderer( Window* const pWindow, Display* const pDisplay )
:	m_Window( pWindow )
,	m_RenderContext( NULL )
,	m_MaxAnisotropy( 0.0f )
{
	SetDisplay( pDisplay );
}

GL2Renderer::~GL2Renderer()
{
	GLERRORCHECK;

	{
#if BUILD_WINDOWS_NO_SDL
		const BOOL Success	= wglMakeCurrent( NULL, NULL );
		Unused( Success );
		ASSERT( Success == TRUE );
#endif
#if BUILD_SDL
		const int Error = SDL_GL_MakeCurrent( NULL, NULL );
		Unused( Error );
		ASSERT( Error == 0 );
#endif
	}

	if( m_RenderContext != NULL )
	{
#if BUILD_WINDOWS_NO_SDL
		const BOOL Success = wglDeleteContext( m_RenderContext );
		Unused( Success );
		ASSERT( Success == TRUE );
#endif
#if BUILD_SDL
		SDL_GL_DeleteContext( m_RenderContext );
#endif
	}
}

void GL2Renderer::Initialize()
{
	RendererCommon::Initialize();

#if BUILD_WINDOWS_NO_SDL
	{
		PIXELFORMATDESCRIPTOR PixelFormatDescriptor = { 0 };

		// TODO PORT: Choose flags to match D3D settings. (Also, maybe move this to a "GetPresentParams" kind of function.)
		// TODO SDL: Do I need to do anything like this? It seemed to work just fine without it.
		PixelFormatDescriptor.nSize			= sizeof( PIXELFORMATDESCRIPTOR );
		PixelFormatDescriptor.nVersion		= 1;
		PixelFormatDescriptor.dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		PixelFormatDescriptor.iPixelType	= PFD_TYPE_RGBA;
		PixelFormatDescriptor.cColorBits	= 24;
		PixelFormatDescriptor.cAlphaBits	= 8;
		PixelFormatDescriptor.cDepthBits	= 32;
		PixelFormatDescriptor.iLayerType	= PFD_MAIN_PLANE;

		const int	PixelFormat	= ChoosePixelFormat( m_Window->GetHDC(), &PixelFormatDescriptor );
		ASSERT( PixelFormat > 0 );

		const BOOL	Success		= SetPixelFormat( m_Window->GetHDC(), PixelFormat, &PixelFormatDescriptor );
		Unused( Success );
		ASSERT( Success == TRUE );
	}
#endif // BUILD_WINDOWS_NO_SDL

#if BUILD_WINDOWS_NO_SDL
	m_RenderContext = wglCreateContext( m_Window->GetHDC() );
	ASSERT( m_RenderContext != NULL );
#endif
#if BUILD_SDL
	m_RenderContext = SDL_GL_CreateContext( m_Window->GetSDLWindow() );
	ASSERT( m_RenderContext != NULL );
#endif

	{
#if BUILD_WINDOWS_NO_SDL
		const BOOL Success	= wglMakeCurrent( m_Window->GetHDC(), m_RenderContext );
		Unused( Success );
		ASSERT( Success == TRUE );
#endif
#if BUILD_SDL
		const int Error = SDL_GL_MakeCurrent( m_Window->GetSDLWindow(), m_RenderContext );
		Unused( Error );
		ASSERT( Error == 0 );
#endif
	}

	{
		const GLenum Error		= glewInit();
		Unused( Error );
		ASSERT( Error == GLEW_OK );
	}

	// TODO: Move to a "test capabilities" kind of function
	{
		ASSERT( GLEW_VERSION_2_1 );
		ASSERT( GLEW_EXT_framebuffer_object || GLEW_ARB_framebuffer_object );
		ASSERT( GLEW_EXT_vertex_array_bgra || GLEW_ARB_vertex_array_bgra );
		ASSERT( GLEW_EXT_texture_compression_s3tc );
		ASSERT( GLEW_EXT_texture_filter_anisotropic );
		STATICASSERT( GL_EXT_texture_cube_map || GL_ARB_texture_cube_map );
#if BUILD_WINDOWS_NO_SDL
		ASSERT( WGLEW_EXT_swap_control );
#endif
	}

	glFrontFace( GL_CCW );
	GLint MaxVertexAttribs;
	glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &MaxVertexAttribs );
	m_MaxVertexAttribs = MaxVertexAttribs;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_MaxAnisotropy );

	ASSERT( m_Display );
	SetVSync( m_Display->m_VSync );

	m_DefaultRenderTarget = new GL2RenderTarget;
}

bool GL2Renderer::SupportsSM3()
{
	// SM2/3 doesn't have any meaning in OpenGL.
	return true;
}

bool GL2Renderer::SupportsSM2()
{
	// SM2/3 doesn't have any meaning in OpenGL.
	return true;
}

/*virtual*/ ERenderTargetFormat GL2Renderer::GetBestSupportedRenderTargetFormat( const ERenderTargetFormat Format ) const
{
	// TODO PORT LATER GetBestSupportedRenderTargetFormat
	// There's a good chance I don't need this, because the OpenGL spec allows the implementation to
	// automatically demote the requested format to a supported format when needed.
	Unused( Format );
	return ERTF_Unknown;
}

/*virtual*/ void GL2Renderer::SetVSync( const bool VSync )
{
	const int SwapInterval = VSync ? 1 : 0;
#if BUILD_WINDOWS_NO_SDL
	wglSwapIntervalEXT( SwapInterval );
#elif BUILD_SDL
	SDL_GL_SetSwapInterval( SwapInterval );
#else
#error Unknown system
#endif
}

void GL2Renderer::SetMultiSampleType( EMultiSampleType MultiSampleType )
{
	// TODO PORT LATER SetMultiSampleType
	Unused( MultiSampleType );
}

void GL2Renderer::GetBestSupportedMultiSampleType( EMultiSampleType& OutMultiSampleType, c_uint32* pOutQualityLevels /*= NULL*/ )
{
	// TODO PORT LATER GetBestSupportedMultiSampleType
	Unused( OutMultiSampleType );
	Unused( pOutQualityLevels );
}

bool GL2Renderer::CanReset()
{
	return true;
}

bool GL2Renderer::Reset()
{
	ASSERT( m_Display );
	SetVSync( m_Display->m_VSync );

	return true;
}

/*virtual*/ void GL2Renderer::SetRestoreDeviceCallback( const SRestoreDeviceCallback& Callback )
{
	Unused( Callback );
}

/*virtual*/ void GL2Renderer::Refresh()
{
#if BUILD_SDL
	// On Linux, at least, SDL needs to be set as the current context *after* the
	// GL window is shown, so this allows be to do it there.
	if( m_Window && m_Window->GetSDLWindow() && m_RenderContext )
	{
		const int Error = SDL_GL_MakeCurrent( m_Window->GetSDLWindow(), m_RenderContext );
		Unused( Error );
		ASSERT( Error == 0 );
	}
#endif
}

void GL2Renderer::Tick()
{
	XTRACE_FUNCTION;

	PROFILE_FUNCTION;

#if BUILD_DEV
	m_DEV_RenderStats.Reset();
	RESET_CLOCK( m_DEV_RenderStats.m_RenderTime );
	START_CLOCK( m_DEV_RenderStats.m_RenderTime );
#endif

	RenderBuckets();
	PostRenderBuckets();

#if BUILD_DEV
	STOP_CLOCK( m_DEV_RenderStats.m_RenderTime );
#endif

	Present();

#if BUILD_DEV
	m_DEV_RenderStats.UpdateFPS();
#endif

#if BUILD_DEBUG
	STATIC_HASHED_STRING( Render );
	DEBUGCATPRINTF( sRender, 2, "%d meshes, %s draw calls, %d primitives\n", m_DEV_RenderStats.m_NumMeshes, m_DEV_RenderStats.m_NumDrawCalls, m_DEV_RenderStats.m_NumPrimitives );
#endif
}

/*virtual*/ void GL2Renderer::SetVertexArrays( Mesh* const pMesh )
{
	XTRACE_FUNCTION;

	const uint	VertexSignature	= pMesh->m_VertexDeclaration->GetSignature();
	uint		Index			= 0;

#define SETSTREAM( STREAM, SIGNATURE, NUMCOMPONENTS, COMPONENTTYPE, NORMALIZE )				\
	if( SIGNATURE == ( VertexSignature & SIGNATURE ) )										\
	{																						\
		const GLuint VBO = *static_cast<GLuint*>( pMesh->m_VertexBuffer->Get##STREAM() );	\
		DEVASSERT( VBO != 0 );																\
		glBindBuffer( GL_ARRAY_BUFFER, VBO );												\
		glEnableVertexAttribArray( Index );													\
		glVertexAttribPointer( Index, NUMCOMPONENTS, COMPONENTTYPE, NORMALIZE, 0, NULL );	\
		++Index;																			\
	}

	SETSTREAM( Positions,	VD_POSITIONS,	3,			GL_FLOAT,			GL_FALSE );
	SETSTREAM( Colors,		VD_COLORS,		GL_BGRA,	GL_UNSIGNED_BYTE,	GL_TRUE );	// GL_BGRA as glVertexAttribPointer size parameter is a hack for swizzling (GL_EXT_vertex_array_bgra/GL_ARB_vertex_array_bgra)
	SETSTREAM( FloatColors,	VD_FLOATCOLORS,	4,			GL_FLOAT,			GL_FALSE );
	SETSTREAM( UVs,			VD_UVS,			2,			GL_FLOAT,			GL_FALSE );
	SETSTREAM( Normals,		VD_NORMALS,		3,			GL_FLOAT,			GL_FALSE );
	SETSTREAM( NormalsB,	VD_NORMALS_B,	3,			GL_FLOAT,			GL_FALSE );
	SETSTREAM( Tangents,	VD_TANGENTS,	4,			GL_FLOAT,			GL_FALSE );
	SETSTREAM( BoneIndices,	VD_BONEINDICES,	4,			GL_UNSIGNED_BYTE,	GL_FALSE );
	SETSTREAM( BoneWeights,	VD_BONEWEIGHTS,	4,			GL_UNSIGNED_BYTE,	GL_TRUE );

#undef SETSTREAM

	// Disable all the attributes we're not using.
	for( uint DisableAttribIndex = Index; DisableAttribIndex < m_MaxVertexAttribs; ++DisableAttribIndex )
	{
		glDisableVertexAttribArray( DisableAttribIndex );
	}

	const GLuint IBO = *static_cast<GLuint*>( pMesh->m_IndexBuffer->GetIndices() );
	DEVASSERT( IBO != 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, IBO );
}

void GL2Renderer::Present()
{
	PROFILE_FUNCTION;

	XTRACE_FUNCTION;

#if BUILD_WINDOWS_NO_SDL
	const BOOL Success = SwapBuffers( m_Window->GetHDC() );
	Unused( Success );
	ASSERT( Success == TRUE );
#endif
#if BUILD_SDL
	SDL_GL_SwapWindow( m_Window->GetSDLWindow() );
#endif
}

void GL2Renderer::Clear( const uint Flags, const uint Color /*= 0xff000000*/, const float Depth /*= 1.0f*/, const uint Stencil /*= 0*/ )
{
	XTRACE_FUNCTION;

	if( Flags == CLEAR_NONE )
	{
		return;
	}

	// HACKHACK: GL global state strikes again! If the last material was disabling
	// a buffer write, glClear won't write to that buffer either. So fix that.
	// (NOTE: I could also maybe use glPush/PopAttrib( GL_<x>_BUFFER_BIT ), but
	// the performance and support for that is unclear, and this works.)
	bool FixColorMask = false;
	bool FixDepthMask = false;
	bool FixStencilMask = false;

	GLbitfield GLFlags = 0;
	if( Flags & CLEAR_COLOR )
	{
		GLFlags |= GL_COLOR_BUFFER_BIT;

		Vector4 ClearColor = Color;
		glClearColor( ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a );

		if( m_RenderState.m_ColorWriteEnable == EE_False )
		{
			FixColorMask = true;
		}
	}

	if( Flags & CLEAR_DEPTH )
	{
		GLFlags |= GL_DEPTH_BUFFER_BIT;
		glClearDepth( Depth );

		if( m_RenderState.m_ZWriteEnable == EE_False )
		{
			FixDepthMask = true;
		}
	}

	if( Flags & CLEAR_STENCIL )
	{
		GLFlags |= GL_STENCIL_BUFFER_BIT;
		glClearStencil( Stencil );

		if( m_RenderState.m_StencilWriteMask != 0xffffffff )
		{
			FixStencilMask = true;
		}
	}

	if( FixColorMask )		{ glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ); }
	if( FixDepthMask )		{ glDepthMask( GL_TRUE ); }
	if( FixStencilMask )	{ glStencilMask( 0xffffffff ); }

	glClear( GLFlags );

	if( FixColorMask )		{ glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE ); }
	if( FixDepthMask )		{ glDepthMask( GL_FALSE ); }
	if( FixStencilMask )	{ glStencilMask( m_RenderState.m_StencilWriteMask ); }
}

IVertexBuffer* GL2Renderer::CreateVertexBuffer()
{
	return new GL2VertexBuffer;
}

IVertexDeclaration* GL2Renderer::CreateVertexDeclaration()
{
	return new GL2VertexDeclaration;
}

IIndexBuffer* GL2Renderer::CreateIndexBuffer()
{
	return new GL2IndexBuffer;
}

ITexture* GL2Renderer::CreateTexture( const char* Filename, const bool NoMips )
{
	XTRACE_FUNCTION;

	GL2Texture* const pTexture = new GL2Texture;
	pTexture->Initialize( Filename, NoMips );
	return pTexture;
}

/*virtual*/ ITexture* GL2Renderer::CreateCubemap( const SimpleString& CubemapDef, const bool NoMips )
{
	XTRACE_FUNCTION;

	GL2Cubemap* const pCubemap = new GL2Cubemap;
	pCubemap->Initialize( CubemapDef, NoMips );
	return pCubemap;
}

/*virtual*/ ITexture* GL2Renderer::CreateCubemap( const SCubemapData& CubemapData )
{
	XTRACE_FUNCTION;

	GL2Cubemap* const pCubemap = new GL2Cubemap;
	pCubemap->CreateCubemap( CubemapData );
	return pCubemap;
}

/*virtual*/ IVertexShader* GL2Renderer::CreateVertexShader( const SimpleString& Filename )
{
	GL2VertexShader* const pVertexShader = new GL2VertexShader;
	pVertexShader->Initialize( PackStream( Filename.CStr() ) );
	return pVertexShader;
}

/*virtual*/ IPixelShader* GL2Renderer::CreatePixelShader( const SimpleString& Filename )
{
	GL2PixelShader* const pPixelShader = new GL2PixelShader;
	pPixelShader->Initialize( PackStream( Filename.CStr() ) );
	return pPixelShader;
}

/*virtual*/ IShaderProgram* GL2Renderer::CreateShaderProgram( IVertexShader* const pVertexShader, IPixelShader* const pPixelShader, IVertexDeclaration* const pVertexDeclaration )
{
	GL2ShaderProgram* const pShaderProgram = new GL2ShaderProgram;
	pShaderProgram->Initialize( pVertexShader, pPixelShader, pVertexDeclaration );
	return pShaderProgram;
}

IRenderTarget* GL2Renderer::CreateRenderTarget( const SRenderTargetParams& Params )
{
	GL2RenderTarget* pTarget = new GL2RenderTarget;
	pTarget->Initialize( Params );
	m_RenderTargets.PushBack( pTarget );
	return pTarget;
}

IRenderTarget* GL2Renderer::CreateRenderTarget()
{
	GL2RenderTarget* pTarget = new GL2RenderTarget;
	m_RenderTargets.PushBack( pTarget );
	return pTarget;
}

IRenderTarget* GL2Renderer::CreateCubeRenderTarget( const SRenderTargetParams& Params )
{
	GL2CubeRenderTarget* pTarget = new GL2CubeRenderTarget;
	pTarget->Initialize( Params );
	m_RenderTargets.PushBack( pTarget );
	return pTarget;
}

void GL2Renderer::SetRenderTarget( IRenderTarget* const pRenderTarget )
{
	XTRACE_FUNCTION;

	m_CurrentRenderTarget = pRenderTarget;

	const GLuint FrameBufferObject = *static_cast<GLuint*>( m_CurrentRenderTarget->GetHandle() );

	ASSERT( GLEW_ARB_framebuffer_object || GLEW_EXT_framebuffer_object );
	if( GLEW_ARB_framebuffer_object )
	{
		// Bind to GL_FRAMEBUFFER, which does both drawing and readback calls,
		// because readback is necessary for Steam overlay to capture screenshots.
		glBindFramebuffer( GL_FRAMEBUFFER, FrameBufferObject );

		if( FrameBufferObject == 0 )	// Handle the default framebuffer
		{
			// Draw to the backbuffer since we're always double buffered
			// (Left/right refers to stereo rendering.)
			glDrawBuffer( GL_BACK_LEFT );
		}
		else
		{
			// Set the draw buffers; hard-coded to a max of four, because really. (See also MAX_MRT_COUNT)
			const GLenum pDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers( pRenderTarget->GetNumSurfaces(), pDrawBuffers );
		}
	}
	else if( GLEW_EXT_framebuffer_object )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, FrameBufferObject );

		if( FrameBufferObject == 0 )
		{
			glDrawBuffer( GL_BACK );
		}
		else
		{
			const GLenum pDrawBuffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT };
			glDrawBuffers( pRenderTarget->GetNumSurfaces(), pDrawBuffers );	// There is no glDrawBuffersEXT, it's a core function
		}
	}

	// GL also requires manually setting the viewport for RTs.
	{
		ASSERT( m_Display );
		const uint ViewportWidth	= ( FrameBufferObject == 0 ) ? m_Display->m_FrameWidth : m_CurrentRenderTarget->GetWidth();
		const uint ViewportHeight	= ( FrameBufferObject == 0 ) ? m_Display->m_FrameHeight : m_CurrentRenderTarget->GetHeight();
		glViewport( 0, 0, ViewportWidth, ViewportHeight );
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

void GL2Renderer::SetCubeRenderTarget( IRenderTarget* const pRenderTarget, const uint Face )
{
	XTRACE_FUNCTION;

	m_CurrentRenderTarget = pRenderTarget;

	const GLuint FrameBufferObject = *static_cast<GLuint*>( m_CurrentRenderTarget->GetHandle() );
	DEVASSERT( FrameBufferObject > 0 );	// Default framebuffer isn't a cube target

	const GLuint CubemapTextureObject = *static_cast<GLuint*>( m_CurrentRenderTarget->GetColorRenderTargetHandle( Face ) );

	ASSERT( GLEW_ARB_framebuffer_object || GLEW_EXT_framebuffer_object );
	if( GLEW_ARB_framebuffer_object )
	{
		// Bind to GL_FRAMEBUFFER, which does both drawing and readback calls,
		// because readback is necessary for Steam overlay to capture screenshots.
		glBindFramebuffer( GL_FRAMEBUFFER, FrameBufferObject );

		// Target the face
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLCubemapTarget[ Face ], CubemapTextureObject, 0 );

		// Set the draw buffers
		const GLenum pDrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers( 1, pDrawBuffers );
	}
	else if( GLEW_EXT_framebuffer_object )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, FrameBufferObject );

		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GLCubemapTarget[ Face ], CubemapTextureObject, 0 );

		const GLenum pDrawBuffers[] = { GL_COLOR_ATTACHMENT0_EXT };
		glDrawBuffers( 1, pDrawBuffers );
	}

	// GL also requires manually setting the viewport for RTs.
	{
		ASSERT( m_Display );
		const uint ViewportWidth	= ( FrameBufferObject == 0 ) ? m_Display->m_FrameWidth : m_CurrentRenderTarget->GetWidth();
		const uint ViewportHeight	= ( FrameBufferObject == 0 ) ? m_Display->m_FrameHeight : m_CurrentRenderTarget->GetHeight();
		glViewport( 0, 0, ViewportWidth, ViewportHeight );
	}
}

static const Angles skCubemapRenderOrientations[] =
{
	Angles( 0.0f, 0.0f, HALFPI ),
	Angles( 0.0f, 0.0f, -HALFPI ),
	Angles( 0.0f, 0.0f, PI ),
	Angles( 0.0f, 0.0f, 0.0f ),
	Angles( -HALFPI, 0.0f, PI ),
	Angles( HALFPI, 0.0f, PI ),
};
const Angles* GL2Renderer::GetCubemapRenderOrientations() const
{
	return skCubemapRenderOrientations;
}

bool GL2Renderer::IsValid()
{
	// GL has no concept of being invalid the way D3D does.
	return true;
}

void GL2Renderer::EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes )
{
	Display::EnumerateDisplayModes( DisplayModes );
}

/*virtual*/ void GL2Renderer::SaveScreenshot( const SimpleString& Filename )
{
	// TODO PORT LATER SaveScreenshot
	Unused( Filename );
}

/*virtual*/ void GL2Renderer::SetVertexShader( IVertexShader* const pVertexShader )
{
	// Not supported in GL. Use SetShaderProgram.
	Unused( pVertexShader );
}

/*virtual*/ void GL2Renderer::SetPixelShader( IPixelShader* const pPixelShader )
{
	// Not supported in GL. Use SetShaderProgram.
	Unused( pPixelShader );
}

/*virtual*/ void GL2Renderer::SetShaderProgram( IShaderProgram* const pShaderProgram )
{
	DEBUGASSERT( pShaderProgram );

#if IGNORE_REDUNDANT_STATE
	if( pShaderProgram == m_ShaderProgram )
	{
		return;
	}
#endif
	m_ShaderProgram = pShaderProgram;

	const GLuint ShaderProgram = *static_cast<GLuint*>( pShaderProgram->GetHandle() );
	glUseProgram( ShaderProgram );
}

/*virtual*/ void GL2Renderer::SetVertexDeclaration( IVertexDeclaration* const pVertexDeclaration )
{
	// Not used in GL.
	Unused( pVertexDeclaration );
}

/*virtual*/ SimpleString GL2Renderer::GetShaderType() const
{
	return "GLSL";
}

void GL2Renderer::glSetEnabled( const GLenum Cap, const bool Enabled )
{
	if( Enabled )
	{
		glEnable( Cap );
	}
	else
	{
		glDisable( Cap );
	}
}

static GLenum GLCullMode[] =
{
	0,			//	ECM_Unknown
	0,
	GL_BACK,
	GL_FRONT,
};

/*virtual*/ void GL2Renderer::SetCullMode( const ECullMode CullMode )
{
	DEBUGASSERT( CullMode > ECM_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( CullMode == m_RenderState.m_CullMode )
	{
		return;
	}
#endif
	m_RenderState.m_CullMode = CullMode;

	if( CullMode == ECM_None )
	{
		glDisable( GL_CULL_FACE );
	}
	else
	{
		glEnable( GL_CULL_FACE );
		glCullFace( GLCullMode[ CullMode ] );
	}
}

/*virtual*/ void GL2Renderer::SetZEnable( const EEnable ZEnable )
{
	DEBUGASSERT( ZEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ZEnable == m_RenderState.m_ZEnable )
	{
		return;
	}
#endif
	m_RenderState.m_ZEnable = ZEnable;

	glSetEnabled( GL_DEPTH_TEST, ( ZEnable == EE_True ) );
}

static GLenum GLFunc[] =
{
	0,						//	EF_Unknown
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS,
};

/*virtual*/ void GL2Renderer::SetZFunc( const EFunc ZFunc )
{
	DEBUGASSERT( ZFunc > EF_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ZFunc == m_RenderState.m_ZFunc )
	{
		return;
	}
#endif
	m_RenderState.m_ZFunc = ZFunc;

	glDepthFunc( GLFunc[ ZFunc ] );
}

/*virtual*/ void GL2Renderer::SetZWriteEnable( const EEnable ZWriteEnable )
{
	DEBUGASSERT( ZWriteEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ZWriteEnable == m_RenderState.m_ZWriteEnable )
	{
		return;
	}
#endif
	m_RenderState.m_ZWriteEnable = ZWriteEnable;

	glDepthMask( ( ZWriteEnable == EE_True ) ? GL_TRUE : GL_FALSE );
}

/*virtual*/ void GL2Renderer::SetZRange( const float DepthMin, const float DepthMax )
{
	DEBUGASSERT( DepthMin >= 0.0f && DepthMax <= 1.0f );

#if IGNORE_REDUNDANT_STATE
	if( DepthMin == m_RenderState.m_DepthMin &&
		DepthMax == m_RenderState.m_DepthMax )
	{
		return;
	}
#endif
	m_RenderState.m_DepthMin = DepthMin;
	m_RenderState.m_DepthMax = DepthMax;

	glDepthRange( DepthMin, DepthMax );
}

/*virtual*/ void GL2Renderer::SetAlphaBlendEnable( const EEnable AlphaBlendEnable )
{
	DEBUGASSERT( AlphaBlendEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( AlphaBlendEnable == m_RenderState.m_AlphaBlendEnable )
	{
		return;
	}
#endif
	m_RenderState.m_AlphaBlendEnable = AlphaBlendEnable;

	glSetEnabled( GL_BLEND, ( AlphaBlendEnable == EE_True ) );
}

static GLenum GLBlend[] =
{
	0,						//	EB_Unknown
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA_SATURATE,
};

/*virtual*/ void GL2Renderer::SetBlend( const EBlend SrcBlend, const EBlend DestBlend )
{
	DEBUGASSERT( SrcBlend > EB_Unknown );
	DEBUGASSERT( DestBlend > EB_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( SrcBlend == m_RenderState.m_SrcBlend &&
		DestBlend == m_RenderState.m_DestBlend )
	{
		return;
	}
#endif
	m_RenderState.m_SrcBlend = SrcBlend;
	m_RenderState.m_DestBlend = DestBlend;

	glBlendFunc( GLBlend[ SrcBlend ], GLBlend[ DestBlend ] );
}

/*virtual*/ void GL2Renderer::SetColorWriteEnable( const EEnable ColorWriteEnable )
{
	DEBUGASSERT( ColorWriteEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ColorWriteEnable == m_RenderState.m_ColorWriteEnable )
	{
		return;
	}
#endif
	m_RenderState.m_ColorWriteEnable = ColorWriteEnable;

	GLboolean ColorWrite = ( ColorWriteEnable == EE_True ) ? GL_TRUE : GL_FALSE;
	glColorMask( ColorWrite, ColorWrite, ColorWrite, ColorWrite );
}

/*virtual*/ void GL2Renderer::SetStencilEnable( const EEnable StencilEnable )
{
	DEBUGASSERT( StencilEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( StencilEnable == m_RenderState.m_StencilEnable )
	{
		return;
	}
#endif
	m_RenderState.m_StencilEnable = StencilEnable;

	glSetEnabled( GL_STENCIL_TEST, ( StencilEnable == EE_True ) );
}

/*virtual*/ void GL2Renderer::SetStencilFunc( const EFunc StencilFunc, const uint StencilRef, const uint StencilMask )
{
	DEBUGASSERT( StencilFunc > EF_Unknown );
	DEBUGASSERT( StencilRef != RENDERSTATE_UNINITIALIZED );
	DEBUGASSERT( StencilMask != RENDERSTATE_UNINITIALIZED );

#if IGNORE_REDUNDANT_STATE
	if( StencilFunc == m_RenderState.m_StencilFunc &&
		StencilRef == m_RenderState.m_StencilRef &&
		StencilMask == m_RenderState.m_StencilMask )
	{
		return;
	}
#endif
	m_RenderState.m_StencilFunc = StencilFunc;
	m_RenderState.m_StencilRef = StencilRef;
	m_RenderState.m_StencilMask = StencilMask;

	glStencilFunc( GLFunc[ StencilFunc ], StencilRef, StencilMask );
}

static GLenum GLStencilOp[] =
{
	0,						//	ESO_Unknown
	GL_KEEP,
	GL_ZERO,
	GL_REPLACE,
	GL_INCR,
	GL_DECR,
	GL_INVERT,
	GL_INCR_WRAP,
	GL_DECR_WRAP,
};

/*virtual*/ void GL2Renderer::SetStencilOp( const ESOp StencilFailOp, const ESOp StencilZFailOp, const ESOp StencilPassOp )
{
	DEBUGASSERT( StencilFailOp > ESO_Unknown );
	DEBUGASSERT( StencilZFailOp > ESO_Unknown );
	DEBUGASSERT( StencilPassOp > ESO_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( StencilFailOp == m_RenderState.m_StencilFailOp &&
		StencilZFailOp == m_RenderState.m_StencilZFailOp &&
		StencilPassOp == m_RenderState.m_StencilPassOp )
	{
		return;
	}
#endif
	m_RenderState.m_StencilFailOp = StencilFailOp;
	m_RenderState.m_StencilZFailOp = StencilZFailOp;
	m_RenderState.m_StencilPassOp = StencilPassOp;

	glStencilOp( GLStencilOp[ StencilFailOp ], GLStencilOp[ StencilZFailOp ], GLStencilOp[ StencilPassOp ] );
}

/*virtual*/ void GL2Renderer::SetStencilWriteMask( const uint StencilWriteMask )
{
	DEBUGASSERT( StencilWriteMask != RENDERSTATE_UNINITIALIZED );

#if IGNORE_REDUNDANT_STATE
	if( StencilWriteMask == m_RenderState.m_StencilWriteMask )
	{
		return;
	}
#endif
	m_RenderState.m_StencilWriteMask = StencilWriteMask;

	glStencilMask( StencilWriteMask );
}

/*virtual*/ void GL2Renderer::SetTexture( const uint SamplerStage, ITexture* const pTexture )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( pTexture );

	// This part is necessary even if the state is redundant, because it
	// controls which sampler state is affected by glTexParameter calls.
	glActiveTexture( GL_TEXTURE0 + SamplerStage );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( pTexture == SamplerState.m_Texture )
	{
#if BUILD_DEBUG
		GLint			OldBinding;
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &OldBinding );
		const GLuint	Texture		= *static_cast<GLuint*>( pTexture->GetHandle() );
		const GLint		NewBinding	= Texture;
		ASSERTDESC( OldBinding == NewBinding, "GL texture state desync!" );
#endif
		return;
	}
#endif
	SamplerState.m_Texture = pTexture;

	const GLuint Texture = *static_cast<GLuint*>( pTexture->GetHandle() );
	glBindTexture( GL_TEXTURE_2D, Texture );
}

// This resets *all* texture targets (2D, cubemap, whatever else I may use)
/*virtual*/ void GL2Renderer::ResetTexture( const uint SamplerStage )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	// This part is necessary even if the state is redundant, because it
	// controls which sampler state is affected by glTexParameter calls.
	glActiveTexture( GL_TEXTURE0 + SamplerStage );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( NULL == SamplerState.m_Texture )
	{
		return;
	}
#endif
	SamplerState.m_Texture = NULL;

	const GLuint Texture = 0;
	glBindTexture( GL_TEXTURE_2D,		Texture );
	glBindTexture( GL_TEXTURE_CUBE_MAP,	Texture );
}

static GLenum GLTextureAddress[] =
{
	0,					//	ETA_Unknown
	GL_REPEAT,
	GL_MIRRORED_REPEAT,
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,
};

/*virtual*/ void GL2Renderer::SetAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( AddressU > ETA_Unknown );
	DEBUGASSERT( AddressV > ETA_Unknown );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( AddressU == SamplerState.m_AddressU &&
		AddressV == SamplerState.m_AddressV )
	{
		return;
	}
#endif
	SamplerState.m_AddressU = AddressU;
	SamplerState.m_AddressV = AddressV;

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLTextureAddress[ AddressU ] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLTextureAddress[ AddressV ] );
}

static GLenum GLMinMipFilters[][ ETF_SIZE ] =
{	//	ETF_Unknown,	ETF_None,	ETF_Point,					ETF_Linear,					ETF_Anisotropic (mip)
	{	0,				0,			0,							0,							0,							},	// ETF_Unknown (min)
	{	0,				0,			0,							0,							0,							},	// ETF_None (min)
	{	0,				GL_NEAREST,	GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	GL_NEAREST_MIPMAP_LINEAR,	},	// ETF_Point (min)
	{	0,				GL_LINEAR,	GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	GL_LINEAR_MIPMAP_LINEAR,	},	// ETF_Linear (min)
	{	0,				0,			GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	GL_LINEAR_MIPMAP_LINEAR,	},	// ETF_Anisotropic (min)
};

/*virtual*/ void GL2Renderer::SetMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( MinFilter > ETF_None );
	DEBUGASSERT( MipFilter > ETF_Unknown );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( MinFilter == SamplerState.m_MinFilter &&
		MipFilter == SamplerState.m_MipFilter )
	{
		return;
	}
#endif
	SamplerState.m_MinFilter = MinFilter;
	SamplerState.m_MipFilter = MipFilter;

	const GLenum GLMinMipFilter = GLMinMipFilters[ MinFilter ][ MipFilter ];
	DEBUGASSERT( GLMinMipFilter != 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLMinMipFilter );
}

static GLenum GLMagFilters[] =
{
	0,			// ETF_Unknown
	0,			// ETF_None
	GL_NEAREST,
	GL_LINEAR,
	0,			// ETF_Anisotropic (not supported on GL)
};

/*virtual*/ void GL2Renderer::SetMagFilter( const uint SamplerStage, const ETextureFilter MagFilter )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( MagFilter > ETF_None );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( MagFilter == SamplerState.m_MagFilter )
	{
		return;
	}
#endif
	SamplerState.m_MagFilter = MagFilter;

	const GLenum GLMagFilter = GLMagFilters[ MagFilter ];
	DEBUGASSERT( GLMagFilter != 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLMagFilter );
}

/*virtual*/ void GL2Renderer::SetMaxAnisotropy( const uint SamplerStage, const uint MaxAnisotropy )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( MaxAnisotropy == SamplerState.m_MaxAnisotropy )
	{
		return;
	}
#endif
	SamplerState.m_MaxAnisotropy = MaxAnisotropy;

	const GLfloat MinMaxAnisotropy = Min( m_MaxAnisotropy, static_cast<float>( MaxAnisotropy ) );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MinMaxAnisotropy );
}

/*virtual*/ void GL2Renderer::SetCubemap( const uint SamplerStage, ITexture* const pCubemap )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( pCubemap );

	// This part is necessary even if the state is redundant, because it
	// controls which sampler state is affected by glTexParameter calls.
	glActiveTexture( GL_TEXTURE0 + SamplerStage );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( pCubemap == SamplerState.m_Texture )
	{
#if BUILD_DEBUG
		GLint			OldBinding;
		glGetIntegerv( GL_TEXTURE_BINDING_CUBE_MAP, &OldBinding );
		const GLuint	Cubemap		= *static_cast<GLuint*>( pCubemap->GetHandle() );
		const GLint		NewBinding	= Cubemap;
		ASSERTDESC( OldBinding == NewBinding, "GL cubemap state desync!" );
#endif
		return;
	}
#endif
	SamplerState.m_Texture = pCubemap;

	const GLuint Cubemap = *static_cast<GLuint*>( pCubemap->GetHandle() );
	glBindTexture( GL_TEXTURE_CUBE_MAP, Cubemap );
}

/*virtual*/ void GL2Renderer::SetCubemapAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV, const ETextureAddress AddressW )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( AddressU > ETA_Unknown );
	DEBUGASSERT( AddressV > ETA_Unknown );
	DEBUGASSERT( AddressW > ETA_Unknown );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( AddressU == SamplerState.m_AddressU &&
		AddressV == SamplerState.m_AddressV &&
		AddressW == SamplerState.m_AddressW )
	{
		return;
	}
#endif
	SamplerState.m_AddressU = AddressU;
	SamplerState.m_AddressV = AddressV;
	SamplerState.m_AddressW = AddressW;

	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GLTextureAddress[ AddressU ] );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GLTextureAddress[ AddressV ] );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GLTextureAddress[ AddressW ] );
}

/*virtual*/ void GL2Renderer::SetCubemapMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( MinFilter > ETF_None );
	DEBUGASSERT( MipFilter > ETF_Unknown );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( MinFilter == SamplerState.m_MinFilter &&
		MipFilter == SamplerState.m_MipFilter )
	{
		return;
	}
#endif
	SamplerState.m_MinFilter = MinFilter;
	SamplerState.m_MipFilter = MipFilter;

	const GLenum GLMinMipFilter = GLMinMipFilters[ MinFilter ][ MipFilter ];
	DEBUGASSERT( GLMinMipFilter != 0 );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GLMinMipFilter );
}

/*virtual*/ void GL2Renderer::SetCubemapMagFilter( const uint SamplerStage, const ETextureFilter MagFilter )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( MagFilter > ETF_None );

	SSamplerState& OuterSamplerState = m_SamplerStates[ SamplerStage ];

	DEBUGASSERT( OuterSamplerState.m_Texture );
	SSamplerState&	SamplerState	= *OuterSamplerState.m_Texture->GetSamplerState();

#if IGNORE_REDUNDANT_SAMPLER_STATE
	if( MagFilter == SamplerState.m_MagFilter )
	{
		return;
	}
#endif
	SamplerState.m_MagFilter = MagFilter;

	const GLenum GLMagFilter = GLMagFilters[ MagFilter ];
	DEBUGASSERT( GLMagFilter != 0 );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GLMagFilter );
}

/*virtual*/ void GL2Renderer::SetVertexShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s )
{
	SetPixelShaderFloat4s( Parameter, pFloats, NumFloat4s );
}

/*virtual*/ void GL2Renderer::SetVertexShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices )
{
	SetPixelShaderMatrices( Parameter, pFloats, NumMatrices );
}

/*virtual*/ void GL2Renderer::SetPixelShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s )
{
	ASSERT( m_ShaderProgram );

	uint Register;
	if( !m_ShaderProgram->GetPixelShaderRegister( Parameter, Register ) )
	{
		return;
	}

	glUniform4fv( Register, NumFloat4s, pFloats );
	GLERRORCHECK;
}

/*virtual*/ void GL2Renderer::SetPixelShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices )
{
	ASSERT( m_ShaderProgram );

	uint Register;
	if( !m_ShaderProgram->GetPixelShaderRegister( Parameter, Register ) )
	{
		return;
	}

	// HACKHACK: If we're passing in a multiple of 4 float4s, assume it's a matrix.
	// (GL doesn't accept multiples of float4s unless the GLSL variable was an array.)
	// Also, transpose matrices while we're at it.
	const GLboolean Transpose = GL_TRUE;
	glUniformMatrix4fv( Register, NumMatrices, Transpose, pFloats );
	GLERRORCHECK;
}

/*virtual*/ void GL2Renderer::DrawElements( IVertexBuffer* const pVertexBuffer, IIndexBuffer* const pIndexBuffer )
{
	XTRACE_FUNCTION;

	Unused( pVertexBuffer );

	GL2IndexBuffer* const pGL2IndexBuffer = static_cast<GL2IndexBuffer*>( pIndexBuffer );
	glDrawElements( pGL2IndexBuffer->GetPrimitiveType(), pIndexBuffer->GetNumIndices(), GLINDEXFORMAT, NULL );
}
