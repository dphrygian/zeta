#ifndef D3D9RENDERER_H
#define D3D9RENDERER_H

// This class encapsulates the D3D interface and one device.
// In the future, I may want to separate those concepts...

#include "renderercommon.h"

#include <Windows.h>
#include <d3d9.h>

class ShaderManager;
class TextureManager;
class FontManager;
class MeshFactory;
class D3D9Shader;

class D3D9Renderer : public RendererCommon
{
public:
	virtual ~D3D9Renderer();
	D3D9Renderer( HWND hWnd, Display* const pDisplay );

	virtual void	Initialize();
	virtual void	Tick();
	virtual void	Clear( const uint Flags, const uint Color = 0xff000000, const float Depth = 1.0f, const uint Stencil = 0 );
	virtual void	Present();
	virtual bool	CanReset();
	virtual bool	Reset();
	virtual void	Refresh() { /* Unused */ }

	virtual bool	IsOpenGL() { return false; }

	virtual void	SetVertexShader( IVertexShader* const pVertexShader );
	virtual void	SetPixelShader( IPixelShader* const pPixelShader );
	virtual void	SetShaderProgram( IShaderProgram* const pShaderProgram );

	virtual void	SetVertexDeclaration( IVertexDeclaration* const pVertexDeclaration );

	virtual SimpleString	GetShaderType() const;

	virtual void	SetCullMode( const ECullMode CullMode );
	virtual void	SetColorWriteEnable( const EEnable ColorWriteEnable );
	virtual void	SetAlphaBlendEnable( const EEnable AlphaBlendEnable );
	virtual void	SetBlend( const EBlend SrcBlend, const EBlend DestBlend );
	virtual void	SetZEnable( const EEnable ZEnable );
	virtual void	SetZFunc( const EFunc ZFunc );
	virtual void	SetZWriteEnable( const EEnable ZWriteEnable );
	virtual void	SetZRange( const float DepthMin, const float DepthMax );
	virtual void	SetStencilEnable( const EEnable StencilEnable );
	virtual void	SetStencilFunc( const EFunc StencilFunc, const uint StencilRef, const uint StencilMask );
	virtual void	SetStencilOp( const ESOp StencilFailOp, const ESOp StencilZFailOp, const ESOp StencilPassOp );
	virtual void	SetStencilWriteMask( const uint StencilWriteMask );

	virtual void	SetTexture( const uint SamplerStage, ITexture* const pTexture );
	virtual void	ResetTexture( const uint SamplerStage );
	virtual void	SetAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV );
	virtual void	SetMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter );
	virtual void	SetMagFilter( const uint SamplerStage, const ETextureFilter MagFilter );
	virtual void	SetMaxAnisotropy( const uint SamplerStage, const uint MaxAnisotropy );

	virtual void	SetCubemap( const uint SamplerStage, ITexture* const pCubemap );
	virtual void	SetCubemapAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV, const ETextureAddress AddressW );
	virtual void	SetCubemapMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter ) { SetMinMipFilters( SamplerStage, MinFilter, MipFilter ); }
	virtual void	SetCubemapMagFilter( const uint SamplerStage, const ETextureFilter MagFilter ) { SetMagFilter( SamplerStage, MagFilter ); }

	virtual void	SetVertexShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s );
	virtual void	SetVertexShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices );
	virtual void	SetPixelShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s );
	virtual void	SetPixelShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices );

	virtual void	DrawElements( IVertexBuffer* const pVertexBuffer, IIndexBuffer* const pIndexBuffer );

	virtual void	SetRestoreDeviceCallback( const SRestoreDeviceCallback& Callback );

	virtual void	SetMultiSampleType( EMultiSampleType MultiSampleType );
	virtual void	GetBestSupportedMultiSampleType( EMultiSampleType& OutMultiSampleType, c_uint32* pOutQualityLevels = NULL );

	virtual ERenderTargetFormat	GetBestSupportedRenderTargetFormat( const ERenderTargetFormat Format ) const;

	virtual IVertexBuffer*		CreateVertexBuffer();
	virtual IVertexDeclaration*	CreateVertexDeclaration();
	virtual IIndexBuffer*		CreateIndexBuffer();
	virtual ITexture*			CreateTexture( const char* Filename, const bool NoMips );
	virtual ITexture*			CreateCubemap( const SimpleString& CubemapDef, const bool NoMips );
	virtual ITexture*			CreateCubemap( const SCubemapData& CubemapData );
	virtual IVertexShader*		CreateVertexShader( const SimpleString& Filename );
	virtual IPixelShader*		CreatePixelShader( const SimpleString& Filename );
	virtual IShaderProgram*		CreateShaderProgram( IVertexShader* const pVertexShader, IPixelShader* const pPixelShader, IVertexDeclaration* const pVertexDeclaration );
	virtual IRenderTarget*		CreateRenderTarget( const SRenderTargetParams& Params );
	virtual IRenderTarget*		CreateRenderTarget();
	virtual IRenderTarget*		CreateCubeRenderTarget( const SRenderTargetParams& Params );

	virtual void			SetRenderTarget( IRenderTarget* const pRenderTarget );
	virtual void			SetCubeRenderTarget( IRenderTarget* const pRenderTarget, const uint Face );
	virtual const Angles*	GetCubemapRenderOrientations() const;

	virtual void			SaveScreenshot( const SimpleString& Filename );

	void					TestDriverCapabilities();
	void					TestDeviceCapabilities();

	virtual bool	IsValid();

	virtual void	EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes );

	virtual bool	SupportsSM3();
	virtual bool	SupportsSM2();

protected:
	D3D9Renderer() {}

	virtual void	SetVertexArrays( Mesh* const pMesh );

	void	EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes, D3DFORMAT Format );

	void	CreateDefaultRenderTarget();
	void	PreReset();
	void	PostReset();
	void	GetPresentParams( D3DPRESENT_PARAMETERS& Params );

	IDirect3D9*			m_D3D;
	IDirect3DDevice9*	m_D3DDevice;

	bool					m_DeviceLost;
	SRestoreDeviceCallback	m_RestoreDeviceCallback;

	HWND				m_hWnd;
	EMultiSampleType	m_MultiSampleType;

	DWORD				m_MaxAnisotropy;
};

#endif // D3D9RENDERER_H
