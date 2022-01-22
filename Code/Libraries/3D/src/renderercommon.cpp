#include "core.h"
#include "renderercommon.h"
#include "bucket.h"
#include "mesh.h"
#include "meshfactory.h"
#include "shadermanager.h"
#include "texturemanager.h"
#include "targetmanager.h"
#include "font.h"
#include "fontmanager.h"
#include "vertexdeclarationmanager.h"
#include "ivertexbuffer.h"
#include "ivertexdeclaration.h"
#include "configmanager.h"
#include "shaderdataprovider.h"
#include "mathcore.h"
#include "itexture.h"

RendererCommon::RendererCommon()
:	m_OrderedBuckets()
,	m_NamedBuckets()
#if BUILD_DEV
,	m_DEV_DeferredDeleteDebugMeshes()
#endif
,	m_WorldMatrix()
,	m_ViewMatrix()
,	m_ProjectionMatrix()
,	m_ViewProjectionMatrix()
,	m_View()
,	m_CurrentRenderTarget( NULL )
,	m_DefaultRenderTarget( NULL )
,	m_RenderTargets()
,	m_DynamicVertexBuffers()
,	m_ShaderManager( NULL )
,	m_TextureManager( NULL )
,	m_TargetManager( NULL )
,	m_FontManager( NULL )
,	m_VertexDeclarationManager( NULL )
,	m_MeshFactory( NULL )
,	m_DoFrustumCulling( false )
#if BUILD_DEV
,	m_DEV_CurrentView( NULL )
,	m_DEV_pLockedFrustumView( NULL )
,	m_DEV_LockedFrustumView()
,	m_DEV_LockedFrustum()
#endif
,	m_DoMaterialSort( false )
,	m_ShaderProgram( NULL )
,	m_VertexShader( NULL )
,	m_PixelShader( NULL )
,	m_VertexDeclaration( NULL )
,	m_RenderState()
,	m_SamplerStates()
,	m_MaxVertexAttribs( 0 )
,	m_Display( NULL )
#if BUILD_DEV
,	m_DEV_RenderStats()
#endif
{
#if BUILD_DEV
	m_DEV_DeferredDeleteDebugMeshes.SetDeflate( false );
#endif
}

RendererCommon::~RendererCommon()
{
	SafeDelete( m_DefaultRenderTarget );

	SafeDelete( m_ShaderManager );
	SafeDelete( m_TextureManager );
	// NOTE: Renderer does not own m_TargetManager! It used to not be a member of the 3D library, and I want to avoid breaking things.
	SafeDelete( m_FontManager );
	SafeDelete( m_VertexDeclarationManager );
	SafeDelete( m_MeshFactory );

	FreeBuckets();
}

void RendererCommon::Initialize()
{
	XTRACE_FUNCTION;

	STATICHASH( Renderer );

	STATICHASH( FrustumCulling );
	m_DoFrustumCulling =
		ConfigManager::GetBool( sFrustumCulling, false, sRenderer ) ||
		ConfigManager::GetBool( sFrustumCulling );	// Support for old projects

	STATICHASH( DoMaterialSort );
	m_DoMaterialSort = ConfigManager::GetBool( sDoMaterialSort, false, sRenderer );
}

void RendererCommon::SetWorldMatrix( const Matrix& WorldMatrix )
{
	m_WorldMatrix = WorldMatrix;
}

void RendererCommon::SetViewMatrix( const Matrix& ViewMatrix )
{
	m_ViewMatrix			= ViewMatrix;
	m_ViewProjectionMatrix	= ViewMatrix * m_ProjectionMatrix;
}

void RendererCommon::SetProjectionMatrix( const Matrix& ProjectionMatrix )
{
	m_ProjectionMatrix		= ProjectionMatrix;
	m_ViewProjectionMatrix	= m_ViewMatrix * ProjectionMatrix;
}

void RendererCommon::AddMesh( Mesh* pMesh )
{
	XTRACE_FUNCTION;

	DEVASSERT( pMesh );

	if( pMesh->m_VertexBuffer->GetNumVertices() == 0 )
	{
		return;
	}

	const HashedString& PrescribedBucket = pMesh->GetBucket();
	if( PrescribedBucket == HashedString::NullString )
	{
		// Pick one or more buckets by flags
#if BUILD_DEV
		uint NumBucketsAdded = 0;
#endif

		const uint MaterialFlags = pMesh->GetMaterialFlags();

		Bucket* pBucket;
		uint NumBuckets = m_OrderedBuckets.Size();
		for( uint i = 0; i < NumBuckets; ++i )
		{
			pBucket = m_OrderedBuckets[i];
			// This conditional means that the mesh must have all the
			// flags of the bucket and none of the filtered flags
			if( ( MaterialFlags & pBucket->m_Flags ) == pBucket->m_Flags &&
				( MaterialFlags & pBucket->m_FilterFlags ) == 0 )
			{
#if BUILD_DEV
				++NumBucketsAdded;
#endif
				pBucket->m_Meshes.PushBack( pMesh );
				if( pBucket->m_ExclusiveMeshes )
				{
					return;
				}
			}
		}

		DEVASSERTDESC( NumBucketsAdded > 0, "Mesh was added but fell into no buckets." );
	}
	else
	{
		Bucket* const pBucket = GetBucket( PrescribedBucket );
		if( pBucket )
		{
			pBucket->m_Meshes.PushBack( pMesh );
		}
		else
		{
			DEVWARNDESC( "Mesh was added but prescribed bucket did not exist." );
		}
	}
}

void RendererCommon::AddBucket( const HashedString& Name, Bucket* pBucket )
{
#if BUILD_DEV
	pBucket->m_DEV_Name = Name;
#endif
	m_OrderedBuckets.PushBack( pBucket );
	m_NamedBuckets[ Name ] = pBucket;
}

Bucket*	RendererCommon::GetBucket( const HashedString& Name ) const
{
	Map<HashedString, Bucket*>::Iterator BucketIter = m_NamedBuckets.Search( Name );
	return BucketIter.IsValid() ? BucketIter.GetValue() : NULL;
}

Bucket*	RendererCommon::GetBucket( uint Index ) const
{
	if( Index < m_OrderedBuckets.Size() )
	{
		return m_OrderedBuckets[ Index ];
	}
	return NULL;
}

void RendererCommon::FreeBuckets()
{
	uint NumBuckets = m_OrderedBuckets.Size();
	for( uint i = 0; i < NumBuckets; ++i )
	{
		SafeDelete( m_OrderedBuckets[i] );
	}
	m_OrderedBuckets.Clear();
	m_NamedBuckets.Clear();
}

/*virtual*/ void RendererCommon::FlushBuckets()
{
	for( uint BucketIndex = 0; BucketIndex < m_OrderedBuckets.Size(); ++BucketIndex )
	{
		Bucket* const pBucket = m_OrderedBuckets[ BucketIndex ];
		if( pBucket )
		{
			pBucket->m_Meshes.Clear();
		}
	}
}

/*virtual*/ void RendererCommon::SetBucketsEnabled( const HashedString& GroupTag, const bool Enabled )
{
	FOR_EACH_ARRAY( BucketIter, m_OrderedBuckets, Bucket* )
	{
		Bucket* const pBucket = BucketIter.GetValue();

		if( pBucket->m_Tag != GroupTag )
		{
			continue;
		}

		pBucket->m_Enabled = Enabled;
	}
}

IVertexDeclaration* RendererCommon::GetVertexDeclaration( const uint VertexSignature )
{
	return GetVertexDeclarationManager()->GetVertexDeclaration( VertexSignature );
}

void RendererCommon::FreeRenderTargets()
{
	m_RenderTargets.Clear();
}

IRenderTarget* RendererCommon::GetCurrentRenderTarget()
{
	return m_CurrentRenderTarget;
}

IRenderTarget* RendererCommon::GetDefaultRenderTarget()
{
	return m_DefaultRenderTarget;
}

Vector2 RendererCommon::GetRenderTargetOrViewportDimensions() const
{
	Vector2 RenderTargetDimensions;

	if( NULL != m_CurrentRenderTarget && m_DefaultRenderTarget != m_CurrentRenderTarget )
	{
		RenderTargetDimensions.x = static_cast<float>( m_CurrentRenderTarget->GetWidth() );
		RenderTargetDimensions.y = static_cast<float>( m_CurrentRenderTarget->GetHeight() );
	}
	else if( NULL != m_Display )
	{
		RenderTargetDimensions.x = static_cast<float>( m_Display->m_Width );
		RenderTargetDimensions.y = static_cast<float>( m_Display->m_Height );
	}
	else
	{
		DEVWARN;
	}

	return RenderTargetDimensions;
}

void RendererCommon::AddDynamicVertexBuffer( IVertexBuffer* pBuffer )
{
	m_DynamicVertexBuffers.Insert( pBuffer );
}

void RendererCommon::RemoveDynamicVertexBuffer( IVertexBuffer* pBuffer )
{
	m_DynamicVertexBuffers.Remove( pBuffer );
}

void RendererCommon::ClearDynamicVertexBuffers()
{
	m_DynamicVertexBuffers.Clear();
}

/*virtual*/ void RendererCommon::SetDisplay( Display* const pDisplay )
{
	m_Display = pDisplay;
}

ShaderManager* RendererCommon::GetShaderManager()
{
	if( !m_ShaderManager )
	{
		m_ShaderManager = new ShaderManager( this );
	}
	return m_ShaderManager;
}

TextureManager* RendererCommon::GetTextureManager()
{
	if( !m_TextureManager )
	{
		m_TextureManager = new TextureManager( this );
	}
	return m_TextureManager;
}

TargetManager* RendererCommon::GetTargetManager()
{
	DEVASSERTDESC( NULL != m_TargetManager, "Must call SetTargetManager before GetTargetManager!" );
	return m_TargetManager;
}

void RendererCommon::SetTargetManager( TargetManager* const pTargetManager )
{
	m_TargetManager = pTargetManager;
}

FontManager* RendererCommon::GetFontManager()
{
	if( !m_FontManager )
	{
		m_FontManager = new FontManager( this );
	}
	return m_FontManager;
}

VertexDeclarationManager* RendererCommon::GetVertexDeclarationManager()
{
	if( !m_VertexDeclarationManager )
	{
		m_VertexDeclarationManager = new VertexDeclarationManager( this );
	}
	return m_VertexDeclarationManager;
}

MeshFactory* RendererCommon::GetMeshFactory()
{
	if( !m_MeshFactory )
	{
		m_MeshFactory = new MeshFactory( this );
	}
	return m_MeshFactory;
}

void RendererCommon::RenderBucket( Bucket* pBucket )
{
	XTRACE_FUNCTION;

	PROFILE_FUNCTION;

	if( !pBucket->m_Enabled )
	{
		return;
	}

	if( pBucket->m_RenderTarget )
	{
		SetRenderTarget( pBucket->m_RenderTarget );
	}

	Clear( pBucket->m_ClearFlags, pBucket->m_ClearColor, pBucket->m_ClearDepth, pBucket->m_ClearStencil );

	// Depth range is set from bucket, not material
	SetZRange( pBucket->m_DepthMin, pBucket->m_DepthMax );

	if( pBucket->m_View )
	{
#if BUILD_DEV
		m_DEV_CurrentView = pBucket->m_View;
#endif
		SetView( *pBucket->m_View );
	}

	if( pBucket->m_AlphaSortMeshes || ( pBucket->m_Flags & MAT_ALPHA ) )
	{
		pBucket->AlphaSortMeshes( m_View );
	}
	else if( m_DoMaterialSort && pBucket->m_SortByMaterial )
	{
		pBucket->SortByMaterials();
	}

	const bool		DoFrustumCulling	= m_DoFrustumCulling && pBucket->m_DoFrustumCulling;
#if BUILD_DEV
	const Frustum	ViewFrustum			= ( m_DEV_pLockedFrustumView == m_DEV_CurrentView ) ? m_DEV_LockedFrustum : Frustum( GetViewProjectionMatrix() );
#else
	const Frustum	ViewFrustum			= Frustum( GetViewProjectionMatrix() );
#endif
	const uint		NumMeshes			= pBucket->m_Meshes.Size();
	for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
	{
		Mesh* const pMesh = pBucket->m_Meshes[ MeshIndex ];

		// Frustum culling--I can't do this earlier, when a mesh is added, because
		// it might be visible in one view (e.g., shadow map depth) and not to player.
		if( !DoFrustumCulling || PassesFrustum( pMesh, ViewFrustum ) )
		{
			RenderBucketMesh( pMesh, pBucket );
		}

#if BUILD_DEV
		if( pMesh->m_DEV_IsDebugMesh )
		{
			m_DEV_DeferredDeleteDebugMeshes.PushBackUnique( pMesh );
		}
#endif
	}
}

bool RendererCommon::PassesFrustum( Mesh* const pMesh, const Frustum& ViewFrustum )
{
	DEVASSERT( pMesh );

	const uint MaterialFlags = pMesh->GetMaterialFlags();

	return
#if BUILD_DEV
		MaterialFlags & MAT_DEBUG_ALWAYS	||
#endif
		MaterialFlags & MAT_ALWAYS			||
		ViewFrustum.Intersects( pMesh->m_AABB );
}

void RendererCommon::RenderBucketMesh( Mesh* const pMesh, Bucket* const pBucket )
{
	XTRACE_FUNCTION;

	// DLP 12 Oct 2021: No need to profile this and it's taking real time.
	//PROFILE_FUNCTION;

	DEVASSERT( pMesh );
	DEVASSERT( pMesh->m_VertexBuffer );
	DEVASSERT( pMesh->m_IndexBuffer );
	DEVASSERT( pMesh->m_VertexBuffer->GetNumVertices() > 0 );
	DEVASSERT( pBucket );

	const bool	HasMaterialOverrides	= pBucket->HasMaterialOverrides();
	const bool	UseMaterialOverride		= HasMaterialOverrides && pMesh->GetAllowMaterialOverrides();

	SetWorldMatrix( pMesh->GetConcatenatedTransforms() );
	SetVertexDeclaration( pMesh->m_VertexDeclaration );
	SetVertexArrays( pMesh );

	if( pMesh->HasMultiPassMaterials() )
	{
		FOR_EACH_ARRAY( MultiPassMaterialIter, pMesh->m_MultiPassMaterials, Material )
		{
			const Material& UsingMultiPassMaterial = UseMaterialOverride ? pBucket->GetOverriddenMaterial( MultiPassMaterialIter.GetValue(), pMesh->m_VertexDeclaration->GetSignature(), this ) : MultiPassMaterialIter.GetValue();
			ApplyMaterial( UsingMultiPassMaterial, pMesh, m_View );
			DrawElements( pMesh->m_VertexBuffer, pMesh->m_IndexBuffer );
#if BUILD_DEV
			++m_DEV_RenderStats.m_NumDrawCalls;
#endif
		}
	}
	else
	{
		const Material& UsingMaterial = UseMaterialOverride ? pBucket->GetOverriddenMaterial( pMesh->m_Material, pMesh->m_VertexDeclaration->GetSignature(), this ) : pMesh->m_Material;
		ApplyMaterial( UsingMaterial, pMesh, m_View );
		DrawElements( pMesh->m_VertexBuffer, pMesh->m_IndexBuffer );
#if BUILD_DEV
		++m_DEV_RenderStats.m_NumDrawCalls;
#endif
	}

#if BUILD_DEV
	++m_DEV_RenderStats.m_NumMeshes;
	m_DEV_RenderStats.m_NumPrimitives += pMesh->m_IndexBuffer->GetNumPrimitives();
#endif
}

void RendererCommon::SetView( const View& rView )
{
	m_View = rView;
	SetViewMatrix( rView.GetViewMatrix() );
	SetProjectionMatrix( rView.GetProjectionMatrix() );
}

// NOTE: This is a stripped-down version of RenderBucket with special rules
// because I can make assumptions about the content.
void RendererCommon::RenderShadowBucket( Bucket* const pShadowLightsBucket, Bucket* const pShadowCastersBucket )
{
	XTRACE_FUNCTION;

	PROFILE_FUNCTION;

	if( !pShadowLightsBucket->m_Enabled ||
		!pShadowCastersBucket->m_Enabled )
	{
		return;
	}

#if BUILD_DEV
	START_CLOCK( m_DEV_RenderStats.m_ShadowTime );
#endif

	IRenderTarget* const pLightRenderTarget = pShadowLightsBucket->m_RenderTarget ? pShadowLightsBucket->m_RenderTarget : m_CurrentRenderTarget;
	IRenderTarget* const pCastersRenderTarget = pShadowCastersBucket->m_RenderTarget;
	DEVASSERT( pCastersRenderTarget );
	DEVASSERT( pShadowCastersBucket->m_View );

	Clear( pShadowLightsBucket->m_ClearFlags, pShadowLightsBucket->m_ClearColor, pShadowLightsBucket->m_ClearDepth, pShadowLightsBucket->m_ClearStencil );

	if( pShadowLightsBucket->m_View )
	{
		SetView( *pShadowLightsBucket->m_View );
	}

	const Angles* const pCubemapRenderOrientations	= GetCubemapRenderOrientations();
	const bool			DoLightFrustumCulling		= true; //m_DoFrustumCulling && pShadowLightsBucket->m_DoFrustumCulling;	// Why would I ever not?
	const Frustum		LightViewFrustum			= Frustum( GetViewProjectionMatrix() );
	const uint			NumLightMeshes				= pShadowLightsBucket->m_Meshes.Size();
	const View			MainView					= m_View;
	View				LightView;
	Material			ShadowMaterial;
#if BUILD_DEV
	uint				NumShadowLightMeshes;
	Set<Mesh*>			UniqueShadowLightMeshes;
	Array<Array<Mesh*>>	PerFaceShadowLightMeshes;
#endif
	for( uint LightMeshIndex = 0; LightMeshIndex < NumLightMeshes; ++LightMeshIndex )
	{
		Mesh* const pLightMesh = pShadowLightsBucket->m_Meshes[ LightMeshIndex ];
		if( !DoLightFrustumCulling || PassesFrustum( pLightMesh, LightViewFrustum ) )
		{
#if BUILD_DEV
			NumShadowLightMeshes = 0;
			UniqueShadowLightMeshes.Clear();
			PerFaceShadowLightMeshes.Clear();
			PerFaceShadowLightMeshes.ResizeZero( 6 );
#endif

			// Bit of a hack, but we can't get light radius from game code here
			const Vector	LightExtents	= pLightMesh->m_AABB.GetExtents();
			const float		LightFar		= Max( LightExtents.x, Max( LightExtents.y, LightExtents.z ) );
			const uint		LightCubeMask	= pLightMesh->m_Material.GetLightCubeMask();

			LightView = *pShadowCastersBucket->m_View;	// This should already be set up with everything except transform
			LightView.SetLocation( pLightMesh->m_Location );
			LightView.SetClipPlanes( LightView.GetNearClip(), LightFar );

			// Making assumption that these are cube RTs for point lights!
			for( uint FaceIndex = 0; FaceIndex < 6; ++FaceIndex )
			{
				if( 0 == ( LightCubeMask & ( 1 << FaceIndex ) ) )
				{
					// Optimization to selectively skip rendering certain side of the cubemap if we're
					// certain they won't be used (e.g. a spotlight pointed straight down). This also
					// skips clearing the target, so be certain it will not be used or we'll sample garbage.
					continue;
				}

				SetCubeRenderTarget( pCastersRenderTarget, FaceIndex );

				Clear( pShadowCastersBucket->m_ClearFlags, pShadowCastersBucket->m_ClearColor, pShadowCastersBucket->m_ClearDepth, pShadowCastersBucket->m_ClearStencil );

				LightView.SetRotation( pCubemapRenderOrientations[ FaceIndex ] );
				SetView( LightView );

				const bool		DoShadowFrustumCulling	= true; //m_DoFrustumCulling && pShadowCastersBucket->m_DoFrustumCulling;	// Why would I ever not?
				const Frustum	ShadowViewFrustum		= Frustum( GetViewProjectionMatrix() );
				const uint		NumShadowMeshes			= pShadowCastersBucket->m_Meshes.Size();
				for( uint ShadowMeshIndex = 0; ShadowMeshIndex < NumShadowMeshes; ++ShadowMeshIndex )
				{
					Mesh* const pShadowMesh = pShadowCastersBucket->m_Meshes[ ShadowMeshIndex ];

					// Skip pShadowMesh if it is excluded for this light (to avoid certain things casting their own shadow)
					if( pLightMesh->m_IgnoredMeshes.Contains( pShadowMesh ) )
					{
						continue;
					}

					// Do an AABB/AABB test before the frustum cull. This is redundant to do for each face,
					// but AABB/frustum tests can have false positives when the AABB is large compared to
					// the frustum, as it is likely to be when dealing with large merged meshes and small
					// light views.
					if( !pLightMesh->m_AABB.Intersects( pShadowMesh->m_AABB ) )
					{
						continue;
					}

					if( DoShadowFrustumCulling && !PassesFrustum( pShadowMesh, ShadowViewFrustum ) )
					{
						continue;
					}

					RenderBucketMesh( pShadowMesh, pShadowCastersBucket );
#if BUILD_DEV
					++NumShadowLightMeshes;
					UniqueShadowLightMeshes.Insert( pShadowMesh );
					PerFaceShadowLightMeshes[ FaceIndex ].PushBack( pShadowMesh );
#endif
				}
			}

			SetRenderTarget( pLightRenderTarget );
			SetView( MainView );
			RenderBucketMesh( pLightMesh, pShadowLightsBucket );
#if BUILD_DEV
			++m_DEV_RenderStats.m_NumShadowLights;
			m_DEV_RenderStats.m_NumShadowMeshes += NumShadowLightMeshes;
			Unused( UniqueShadowLightMeshes );
			Unused( PerFaceShadowLightMeshes );
			SDEV_RenderStats::SDEV_ShadowLight& ShadowLight = m_DEV_RenderStats.m_ShadowLights.PushBack();
			ShadowLight.m_Location	= pLightMesh->m_Location;
			ShadowLight.m_Radius	= LightFar;
			ShadowLight.m_NumMeshes	= NumShadowLightMeshes;
#endif
		}
	}

#if BUILD_DEV
	STOP_CLOCK( m_DEV_RenderStats.m_ShadowTime );
#endif
}

void RendererCommon::RenderBuckets()
{
	XTRACE_FUNCTION;

	for( uint BucketIndex = 0; BucketIndex < m_OrderedBuckets.Size(); ++BucketIndex )
	{
		Bucket* const pBucket = m_OrderedBuckets[ BucketIndex ];
		if( pBucket->m_IsShadowMaster )
		{
			// For shadow-casting lights, render the following bucket once for each mesh in this bucket
			Bucket* const pBucketShadowCasters = m_OrderedBuckets[ ++BucketIndex ];
			RenderShadowBucket( pBucket, pBucketShadowCasters );
		}
		else
		{
			RenderBucket( pBucket );
		}
	}
}

void RendererCommon::PostRenderBuckets()
{
	XTRACE_FUNCTION;

	for( uint i = 0; i < m_OrderedBuckets.Size(); ++i )
	{
		Bucket* pBucket = m_OrderedBuckets[i];
		pBucket->m_Meshes.Clear();
	}

#if BUILD_DEV
	PROFILE_BEGIN( RendererCommon_DeferredDeleteDebugMeshes );
	const uint NumDeferredDeleteDebugMeshes = m_DEV_DeferredDeleteDebugMeshes.Size();
	for( uint DebugMeshIndex = 0; DebugMeshIndex < NumDeferredDeleteDebugMeshes; ++DebugMeshIndex )
	{
		Mesh* pDebugMesh = m_DEV_DeferredDeleteDebugMeshes[ DebugMeshIndex ];
		ASSERT( pDebugMesh );
		SafeDelete( pDebugMesh );
	}
	m_DEV_DeferredDeleteDebugMeshes.Clear();
	PROFILE_END;
#endif
}

#if BUILD_DEV
void RendererCommon::DEBUGDrawLine( const Vector& Start, const Vector& End, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* LineMesh = GetMeshFactory()->CreateDebugLine( Start, End, Color );
	LineMesh->m_DEV_IsDebugMesh = true;
	LineMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	LineMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	LineMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( LineMesh );
}

void RendererCommon::DEBUGDrawTriangle( const Vector& V1, const Vector& V2, const Vector& V3, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* TriMesh = GetMeshFactory()->CreateDebugTriangle( V1, V2, V3, Color );
	TriMesh->m_DEV_IsDebugMesh = true;
	TriMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	TriMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	TriMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( TriMesh );
}

void RendererCommon::DEBUGDrawBox( const Vector& Min, const Vector& Max, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* BoxMesh = GetMeshFactory()->CreateDebugBox( Min, Max, Color );
	BoxMesh->m_DEV_IsDebugMesh = true;
	BoxMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	BoxMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	BoxMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( BoxMesh );
}

void RendererCommon::DEBUGDrawFrustum( const Frustum& rFrustum, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* FrustumMesh = GetMeshFactory()->CreateDebugFrustum( rFrustum, Color );
	FrustumMesh->m_DEV_IsDebugMesh = true;
	FrustumMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	FrustumMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	FrustumMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( FrustumMesh );
}

void RendererCommon::DEBUGDrawFrustum( const View& rView, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* FrustumMesh = GetMeshFactory()->CreateDebugFrustum( rView, Color );
	FrustumMesh->m_DEV_IsDebugMesh = true;
	FrustumMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	FrustumMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	FrustumMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( FrustumMesh );
}

/*virtual*/ void RendererCommon::DEBUGDrawCircleXY( const Vector& Center, float Radius, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* CircleXYMesh = GetMeshFactory()->CreateDebugCircleXY( Center, Radius, Color );
	CircleXYMesh->m_DEV_IsDebugMesh = true;
	CircleXYMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	CircleXYMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	CircleXYMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( CircleXYMesh );
}

void RendererCommon::DEBUGDrawSphere( const Vector& Center, float Radius, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* SphereMesh = GetMeshFactory()->CreateDebugSphere( Center, Radius, Color );
	SphereMesh->m_DEV_IsDebugMesh = true;
	SphereMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	SphereMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	SphereMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( SphereMesh );
}

void RendererCommon::DEBUGDrawEllipsoid( const Vector& Center, const Vector& Extents, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* EllipsoidMesh = GetMeshFactory()->CreateDebugEllipsoid( Center, Extents, Color );
	EllipsoidMesh->m_DEV_IsDebugMesh = true;
	EllipsoidMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	EllipsoidMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	EllipsoidMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( EllipsoidMesh );
}

void RendererCommon::DEBUGDrawCross( const Vector& Center, const float Length, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* CrossMesh = GetMeshFactory()->CreateDebugCross( Center, Length, Color );
	CrossMesh->m_DEV_IsDebugMesh = true;
	CrossMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	CrossMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	CrossMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( CrossMesh );
}

void RendererCommon::DEBUGDrawArrow( const Vector& Root, const Angles& Direction, const float Length, unsigned int Color, const bool DepthTest /*= true*/ )
{
	Mesh* ArrowMesh = GetMeshFactory()->CreateDebugArrow( Root, Direction, Length, Color );
	ArrowMesh->m_DEV_IsDebugMesh = true;
	ArrowMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	ArrowMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	ArrowMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( ArrowMesh );
}

void RendererCommon::DEBUGDrawCoords( const Vector& Location, const Angles& Orientation, const float Length, const bool DepthTest )
{
	Mesh* CoordsMesh = GetMeshFactory()->CreateDebugCoords( Location, Orientation, Length );
	CoordsMesh->m_DEV_IsDebugMesh = true;
	CoordsMesh->SetMaterialDefinition( DepthTest ? DEFAULT_MATERIAL : DEFAULT_MATERIAL_NODEPTHTEST, this );
	CoordsMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	CoordsMesh->SetMaterialFlags( MAT_DEBUG_WORLD );
	AddMesh( CoordsMesh );
}

void RendererCommon::DEBUGDrawLine2D( const Vector& Start, const Vector& End, unsigned int Color )
{
	Mesh* LineMesh = GetMeshFactory()->CreateDebugLine( Start, End, Color );
	LineMesh->m_DEV_IsDebugMesh = true;
	LineMesh->SetMaterialDefinition( DEFAULT_MATERIAL_HUD, this );
	LineMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	LineMesh->SetMaterialFlags( MAT_DEBUG_HUD );
	AddMesh( LineMesh );
}

void RendererCommon::DEBUGDrawBox2D( const Vector& Min, const Vector& Max, unsigned int Color )
{
	Mesh* BoxMesh = GetMeshFactory()->CreateDebugBox( Min, Max, Color );
	BoxMesh->m_DEV_IsDebugMesh = true;
	BoxMesh->SetMaterialDefinition( DEFAULT_MATERIAL_HUD, this );
	BoxMesh->SetTexture( 0, GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent ) );
	BoxMesh->SetMaterialFlags( MAT_DEBUG_HUD );
	AddMesh( BoxMesh );
}

void RendererCommon::DEBUGPrint( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, const Vector4& Color )
{
	Mesh* const	pPrintMesh		= Print( UTF8String, pFont, Bounds, 0 );
	pPrintMesh->m_DEV_IsDebugMesh	= true;
	pPrintMesh->m_ConstantColor	= Color;
	pPrintMesh->SetMaterialFlags( MAT_DEBUG_HUD );
	AddMesh( pPrintMesh );
}

/*virtual*/ void RendererCommon::DEBUGPrint( const SimpleString& UTF8String, const SRect& Bounds, const SimpleString& FontName, const Vector4& Color, const Vector4& ShadowColor )
{
	const SRect	ShadowRect	= SRect( Bounds.m_Left + 1.0f, Bounds.m_Top + 1.0f, 0.0f, 0.0f );
	Font* const	pFont		= GetFontManager()->GetFont( FontName.CStr() );

	DEBUGPrint( UTF8String, pFont, ShadowRect, ShadowColor );
	DEBUGPrint( UTF8String, pFont, Bounds, Color );
}

/*virtual*/ void RendererCommon::DEBUGPrint( const SimpleString& UTF8String, const Vector& Location, const View* const pView, const Display* const pDisplay, const SimpleString& FontName, const Vector4& Color, const Vector4& ShadowColor )
{
	DEVASSERT( pView );
	DEVASSERT( pDisplay );

	const Matrix	VPMatrix			= pView->GetViewProjectionMatrix();
	const Vector4	ProjectedLocation	= Vector4( Location ) * VPMatrix;

	if( ProjectedLocation.z < 0.0f )
	{
		return;
	}

	const float		DisplayWidth		= static_cast<float>( pDisplay->m_Width );
	const float		DisplayHeight		= static_cast<float>( pDisplay->m_Height );
	const Vector4	NormalizedLocation	= ProjectedLocation / ProjectedLocation.w;
	const Vector2	ScaledLocation		= Vector2( DisplayWidth * ( NormalizedLocation.x * 0.5f + 0.5f ), DisplayHeight * ( -NormalizedLocation.y * 0.5f + 0.5f ) );
	const SRect		Rect				= SRect( Floor( ScaledLocation.x ), Floor( ScaledLocation.y ), 0.0f, 0.0f );
	const SRect		ShadowRect			= SRect( Rect.m_Left + 1.0f, Rect.m_Top + 1.0f, 0.0f, 0.0f );
	Font* const		pFont				= GetFontManager()->GetFont( FontName.CStr() );

	DEBUGPrint( UTF8String, pFont, ShadowRect, ShadowColor );
	DEBUGPrint( UTF8String, pFont, Rect, Color );
}
#endif // BUILD_DEV

#if BUILD_DEV
void RendererCommon::DEV_SetLockedFrustum( View* const pLockedView )
{
	m_DEV_pLockedFrustumView = pLockedView;
	if( pLockedView )
	{
		m_DEV_LockedFrustumView = *pLockedView;
		m_DEV_LockedFrustum = Frustum( pLockedView->GetViewProjectionMatrix() );
	}
}

bool RendererCommon::DEV_IsLockedFrustum() const
{
	return m_DEV_pLockedFrustumView != NULL;
}

const View& RendererCommon::DEV_GetLockedFrustumView() const
{
	return m_DEV_LockedFrustumView;
}

const Frustum& RendererCommon::DEV_GetLockedFrustum() const
{
	return m_DEV_LockedFrustum;
}
#endif

Mesh* RendererCommon::Print( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, unsigned int Flags )
{
	DEVASSERT( pFont );
	Mesh* StringMesh = pFont->Print( UTF8String, Bounds, Flags );
	StringMesh->SetMaterialFlags( MAT_HUD );
	StringMesh->SetMaterialDefinition( "Material_HUD", this );
	return StringMesh;
}

/*virtual*/ void RendererCommon::Arrange( const SimpleString& UTF8String, const Font* const pFont, const SRect& Bounds, unsigned int Flags, Vector2& OutExtents )
{
	DEVASSERT( pFont );
	Array<STypesetGlyph> UnusedTypesetting;
	pFont->Arrange( UTF8String, Bounds, Flags, UnusedTypesetting, OutExtents );
}

#if BUILD_DEV
IRenderer::SDEV_RenderStats& RendererCommon::DEV_GetStats()
{
	return m_DEV_RenderStats;
}
#endif

void RendererCommon::ApplyMaterial( const Material& Material, Mesh* const pMesh, const View& CurrentView )
{
	ExecuteRenderOps( Material.GetRenderOps() );
	SetShaderProgram( Material.GetShaderProgram() );
	ApplyRenderState( Material.GetRenderState() );

	const uint	NumSamplers		= Material.GetNumSamplers();
	uint		SamplerIndex	= 0;
	for( ; SamplerIndex < NumSamplers; ++SamplerIndex )
	{
		ApplySamplerState( SamplerIndex, Material.GetSamplerState( SamplerIndex ) );
	}
	for( ; SamplerIndex < MAX_TEXTURE_STAGES; ++SamplerIndex )
	{
		ResetSamplerState( SamplerIndex );
	}

	DEBUGASSERT( Material.GetSDP() );
	Material.GetSDP()->SetShaderParameters( this, pMesh, CurrentView );

#if BUILD_DEV
	// Make sure we've got the right vertex streams bound; seems like there should be a better way to automate this but shrug
	const uint ExpectedVD	= Material.GetExpectedVD();
	const uint VDSignature	= pMesh->m_VertexDeclaration->GetSignature();
	ASSERT( ExpectedVD == ( VDSignature & ExpectedVD ) );
#endif
}

// NOTE: This is pretty stubby, since clearing the stencil buffer is the only
// per-material thing I support right now. As I add more, break it out into
// functions like ApplyRenderState.
void RendererCommon::ExecuteRenderOps( const SRenderOps& RenderOps )
{
	if( RenderOps.m_RenderTarget )
	{
		SetRenderTarget( RenderOps.m_RenderTarget );
	}

	uint ClearFlags = CLEAR_NONE;

	if( RenderOps.m_ClearColor == EE_True )
	{
		ClearFlags |= CLEAR_COLOR;
	}

	if( RenderOps.m_ClearStencil == EE_True )
	{
		ClearFlags |= CLEAR_STENCIL;
	}

	if( RenderOps.m_ClearDepth == EE_True )
	{
		ClearFlags |= CLEAR_DEPTH;
	}

	if( ClearFlags != CLEAR_NONE )
	{
		Clear( ClearFlags, RenderOps.m_ClearColorValue, RenderOps.m_ClearDepthValue, RenderOps.m_ClearStencilValue );
	}
}

ECullMode RendererCommon::ResolveCullMode( const ECullMode CullMode ) const
{
	if( m_View.GetMirrorX() )
	{
		if( ECM_CW == CullMode )
		{
			return ECM_CCW;
		}
		else if( ECM_CCW == CullMode )
		{
			return ECM_CW;
		}
	}

	return CullMode;
}

void RendererCommon::ApplyRenderState( const SRenderState& RenderState )
{
	SetCullMode(			ResolveCullMode( RenderState.m_CullMode ) );
	SetColorWriteEnable(	RenderState.m_ColorWriteEnable );
	SetAlphaBlendEnable(	RenderState.m_AlphaBlendEnable );
	SetZEnable(				RenderState.m_ZEnable );
	SetZWriteEnable(		RenderState.m_ZWriteEnable );
	SetStencilEnable(		RenderState.m_StencilEnable );

	if( RenderState.m_AlphaBlendEnable == EE_True )
	{
		SetBlend( RenderState.m_SrcBlend, RenderState.m_DestBlend );
	}

	if( RenderState.m_ZEnable == EE_True )
	{
		SetZFunc( RenderState.m_ZFunc );
	}

	if( RenderState.m_StencilEnable == EE_True )
	{
		SetStencilFunc(			RenderState.m_StencilFunc,		RenderState.m_StencilRef,		RenderState.m_StencilMask );
		SetStencilOp(			RenderState.m_StencilFailOp,	RenderState.m_StencilZFailOp,	RenderState.m_StencilPassOp );
		SetStencilWriteMask(	RenderState.m_StencilWriteMask );
	}
}

void RendererCommon::ApplySamplerState( const uint SamplerStage, const SSamplerState& SamplerState )
{
	DEBUGASSERT( SamplerState.m_Texture );

	if( SamplerState.m_Texture->IsCubemap() )
	{
		SetCubemap(					SamplerStage, SamplerState.m_Texture );
		SetCubemapAddressing(		SamplerStage, SamplerState.m_AddressU, SamplerState.m_AddressV, SamplerState.m_AddressW );
		SetCubemapMinMipFilters(	SamplerStage, SamplerState.m_MinFilter, SamplerState.m_MipFilter );
		SetCubemapMagFilter(		SamplerStage, SamplerState.m_MagFilter );
	}
	else
	{
		SetTexture(			SamplerStage, static_cast<ITexture*>( SamplerState.m_Texture ) );
		SetAddressing(		SamplerStage, SamplerState.m_AddressU, SamplerState.m_AddressV );
		SetMinMipFilters(	SamplerStage, SamplerState.m_MinFilter, SamplerState.m_MipFilter );
		SetMagFilter(		SamplerStage, SamplerState.m_MagFilter );
		SetMaxAnisotropy(	SamplerStage, SamplerState.m_MaxAnisotropy );
	}
}

void RendererCommon::ResetSamplerState( const uint SamplerStage )
{
	ResetTexture( SamplerStage );
}

void RendererCommon::ResetRenderState()
{
	// Reset our shadowed state so we'll update the D3D state properly after reset.
	m_RenderState = SRenderState();
	for( uint SamplerStage = 0; SamplerStage < MAX_TEXTURE_STAGES; ++SamplerStage )
	{
		m_SamplerStates[ SamplerStage ] = SSamplerState();
	}
}
