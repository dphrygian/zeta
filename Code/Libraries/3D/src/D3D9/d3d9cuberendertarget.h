#ifndef D3D9CUBERENDERTARGET_H
#define D3D9CUBERENDERTARGET_H

#include "irendertarget.h"
#include "array.h"

struct IDirect3DDevice9;
struct IDirect3DSurface9;
class IRenderer;

class D3D9CubeRenderTarget : public IRenderTarget
{
public:
	D3D9CubeRenderTarget( IRenderer* const pRenderer, IDirect3DDevice9* const pD3DDevice );
	virtual ~D3D9CubeRenderTarget();

	virtual void		Initialize( const SRenderTargetParams& Params );

	virtual void		Release();
	virtual void		Reset();

	virtual uint		GetWidth()			{ return m_Params.Width; }
	virtual uint		GetHeight()			{ return m_Params.Width; }
	virtual uint		GetNumSurfaces()	{ return 1; }

	virtual void*		GetHandle()										{ return NULL; }
	virtual void*		GetColorRenderTargetHandle( const uint Index )	{ return m_CubemapSurfaces[ Index ]; }
	virtual void*		GetDepthStencilRenderTargetHandle()				{ return m_DepthStencilSurface; }
	virtual ITexture*	GetColorTextureHandle( const uint Index )		{ Unused( Index ); return m_CubemapTexture; }

	virtual void		AttachColorFrom( IRenderTarget* const pRenderTarget, const uint Index ) { Unused( pRenderTarget ); Unused( Index ); }
	virtual void		AttachDepthStencilFrom( IRenderTarget* const pRenderTarget ) { Unused( pRenderTarget ); }
	virtual void		FinishAttach() {}

protected:
	void				BuildCubeRenderTarget();

	SRenderTargetParams			m_Params;
	IRenderer*					m_Renderer;
	IDirect3DDevice9*			m_D3DDevice;

	ITexture*					m_CubemapTexture;
	ITexture*					m_DepthStencilTexture;	// Not used as a texture, but needed to manage AddRef/Release refcounting
	Array<IDirect3DSurface9*>	m_CubemapSurfaces;
	IDirect3DSurface9*			m_DepthStencilSurface;
};

#endif // D3D9CUBERENDERTARGET_H
