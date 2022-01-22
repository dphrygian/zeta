#ifndef IRENDERTARGET_H
#define IRENDERTARGET_H

enum ERenderTargetFormat
{
	ERTF_Unknown,
	ERTF_None,
	ERTF_X8R8G8B8,
	ERTF_A8R8G8B8,
	ERTF_A16B16G16R16,
	ERTF_A16B16G16R16F,
	ERTF_A32B32G32R32F,
	ERTF_R32F,
	ERTF_R32G32F,
	ERTF_D24S8,
};

struct SRenderTargetParams
{
	SRenderTargetParams(
		uint inWidth = 0,
		uint inHeight = 0,
		ERenderTargetFormat inColorFormat = ERTF_Unknown,
		ERenderTargetFormat inDepthStencilFormat = ERTF_Unknown )
	:	Width( inWidth )
	,	Height( inHeight )
	,	ColorFormat( inColorFormat )
	,	DepthStencilFormat( inDepthStencilFormat )
	{
	}

	uint				Width;
	uint				Height;
	ERenderTargetFormat	ColorFormat;
	ERenderTargetFormat	DepthStencilFormat;
};

class ITexture;

class IRenderTarget
{
public:
	virtual ~IRenderTarget() {}

	virtual void		Initialize( const SRenderTargetParams& Params ) = 0;

	virtual void		Release() = 0;
	virtual void		Reset() = 0;

	virtual uint		GetWidth() = 0;
	virtual uint		GetHeight() = 0;
	virtual uint		GetNumSurfaces() = 0;

	virtual void*		GetHandle() = 0;
	virtual void*		GetColorRenderTargetHandle( const uint Index ) = 0;
	virtual void*		GetDepthStencilRenderTargetHandle() = 0;
	virtual ITexture*	GetColorTextureHandle( const uint Index ) = 0;

	// For creating an RT from other existing RT resources
	// This will create a half-formed RT, only usable for writing to, *not* sampling from.
	virtual void		AttachColorFrom( IRenderTarget* const pRenderTarget, const uint Index ) = 0;
	virtual void		AttachDepthStencilFrom( IRenderTarget* const pRenderTarget ) = 0;
	virtual void		FinishAttach() = 0;
};

#endif // IRENDERTARGET_H
