#ifndef GL2CUBERENDERTARGET_H
#define GL2CUBERENDERTARGET_H

#include "irendertarget.h"
#include "gl2.h"

class GL2CubeRenderTarget : public IRenderTarget
{
public:
	GL2CubeRenderTarget();
	virtual ~GL2CubeRenderTarget();

	virtual void		Initialize( const SRenderTargetParams& Params );

	virtual void		Release() {}
	virtual void		Reset() {}

	virtual uint		GetWidth()			{ return m_Size; }
	virtual uint		GetHeight()			{ return m_Size; }
	virtual uint		GetNumSurfaces()	{ return 1; }

	virtual void*		GetHandle()										{ return &m_FrameBufferObject; }
	virtual void*		GetColorRenderTargetHandle( const uint Index )	{ Unused( Index ); return &m_CubemapTextureObject; }
	virtual void*		GetDepthStencilRenderTargetHandle()				{ return &m_DepthStencilRenderBufferObject; }
	virtual ITexture*	GetColorTextureHandle( const uint Index )		{ Unused( Index ); return m_CubemapTexture; }

	virtual void		AttachColorFrom( IRenderTarget* const pRenderTarget, const uint Index ) { Unused( pRenderTarget ); Unused( Index ); }
	virtual void		AttachDepthStencilFrom( IRenderTarget* const pRenderTarget ) { Unused( pRenderTarget ); }
	virtual void		FinishAttach() {}

private:
	void		CreateFBO();

	GLuint		m_FrameBufferObject;
	GLuint		m_CubemapTextureObject;
	ITexture*	m_CubemapTexture;
	GLuint		m_DepthStencilRenderBufferObject;

	uint		m_Size;
};

#endif // GL2CUBERENDERTARGET_H
