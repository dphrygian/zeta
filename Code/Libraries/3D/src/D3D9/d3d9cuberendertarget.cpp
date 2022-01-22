#include "core.h"
#include "d3d9cuberendertarget.h"
#include "d3d9rendertarget.h"
#include "d3d9cubemap.h"
#include "irenderer.h"

D3D9CubeRenderTarget::D3D9CubeRenderTarget( IRenderer* const pRenderer, IDirect3DDevice9* const pD3DDevice )
:	m_Params()
,	m_Renderer( pRenderer )
,	m_D3DDevice( pD3DDevice )
,	m_CubemapTexture( NULL )
,	m_DepthStencilTexture( NULL )
,	m_CubemapSurfaces()
,	m_DepthStencilSurface( NULL )
{
}

D3D9CubeRenderTarget::~D3D9CubeRenderTarget()
{
	Release();
}

/*virtual*/ void D3D9CubeRenderTarget::Initialize( const SRenderTargetParams& Params )
{
	m_Params = Params;

	BuildCubeRenderTarget();
}

void D3D9CubeRenderTarget::Release()
{
	FOR_EACH_ARRAY( CubemapSurfaceIter, m_CubemapSurfaces, IDirect3DSurface9* )
	{
		IDirect3DSurface9* pSurface = CubemapSurfaceIter.GetValue();
		SafeRelease( pSurface );
	}
	m_CubemapSurfaces.Clear();

	SafeRelease( m_DepthStencilSurface );

	SafeDelete( m_CubemapTexture );
	SafeDelete( m_DepthStencilTexture );
}

void D3D9CubeRenderTarget::Reset()
{
	BuildCubeRenderTarget();
}

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

void D3D9CubeRenderTarget::BuildCubeRenderTarget()
{
	XTRACE_FUNCTION;

	ASSERT( m_Params.Width );

	STATIC_HASHED_STRING( Render );
	CATPRINTF( sRender, 1, "Building cube render target...\n" );

	const uint		RenderTargetUsage	= D3DUSAGE_RENDERTARGET;
	const uint		DepthStencilUsage	= D3DUSAGE_DEPTHSTENCIL;
	const uint		MipLevels			= 1;
	HANDLE* const	pNullSharedHandle	= NULL;

	if( m_Params.ColorFormat != ERTF_None &&
		m_Params.ColorFormat != ERTF_Unknown )
	{
		CATPRINTF( sRender, 1, "Trying to create cube texture with format %d\n", m_Params.ColorFormat );

		const ERenderTargetFormat	SupportedFormat	= m_Renderer->GetBestSupportedRenderTargetFormat( m_Params.ColorFormat );
		const D3DFORMAT				D3DFormat		= D3D9RenderTarget::GetD3DFormat( SupportedFormat );
		CATPRINTF( sRender, 1, "Creating cube texture with format %d...\n", SupportedFormat );

		IDirect3DCubeTexture9* pCubeTexture;
		HRESULT hr = m_D3DDevice->CreateCubeTexture( m_Params.Width, MipLevels, RenderTargetUsage, D3DFormat, D3DPOOL_DEFAULT, &pCubeTexture, pNullSharedHandle );

		if( D3D_OK != hr )
		{
			PRINTF( "CreateCubeTexture result: %d\n", hr );
		}
		ASSERT( hr == D3D_OK );

		m_CubemapTexture = new D3D9Cubemap( m_D3DDevice, pCubeTexture );

		for( uint Side = 0; Side < 6; ++Side )
		{
			const D3DCUBEMAP_FACES FaceType = D3DCubemapFaceTypes[ Side ];

			IDirect3DSurface9*& pSurface = m_CubemapSurfaces.PushBack();
			pCubeTexture->GetCubeMapSurface( FaceType, 0, &pSurface );
		}
	}

	// There is only one depth/stencil surface; D3D9 does not apparently support cube textures
	// with depth formats. So reuse it for rendering each face.
	if( m_Params.DepthStencilFormat != ERTF_None &&
		m_Params.DepthStencilFormat != ERTF_Unknown )
	{
		CATPRINTF( sRender, 1, "Trying to create depth/stencil texture with format %d\n", m_Params.DepthStencilFormat );

		const ERenderTargetFormat	SupportedFormat	= m_Renderer->GetBestSupportedRenderTargetFormat( m_Params.DepthStencilFormat );
		const D3DFORMAT				D3DFormat		= D3D9RenderTarget::GetD3DFormat( SupportedFormat );
		CATPRINTF( sRender, 1, "Creating depth/stencil texture with format %d...\n", SupportedFormat );

		IDirect3DTexture9* pDepthStencilTexture;
		HRESULT hr = m_D3DDevice->CreateTexture( m_Params.Width, m_Params.Width, MipLevels, DepthStencilUsage, D3DFormat, D3DPOOL_DEFAULT, &pDepthStencilTexture, NULL );

		if( D3D_OK != hr )
		{
			PRINTF( "Depth/stencil CreateTexture result: %d\n", hr );
		}
		ASSERT( hr == D3D_OK );

		m_DepthStencilTexture = new D3D9Texture( m_D3DDevice, pDepthStencilTexture );
		pDepthStencilTexture->GetSurfaceLevel( 0, &m_DepthStencilSurface );
	}

	CATPRINTF( sRender, 1, "Render target built.\n" );
}
