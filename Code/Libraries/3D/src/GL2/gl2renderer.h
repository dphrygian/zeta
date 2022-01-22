#ifndef GL2RENDERER_H
#define GL2RENDERER_H

#include "renderercommon.h"
#include "gl2.h"

class Window;

#if BUILD_SDL
#include "SDL2/SDL.h"
#endif

class GL2Renderer : public RendererCommon
{
public:
	GL2Renderer( Window* const pWindow, Display* const pDisplay );
	virtual ~GL2Renderer();

	virtual void	Initialize();
	virtual void	Tick();
	virtual void	Clear( const uint Flags, const uint Color = 0xff000000, const float Depth = 1.0f, const uint Stencil = 0 );
	virtual void	Present();
	virtual bool	CanReset();
	virtual bool	Reset();
	virtual void	Refresh();

	virtual bool	IsOpenGL() { return true; }

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
	virtual void	SetCubemapMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter );
	virtual void	SetCubemapMagFilter( const uint SamplerStage, const ETextureFilter MagFilter );

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

	virtual bool	IsValid();

	virtual void	EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes );

	virtual bool	SupportsSM3();
	virtual bool	SupportsSM2();

protected:
	void			glSetEnabled( const GLenum Cap, const bool Enabled );

	virtual void	SetVertexArrays( Mesh* const pMesh );

	void			SetVSync( const bool VSync );

	Window*			m_Window;

#if BUILD_WINDOWS_NO_SDL
	HGLRC			m_RenderContext;
#endif
#if BUILD_SDL
	SDL_GLContext	m_RenderContext;
#endif

	GLfloat			m_MaxAnisotropy;
};

#endif // GL2RENDERER_H
