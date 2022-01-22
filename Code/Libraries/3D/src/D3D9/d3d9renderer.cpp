#include "core.h"
#include "3d.h"
#include "d3d9renderer.h"
#include "d3d9vertexbuffer.h"
#include "d3d9indexbuffer.h"
#include "d3d9vertexdeclaration.h"
#include "d3d9texture.h"
#include "d3d9cubemap.h"
#include "d3d9rendertarget.h"
#include "d3d9cuberendertarget.h"
#include "d3d9vertexshader.h"
#include "d3d9pixelshader.h"
#include "d3d9shaderprogram.h"
#include "vector.h"
#include "vector4.h"
#include "vector2.h"
#include "matrix.h"
#include "mesh.h"
#include "shadermanager.h"
#include "texturemanager.h"
#include "vertexdeclarationmanager.h"
#include "meshfactory.h"
#include "bucket.h"
#include "view.h"
#include "font.h"
#include "configmanager.h"
#include "packstream.h"
#include "simplestring.h"
#include "mathcore.h"

#include <d3d9.h>

#if BUILD_DEV || !BUILD_STEAM
#include <d3dx9.h>	// Included only for saving screenshots now!
#endif

#define IGNORE_REDUNDANT_STATE	1

D3DMULTISAMPLE_TYPE GetD3DMultiSampleType( EMultiSampleType MultiSampleType )
{
	switch( MultiSampleType )
	{
	case EMST_None:
		return D3DMULTISAMPLE_NONE;
	case EMST_2X:
		return D3DMULTISAMPLE_2_SAMPLES;
	case EMST_4X:
		return D3DMULTISAMPLE_4_SAMPLES;
	case EMST_8X:
		return D3DMULTISAMPLE_8_SAMPLES;
	default:
		WARNDESC( "D3D multisample type not matched" );
		return D3DMULTISAMPLE_NONE;
	}
}

D3D9Renderer::D3D9Renderer( HWND hWnd, Display* const pDisplay )
:	m_D3D( NULL )
,	m_D3DDevice( NULL )
,	m_DeviceLost( false )
,	m_RestoreDeviceCallback()
,	m_hWnd( hWnd )
,	m_MultiSampleType( EMST_None )
,	m_MaxAnisotropy( 0 )
{
	SetDisplay( pDisplay );
}

void D3D9Renderer::Initialize()
{
	XTRACE_FUNCTION;

	RendererCommon::Initialize();

	STATIC_HASHED_STRING( Render );
	CATPRINTF( sRender, 1, "Creating Direct3D system...\n" );

	ASSERT( !m_D3D );
	m_D3D = Direct3DCreate9( D3D_SDK_VERSION );
	ASSERT( m_D3D );

	DEBUGPRINTF( "Initializing Direct3D...\n" );

	STATICHASH( Renderer );
	STATICHASH( TestCaps );
	const bool TestCaps = ConfigManager::GetBool( sTestCaps, false, sRenderer );
	if( TestCaps )
	{
		TestDriverCapabilities();
	}

	D3DPRESENT_PARAMETERS D3DParams;
	GetPresentParams( D3DParams );

	CATPRINTF( sRender, 1, "Creating Direct3D device...\n" );

	HRESULT Result = 0;
	Result = m_D3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_FPU_PRESERVE | D3DCREATE_HARDWARE_VERTEXPROCESSING, &D3DParams, &m_D3DDevice );
	if( Result != D3D_OK )
	{
		CATPRINTF( sRender, 1, "\tCreateDevice returned 0x%08X, trying again with software processing\n", Result );
		Result = m_D3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3DParams, &m_D3DDevice );
	}
	CATPRINTF( sRender, 1, "\tCreateDevice returned 0x%08X\n", Result );
	ASSERT( Result == D3D_OK );
	ASSERT( m_D3DDevice );

	TestDeviceCapabilities();

	CATPRINTF( sRender, 1, "Device created.\n" );
	CATPRINTF( sRender, 1, "Creating default render target...\n" );

	CreateDefaultRenderTarget();

	CATPRINTF( sRender, 1, "Default render target created.\n" );
	CATPRINTF( sRender, 1, "Initializing render state.\n" );

	// I never want to use built-in lighting. All other render state should be driven by materials.
	Result = m_D3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	ASSERT( Result == D3D_OK );

	D3DCAPS9 Caps;
	m_D3DDevice->GetDeviceCaps( &Caps );
	m_MaxVertexAttribs	= Caps.MaxStreams;
	m_MaxAnisotropy		= Caps.MaxAnisotropy;

	CATPRINTF( sRender, 1, "Render state initialized.\n" );
	CATPRINTF( sRender, 1, "Direct3D initialized.\n" );
}

D3D9Renderer::~D3D9Renderer()
{
	DEBUGPRINTF( "Shutting down Direct3D...\n" );

	SafeRelease( m_D3DDevice );
	SafeRelease( m_D3D );
}

bool D3D9Renderer::SupportsSM3()
{
	D3DCAPS9 Caps;
	m_D3DDevice->GetDeviceCaps( &Caps );
	return(	Caps.PixelShaderVersion >= D3DPS_VERSION( 3, 0 ) &&
			Caps.VertexShaderVersion >= D3DVS_VERSION( 3, 0 ) );
}

bool D3D9Renderer::SupportsSM2()
{
	D3DCAPS9 Caps;
	m_D3DDevice->GetDeviceCaps( &Caps );
	return(	Caps.PixelShaderVersion >= D3DPS_VERSION( 2, 0 ) &&
		Caps.VertexShaderVersion >= D3DVS_VERSION( 2, 0 ) );
}

/*virtual*/ ERenderTargetFormat D3D9Renderer::GetBestSupportedRenderTargetFormat( const ERenderTargetFormat Format ) const
{
	ERenderTargetFormat ReturnFormat = Format;

#define CHECK_AND_DEMOTE( fmt, dmt ) \
	if( ReturnFormat == fmt ) \
	{ \
		const D3DFORMAT D3DFormat = D3D9RenderTarget::GetD3DFormat( ReturnFormat ); \
		const HRESULT hr = m_D3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFormat ); \
		if( hr != D3D_OK ) \
		{ \
			ReturnFormat = dmt; \
		} \
	}

	CHECK_AND_DEMOTE( ERTF_A32B32G32R32F, ERTF_A16B16G16R16F );
	CHECK_AND_DEMOTE( ERTF_A16B16G16R16F, ERTF_A16B16G16R16 );
	CHECK_AND_DEMOTE( ERTF_A16B16G16R16, ERTF_A8R8G8B8 );

#undef CHECK_AND_DEMOTE

	return ReturnFormat;
}

void D3D9Renderer::SetMultiSampleType( EMultiSampleType MultiSampleType )
{
	m_MultiSampleType = MultiSampleType;
}

void D3D9Renderer::GetBestSupportedMultiSampleType( EMultiSampleType& OutMultiSampleType, c_uint32* pOutQualityLevels /*= NULL*/ )
{
	ASSERT( m_Display );

	// NOTE: This assumes the desired resolution is set before calling this function.
	D3DDISPLAYMODE D3DDisplayMode;
	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &D3DDisplayMode );

	OutMultiSampleType = EMST_None;

	for( int MultiSampleType = EMST_8X; MultiSampleType > EMST_None; --MultiSampleType )
	{
		D3DMULTISAMPLE_TYPE DesiredMultisampleType = GetD3DMultiSampleType( static_cast<EMultiSampleType>( MultiSampleType ) );
		DWORD NumQualityLevels = 0;
		HRESULT hr = m_D3D->CheckDeviceMultiSampleType(	D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DDisplayMode.Format, !m_Display->m_Fullscreen, DesiredMultisampleType, &NumQualityLevels );
		if( hr == D3D_OK )
		{
			OutMultiSampleType = static_cast<EMultiSampleType>( MultiSampleType );
			if( pOutQualityLevels )
			{
				*pOutQualityLevels = NumQualityLevels;
			}
			return;
		}
	}
}

void D3D9Renderer::CreateDefaultRenderTarget()
{
	XTRACE_FUNCTION;

	// Create the default render target
	IDirect3DSurface9* ColorRenderTarget;
	IDirect3DSurface9* DepthStencilSurface;
	m_D3DDevice->GetRenderTarget( 0, &ColorRenderTarget );
	m_D3DDevice->GetDepthStencilSurface( &DepthStencilSurface );
	if( m_DefaultRenderTarget )
	{
		// Hackity hack; after reset, we don't want to make a new RenderTarget,
		// so just inject the new surfaces into the existing one.
		static_cast< D3D9RenderTarget* >( m_DefaultRenderTarget )->Reset( ColorRenderTarget, DepthStencilSurface );
	}
	else
	{
		m_DefaultRenderTarget = new D3D9RenderTarget( this, m_D3DDevice, ColorRenderTarget, DepthStencilSurface );
	}
	m_CurrentRenderTarget = m_DefaultRenderTarget;
}

// Return true if the device is working fine or if
// it has been lost but is able to be reset now
bool D3D9Renderer::CanReset()
{
	XTRACE_FUNCTION;

	HRESULT hr = m_D3DDevice->TestCooperativeLevel();
	return ( hr == D3D_OK || hr == D3DERR_DEVICENOTRESET );
}

// Recover from a lost device
bool D3D9Renderer::Reset()
{
	XTRACE_FUNCTION;

	if( !CanReset() )
	{
		return false;
	}

	PreReset();

	D3DPRESENT_PARAMETERS Params;
	GetPresentParams( Params );

	XTRACE_BEGIN( ResetDevice );
		const HRESULT hr = m_D3DDevice->Reset( &Params );
		if( hr != D3D_OK )
		{
			return false;
		}
	XTRACE_END;

	m_DeviceLost = false;
	PostReset();

	return true;
}

void D3D9Renderer::PreReset()
{
	XTRACE_FUNCTION;

	// Release all D3DPOOL_DEFAULT resources
	FOR_EACH_LIST( TargetIter, m_RenderTargets, IRenderTarget* )
	{
		( *TargetIter )->Release();
	}
	FOR_EACH_SET( VertexBufferIter, m_DynamicVertexBuffers, IVertexBuffer* )
	{
		( *VertexBufferIter )->DeviceRelease();
	}

	m_DefaultRenderTarget->Release();
	m_CurrentRenderTarget = NULL;
}

void D3D9Renderer::PostReset()
{
	XTRACE_FUNCTION;

	CreateDefaultRenderTarget();

	FOR_EACH_LIST( TargetIter, m_RenderTargets, IRenderTarget* )
	{
		( *TargetIter )->Reset();
	}
	FOR_EACH_SET( VertexBufferIter, m_DynamicVertexBuffers, IVertexBuffer* )
	{
		( *VertexBufferIter )->DeviceReset();
	}

	ResetRenderState();
}

/*virtual*/ void D3D9Renderer::SetRestoreDeviceCallback( const SRestoreDeviceCallback& Callback )
{
	m_RestoreDeviceCallback = Callback;
}

void D3D9Renderer::Tick()
{
	XTRACE_FUNCTION;

	PROFILE_FUNCTION;

	// Restore lost device
	if( !IsValid() && CanReset() )
	{
		if( m_RestoreDeviceCallback.m_Callback )
		{
			m_RestoreDeviceCallback.m_Callback( m_RestoreDeviceCallback.m_Void );
		}
		return;
	}

#if BUILD_DEV
	m_DEV_RenderStats.Reset();
	RESET_CLOCK( m_DEV_RenderStats.m_RenderTime );
	START_CLOCK( m_DEV_RenderStats.m_RenderTime );
#endif

	m_D3DDevice->BeginScene();

	RenderBuckets();
	PostRenderBuckets();

	m_D3DDevice->EndScene();

#if BUILD_DEV
	STOP_CLOCK( m_DEV_RenderStats.m_RenderTime );
#endif

	Present();

#if BUILD_DEV
	m_DEV_RenderStats.UpdateFPS();
#endif

#if BUILD_DEBUG
	STATIC_HASHED_STRING( Render );
	DEBUGCATPRINTF( sRender, 2, "%d meshes, %d draw calls, %d primitives\n", m_DEV_RenderStats.m_NumMeshes, m_DEV_RenderStats.m_NumDrawCalls, m_DEV_RenderStats.m_NumPrimitives );
#endif
}

/*virtual*/ void D3D9Renderer::SetVertexArrays( Mesh* const pMesh )
{
	XTRACE_FUNCTION;

	const uint	VertexSignature	= pMesh->m_VertexDeclaration->GetSignature();
	uint		Index			= 0;
#define SETSTREAM( STREAM, SIGNATURE, TYPE )																						\
	if( SIGNATURE == ( VertexSignature & SIGNATURE ) )																				\
	{																																\
		IDirect3DVertexBuffer9* const pVertexBuffer = static_cast<IDirect3DVertexBuffer9*>( pMesh->m_VertexBuffer->Get##STREAM() );	\
		DEVASSERT( pVertexBuffer );																									\
		const HRESULT Result = m_D3DDevice->SetStreamSource( Index++, pVertexBuffer, 0, sizeof( TYPE ) );							\
		DEBUGASSERT( Result == D3D_OK );																							\
		Unused( Result );																											\
	}

	SETSTREAM( Positions,	VD_POSITIONS,	Vector );
	SETSTREAM( Colors,		VD_COLORS,		uint );
	SETSTREAM( FloatColors,	VD_FLOATCOLORS,	Vector4 );
	SETSTREAM( UVs,			VD_UVS,			Vector2 );
	SETSTREAM( Normals,		VD_NORMALS,		Vector );
	SETSTREAM( NormalsB,	VD_NORMALS_B,	Vector );
	SETSTREAM( Tangents,	VD_TANGENTS,	Vector4 );
	SETSTREAM( BoneIndices,	VD_BONEINDICES,	SBoneData );
	SETSTREAM( BoneWeights,	VD_BONEWEIGHTS,	SBoneData );

#undef SETSTREAM

	// Disable all the attributes we're not using.
	for( uint DisableAttribIndex = Index; DisableAttribIndex < m_MaxVertexAttribs; ++DisableAttribIndex )
	{
		const HRESULT Result = m_D3DDevice->SetStreamSource( DisableAttribIndex, NULL, 0, 0 );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	IDirect3DIndexBuffer9* const pIndexBuffer = static_cast<IDirect3DIndexBuffer9*>( pMesh->m_IndexBuffer->GetIndices() );
	DEVASSERT( pIndexBuffer );
	m_D3DDevice->SetIndices( pIndexBuffer );
}

void D3D9Renderer::Present()
{
	PROFILE_FUNCTION;

	XTRACE_FUNCTION;

	const HRESULT Result = m_D3DDevice->Present( NULL, NULL, NULL, NULL );

	m_DeviceLost = ( Result == D3DERR_DEVICELOST );
}

void D3D9Renderer::Clear( const uint Flags, const uint Color /*= 0xff000000*/, const float Depth /*= 1.0f*/, const uint Stencil /*= 0*/ )
{
	XTRACE_FUNCTION;

	if( Flags == CLEAR_NONE )
	{
		return;
	}

	uint D3DFlags = 0;
	if( Flags & CLEAR_COLOR )
	{
		D3DFlags |= D3DCLEAR_TARGET;
	}
	if( Flags & CLEAR_DEPTH )
	{
		D3DFlags |= D3DCLEAR_ZBUFFER;
	}
	if( Flags & CLEAR_STENCIL )
	{
		D3DFlags |= D3DCLEAR_STENCIL;
	}

	const HRESULT Result = m_D3DDevice->Clear( 0, NULL, D3DFlags, Color, Depth, Stencil );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

IVertexBuffer* D3D9Renderer::CreateVertexBuffer()
{
	return new D3D9VertexBuffer( m_D3DDevice );
}

IVertexDeclaration* D3D9Renderer::CreateVertexDeclaration()
{
	return new D3D9VertexDeclaration( m_D3DDevice );
}

IIndexBuffer* D3D9Renderer::CreateIndexBuffer()
{
	return new D3D9IndexBuffer( m_D3DDevice );
}

ITexture* D3D9Renderer::CreateTexture( const char* Filename, const bool NoMips )
{
	XTRACE_FUNCTION;

	D3D9Texture* const pTexture = new D3D9Texture( m_D3DDevice );
	pTexture->Initialize( Filename, NoMips );
	return pTexture;
}

/*virtual*/ ITexture* D3D9Renderer::CreateCubemap( const SimpleString& CubemapDef, const bool NoMips )
{
	XTRACE_FUNCTION;

	D3D9Cubemap* const pCubemap = new D3D9Cubemap( m_D3DDevice );
	pCubemap->Initialize( CubemapDef, NoMips );
	return pCubemap;
}

/*virtual*/ ITexture* D3D9Renderer::CreateCubemap( const SCubemapData& CubemapData )
{
	XTRACE_FUNCTION;

	D3D9Cubemap* const pCubemap = new D3D9Cubemap( m_D3DDevice );
	pCubemap->CreateCubemap( CubemapData );
	return pCubemap;
}

/*virtual*/ IVertexShader* D3D9Renderer::CreateVertexShader( const SimpleString& Filename )
{
	D3D9VertexShader* const pVertexShader = new D3D9VertexShader;
	pVertexShader->Initialize( m_D3DDevice, PackStream( Filename.CStr() ) );
	return pVertexShader;
}

/*virtual*/ IPixelShader* D3D9Renderer::CreatePixelShader( const SimpleString& Filename )
{
	D3D9PixelShader* const pPixelShader = new D3D9PixelShader;
	pPixelShader->Initialize( m_D3DDevice, PackStream( Filename.CStr() ) );
	return pPixelShader;
}

/*virtual*/ IShaderProgram* D3D9Renderer::CreateShaderProgram( IVertexShader* const pVertexShader, IPixelShader* const pPixelShader, IVertexDeclaration* const pVertexDeclaration )
{
	D3D9ShaderProgram* const pShaderProgram = new D3D9ShaderProgram;
	pShaderProgram->Initialize( pVertexShader, pPixelShader, pVertexDeclaration );
	return pShaderProgram;
}

IRenderTarget* D3D9Renderer::CreateRenderTarget( const SRenderTargetParams& Params )
{
	D3D9RenderTarget* pTarget = new D3D9RenderTarget( this, m_D3DDevice );
	pTarget->Initialize( Params );
	m_RenderTargets.PushBack( pTarget );
	return pTarget;
}

IRenderTarget* D3D9Renderer::CreateRenderTarget()
{
	D3D9RenderTarget* pTarget = new D3D9RenderTarget( this, m_D3DDevice );
	m_RenderTargets.PushBack( pTarget );
	return pTarget;
}

IRenderTarget* D3D9Renderer::CreateCubeRenderTarget( const SRenderTargetParams& Params )
{
	D3D9CubeRenderTarget* pTarget = new D3D9CubeRenderTarget( this, m_D3DDevice );
	pTarget->Initialize( Params );
	m_RenderTargets.PushBack( pTarget );
	return pTarget;
}

void D3D9Renderer::SetRenderTarget( IRenderTarget* const pRenderTarget )
{
	XTRACE_FUNCTION;

	m_CurrentRenderTarget = pRenderTarget;

	// Set the RTs; hard-coded to a max of four, because really.
	const uint NumSurfaces = pRenderTarget->GetNumSurfaces();
	for( uint SurfaceIndex = 0; SurfaceIndex < MAX_MRT_COUNT; ++SurfaceIndex )
	{
		const bool					HasSurface	= SurfaceIndex < NumSurfaces;
		IDirect3DSurface9* const	pSurface	= HasSurface ? static_cast<IDirect3DSurface9*>( m_CurrentRenderTarget->GetColorRenderTargetHandle( SurfaceIndex ) ) : NULL;
		const HRESULT Result = m_D3DDevice->SetRenderTarget( SurfaceIndex, pSurface );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		IDirect3DSurface9* const pSurface = static_cast<IDirect3DSurface9*>( m_CurrentRenderTarget->GetDepthStencilRenderTargetHandle() );
		const HRESULT Result = m_D3DDevice->SetDepthStencilSurface( pSurface );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

void D3D9Renderer::SetCubeRenderTarget( IRenderTarget* const pRenderTarget, const uint Face )
{
	XTRACE_FUNCTION;

	m_CurrentRenderTarget = pRenderTarget;

	{
		DEBUGASSERT( Face < 6 );
		IDirect3DSurface9* const pSurface = static_cast<IDirect3DSurface9*>( m_CurrentRenderTarget->GetColorRenderTargetHandle( Face ) );
		const HRESULT Result = m_D3DDevice->SetRenderTarget( 0, pSurface );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	// Disable other RTs
	for( uint SurfaceIndex = 1; SurfaceIndex < MAX_MRT_COUNT; ++SurfaceIndex )
	{
		const HRESULT Result = m_D3DDevice->SetRenderTarget( SurfaceIndex, NULL );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		IDirect3DSurface9* const pSurface = static_cast<IDirect3DSurface9*>( m_CurrentRenderTarget->GetDepthStencilRenderTargetHandle() );
		const HRESULT Result = m_D3DDevice->SetDepthStencilSurface( pSurface );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

static const Angles skCubemapRenderOrientations[] =
{
	Angles( 0.0f, 0.0f, -HALFPI ),
	Angles( 0.0f, 0.0f, HALFPI ),
	Angles( 0.0f, 0.0f, 0.0f ),
	Angles( 0.0f, 0.0f, PI ),
	Angles( HALFPI, 0.0f, 0.0f ),
	Angles( -HALFPI, 0.0f, 0.0f ),
};
const Angles* D3D9Renderer::GetCubemapRenderOrientations() const
{
	return skCubemapRenderOrientations;
}

bool D3D9Renderer::IsValid()
{
	return !m_DeviceLost;
}

void D3D9Renderer::GetPresentParams( D3DPRESENT_PARAMETERS& Params )
{
	XTRACE_FUNCTION;

	ASSERT( m_Display );

	D3DDISPLAYMODE D3DDisplayMode;
	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &D3DDisplayMode );	// NOTE: This assumes the desired resolution is set before calling this for fullscreen mode

	ZeroMemory( &Params, sizeof( D3DPRESENT_PARAMETERS ) );

	if( m_Display->m_Fullscreen )
	{
		Params.BackBufferWidth = D3DDisplayMode.Width;
		Params.BackBufferHeight = D3DDisplayMode.Height;
		Params.BackBufferFormat = D3DDisplayMode.Format;

		Params.Windowed = false;
		Params.FullScreen_RefreshRateInHz = D3DDisplayMode.RefreshRate;
	}
	else
	{
		STATICHASH( DisplayWidth );
		STATICHASH( DisplayHeight );
		Params.BackBufferWidth = ConfigManager::GetInt( sDisplayWidth );
		Params.BackBufferHeight = ConfigManager::GetInt( sDisplayHeight );
		Params.BackBufferFormat = D3DFMT_UNKNOWN;

		Params.Windowed = true;
		Params.FullScreen_RefreshRateInHz = 0;
	}

	Params.BackBufferCount = 1;						// Could be 1, 2 or 3... Faster? Slower? Test it...

	D3DMULTISAMPLE_TYPE DesiredMultisampleType = GetD3DMultiSampleType( m_MultiSampleType );
	DWORD NumQualityLevels = 0;
	HRESULT hr = m_D3D->CheckDeviceMultiSampleType(	D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DDisplayMode.Format, Params.Windowed, DesiredMultisampleType, &NumQualityLevels );
	if( hr == D3D_OK )
	{
		Params.MultiSampleType = DesiredMultisampleType;
		Params.MultiSampleQuality = NumQualityLevels - 1;
	}
	else
	{
		Params.MultiSampleType = D3DMULTISAMPLE_NONE;
		Params.MultiSampleQuality = 0;
	}

	Params.SwapEffect = D3DSWAPEFFECT_DISCARD;		// Is apparently fastest (and allows multisampling)

	Params.hDeviceWindow = m_hWnd;

	Params.EnableAutoDepthStencil = true;
	Params.AutoDepthStencilFormat = ( D3DDisplayMode.Format == D3DFMT_X8R8G8B8 ) ? D3DFMT_D24S8 : D3DFMT_D16;

	Params.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;	// Discard z-buffer after Present()ing, can improve performance

	if( m_Display->m_VSync )
	{
		Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;		// D3DPRESENT_INTERVAL_DEFAULT is basically the same; coarser time resolution but a bit faster?
	}
	else
	{
		Params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}
}

/*virtual*/ void D3D9Renderer::EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes )
{
	DisplayModes.Clear();

	EnumerateDisplayModes( DisplayModes, D3DFMT_X8R8G8B8 );
	EnumerateDisplayModes( DisplayModes, D3DFMT_A8R8G8B8 );

	ASSERT( DisplayModes.Size() );
}

void D3D9Renderer::EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes, D3DFORMAT Format )
{
	uint NumModes = m_D3D->GetAdapterModeCount( D3DADAPTER_DEFAULT, Format );
	for( uint i = 0; i < NumModes; ++i )
	{
		D3DDISPLAYMODE D3DMode = {0};

		m_D3D->EnumAdapterModes( D3DADAPTER_DEFAULT, Format, i, &D3DMode );

		SDisplayMode Mode;
		Mode.Width = D3DMode.Width;
		Mode.Height = D3DMode.Height;
		DisplayModes.PushBackUnique( Mode );
	}
}

/*virtual*/ void D3D9Renderer::SaveScreenshot( const SimpleString& Filename )
{
#if !BUILD_DEV && BUILD_STEAM
	Unused( Filename );
#else
	ASSERT( m_CurrentRenderTarget );

	IDirect3DSurface9* const pRenderTargetSurface = static_cast<IDirect3DSurface9*>( m_CurrentRenderTarget->GetColorRenderTargetHandle( 0 ) );
	ASSERT( pRenderTargetSurface );

	D3DXSaveSurfaceToFile( Filename.CStr(), D3DXIFF_PNG, pRenderTargetSurface, NULL, NULL );
#endif
}

void D3D9Renderer::TestDeviceCapabilities()
{
	const UINT AvailableTextureMemoryB	= m_D3DDevice->GetAvailableTextureMem();
	const UINT AvailableTextureMemoryMB	= AvailableTextureMemoryB >> 20;
	PRINTF( "Available texture memory: %u MB\n", AvailableTextureMemoryMB );
}

void D3D9Renderer::TestDriverCapabilities()
{
	PRINTF( "Testing Direct3D device capabilities\n" );

	HRESULT hr;

	D3DADAPTER_IDENTIFIER9 ID;
	hr = m_D3D->GetAdapterIdentifier( D3DADAPTER_DEFAULT, 0, &ID );
	ASSERT( D3D_OK == hr );

	PRINTF( "Device description: %s\n", ID.Description );

	PRINTF( "Device caps:\n" );

	D3DCAPS9 Caps;
	hr = m_D3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &Caps );
	ASSERT( D3D_OK == hr );

#define PRINTCAP( cap, format ) PRINTF( "\t" #cap ": " format "\n", Caps.cap )

	PRINTCAP( DeviceType, "%d" );
	PRINTCAP( AdapterOrdinal, "%d" );
	PRINTCAP( Caps, "0x%08X" );
	PRINTCAP( Caps2, "0x%08X" );
	PRINTCAP( Caps3, "0x%08X" );
	PRINTCAP( PresentationIntervals, "0x%08X" );
	PRINTCAP( CursorCaps, "0x%08X" );
	PRINTCAP( DevCaps, "0x%08X" );
	PRINTCAP( PrimitiveMiscCaps, "0x%08X" );
	PRINTCAP( RasterCaps, "0x%08X" );
	PRINTCAP( ZCmpCaps, "0x%08X" );
	PRINTCAP( SrcBlendCaps, "0x%08X" );
	PRINTCAP( DestBlendCaps, "0x%08X" );
	PRINTCAP( AlphaCmpCaps, "0x%08X" );
	PRINTCAP( ShadeCaps, "0x%08X" );
	PRINTCAP( TextureCaps, "0x%08X" );
	PRINTCAP( TextureFilterCaps, "0x%08X" );
	PRINTCAP( CubeTextureFilterCaps, "0x%08X" );
	PRINTCAP( VolumeTextureFilterCaps, "0x%08X" );
	PRINTCAP( TextureAddressCaps, "0x%08X" );
	PRINTCAP( VolumeTextureAddressCaps, "0x%08X" );
	PRINTCAP( LineCaps, "0x%08X" );
	PRINTCAP( MaxTextureWidth, "%d" );
	PRINTCAP( MaxTextureHeight, "%d" );
	PRINTCAP( MaxVolumeExtent, "%d" );
	PRINTCAP( MaxTextureRepeat, "%d" );
	PRINTCAP( MaxTextureAspectRatio, "%d" );
	PRINTCAP( MaxAnisotropy, "%d" );
	PRINTCAP( MaxVertexW, "%f" );
	PRINTCAP( GuardBandLeft, "%f" );
	PRINTCAP( GuardBandTop, "%f" );
	PRINTCAP( GuardBandRight, "%f" );
	PRINTCAP( GuardBandBottom, "%f" );
	PRINTCAP( ExtentsAdjust, "%f" );
	PRINTCAP( StencilCaps, "0x%08X" );
	PRINTCAP( FVFCaps, "0x%08X" );
	PRINTCAP( TextureOpCaps, "0x%08X" );
	PRINTCAP( MaxTextureBlendStages, "%d" );
	PRINTCAP( MaxSimultaneousTextures, "%d" );
	PRINTCAP( VertexProcessingCaps, "0x%08X" );
	PRINTCAP( MaxActiveLights, "%d" );
	PRINTCAP( MaxUserClipPlanes, "%d" );
	PRINTCAP( MaxVertexBlendMatrices, "%d" );
	PRINTCAP( MaxVertexBlendMatrixIndex, "%d" );
	PRINTCAP( MaxPointSize, "%f" );
	PRINTCAP( MaxPrimitiveCount, "%d" );
	PRINTCAP( MaxVertexIndex, "%d" );
	PRINTCAP( MaxStreams, "%d" );
	PRINTCAP( MaxStreamStride, "%d" );
	PRINTCAP( VertexShaderVersion, "0x%08X" );
	PRINTCAP( MaxVertexShaderConst, "%d" );
	PRINTCAP( PixelShaderVersion, "0x%08X" );
	PRINTCAP( PixelShader1xMaxValue, "%f" );
	PRINTCAP( DevCaps2, "0x%08X" );
	PRINTCAP( MaxNpatchTessellationLevel, "%f" );
	PRINTCAP( MasterAdapterOrdinal, "%d" );
	PRINTCAP( AdapterOrdinalInGroup, "%d" );
	PRINTCAP( NumberOfAdaptersInGroup, "%d" );
	PRINTCAP( DeclTypes, "0x%08X" );
	PRINTCAP( NumSimultaneousRTs, "%d" );
	PRINTCAP( StretchRectFilterCaps, "0x%08X" );
	PRINTCAP( VS20Caps.StaticFlowControlDepth, "%d" );
	PRINTCAP( VS20Caps.DynamicFlowControlDepth, "%d" );
	PRINTCAP( VS20Caps.NumTemps, "%d" );
	PRINTCAP( VS20Caps.StaticFlowControlDepth, "%d" );
	PRINTCAP( PS20Caps.Caps, "%d" );
	PRINTCAP( PS20Caps.DynamicFlowControlDepth, "%d" );
	PRINTCAP( PS20Caps.NumTemps, "%d" );
	PRINTCAP( PS20Caps.StaticFlowControlDepth, "%d" );
	PRINTCAP( PS20Caps.NumInstructionSlots, "%d" );
	PRINTCAP( VertexTextureFilterCaps, "0x%08X" );
	PRINTCAP( MaxVShaderInstructionsExecuted, "%d" );
	PRINTCAP( MaxPShaderInstructionsExecuted, "%d" );
	PRINTCAP( MaxVertexShader30InstructionSlots, "%d" );
	PRINTCAP( MaxPixelShader30InstructionSlots, "%d" );
#undef PRINTCAP

	PRINTF( "Device format checks:\n" );

#define TESTFORMAT( adapterfmt, usage, rtype, fmt ) \
	hr = m_D3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapterfmt, usage, rtype, fmt ); \
	PRINTF( "\t" #adapterfmt " " #usage " " #rtype " " #fmt ": 0x%08X\n", hr )

	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_DEPTHSTENCIL,	D3DRTYPE_SURFACE,		D3DFMT_D24S8 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_DEPTHSTENCIL,	D3DRTYPE_SURFACE,		D3DFMT_D16 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	0,						D3DRTYPE_TEXTURE,		D3DFMT_DXT1 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	0,						D3DRTYPE_TEXTURE,		D3DFMT_DXT2 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	0,						D3DRTYPE_TEXTURE,		D3DFMT_DXT3 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	0,						D3DRTYPE_TEXTURE,		D3DFMT_DXT4 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	0,						D3DRTYPE_TEXTURE,		D3DFMT_DXT5 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	0,						D3DRTYPE_TEXTURE,		D3DFMT_A8R8G8B8 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_X8R8G8B8 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_X8R8G8B8 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_A8R8G8B8 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_A8R8G8B8 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_A16B16G16R16 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_A16B16G16R16 );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_A16B16G16R16F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_A16B16G16R16F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_A32B32G32R32F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_A32B32G32R32F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_R32F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_R32F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_SURFACE,		D3DFMT_G32R32F );
	TESTFORMAT( D3DFMT_X8R8G8B8,	D3DUSAGE_RENDERTARGET,	D3DRTYPE_TEXTURE,		D3DFMT_G32R32F );

#undef TESTFORMAT
}

/*virtual*/ void D3D9Renderer::SetVertexShader( IVertexShader* const pVertexShader )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( pVertexShader );

#if IGNORE_REDUNDANT_STATE
	if( pVertexShader == m_VertexShader )
	{
		return;
	}
#endif
	m_VertexShader = pVertexShader;

	IDirect3DVertexShader9* const pD3DVertexShader = static_cast<IDirect3DVertexShader9*>( pVertexShader->GetHandle() );
	const HRESULT Result = m_D3DDevice->SetVertexShader( pD3DVertexShader );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetPixelShader( IPixelShader* const pPixelShader )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( pPixelShader );

#if IGNORE_REDUNDANT_STATE
	if( pPixelShader == m_PixelShader )
	{
		return;
	}
#endif
	m_PixelShader = pPixelShader;

	IDirect3DPixelShader9* const pD3DPixelShader = static_cast<IDirect3DPixelShader9*>( pPixelShader->GetHandle() );
	const HRESULT Result = m_D3DDevice->SetPixelShader( pD3DPixelShader );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetShaderProgram( IShaderProgram* const pShaderProgram )
{
	DEBUGASSERT( pShaderProgram );

#if IGNORE_REDUNDANT_STATE
	if( pShaderProgram == m_ShaderProgram )
	{
		return;
	}
#endif
	m_ShaderProgram = pShaderProgram;

	SetVertexShader( pShaderProgram->GetVertexShader() );
	SetPixelShader( pShaderProgram->GetPixelShader() );
}

/*virtual*/ void D3D9Renderer::SetVertexDeclaration( IVertexDeclaration* const pVertexDeclaration )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( pVertexDeclaration );

#if IGNORE_REDUNDANT_STATE
	if( pVertexDeclaration == m_VertexDeclaration )
	{
		return;
	}
#endif
	m_VertexDeclaration = pVertexDeclaration;

	IDirect3DVertexDeclaration9* const pD3DVertexDeclaration = static_cast<IDirect3DVertexDeclaration9*>( pVertexDeclaration->GetDeclaration() );
	const HRESULT Result = m_D3DDevice->SetVertexDeclaration( pD3DVertexDeclaration );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ SimpleString D3D9Renderer::GetShaderType() const
{
	return "HLSL";
}

static DWORD D3DCullMode[] =
{
	0,							//	ECM_Unknown
	D3DCULL_NONE,
	D3DCULL_CW,
	D3DCULL_CCW,
};

/*virtual*/ void D3D9Renderer::SetCullMode( const ECullMode CullMode )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( CullMode > ECM_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( CullMode == m_RenderState.m_CullMode )
	{
		return;
	}
#endif
	m_RenderState.m_CullMode = CullMode;

	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCullMode[ CullMode ] );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetZEnable( const EEnable ZEnable )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( ZEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ZEnable == m_RenderState.m_ZEnable )
	{
		return;
	}
#endif
	m_RenderState.m_ZEnable = ZEnable;

	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_ZENABLE, ( ZEnable == EE_True ) ? TRUE : FALSE );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

static DWORD D3DFunc[] =
{
	0,							//	EF_Unknown
	D3DCMP_NEVER,
	D3DCMP_LESS,
	D3DCMP_EQUAL,
	D3DCMP_LESSEQUAL,
	D3DCMP_GREATER,
	D3DCMP_NOTEQUAL,
	D3DCMP_GREATEREQUAL,
	D3DCMP_ALWAYS,
};

/*virtual*/ void D3D9Renderer::SetZFunc( const EFunc ZFunc )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( ZFunc > EF_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ZFunc == m_RenderState.m_ZFunc )
	{
		return;
	}
#endif
	m_RenderState.m_ZFunc = ZFunc;

	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_ZFUNC, D3DFunc[ ZFunc ] );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetZWriteEnable( const EEnable ZWriteEnable )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( ZWriteEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ZWriteEnable == m_RenderState.m_ZWriteEnable )
	{
		return;
	}
#endif
	m_RenderState.m_ZWriteEnable = ZWriteEnable;

	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_ZWRITEENABLE, ( ZWriteEnable == EE_True ) ? TRUE : FALSE );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetZRange( const float DepthMin, const float DepthMax )
{
	DEBUGASSERT( DepthMin >= 0.0f && DepthMax <= 1.0f );

	// NOTE: Don't ever ignore redundant state here.
	// D3D associates viewports with RTs, so it turns
	// out to be easier to just always set the viewport.
	// (Tracking state per RT wouldn't work for GL.)
//#if IGNORE_REDUNDANT_STATE
//	if( DepthMin == m_RenderState.m_DepthMin &&
//		DepthMax == m_RenderState.m_DepthMax )
//	{
//		return;
//	}
//#endif
	m_RenderState.m_DepthMin = DepthMin;
	m_RenderState.m_DepthMax = DepthMax;

	D3DVIEWPORT9 Viewport;
	{
		const HRESULT Result = m_D3DDevice->GetViewport( &Viewport );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	Viewport.MinZ	= DepthMin;
	Viewport.MaxZ	= DepthMax;

	{
		const HRESULT Result = m_D3DDevice->SetViewport( &Viewport );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

/*virtual*/ void D3D9Renderer::SetAlphaBlendEnable( const EEnable AlphaBlendEnable )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( AlphaBlendEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( AlphaBlendEnable == m_RenderState.m_AlphaBlendEnable )
	{
		return;
	}
#endif
	m_RenderState.m_AlphaBlendEnable = AlphaBlendEnable;

	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, ( AlphaBlendEnable == EE_True ) ? TRUE : FALSE );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

static DWORD D3DBlend[] =
{
	0,							//	EB_Unknown
	D3DBLEND_ZERO,
	D3DBLEND_ONE,
	D3DBLEND_SRCCOLOR,
	D3DBLEND_INVSRCCOLOR,
	D3DBLEND_SRCALPHA,
	D3DBLEND_INVSRCALPHA,
	D3DBLEND_DESTALPHA,
	D3DBLEND_INVDESTALPHA,
	D3DBLEND_DESTCOLOR,
	D3DBLEND_INVDESTCOLOR,
	D3DBLEND_SRCALPHASAT,
};

/*virtual*/ void D3D9Renderer::SetBlend( const EBlend SrcBlend, const EBlend DestBlend )
{
	DEBUGASSERT( m_D3DDevice );
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

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBlend[ SrcBlend ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBlend[ DestBlend ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

/*virtual*/ void D3D9Renderer::SetColorWriteEnable( const EEnable ColorWriteEnable )
{
	DEBUGASSERT( ColorWriteEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( ColorWriteEnable == m_RenderState.m_ColorWriteEnable )
	{
		return;
	}
#endif
	m_RenderState.m_ColorWriteEnable = ColorWriteEnable;

	static const DWORD AllChannels = D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE;
	DWORD ColorWrite = ( ColorWriteEnable == EE_True ) ? AllChannels : 0;
	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_COLORWRITEENABLE, ColorWrite );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetStencilEnable( const EEnable StencilEnable )
{
	DEBUGASSERT( StencilEnable > EE_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( StencilEnable == m_RenderState.m_StencilEnable )
	{
		return;
	}
#endif
	m_RenderState.m_StencilEnable = StencilEnable;

	const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILENABLE, ( StencilEnable == EE_True ) ? TRUE : FALSE );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetStencilFunc( const EFunc StencilFunc, const uint StencilRef, const uint StencilMask )
{
	DEBUGASSERT( StencilFunc > EF_Unknown );
	DEBUGASSERT( StencilRef != RENDERSTATE_UNINITIALIZED );
	DEBUGASSERT( StencilMask != RENDERSTATE_UNINITIALIZED );

#if IGNORE_REDUNDANT_STATE
	if( StencilFunc	== m_RenderState.m_StencilFunc &&
		StencilRef	== m_RenderState.m_StencilRef &&
		StencilMask	== m_RenderState.m_StencilMask )
	{
		return;
	}
#endif
	m_RenderState.m_StencilFunc	= StencilFunc;
	m_RenderState.m_StencilRef	= StencilRef;
	m_RenderState.m_StencilMask	= StencilMask;

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILFUNC, D3DFunc[ StencilFunc ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILREF, StencilRef );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILMASK, StencilMask );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

static DWORD D3DStencilOp[] =
{
	0,						//	ESO_Unknown
	D3DSTENCILOP_KEEP,
	D3DSTENCILOP_ZERO,
	D3DSTENCILOP_REPLACE,
	D3DSTENCILOP_INCRSAT,
	D3DSTENCILOP_DECRSAT,
	D3DSTENCILOP_INVERT,
	D3DSTENCILOP_INCR,
	D3DSTENCILOP_DECR,
};

/*virtual*/ void D3D9Renderer::SetStencilOp( const ESOp StencilFailOp, const ESOp StencilZFailOp, const ESOp StencilPassOp )
{
	DEBUGASSERT( StencilFailOp > ESO_Unknown );
	DEBUGASSERT( StencilZFailOp > ESO_Unknown );
	DEBUGASSERT( StencilPassOp > ESO_Unknown );

#if IGNORE_REDUNDANT_STATE
	if( StencilFailOp	== m_RenderState.m_StencilFailOp &&
		StencilZFailOp	== m_RenderState.m_StencilZFailOp &&
		StencilPassOp	== m_RenderState.m_StencilPassOp )
	{
		return;
	}
#endif
	m_RenderState.m_StencilFailOp	= StencilFailOp;
	m_RenderState.m_StencilZFailOp	= StencilZFailOp;
	m_RenderState.m_StencilPassOp	= StencilPassOp;

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILFAIL, D3DStencilOp[ StencilFailOp ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILZFAIL, D3DStencilOp[ StencilZFailOp ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILPASS, D3DStencilOp[ StencilPassOp ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

/*virtual*/ void D3D9Renderer::SetStencilWriteMask( const uint StencilWriteMask )
{
	DEBUGASSERT( StencilWriteMask != RENDERSTATE_UNINITIALIZED );

#if IGNORE_REDUNDANT_STATE
	if( StencilWriteMask == m_RenderState.m_StencilWriteMask )
	{
		return;
	}
#endif
	m_RenderState.m_StencilWriteMask = StencilWriteMask;

	{
		const HRESULT Result = m_D3DDevice->SetRenderState( D3DRS_STENCILWRITEMASK, StencilWriteMask );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

/*virtual*/ void D3D9Renderer::SetTexture( const uint SamplerStage, ITexture* const pTexture )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( pTexture );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( pTexture == SamplerState.m_Texture )
	{
		return;
	}
#endif
	SamplerState.m_Texture = pTexture;

	IDirect3DTexture9* const pD3DTexture = static_cast<IDirect3DTexture9*>( pTexture->GetHandle() );
	const HRESULT Result = m_D3DDevice->SetTexture( SamplerStage, pD3DTexture );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::ResetTexture( const uint SamplerStage )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( NULL == SamplerState.m_Texture )
	{
		return;
	}
#endif
	SamplerState.m_Texture = NULL;

	IDirect3DTexture9* const pD3DTexture = NULL;
	const HRESULT Result = m_D3DDevice->SetTexture( SamplerStage, pD3DTexture );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

static DWORD D3DTextureAddress[] =
{
	0,					//	ETA_Unknown
	D3DTADDRESS_WRAP,
	D3DTADDRESS_MIRROR,
	D3DTADDRESS_CLAMP,
	D3DTADDRESS_BORDER,
};

/*virtual*/ void D3D9Renderer::SetAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( AddressU > ETA_Unknown );
	DEBUGASSERT( AddressV > ETA_Unknown );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( AddressU == SamplerState.m_AddressU &&
		AddressV == SamplerState.m_AddressV )
	{
		return;
	}
#endif
	SamplerState.m_AddressU = AddressU;
	SamplerState.m_AddressV = AddressV;

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_ADDRESSU, D3DTextureAddress[ AddressU ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_ADDRESSV, D3DTextureAddress[ AddressV ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

static DWORD D3DTextureFilter[] =
{
	0,					// ETF_Unknown
	D3DTEXF_NONE,
	D3DTEXF_POINT,
	D3DTEXF_LINEAR,
	D3DTEXF_ANISOTROPIC,
};

/*virtual*/ void D3D9Renderer::SetMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( MinFilter > ETF_None );
	DEBUGASSERT( MipFilter > ETF_Unknown );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( MinFilter == SamplerState.m_MinFilter &&
		MipFilter == SamplerState.m_MipFilter )
	{
		return;
	}
#endif
	SamplerState.m_MinFilter = MinFilter;
	SamplerState.m_MipFilter = MipFilter;

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_MINFILTER, D3DTextureFilter[ MinFilter ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_MIPFILTER, D3DTextureFilter[ MipFilter ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

/*virtual*/ void D3D9Renderer::SetMagFilter( const uint SamplerStage, const ETextureFilter MagFilter )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( MagFilter > ETF_None );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( MagFilter == SamplerState.m_MagFilter )
	{
		return;
	}
#endif
	SamplerState.m_MagFilter = MagFilter;

	const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_MAGFILTER, D3DTextureFilter[ MagFilter ] );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetMaxAnisotropy( const uint SamplerStage, const uint MaxAnisotropy )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( MaxAnisotropy == SamplerState.m_MaxAnisotropy )
	{
		return;
	}
#endif
	SamplerState.m_MaxAnisotropy = MaxAnisotropy;

	const DWORD MinMaxAnisotropy = Min( m_MaxAnisotropy, MaxAnisotropy );
	const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_MAXANISOTROPY, MinMaxAnisotropy );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetCubemap( const uint SamplerStage, ITexture* const pCubemap )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( pCubemap );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
	if( pCubemap == SamplerState.m_Texture )
	{
		return;
	}
#endif
	SamplerState.m_Texture = pCubemap;

	IDirect3DCubeTexture9* const pD3DCubemap = static_cast<IDirect3DCubeTexture9*>( pCubemap->GetHandle() );
	const HRESULT Result = m_D3DDevice->SetTexture( SamplerStage, pD3DCubemap );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetCubemapAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV, const ETextureAddress AddressW )
{
	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );
	DEBUGASSERT( AddressU > ETA_Unknown );
	DEBUGASSERT( AddressV > ETA_Unknown );
	DEBUGASSERT( AddressW > ETA_Unknown );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

#if IGNORE_REDUNDANT_STATE
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

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_ADDRESSU, D3DTextureAddress[ AddressU ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_ADDRESSV, D3DTextureAddress[ AddressV ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}

	{
		const HRESULT Result = m_D3DDevice->SetSamplerState( SamplerStage, D3DSAMP_ADDRESSW, D3DTextureAddress[ AddressW ] );
		DEBUGASSERT( Result == D3D_OK );
		Unused( Result );
	}
}

/*virtual*/ void D3D9Renderer::SetVertexShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s )
{
	ASSERT( m_VertexShader );

	uint Register;
	if( !m_VertexShader->GetRegister( Parameter, Register ) )
	{
		return;
	}

	const HRESULT Result = m_D3DDevice->SetVertexShaderConstantF( Register, pFloats, NumFloat4s );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetVertexShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices )
{
	ASSERT( m_VertexShader );

	uint Register;
	if( !m_VertexShader->GetRegister( Parameter, Register ) )
	{
		return;
	}

	const HRESULT Result = m_D3DDevice->SetVertexShaderConstantF( Register, pFloats, NumMatrices * 4 );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetPixelShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s )
{
	ASSERT( m_PixelShader );

	uint Register;
	if( !m_PixelShader->GetRegister( Parameter, Register ) )
	{
		return;
	}

	const HRESULT Result = m_D3DDevice->SetPixelShaderConstantF( Register, pFloats, NumFloat4s );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::SetPixelShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices )
{
	ASSERT( m_PixelShader );

	uint Register;
	if( !m_PixelShader->GetRegister( Parameter, Register ) )
	{
		return;
	}

	const HRESULT Result = m_D3DDevice->SetPixelShaderConstantF( Register, pFloats, NumMatrices * 4 );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}

/*virtual*/ void D3D9Renderer::DrawElements( IVertexBuffer* const pVertexBuffer, IIndexBuffer* const pIndexBuffer )
{
	XTRACE_FUNCTION;

	DEBUGASSERT( m_D3DDevice );
	DEBUGASSERT( pVertexBuffer );
	DEBUGASSERT( pIndexBuffer );

	D3D9IndexBuffer* const pD3D9IndexBuffer = static_cast<D3D9IndexBuffer*>( pIndexBuffer );
	const HRESULT Result = m_D3DDevice->DrawIndexedPrimitive( pD3D9IndexBuffer->GetPrimitiveType(), 0, 0, pVertexBuffer->GetNumVertices(), 0, pIndexBuffer->GetNumPrimitives() );
	DEBUGASSERT( Result == D3D_OK );
	Unused( Result );
}
