#ifndef RENDERERCOMMON_H
#define RENDERERCOMMON_H

#include "irenderer.h"
#include "matrix.h"
#include "array.h"
#include "view.h"
#include "map.h"
#include "list.h"
#include "set.h"
#include "vector4.h"
#include "frustum.h"

class Material;

class RendererCommon : public IRenderer
{
public:
	RendererCommon();
	~RendererCommon();

	virtual void	Initialize();

	virtual ShaderManager*				GetShaderManager();
	virtual TextureManager*				GetTextureManager();
	virtual TargetManager*				GetTargetManager();
	virtual void						SetTargetManager( TargetManager* const pTargetManager );
	virtual FontManager*				GetFontManager();
	virtual VertexDeclarationManager*	GetVertexDeclarationManager();
	virtual MeshFactory*				GetMeshFactory();

	virtual void	SetDisplay( Display* const pDisplay );

	virtual void	SetWorldMatrix( const Matrix& WorldMatrix );
	virtual void	SetViewMatrix( const Matrix& ViewMatrix );
	virtual void	SetProjectionMatrix( const Matrix& ProjectionMatrix );
	virtual const Matrix&	GetWorldMatrix()			{ return m_WorldMatrix; }
	virtual const Matrix&	GetViewMatrix()				{ return m_ViewMatrix; }
	virtual const Matrix&	GetProjectionMatrix()		{ return m_ProjectionMatrix; }
	virtual const Matrix&	GetViewProjectionMatrix()	{ return m_ViewProjectionMatrix; }

	virtual void	SetVertexShaderUniform( const HashedString& Parameter, const Vector4& Value )	{ SetVertexShaderFloat4s(	Parameter, Value.GetArray(), 1 ); }
	virtual void	SetVertexShaderUniform( const HashedString& Parameter, const Matrix& Value )	{ SetVertexShaderMatrices(	Parameter, Value.GetArray(), 1 ); }
	virtual void	SetPixelShaderUniform( const HashedString& Parameter, const Vector4& Value )	{ SetPixelShaderFloat4s(	Parameter, Value.GetArray(), 1 ); }
	virtual void	SetPixelShaderUniform( const HashedString& Parameter, const Matrix& Value )		{ SetPixelShaderMatrices(	Parameter, Value.GetArray(), 1 ); }

	virtual void	AddMesh( Mesh* pMesh );
	virtual void	AddBucket( const HashedString& Name, Bucket* pBucket );
	virtual Bucket*	GetBucket( const HashedString& Name ) const;
	virtual Bucket*	GetBucket( uint Index ) const;
	virtual void	FreeBuckets();
	virtual void	FlushBuckets();
	virtual void	SetBucketsEnabled( const HashedString& GroupTag, const bool Enabled );

	virtual void			FreeRenderTargets();
	virtual IRenderTarget*	GetCurrentRenderTarget();
	virtual IRenderTarget*	GetDefaultRenderTarget();

	// HACKHACK: Helper function because default RTs don't store their dimensions
	virtual Vector2			GetRenderTargetOrViewportDimensions() const;

	virtual void	AddDynamicVertexBuffer( IVertexBuffer* pBuffer );
	virtual void	RemoveDynamicVertexBuffer( IVertexBuffer* pBuffer );
	virtual void	ClearDynamicVertexBuffers();

	virtual IVertexDeclaration*	GetVertexDeclaration( const uint VertexSignature );

#if BUILD_DEV
	virtual void	DEBUGDrawLine( const Vector& Start, const Vector& End, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawTriangle( const Vector& V1, const Vector& V2, const Vector& V3, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawBox( const Vector& Min, const Vector& Max, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawFrustum( const Frustum& rFrustum, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawFrustum( const View& rView, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawCircleXY( const Vector& Center, float Radius, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawSphere( const Vector& Center, float Radius, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawEllipsoid( const Vector& Center, const Vector& Extents, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawCross( const Vector& Center, const float Length, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawArrow( const Vector& Root, const Angles& Direction, const float Length, unsigned int Color, const bool DepthTest = true );
	virtual void	DEBUGDrawCoords( const Vector& Location, const Angles& Orientation, const float Length, const bool DepthTest = true );

	virtual void	DEBUGDrawLine2D( const Vector& Start, const Vector& End, unsigned int Color );
	virtual void	DEBUGDrawBox2D( const Vector& Min, const Vector& Max, unsigned int Color );

	virtual void	DEBUGPrint( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, const Vector4& Color );
	virtual void	DEBUGPrint( const SimpleString& UTF8String, const SRect& Bounds, const SimpleString& FontName, const Vector4& Color, const Vector4& ShadowColor );
	virtual void	DEBUGPrint( const SimpleString& UTF8String, const Vector& Location, const View* const pView, const Display* const pDisplay, const SimpleString& FontName, const Vector4& Color, const Vector4& ShadowColor );
#endif // BUILD_DEV

#if BUILD_DEV
	virtual void			DEV_SetLockedFrustum( View* const pLockedView );
	virtual	bool			DEV_IsLockedFrustum() const;
	virtual const View&		DEV_GetLockedFrustumView() const;
	virtual const Frustum&	DEV_GetLockedFrustum() const;
#endif

	virtual Mesh*	Print( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, unsigned int Flags );
	virtual void	Arrange( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, unsigned int Flags, Vector2& OutExtents );

#if BUILD_DEV
	virtual SDEV_RenderStats&	DEV_GetStats();
#endif

protected:
	// NOTE: RenderBucket used to be virtual to minimize the number of virtual function calls performed
	// by the main render loop. I'm switching to a consolidated method with a virtual call per mesh,
	// which may adversely impact performance. Something to keep an eye on.
	virtual void	SetVertexArrays( Mesh* const pMesh ) = 0;
	bool			PassesFrustum( Mesh* const pMesh, const Frustum& ViewFrustum );
	void			RenderBucketMesh( Mesh* const pMesh, Bucket* const pBucket );
	void			RenderBucket( Bucket* pBucket );
	void			RenderShadowBucket( Bucket* const pShadowLightsBucket, Bucket* const pShadowCastersBucket );
	void			RenderBuckets();
	void			PostRenderBuckets();

	void			SetView( const View& rView );

	ECullMode		ResolveCullMode( const ECullMode CullMode ) const;

	void			ApplyMaterial( const Material& Material, Mesh* const pMesh, const View& CurrentView );
	void			ExecuteRenderOps( const SRenderOps& RenderOps );
	void			ApplyRenderState( const SRenderState& RenderState );
	void			ApplySamplerState( const uint SamplerStage, const SSamplerState& SamplerState );
	void			ResetSamplerState( const uint SamplerStage );

	void			ResetRenderState();

	Array<Bucket*>				m_OrderedBuckets;
	Map<HashedString, Bucket*>	m_NamedBuckets;

#if BUILD_DEV
	Array<Mesh*>				m_DEV_DeferredDeleteDebugMeshes;
#endif

	Matrix				m_WorldMatrix;
	Matrix				m_ViewMatrix;
	Matrix				m_ProjectionMatrix;

	// Cached to save remultiplying all the time
	Matrix				m_ViewProjectionMatrix;

	View				m_View;

	IRenderTarget*			m_CurrentRenderTarget;
	IRenderTarget*			m_DefaultRenderTarget;
	List<IRenderTarget*>	m_RenderTargets;	// TODO: Replace this with functionality in TargetManager
	Set<IVertexBuffer*>		m_DynamicVertexBuffers;	// Things wot get released and rebuilt when device is reset

	ShaderManager*				m_ShaderManager;
	TextureManager*				m_TextureManager;
	TargetManager*				m_TargetManager;
	FontManager*				m_FontManager;
	VertexDeclarationManager*	m_VertexDeclarationManager;
	MeshFactory*				m_MeshFactory;

	bool					m_DoFrustumCulling;
#if BUILD_DEV
	View*					m_DEV_CurrentView;
	View*					m_DEV_pLockedFrustumView;	// Pointer to compare with m_DEV_CurrentView
	View					m_DEV_LockedFrustumView;	// Saved view with location and rotation intact,e.g., for doing GatherVisibleSectors in world
	Frustum					m_DEV_LockedFrustum;		// Saved frustum itself, no need to rebuild every frame
#endif

	bool					m_DoMaterialSort;

	// Shadow device state to avoid querying hardware
	IShaderProgram*		m_ShaderProgram;
	IVertexShader*		m_VertexShader;
	IPixelShader*		m_PixelShader;
	IVertexDeclaration*	m_VertexDeclaration;
	SRenderState		m_RenderState;
	SSamplerState		m_SamplerStates[ MAX_TEXTURE_STAGES ];

	uint				m_MaxVertexAttribs;

	Display*			m_Display;

#if BUILD_DEV
	SDEV_RenderStats	m_DEV_RenderStats;
#endif
};

#endif // RENDERERCOMMON_H
