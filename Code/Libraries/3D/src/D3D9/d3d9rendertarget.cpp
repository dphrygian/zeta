#include "core.h"
#include "D3D9/d3d9rendertarget.h"
#include "D3D9/d3d9texture.h"
#include "irenderer.h"

/*static*/ D3DFORMAT D3D9RenderTarget::GetD3DFormat( const ERenderTargetFormat Format )
{
	switch( Format )
	{
	case ERTF_Unknown:
		return D3DFMT_UNKNOWN;
	case ERTF_X8R8G8B8:
		return D3DFMT_X8R8G8B8;
	case ERTF_A8R8G8B8:
		return D3DFMT_A8R8G8B8;
	case ERTF_A16B16G16R16:
		return D3DFMT_A16B16G16R16;
	case ERTF_A16B16G16R16F:
		return D3DFMT_A16B16G16R16F;
	case ERTF_A32B32G32R32F:
		return D3DFMT_A32B32G32R32F;
	case ERTF_R32F:
		return D3DFMT_R32F;
	case ERTF_R32G32F:
		return D3DFMT_G32R32F;
	case ERTF_D24S8:
		return D3DFMT_D24S8;
	default:
		WARNDESC( "D3D texture format not matched" );
		return D3DFMT_UNKNOWN;
	}
}

D3D9RenderTarget::D3D9RenderTarget( IRenderer* const pRenderer, IDirect3DDevice9* const pD3DDevice )
:	m_Params()
,	m_Renderer( pRenderer )
,	m_D3DDevice( pD3DDevice )
,	m_ColorTextures()
,	m_DepthStencilTexture( NULL )
,	m_ColorSurfaces()
,	m_DepthStencilSurface( NULL )
,	m_ColorParents()
,	m_DepthStencilParent( NULL )
,	m_IsChild( false )
{
}

// Used for creating the default RT
D3D9RenderTarget::D3D9RenderTarget( IRenderer* const pRenderer, IDirect3DDevice9* const pD3DDevice, IDirect3DSurface9* const pColorSurface, IDirect3DSurface9* const pDepthStencilSurface )
:	m_Params()
,	m_Renderer( pRenderer )
,	m_D3DDevice( pD3DDevice )
,	m_ColorTextures()
,	m_DepthStencilTexture( NULL )
,	m_ColorSurfaces()
,	m_DepthStencilSurface( pDepthStencilSurface )
,	m_ColorParents()
,	m_DepthStencilParent( NULL )
,	m_IsChild( false )
{
	m_ColorSurfaces.PushBack( pColorSurface );
}

D3D9RenderTarget::~D3D9RenderTarget()
{
	Release();
}

/*virtual*/ void D3D9RenderTarget::Initialize( const SRenderTargetParams& Params )
{
	m_Params = Params;

	BuildRenderTarget();
}

void D3D9RenderTarget::Release()
{
	FOR_EACH_ARRAY( ColorSurfaceIter, m_ColorSurfaces, IDirect3DSurface9* )
	{
		IDirect3DSurface9* pSurface = ColorSurfaceIter.GetValue();
		SafeRelease( pSurface );
	}
	m_ColorSurfaces.Clear();

	SafeRelease( m_DepthStencilSurface );

	if( m_IsChild )
	{
		// Don't delete textures, we don't own them
	}
	else
	{
		FOR_EACH_ARRAY( ColorTextureIter, m_ColorTextures, ITexture* )
		{
			ITexture* pTexture = ColorTextureIter.GetValue();
			SafeDelete( pTexture );
		}
	}
	m_ColorTextures.Clear();

	SafeDelete( m_DepthStencilTexture );
}

void D3D9RenderTarget::Reset()
{
	if( m_IsChild )
	{
		// HACKHACK: This is a "child" RT that just references
		// the surfaces owned by other RTs. Relink to those.

		Array<SParent> ColorParents = m_ColorParents;
		m_ColorParents.Clear();
		m_ColorSurfaces.Clear();
		m_ColorTextures.Clear();
		m_DepthStencilSurface = NULL;

		FOR_EACH_ARRAY( ColorParentIter, ColorParents, SParent )
		{
			const SParent& Parent = ColorParentIter.GetValue();
			AttachColorFrom( Parent.m_ParentRT, Parent.m_ParentIndex );
		}

		if( m_DepthStencilParent )
		{
			AttachDepthStencilFrom( m_DepthStencilParent );
		}
	}
	else
	{
		BuildRenderTarget();
	}
}

// Used for resetting the default RT
void D3D9RenderTarget::Reset( IDirect3DSurface9* const pColorSurface, IDirect3DSurface9* const pDepthStencilSurface )
{
	// No need to AddRef these; when called from D3D9Renderer::CreateDefaultRenderTarget, they have
	// already been implicitly AddRef-ed by the GetRenderTarget/GetDepthStencilSurface calls.

	ASSERT( m_ColorSurfaces.Empty() );

	m_ColorSurfaces.PushBack( pColorSurface );
	m_DepthStencilSurface = pDepthStencilSurface;
}

void D3D9RenderTarget::BuildRenderTarget()
{
	XTRACE_FUNCTION;

	ASSERT( m_Params.Width );
	ASSERT( m_Params.Height );

	STATIC_HASHED_STRING( Render );
	CATPRINTF( sRender, 1, "Building render target...\n" );

	const uint ColorUsage			= D3DUSAGE_RENDERTARGET;
	const uint DepthStencilUsage	= D3DUSAGE_DEPTHSTENCIL;
	const uint MipLevels			= 1;

	if( m_Params.ColorFormat != ERTF_None &&
		m_Params.ColorFormat != ERTF_Unknown )
	{
		CATPRINTF( sRender, 1, "\tTrying to create color texture with desired format %d\n", m_Params.ColorFormat );

		const ERenderTargetFormat SupportedFormat = m_Renderer->GetBestSupportedRenderTargetFormat( m_Params.ColorFormat );
		CATPRINTF( sRender, 1, "\tCreating color texture with actual format %d...\n", SupportedFormat );

		IDirect3DTexture9* ColorTexture;
		HRESULT hr = m_D3DDevice->CreateTexture(
			m_Params.Width,
			m_Params.Height,
			MipLevels,
			ColorUsage,
			GetD3DFormat( SupportedFormat ),
			D3DPOOL_DEFAULT,
			&ColorTexture,
			NULL );

		if( D3D_OK != hr )
		{
			PRINTF( "\tColor CreateTexture result: %d\n", hr );
		}
		ASSERT( hr == D3D_OK );

		m_ColorTextures.PushBack( new D3D9Texture( m_D3DDevice, ColorTexture ) );

		IDirect3DSurface9*& pSurface = m_ColorSurfaces.PushBack();
		ColorTexture->GetSurfaceLevel( 0, &pSurface );
	}

	if( m_Params.DepthStencilFormat != ERTF_None &&
		m_Params.DepthStencilFormat != ERTF_Unknown )
	{
		CATPRINTF( sRender, 1, "\tTrying to create depth/stencil texture with desired format %d\n", m_Params.DepthStencilFormat );

		const ERenderTargetFormat SupportedFormat = m_Renderer->GetBestSupportedRenderTargetFormat( m_Params.DepthStencilFormat );
		CATPRINTF( sRender, 1, "\tCreating depth/stencil texture with actual format %d...\n", SupportedFormat );

		IDirect3DTexture9* DepthStencilTexture;
		HRESULT hr = m_D3DDevice->CreateTexture(
			m_Params.Width,
			m_Params.Height,
			1,
			DepthStencilUsage,
			GetD3DFormat( SupportedFormat ),
			D3DPOOL_DEFAULT,
			&DepthStencilTexture,
			NULL );

		if( D3D_OK != hr )
		{
			PRINTF( "\tDepth/stencil CreateTexture result: %d\n", hr );
		}
		ASSERT( hr == D3D_OK );

		m_DepthStencilTexture = new D3D9Texture( m_D3DDevice, DepthStencilTexture );
		DepthStencilTexture->GetSurfaceLevel( 0, &m_DepthStencilSurface );
	}

	CATPRINTF( sRender, 1, "Render target built.\n" );
}

void* D3D9RenderTarget::GetHandle()
{
	// Not used by D3D
	return NULL;
}

void* D3D9RenderTarget::GetColorRenderTargetHandle( const uint Index )
{
	return m_ColorSurfaces[ Index ];
}

void* D3D9RenderTarget::GetDepthStencilRenderTargetHandle()
{
	return m_DepthStencilSurface;
}

ITexture* D3D9RenderTarget::GetColorTextureHandle( const uint Index )
{
	return m_ColorTextures[ Index ];
}

/*virtual*/ void D3D9RenderTarget::AttachColorFrom( IRenderTarget* const pRenderTarget, const uint Index )
{
	// Fix up dimensions; other params will remain uninitialized
	if( m_Params.Width == 0 || m_Params.Height == 0 )
	{
		m_Params.Width = pRenderTarget->GetWidth();
		m_Params.Height = pRenderTarget->GetHeight();
	}

	ASSERT( m_Params.Width == pRenderTarget->GetWidth() );
	ASSERT( m_Params.Height == pRenderTarget->GetHeight() );

	D3D9RenderTarget* const pD3D9RenderTarget = static_cast<D3D9RenderTarget*>( pRenderTarget );
	IDirect3DSurface9* const pSurface = static_cast<IDirect3DSurface9*>( pD3D9RenderTarget->GetColorRenderTargetHandle( Index ) );
	pSurface->AddRef();
	m_ColorSurfaces.PushBack( pSurface );
	ITexture* const pTexture = pD3D9RenderTarget->GetColorTextureHandle( Index );
	m_ColorTextures.PushBack( pTexture );

	SParent Parent;
	Parent.m_ParentRT = pD3D9RenderTarget;
	Parent.m_ParentIndex = Index;
	m_ColorParents.PushBack( Parent );
}

/*virtual*/ void D3D9RenderTarget::AttachDepthStencilFrom( IRenderTarget* const pRenderTarget )
{
	ASSERT( m_Params.Width == pRenderTarget->GetWidth() );
	ASSERT( m_Params.Height == pRenderTarget->GetHeight() );

	D3D9RenderTarget* const pD3D9RenderTarget = static_cast<D3D9RenderTarget*>( pRenderTarget );
	IDirect3DSurface9* const pSurface = static_cast<IDirect3DSurface9*>( pD3D9RenderTarget->GetDepthStencilRenderTargetHandle() );
	pSurface->AddRef();
	m_DepthStencilSurface = pSurface;

	m_DepthStencilParent = pD3D9RenderTarget;
}

/*virtual*/ void D3D9RenderTarget::FinishAttach()
{
	m_IsChild = true;
}
