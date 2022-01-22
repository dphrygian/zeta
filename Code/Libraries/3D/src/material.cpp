#include "core.h"
#include "material.h"
#include "configmanager.h"
#include "irenderer.h"
#include "shadermanager.h"
#include "sdpfactory.h"
#include "mathcore.h"
#include "targetmanager.h"

Material::Material()
:	m_Name()
,	m_ShaderProgram( NULL )
,	m_SDP( NULL )
,	m_Flags( 0 )
,	m_Bucket()
,	m_LightCubeMask( 0 )
,	m_RenderOps()
,	m_RenderState()
,	m_SamplerStates()
,	m_NumSamplers( 0 )
#if BUILD_DEV
,	m_ExpectedVD( 0 )
#endif
{
}

Material::~Material()
{
}

void Material::SetDefinition( const SimpleString& DefinitionName, IRenderer* const pRenderer, const uint VertexSignature )
{
	DEBUGASSERT( pRenderer );

	MAKEHASH( DefinitionName );

	m_Name = DefinitionName;

	STATICHASH( ClearColor );
	const bool ClearColor = ConfigManager::GetInheritedBool( sClearColor, false, sDefinitionName );
	m_RenderOps.m_ClearColor = ClearColor ? EE_True : EE_False;

	STATICHASH( ClearColorValue );
	const uint ClearColorValue = ConfigManager::GetInheritedInt( sClearColorValue, 0, sDefinitionName );
	m_RenderOps.m_ClearColorValue = ClearColorValue;

	STATICHASH( ClearStencil );
	const bool ClearStencil = ConfigManager::GetInheritedBool( sClearStencil, false, sDefinitionName );
	m_RenderOps.m_ClearStencil = ClearStencil ? EE_True : EE_False;

	STATICHASH( ClearStencilValue );
	const uint ClearStencilValue = ConfigManager::GetInheritedInt( sClearStencilValue, 0, sDefinitionName );
	m_RenderOps.m_ClearStencilValue = ClearStencilValue;

	STATICHASH( ClearDepth );
	const bool ClearDepth = ConfigManager::GetInheritedBool( sClearDepth, false, sDefinitionName );
	m_RenderOps.m_ClearDepth = ClearDepth ? EE_True : EE_False;

	STATICHASH( ClearDepthValue );
	const float ClearDepthValue = ConfigManager::GetInheritedFloat( sClearDepthValue, 0.0f, sDefinitionName );
	m_RenderOps.m_ClearDepthValue = ClearDepthValue;

	// DLP 29 May 2021: HACKHACK to let overrides change render target texture references.
	STATICHASH( RenderTarget );
	const HashedString RenderTarget = ConfigManager::GetInheritedHash( sRenderTarget, "", sDefinitionName );
	if( HashedString::NullString != RenderTarget )
	{
		TargetManager* const	pTargetManager	= pRenderer->GetTargetManager();
		DEVASSERT( pTargetManager );

		m_RenderOps.m_RenderTarget				= pTargetManager->GetRenderTarget( RenderTarget );
	}

	STATICHASH( CullMode );
	STATIC_HASHED_STRING( CW );
	const HashedString CullMode = ConfigManager::GetInheritedHash( sCullMode, sCW, sDefinitionName );
	m_RenderState.m_CullMode = RenderStates::GetCullMode( CullMode );

	STATICHASH( ZEnable );
	const bool ZEnable = ConfigManager::GetInheritedBool( sZEnable, true, sDefinitionName );
	m_RenderState.m_ZEnable = ZEnable ? EE_True : EE_False;

	STATICHASH( ZFunc );
	STATIC_HASHED_STRING( LessEqual );
	const HashedString ZFunc = ConfigManager::GetInheritedHash( sZFunc, sLessEqual, sDefinitionName );
	m_RenderState.m_ZFunc = RenderStates::GetFunc( ZFunc );

	STATICHASH( ZWriteEnable );
	const bool ZWriteEnable = ConfigManager::GetInheritedBool( sZWriteEnable, true, sDefinitionName );
	m_RenderState.m_ZWriteEnable = ZWriteEnable ? EE_True : EE_False;

	STATICHASH( AlphaBlendEnable );
	const bool AlphaBlendEnable = ConfigManager::GetInheritedBool( sAlphaBlendEnable, false, sDefinitionName );
	m_RenderState.m_AlphaBlendEnable = AlphaBlendEnable ? EE_True : EE_False;

	STATICHASH( SrcBlend );
	STATIC_HASHED_STRING( One );
	const HashedString SrcBlend = ConfigManager::GetInheritedHash( sSrcBlend, sOne, sDefinitionName );
	m_RenderState.m_SrcBlend = RenderStates::GetBlend( SrcBlend );

	STATICHASH( DestBlend );
	STATIC_HASHED_STRING( Zero );
	const HashedString DestBlend = ConfigManager::GetInheritedHash( sDestBlend, sZero, sDefinitionName );
	m_RenderState.m_DestBlend = RenderStates::GetBlend( DestBlend );

	STATICHASH( ColorWriteEnable );
	const bool ColorWriteEnable = ConfigManager::GetInheritedBool( sColorWriteEnable, true, sDefinitionName );
	m_RenderState.m_ColorWriteEnable = ColorWriteEnable ? EE_True : EE_False;

	STATICHASH( StencilEnable );
	const bool StencilEnable = ConfigManager::GetInheritedBool( sStencilEnable, false, sDefinitionName );
	m_RenderState.m_StencilEnable = StencilEnable ? EE_True : EE_False;

	STATICHASH( StencilFunc );
	STATIC_HASHED_STRING( Always );
	const HashedString StencilFunc = ConfigManager::GetInheritedHash( sStencilFunc, sAlways, sDefinitionName );
	m_RenderState.m_StencilFunc = RenderStates::GetFunc( StencilFunc );

	STATICHASH( StencilRef );
	const uint StencilRef = ConfigManager::GetInheritedInt( sStencilRef, 0, sDefinitionName );
	m_RenderState.m_StencilRef = StencilRef;

	STATICHASH( StencilMask );
	const uint StencilMask = ConfigManager::GetInheritedInt( sStencilMask, 0xffffffff, sDefinitionName );
	m_RenderState.m_StencilMask = StencilMask;

	STATICHASH( StencilWriteMask );
	const uint StencilWriteMask = ConfigManager::GetInheritedInt( sStencilWriteMask, 0xffffffff, sDefinitionName );
	m_RenderState.m_StencilWriteMask = StencilWriteMask;

	STATICHASH( StencilFailOp );
	STATIC_HASHED_STRING( Keep );
	const HashedString StencilFailOp = ConfigManager::GetInheritedHash( sStencilFailOp, sKeep, sDefinitionName );
	m_RenderState.m_StencilFailOp = RenderStates::GetStencilOp( StencilFailOp );

	STATICHASH( StencilZFailOp );
	const HashedString StencilZFailOp = ConfigManager::GetInheritedHash( sStencilZFailOp, sKeep, sDefinitionName );
	m_RenderState.m_StencilZFailOp = RenderStates::GetStencilOp( StencilZFailOp );

	STATICHASH( StencilPassOp );
	const HashedString StencilPassOp = ConfigManager::GetInheritedHash( sStencilPassOp, sKeep, sDefinitionName );
	m_RenderState.m_StencilPassOp = RenderStates::GetStencilOp( StencilPassOp );

	STATICHASH( SDP );
	STATIC_HASHED_STRING( Base );
	const HashedString SDP = ConfigManager::GetInheritedHash( sSDP, sBase, sDefinitionName );
	m_SDP = SDPFactory::GetSDPInstance( SDP );
	DEVASSERT( m_SDP );

	ShaderManager* const pShaderManager = pRenderer->GetShaderManager();
	DEBUGASSERT( pShaderManager );

	STATICHASH( VertexShader );
	const SimpleString VertexShader = ConfigManager::GetInheritedString( sVertexShader, "", sDefinitionName );

	STATICHASH( PixelShader );
	const SimpleString PixelShader = ConfigManager::GetInheritedString( sPixelShader, "", sDefinitionName );

	const SimpleString ShaderType = pRenderer->GetShaderType();
	MAKEHASH( ShaderType );

	MAKEHASHFROM( VertexShaderDef, VertexShader );
	const SimpleString VertexShaderVersion = ConfigManager::GetInheritedString( sShaderType, "", sVertexShaderDef );

	MAKEHASHFROM( PixelShaderDef, PixelShader );
	const SimpleString PixelShaderVersion = ConfigManager::GetInheritedString( sShaderType, "", sPixelShaderDef );

	m_ShaderProgram = pShaderManager->GetShaderProgram( VertexShaderVersion, PixelShaderVersion, VertexSignature );
	DEVASSERT( m_ShaderProgram );

	STATICHASH( NumSamplers );
	m_NumSamplers = ConfigManager::GetInheritedInt( sNumSamplers, 0, sDefinitionName );
	ASSERT( m_NumSamplers <= MAX_TEXTURE_STAGES );
	for( uint SamplerIndex = 0; SamplerIndex < m_NumSamplers; ++SamplerIndex )
	{
		const SimpleString SamplerDefinitionName = ConfigManager::GetInheritedSequenceString( "Sampler%d", SamplerIndex, "", sDefinitionName );
		SetSamplerDefinition( SamplerIndex, SamplerDefinitionName );
	}

	// DLP 29 May 2021: HACKHACK to let overrides change render target texture references.
	for( uint TextureIndex = 0; TextureIndex < m_NumSamplers; ++TextureIndex )
	{
		const HashedString		TextureTargetName	= ConfigManager::GetInheritedSequenceHash( "Texture%d", TextureIndex, "", sDefinitionName );
		if( HashedString::NullString != TextureTargetName )
		{
			TargetManager* const	pTargetManager		= pRenderer->GetTargetManager();
			DEVASSERT( pTargetManager );

			IRenderTarget* const	pRenderTarget		= pTargetManager->GetRenderTarget( TextureTargetName );
			DEVASSERT( pRenderTarget );

			ITexture* const			pTexture			= pRenderTarget->GetColorTextureHandle( 0 );
			DEVASSERT( pTexture );

			SetTexture( TextureIndex, pTexture );
		}
	}

#if BUILD_DEV
	STATICHASH( ExpectedVD );
	m_ExpectedVD = ConfigManager::GetInheritedInt( sExpectedVD, 0, sDefinitionName );
	ASSERTDESC( m_ExpectedVD != 0, "Material has no ExpectedVD, this should be added to make sure any mismatches are caught" );
#endif
}

void Material::SetSamplerDefinition( const uint SamplerStage, const SimpleString& SamplerDefinitionName )
{
	DEVASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	MAKEHASH( SamplerDefinitionName );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];

	STATICHASH( AddressU );
	STATIC_HASHED_STRING( Wrap );
	const HashedString AddressU = ConfigManager::GetInheritedHash( sAddressU, sWrap, sSamplerDefinitionName );
	SamplerState.m_AddressU = RenderStates::GetTextureAddress( AddressU );

	STATICHASH( AddressV );
	const HashedString AddressV = ConfigManager::GetInheritedHash( sAddressV, sWrap, sSamplerDefinitionName );
	SamplerState.m_AddressV = RenderStates::GetTextureAddress( AddressV );

	STATICHASH( AddressW );
	const HashedString AddressW = ConfigManager::GetInheritedHash( sAddressW, sWrap, sSamplerDefinitionName );
	SamplerState.m_AddressW = RenderStates::GetTextureAddress( AddressW );

	STATICHASH( MinFilter );
	STATIC_HASHED_STRING( Point );
	const HashedString MinFilter = ConfigManager::GetInheritedHash( sMinFilter, sPoint, sSamplerDefinitionName );
	SamplerState.m_MinFilter = RenderStates::GetTextureFilter( MinFilter );

	STATICHASH( MagFilter );
	const HashedString MagFilter = ConfigManager::GetInheritedHash( sMagFilter, sPoint, sSamplerDefinitionName );
	SamplerState.m_MagFilter = RenderStates::GetTextureFilter( MagFilter );

	STATICHASH( MipFilter );
	STATIC_HASHED_STRING( None );
	const HashedString MipFilter = ConfigManager::GetInheritedHash( sMipFilter, sNone, sSamplerDefinitionName );
	SamplerState.m_MipFilter = RenderStates::GetTextureFilter( MipFilter );

	STATICHASH( MaxAnisotropy );
	const int DefaultMaxAnisotropy	= ConfigManager::GetInt( sMaxAnisotropy, 8 );
	SamplerState.m_MaxAnisotropy	= Min( DefaultMaxAnisotropy, ConfigManager::GetInheritedInt( sMaxAnisotropy, 1, sSamplerDefinitionName ) );
}

void Material::SetTexture( const uint SamplerStage, ITexture* const pTexture )
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	SSamplerState& SamplerState = m_SamplerStates[ SamplerStage ];
	SamplerState.m_Texture		= pTexture;
}

ITexture* Material::GetTexture( const uint SamplerStage ) const
{
	//DEBUGASSERT( SamplerStage < m_NumSamplers );	// This can be legit if the mesh is created but never given a material definition; it has textures but not samplers per se
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	const SSamplerState&	SamplerState	= m_SamplerStates[ SamplerStage ];
	return SamplerState.m_Texture;
}

const SSamplerState& Material::GetSamplerState( const uint SamplerStage ) const
{
	DEBUGASSERT( SamplerStage < m_NumSamplers );
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	return m_SamplerStates[ SamplerStage ];
}

bool Material::SupportsTexture( const uint SamplerStage ) const
{
	DEBUGASSERT( SamplerStage < MAX_TEXTURE_STAGES );

	return SamplerStage < m_NumSamplers;
}

bool Material::SupportsAlphaBlend() const
{
	return ( m_RenderState.m_AlphaBlendEnable == EE_True );
}

void Material::SetFlags( const uint Flags, const uint Mask /*= MAT_ALL*/ )
{
	m_Flags &= ~Mask;
	m_Flags |= Flags;
}

void Material::SetFlag( const uint Flag, const bool Set )
{
	if( Set )
	{
		m_Flags |= Flag;
	}
	else
	{
		m_Flags &= ~Flag;
	}
}

void Material::CopySamplerStatesFrom( const Material& OtherMaterial )
{
	m_NumSamplers = OtherMaterial.m_NumSamplers;
	for( uint SamplerStageIndex = 0; SamplerStageIndex < OtherMaterial.m_NumSamplers; ++SamplerStageIndex )
	{
		// DLP 29 May 2021: I'm now using this for other things than just shadow overrides, and
		// there may be a render target texture already in this sample state that I want to keep.
		// I'll assume that if I have a texture, the rest of the sampler state is good too.
		ITexture* const pOldTexture = m_SamplerStates[ SamplerStageIndex ].m_Texture;
		if( NULL == pOldTexture )
		{
			m_SamplerStates[ SamplerStageIndex ] = OtherMaterial.m_SamplerStates[ SamplerStageIndex ];
		}
	}
	for( uint SamplerStageIndex = OtherMaterial.m_NumSamplers; SamplerStageIndex < MAX_TEXTURE_STAGES; ++SamplerStageIndex )
	{
		// Clear the rest to be safe
		m_SamplerStates[ SamplerStageIndex ] = SSamplerState();
	}
}
