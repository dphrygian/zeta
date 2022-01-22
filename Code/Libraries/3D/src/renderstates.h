#ifndef RENDERSTATES_H
#define RENDERSTATES_H

#define RENDERSTATE_UNINITIALIZED 0x00facade	// HACKHACK: Initializing to an unlikely value because unsigned ints

class HashedString;
class ITexture;

enum ECullMode
{
	ECM_Unknown,
	ECM_None,
	ECM_CW,
	ECM_CCW,
};

enum EEnable
{
	EE_Unknown,
	EE_False,
	EE_True,
};

enum EFunc
{
	EF_Unknown,
	EF_Never,
	EF_Less,
	EF_Equal,
	EF_LessEqual,
	EF_Greater,
	EF_NotEqual,
	EF_GreaterEqual,
	EF_Always,
};

enum ESOp
{
	ESO_Unknown,
	ESO_Keep,
	ESO_Zero,
	ESO_Replace,
	ESO_Incr,		// Saturate
	ESO_Decr,		// Saturate
	ESO_Invert,
	ESO_IncrWrap,
	ESO_DecrWrap,
};

enum EBlend
{
	EB_Unknown,
	EB_Zero,
	EB_One,
	EB_SrcColor,
	EB_InvSrcColor,
	EB_SrcAlpha,
	EB_InvSrcAlpha,
	EB_DestAlpha,
	EB_InvDestAlpha,
	EB_DestColor,
	EB_InvDestColor,
	EB_SrcAlphaSat,
};

enum ETextureAddress
{
	ETA_Unknown,
	ETA_Wrap,
	ETA_Mirror,
	ETA_Clamp,
	ETA_Border,
};

enum ETextureFilter
{
	ETF_Unknown,
	ETF_None,		// For mipmapping
	ETF_Point,
	ETF_Linear,
	ETF_Anisotropic,
	ETF_SIZE
};

struct SRenderOps
{
	SRenderOps()
	:	m_ClearColor( EE_Unknown )
	,	m_ClearColorValue( RENDERSTATE_UNINITIALIZED )
	,	m_ClearStencil( EE_Unknown )
	,	m_ClearStencilValue( RENDERSTATE_UNINITIALIZED )
	,	m_ClearDepth( EE_Unknown )
	,	m_ClearDepthValue( -1.0f )
	,	m_RenderTarget( NULL )
	{
	}

	EEnable					m_ClearColor;
	uint					m_ClearColorValue;
	EEnable					m_ClearStencil;
	uint					m_ClearStencilValue;
	EEnable					m_ClearDepth;
	float					m_ClearDepthValue;
	class IRenderTarget*	m_RenderTarget;
};

struct SRenderState
{
	SRenderState()
	:	m_CullMode( ECM_Unknown )
	,	m_ZEnable( EE_Unknown )
	,	m_ZFunc( EF_Unknown )
	,	m_ZWriteEnable( EE_Unknown )
	,	m_DepthMin( -1.0f )
	,	m_DepthMax( -1.0f )
	,	m_AlphaBlendEnable( EE_Unknown )
	,	m_SrcBlend( EB_Unknown )
	,	m_DestBlend( EB_Unknown )
	,	m_ColorWriteEnable( EE_Unknown )
	,	m_StencilEnable( EE_Unknown )
	,	m_StencilFunc( EF_Unknown )
	,	m_StencilRef( RENDERSTATE_UNINITIALIZED )
	,	m_StencilMask( RENDERSTATE_UNINITIALIZED )
	,	m_StencilWriteMask( RENDERSTATE_UNINITIALIZED )
	,	m_StencilFailOp( ESO_Unknown )
	,	m_StencilZFailOp( ESO_Unknown )
	,	m_StencilPassOp( ESO_Unknown )
	{
	}

	// Instructions

	ECullMode	m_CullMode;
	EEnable		m_ZEnable;
	EFunc		m_ZFunc;
	EEnable		m_ZWriteEnable;
	float		m_DepthMin;
	float		m_DepthMax;
	EEnable		m_AlphaBlendEnable;
	EBlend		m_SrcBlend;
	EBlend		m_DestBlend;
	EEnable		m_ColorWriteEnable;
	EEnable		m_StencilEnable;
	EFunc		m_StencilFunc;
	uint		m_StencilRef;
	uint		m_StencilMask;
	uint		m_StencilWriteMask;
	ESOp		m_StencilFailOp;
	ESOp		m_StencilZFailOp;
	ESOp		m_StencilPassOp;
};

struct SSamplerState
{
	SSamplerState()
	:	m_Texture( NULL )
	,	m_AddressU( ETA_Unknown )
	,	m_AddressV( ETA_Unknown )
	,	m_AddressW( ETA_Unknown )
	,	m_MinFilter( ETF_Unknown )
	,	m_MagFilter( ETF_Unknown )
	,	m_MipFilter( ETF_Unknown )
	,	m_MaxAnisotropy( 0 )
	{
	}

	ITexture*		m_Texture;
	ETextureAddress	m_AddressU;
	ETextureAddress	m_AddressV;
	ETextureAddress	m_AddressW;
	ETextureFilter	m_MinFilter;
	ETextureFilter	m_MagFilter;
	ETextureFilter	m_MipFilter;
	uint			m_MaxAnisotropy;
};

namespace RenderStates
{
	ECullMode		GetCullMode( const HashedString& CullMode );
	EFunc			GetFunc( const HashedString& ZFunc );
	EBlend			GetBlend( const HashedString& Blend );
	ESOp			GetStencilOp( const HashedString& Op );
	ETextureAddress	GetTextureAddress( const HashedString& TextureAddress );
	ETextureFilter	GetTextureFilter( const HashedString& TextureFilter );
}

#endif // RENDERSTATES_H
