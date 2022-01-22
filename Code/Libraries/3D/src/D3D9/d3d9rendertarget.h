#ifndef D3D9RENDERTARGET_H
#define D3D9RENDERTARGET_H

#include "irendertarget.h"
#include "array.h"

// Included for D3DFORMAT, which can't be forward declared because C++.
#include <d3d9.h>

struct IDirect3DDevice9;
struct IDirect3DSurface9;
class IRenderer;

class D3D9RenderTarget : public IRenderTarget
{
public:
	D3D9RenderTarget( IRenderer* const pRenderer, IDirect3DDevice9* const pD3DDevice );
	D3D9RenderTarget( IRenderer* const pRenderer, IDirect3DDevice9* const pD3DDevice, IDirect3DSurface9* const pColorSurface, IDirect3DSurface9* const pDepthStencilSurface );
	virtual ~D3D9RenderTarget();

	virtual void		Initialize( const SRenderTargetParams& Params );

	virtual void		Release();
	virtual void		Reset();
	void				Reset( IDirect3DSurface9* const pColorSurface, IDirect3DSurface9* const pDepthStencilSurface );

	virtual uint		GetWidth() { return m_Params.Width; }
	virtual uint		GetHeight() { return m_Params.Height; }
	virtual uint		GetNumSurfaces() { return m_ColorSurfaces.Size(); }

	virtual void*		GetHandle();
	virtual void*		GetColorRenderTargetHandle( const uint Index );
	virtual void*		GetDepthStencilRenderTargetHandle();
	virtual ITexture*	GetColorTextureHandle( const uint Index );

	virtual void		AttachColorFrom( IRenderTarget* const pRenderTarget, const uint Index );
	virtual void		AttachDepthStencilFrom( IRenderTarget* const pRenderTarget );
	virtual void		FinishAttach();

	static D3DFORMAT	GetD3DFormat( const ERenderTargetFormat Format );

protected:
	void				BuildRenderTarget();

	SRenderTargetParams			m_Params;
	IRenderer*					m_Renderer;
	IDirect3DDevice9*			m_D3DDevice;

	Array<ITexture*>			m_ColorTextures;
	Array<IDirect3DSurface9*>	m_ColorSurfaces;
	ITexture*					m_DepthStencilTexture;	// Not used as a texture, but needed to manage AddRef/Release refcounting
	IDirect3DSurface9*			m_DepthStencilSurface;

	// HACKHACK: Best way I can think to reset "child" RTs is to explicitly parent them
	struct SParent
	{
		D3D9RenderTarget*	m_ParentRT;
		uint				m_ParentIndex;
	};
	Array<SParent>			m_ColorParents;
	D3D9RenderTarget*		m_DepthStencilParent;

	bool					m_IsChild;	// Doesn't own its textures
};

#endif // D3D9RENDERTARGET_H
