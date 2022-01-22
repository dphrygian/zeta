#include "core.h"
#include "3d.h"
#include "bucket.h"
#include "mesh.h"
#include "view.h"
#include "ishaderprogram.h"
#include "mathcore.h"
#include "configmanager.h"

Bucket::Bucket(
	View* pView,
	IRenderTarget* RenderTarget,
	uint Flags,
	uint FilterFlags /*= MAT_NONE*/,
	const HashedString& Tag /*= HashedString::NullString*/,
	bool DoFrustumCulling /*= true*/,
	bool ExclusiveMeshes /*= false*/,
	uint ClearFlags /*= CLEAR_NONE*/,
	uint ClearColor /*= 0xff000000*/,
	float ClearDepth /*= 1.0f*/,
	uint ClearStencil /*= 0*/ )
:	m_View( pView )
,	m_RenderTarget( RenderTarget )
,	m_Flags( Flags )
,	m_FilterFlags( FilterFlags )
,	m_DoFrustumCulling( DoFrustumCulling )
,	m_ExclusiveMeshes( ExclusiveMeshes )
,	m_AlphaSortMeshes( false )
,	m_SortByMaterial( false )
,	m_Meshes()
,	m_ClearFlags( ClearFlags )
,	m_ClearColor( ClearColor )
,	m_ClearDepth( ClearDepth )
,	m_ClearStencil( ClearStencil )
,	m_DepthMin( 0.0f )
,	m_DepthMax( 1.0f )
,	m_Enabled( true )
,	m_IsShadowMaster( false )
,	m_Tag( Tag )
#if BUILD_DEV
,	m_DEV_Name()
#endif
,	m_MaterialOverridesDefinition()
,	m_MaterialOverrideMap()
,	m_SortHelpers()
,	m_MatSortHelpers()
{
	// Don't reallocate mesh buckets every frame
	m_Meshes.SetDeflate( false );
	m_SortHelpers.SetDeflate( false );
	m_MatSortHelpers.SetDeflate( false );
}

void Bucket::SetMaterialOverridesDefinition( const HashedString& DefinitionName )
{
	m_MaterialOverridesDefinition = DefinitionName;

	// DLP 29 May 2021: Flush material overrides so we'll make them again,
	// in case we're holding references to managed RTs that have been changed.
	m_MaterialOverrideMap.Clear();
}

// NOTE: Need to create a unique material per vertex signature! Shader programs in GL are compiled
// based on vertex signature, so two meshes with different signatures can't share the same material.
const Material& Bucket::InternalGetMaterialOverride( const Material& InMaterial, const uint VertexSignature, IRenderer* const pRenderer )
{
	SMaterialOverrideKey Key;
	Key.m_MaterialName = InMaterial.GetName();
	Key.m_VertexSignature = VertexSignature;

	Map<SMaterialOverrideKey, Material>::Iterator MaterialOverrideIter = m_MaterialOverrideMap.Search( Key );
	if( MaterialOverrideIter.IsValid() )
	{
		return MaterialOverrideIter.GetValue();
	}

	const SimpleString MaterialOverrideDefinition = ConfigManager::GetString( Key.m_MaterialName, "", m_MaterialOverridesDefinition );
	DEVASSERT( MaterialOverrideDefinition != "" );

	// Lazily create a material and add it
	Material& MaterialOverride = m_MaterialOverrideMap.Insert( Key );
	MaterialOverride.SetDefinition( MaterialOverrideDefinition, pRenderer, VertexSignature );
	return MaterialOverride;
}

Material Bucket::GetOverriddenMaterial( const Material& InMaterial, const uint VertexSignature, IRenderer* const pRenderer )
{
	Material OverriddenMaterial = InternalGetMaterialOverride( InMaterial, VertexSignature, pRenderer );
	OverriddenMaterial.CopySamplerStatesFrom( InMaterial );
	return OverriddenMaterial;
}

void Bucket::AlphaSortMeshes( const View& CurrentView )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	const Vector&	ViewLocation	= CurrentView.GetLocation();
	const Vector	ViewDirection	= CurrentView.GetRotation().ToVector();
	const uint		NumMeshes		= m_Meshes.Size();

	m_SortHelpers.Resize( NumMeshes );

	for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
	{
		Mesh* const		pMesh	= m_Meshes[ MeshIndex ];
		DEVASSERT( pMesh );
		SSortHelper&	Helper	= m_SortHelpers[ MeshIndex ];

		// NOTE: Sorting based on distance of offset projected onto view direction
		// would be more correct; but it will rarely make a difference, and is more ops.

		Helper.m_Mesh						= pMesh;
		//const Vector	ToMesh				= pMesh->GetSortLocation() - ViewLocation;
		// For fog meshes, I'm using the nearest point to view location instead of centroid.
		// Might need to revisit this later. It works decently but not perfect in all cases.
		const Vector	ToMesh				= pMesh->GetAABB().GetClosestPoint( ViewLocation ) - ViewLocation;
		const float		SortDistanceSq		= ToMesh.LengthSquared();
		const float		SortDistanceSign	= Sign( ViewDirection.Dot( ToMesh ) );
		Helper.m_SortDistanceSq				= SortDistanceSq * SortDistanceSign;
	}

	m_SortHelpers.InsertionSort();

	for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
	{
		const SSortHelper& Helper	= m_SortHelpers[ MeshIndex ];
		m_Meshes[ MeshIndex ]		= Helper.m_Mesh;
	}
}

void Bucket::SortByMaterials()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	const uint		NumMeshes		= m_Meshes.Size();
	m_MatSortHelpers.Resize( NumMeshes );

	for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
	{
		Mesh* const		pMesh	= m_Meshes[ MeshIndex ];
		DEVASSERT( pMesh );
		SMatSortHelper&	Helper	= m_MatSortHelpers[ MeshIndex ];

		const Material&			Material		= pMesh->GetMaterial();
		IShaderProgram* const	pShaderProgram	= Material.GetShaderProgram();
		DEBUGASSERT( pShaderProgram );

		Helper.m_Mesh			= pMesh;
		Helper.m_VertexShader	= pShaderProgram->GetVertexShader();
		Helper.m_PixelShader	= pShaderProgram->GetPixelShader();
		Helper.m_BaseTexture	= Material.GetTexture( 0 );
	}

	m_MatSortHelpers.InsertionSort();

	for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
	{
		const SMatSortHelper& Helper	= m_MatSortHelpers[ MeshIndex ];
		m_Meshes[ MeshIndex ]			= Helper.m_Mesh;
	}
}
