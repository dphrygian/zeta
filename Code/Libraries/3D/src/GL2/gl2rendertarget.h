#ifndef GL2RENDERTARGET_H
#define GL2RENDERTARGET_H

#include "irendertarget.h"
#include "gl2.h"
#include "array.h"

class GL2RenderTarget : public IRenderTarget
{
public:
	GL2RenderTarget();
	virtual ~GL2RenderTarget();

	virtual void		Initialize( const SRenderTargetParams& Params );

	virtual void		Release();
	virtual void		Reset();

	virtual uint		GetWidth() { return m_Width; }
	virtual uint		GetHeight() { return m_Height; }
	virtual uint		GetNumSurfaces() { return m_ColorTextureObjects.Size(); }

	virtual void*		GetHandle();
	virtual void*		GetColorRenderTargetHandle( const uint Index );
	virtual void*		GetDepthStencilRenderTargetHandle();
	virtual ITexture*	GetColorTextureHandle( const uint Index );

	virtual void		AttachColorFrom( IRenderTarget* const pRenderTarget, const uint Index );
	virtual void		AttachDepthStencilFrom( IRenderTarget* const pRenderTarget );
	virtual void		FinishAttach();

	static GLenum		GetGLFormat( const ERenderTargetFormat Format );

private:
	void				CreateFBO();

	GLuint				m_FrameBufferObject;
	Array<GLuint>		m_ColorTextureObjects;
	Array<ITexture*>	m_ColorTextures;
	GLuint				m_DepthStencilRenderBufferObject;

	uint				m_Width;
	uint				m_Height;

	bool				m_IsChild;	// Doesn't own its textures
};

#endif // GL2RENDERTARGET_H
