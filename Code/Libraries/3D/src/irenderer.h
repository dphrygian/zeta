#ifndef IRENDERER_H
#define IRENDERER_H

#include "3d.h"
#include "array.h"
#include "display.h"
#include "irendertarget.h"
#include "renderstates.h"
#include "simplestring.h"
#include "vector.h"
#include "clock.h"

class Display;
class Matrix;
class View;
class IVertexBuffer;
class IVertexDeclaration;
class IIndexBuffer;
class Mesh;
class ITexture;
class IVertexShader;
class IPixelShader;
class IShaderProgram;
class IRenderTarget;
class ShaderManager;
class TextureManager;
class TargetManager;
class FontManager;
class VertexDeclarationManager;
class MeshFactory;
class Bucket;
class Font;
class Angles;
class Vector4;
class Vector2;

struct SRect;
struct SRenderTargetParams;
struct SCubemapData;

#define MAX_MRT_COUNT 4

enum EMultiSampleType
{
	EMST_None,
	//EMST_NONMASKABLE,
	EMST_2X,
	EMST_4X,
	EMST_8X,
};

class IRenderer
{
public:
	virtual ~IRenderer() {}

	virtual void	Initialize() = 0;
	virtual void	Tick() = 0;
	virtual void	Clear( const uint Flags, const uint Color = 0xff000000, const float Depth = 1.0f, const uint Stencil = 0 ) = 0;
	virtual void	Present() = 0;
	virtual bool	CanReset() = 0;
	virtual bool	Reset() = 0;
	virtual void	Refresh() = 0;

	// Currently only used to determine which projection matrix to use.
	virtual bool	IsOpenGL() = 0;

	virtual void	SetVertexShader( IVertexShader* const pVertexShader ) = 0;
	virtual void	SetPixelShader( IPixelShader* const pPixelShader ) = 0;
	virtual void	SetShaderProgram( IShaderProgram* const pShaderProgram ) = 0;

	virtual SimpleString	GetShaderType() const = 0;

	virtual void	SetCullMode( const ECullMode CullMode ) = 0;
	virtual void	SetColorWriteEnable( const EEnable ColorWriteEnable ) = 0;
	virtual void	SetAlphaBlendEnable( const EEnable AlphaBlendEnable ) = 0;
	virtual void	SetBlend( const EBlend SrcBlend, const EBlend DestBlend ) = 0;
	virtual void	SetZEnable( const EEnable ZEnable ) = 0;
	virtual void	SetZFunc( const EFunc ZFunc ) = 0;
	virtual void	SetZWriteEnable( const EEnable ZWriteEnable ) = 0;
	virtual void	SetZRange( const float DepthMin, const float DepthMax ) = 0;	// Set from bucket, not material
	virtual void	SetStencilEnable( const EEnable StencilEnable ) = 0;
	virtual void	SetStencilFunc( const EFunc StencilFunc, const uint StencilRef, const uint StencilMask ) = 0;
	virtual void	SetStencilOp( const ESOp StencilFailOp, const ESOp StencilZFailOp, const ESOp StencilPassOp ) = 0;
	virtual void	SetStencilWriteMask( const uint StencilWriteMask ) = 0;

	virtual void	SetTexture( const uint SamplerStage, ITexture* const pTexture ) = 0;
	virtual void	ResetTexture( const uint SamplerStage ) = 0;
	virtual void	SetAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV ) = 0;
	virtual void	SetMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter ) = 0;
	virtual void	SetMagFilter( const uint SamplerStage, const ETextureFilter MagFilter ) = 0;
	virtual void	SetMaxAnisotropy( const uint SamplerStage, const uint MaxAnisotropy ) = 0;

	virtual void	SetCubemap( const uint SamplerStage, ITexture* const pCubemap ) = 0;
	virtual void	SetCubemapAddressing( const uint SamplerStage, const ETextureAddress AddressU, const ETextureAddress AddressV, const ETextureAddress AddressW ) = 0;
	virtual void	SetCubemapMinMipFilters( const uint SamplerStage, const ETextureFilter MinFilter, const ETextureFilter MipFilter ) = 0;
	virtual void	SetCubemapMagFilter( const uint SamplerStage, const ETextureFilter MagFilter ) = 0;

	virtual void	SetVertexShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s ) = 0;
	virtual void	SetVertexShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices ) = 0;
	virtual void	SetPixelShaderFloat4s( const HashedString& Parameter, const float* const pFloats, const uint NumFloat4s ) = 0;
	virtual void	SetPixelShaderMatrices( const HashedString& Parameter, const float* const pFloats, const uint NumMatrices ) = 0;

	// Helper functions for the common cases
	virtual void	SetVertexShaderUniform( const HashedString& Parameter, const Vector4& Value ) = 0;
	virtual void	SetVertexShaderUniform( const HashedString& Parameter, const Matrix& Value ) = 0;
	virtual void	SetPixelShaderUniform( const HashedString& Parameter, const Vector4& Value ) = 0;
	virtual void	SetPixelShaderUniform( const HashedString& Parameter, const Matrix& Value ) = 0;

	virtual void	SetVertexDeclaration( IVertexDeclaration* const pVertexDeclaration ) = 0;
	virtual void	DrawElements( IVertexBuffer* const pVertexBuffer, IIndexBuffer* const pIndexBuffer ) = 0;

	typedef void ( *RestoreDeviceCallback )( void* );
	struct SRestoreDeviceCallback
	{
		SRestoreDeviceCallback()
		:	m_Callback( NULL )
		,	m_Void( NULL )
		{
		}

		RestoreDeviceCallback	m_Callback;
		void*					m_Void;
	};
	virtual void	SetRestoreDeviceCallback( const SRestoreDeviceCallback& Callback ) = 0;

	virtual void	SetMultiSampleType( EMultiSampleType MultiSampleType ) = 0;
	virtual void	GetBestSupportedMultiSampleType( EMultiSampleType& OutMultiSampleType, c_uint32* pOutQualityLevels = NULL ) = 0;

	virtual ERenderTargetFormat	GetBestSupportedRenderTargetFormat( const ERenderTargetFormat Format ) const = 0;

	virtual void	SetWorldMatrix( const Matrix& WorldMatrix ) = 0;
	virtual void	SetViewMatrix( const Matrix& ViewMatrix ) = 0;
	virtual void	SetProjectionMatrix( const Matrix& ProjectionMatrix ) = 0;
	virtual const Matrix&	GetWorldMatrix() = 0;
	virtual const Matrix&	GetViewMatrix() = 0;
	virtual const Matrix&	GetProjectionMatrix() = 0;
	virtual const Matrix&	GetViewProjectionMatrix() = 0;

	virtual void	AddMesh( Mesh* pMesh ) = 0;
	virtual void	AddBucket( const HashedString& Name, Bucket* pBucket ) = 0;	// Add (ordered) buckets for doing different render passes. Renderer owns memory.
	virtual Bucket*	GetBucket( const HashedString& Name ) const = 0;
	virtual Bucket*	GetBucket( uint Index ) const = 0;
	virtual void	FreeBuckets() = 0;
	virtual void	FlushBuckets() = 0;
	virtual void	SetBucketsEnabled( const HashedString& GroupTag, const bool Enabled ) = 0;

	virtual IVertexBuffer*		CreateVertexBuffer() = 0;
	virtual void				AddDynamicVertexBuffer( IVertexBuffer* pBuffer ) = 0;
	virtual void				RemoveDynamicVertexBuffer( IVertexBuffer* pBuffer ) = 0;
	virtual void				ClearDynamicVertexBuffers() = 0;
	virtual IVertexDeclaration*	GetVertexDeclaration( const uint VertexSignature ) = 0;
	virtual IVertexDeclaration*	CreateVertexDeclaration() = 0;
	virtual IIndexBuffer*		CreateIndexBuffer() = 0;
	virtual ITexture*			CreateTexture( const char* Filename, const bool NoMips ) = 0;
	virtual ITexture*			CreateCubemap( const SimpleString& CubemapDef, const bool NoMips ) = 0;
	virtual ITexture*			CreateCubemap( const SCubemapData& CubemapData ) = 0;
	virtual IVertexShader*		CreateVertexShader( const SimpleString& Filename ) = 0;
	virtual IPixelShader*		CreatePixelShader( const SimpleString& Filename ) = 0;
	virtual IShaderProgram*		CreateShaderProgram( IVertexShader* const pVertexShader, IPixelShader* const pPixelShader, IVertexDeclaration* const pVertexDeclaration ) = 0;
	virtual IRenderTarget*		CreateRenderTarget( const SRenderTargetParams& Params ) = 0;
	virtual IRenderTarget*		CreateRenderTarget() = 0;
	virtual IRenderTarget*		CreateCubeRenderTarget( const SRenderTargetParams& Params ) = 0;
	virtual void				FreeRenderTargets() = 0;

	virtual void			SetRenderTarget( IRenderTarget* const pRenderTarget ) = 0;
	virtual void			SetCubeRenderTarget( IRenderTarget* const pRenderTarget, const uint Face ) = 0;
	virtual const Angles*	GetCubemapRenderOrientations() const = 0;

	virtual IRenderTarget*	GetCurrentRenderTarget() = 0;
	virtual IRenderTarget*	GetDefaultRenderTarget() = 0;

	// HACKHACK: Helper function because default RTs don't store their dimensions
	virtual Vector2			GetRenderTargetOrViewportDimensions() const = 0;

	virtual void			SaveScreenshot( const SimpleString& Filename ) = 0;

#if BUILD_DEV
	virtual void	DEBUGDrawLine( const Vector& Start, const Vector& End, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawTriangle( const Vector& V1, const Vector& V2, const Vector& V3, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawBox( const Vector& Min, const Vector& Max, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawFrustum( const Frustum& rFrustum, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawFrustum( const View& rView, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawCircleXY( const Vector& Center, float Radius, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawSphere( const Vector& Center, float Radius, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawEllipsoid( const Vector& Center, const Vector& Extents, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawCross( const Vector& Center, const float Length, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawArrow( const Vector& Root, const Angles& Direction, const float Length, unsigned int Color, const bool DepthTest = true ) = 0;
	virtual void	DEBUGDrawCoords( const Vector& Location, const Angles& Orientation, const float Length, const bool DepthTest = true ) = 0;

	// 2D versions draw to the HUD instead of the world.
	virtual void	DEBUGDrawLine2D( const Vector& Start, const Vector& End, unsigned int Color ) = 0;
	virtual void	DEBUGDrawBox2D( const Vector& Min, const Vector& Max, unsigned int Color ) = 0;

	virtual void	DEBUGPrint( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, const Vector4& Color ) = 0;
	virtual void	DEBUGPrint( const SimpleString& UTF8String, const SRect& Bounds, const SimpleString& FontName, const Vector4& Color, const Vector4& ShadowColor ) = 0;
	virtual void	DEBUGPrint( const SimpleString& UTF8String, const Vector& Location, const View* const pView, const Display* const pDisplay, const SimpleString& FontName, const Vector4& Color, const Vector4& ShadowColor ) = 0;
#endif // BUILD_DEV

#if BUILD_DEV
	virtual void			DEV_SetLockedFrustum( View* const pLockedView ) = 0;
	virtual	bool			DEV_IsLockedFrustum() const = 0;
	virtual const View&		DEV_GetLockedFrustumView() const = 0;
	virtual const Frustum&	DEV_GetLockedFrustum() const = 0;
#endif

	// Flags are defined in font.h
	virtual Mesh*	Print( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, unsigned int Flags ) = 0;
	virtual void	Arrange( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, unsigned int Flags, Vector2& OutExtents ) = 0;

	virtual ShaderManager*				GetShaderManager() = 0;
	virtual TextureManager*				GetTextureManager() = 0;
	virtual TargetManager*				GetTargetManager() = 0;
	virtual void						SetTargetManager( TargetManager* const pTargetManager ) = 0;	// Target manager will be created at the application level
	virtual FontManager*				GetFontManager() = 0;
	virtual VertexDeclarationManager*	GetVertexDeclarationManager() = 0;
	virtual MeshFactory*				GetMeshFactory() = 0;

	virtual bool	IsValid() = 0;

	virtual void	EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes ) = 0;

	virtual bool	SupportsSM3() = 0;
	virtual bool	SupportsSM2() = 0;

	virtual void	SetDisplay( Display* const pDisplay ) = 0;

#if BUILD_DEV
	struct SDEV_RenderStats
	{
		struct SDEV_ShadowLight
		{
			SDEV_ShadowLight()
			:	m_Location()
			,	m_Radius( 0.0f )
			,	m_NumMeshes( 0 )
			{
			}

			Vector	m_Location;
			float	m_Radius;
			uint	m_NumMeshes;
		};

		SDEV_RenderStats()
		:	m_NumMeshes( 0 )
		,	m_NumDrawCalls( 0 )
		,	m_NumPrimitives( 0 )
		,	m_NumShadowLights( 0 )
		,	m_NumShadowMeshes( 0 )
		,	m_ShadowLights()
		,	m_ShadowTime( 0 )
		,	m_LastClockTime( 0 )
		,	m_FPS( 0.0f )
		,	m_FPSHistory()
		,	m_FPSHistoryIndex( 0 )
		,	m_AvgFPS( 0.0f )
		,	m_RenderTime( 0 )
		{
			m_ShadowLights.SetDeflate( false );
			m_FPSHistory.SetDeflate( false );
			m_FPSHistory.Reserve( 15 );	// How many frames we smooth the average FPS over... I'm trying to keep this small so it updates quickly, while still being stable.
		}

		void	Reset()
		{
			m_NumMeshes			= 0;
			m_NumDrawCalls		= 0;
			m_NumPrimitives		= 0;
			m_NumShadowLights	= 0;
			m_NumShadowMeshes	= 0;
			m_ShadowLights.Clear();
			RESET_CLOCK( m_ShadowTime );
			m_FPS				= 0.0f;
			// Don't reset FPS history stuff each frame
			m_RenderTime		= 0;
		}

		void	UpdateFPS()
		{
			const CLOCK_T	CurrentTime			= Clock::GetCurrentTimeCounter();
			const float		DeltaTime			= Clock::GetDeltaTimeSeconds( CurrentTime - m_LastClockTime );
			m_LastClockTime						= CurrentTime;

			m_FPS								= 1.0f / DeltaTime;

			if( m_FPSHistory.Size() <= m_FPSHistoryIndex )
			{
				m_FPSHistory.Resize( m_FPSHistoryIndex + 1 );
			}
			m_FPSHistory[ m_FPSHistoryIndex ]	= m_FPS;
			m_FPSHistoryIndex					= ( m_FPSHistoryIndex + 1 ) % m_FPSHistory.GetCapacity();

			float			FPSHistoriesSum		= 0.0f;
			FOR_EACH_ARRAY( FPSHistoryIter, m_FPSHistory, float )
			{
				const float FPSHistory			= FPSHistoryIter.GetValue();
				FPSHistoriesSum					+= FPSHistory;
			}
			m_AvgFPS							= FPSHistoriesSum / m_FPSHistory.Size();
		}

		uint					m_NumMeshes;
		uint					m_NumDrawCalls;
		uint					m_NumPrimitives;

		uint					m_NumShadowLights;
		uint					m_NumShadowMeshes;
		Array<SDEV_ShadowLight>	m_ShadowLights;
		CLOCK_T					m_ShadowTime;

		CLOCK_T					m_LastClockTime;
		float					m_FPS;
		Array<float>			m_FPSHistory;
		uint					m_FPSHistoryIndex;
		float					m_AvgFPS;

		CLOCK_T					m_RenderTime;
	};

	virtual SDEV_RenderStats&	DEV_GetStats() = 0;
#endif
};

#endif // IRENDERER_H
