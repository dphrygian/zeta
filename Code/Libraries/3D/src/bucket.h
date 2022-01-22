#ifndef BUCKET_H
#define BUCKET_H

#include "array.h"
#include "hashedstring.h"
#include "map.h"
#include "material.h"

#if BUILD_DEBUG
#include "view.h"
#endif

class View;
class IRenderer;
class IRenderTarget;
class ITexture;
class IVertexShader;
class IPixelShader;
class Mesh;
class View;

class Bucket
{
public:
	Bucket(
		View*				pView,
		IRenderTarget*		RenderTarget,
		uint				Flags,
		uint				FilterFlags			= 0,
		const HashedString&	Tag					= HashedString::NullString,
		bool				DoFrustumCulling	= true,
		bool				ExclusiveMeshes		= false,
		uint				ClearFlags			= CLEAR_NONE,
		uint				ClearColor			= 0xff000000,
		float				ClearDepth			= 1.0f,
		uint				ClearStencil		= 0 );

	void			AlphaSortMeshes( const View& CurrentView );	// Sort meshes back-to-front for alpha rendering
	void			SortByMaterials();

	void			SetMaterialOverridesDefinition( const HashedString& DefinitionName );
	bool			HasMaterialOverrides() const { return HashedString::NullString != m_MaterialOverridesDefinition; }
	Material		GetOverriddenMaterial( const Material& InMaterial, const uint VertexSignature, IRenderer* const pRenderer );
	const Material&	InternalGetMaterialOverride( const Material& InMaterial, const uint VertexSignature, IRenderer* const pRenderer );

	View*			m_View;				// If NULL, the current one will be reused
	IRenderTarget*	m_RenderTarget;		// If NULL, the current one will be reused
	uint			m_Flags;			// MAT_ flags, defined in material.h: a mesh must have all the flags of the bucket to be added
	uint			m_FilterFlags;		// Reject the mesh if it matches any of these flags
	bool			m_DoFrustumCulling;	// Never frustum cull this bucket (always draw every mesh in the bucket)
	bool			m_ExclusiveMeshes;	// If true, meshes in this bucket are not added to any later buckets (ignored for prescribed buckets)
	bool			m_AlphaSortMeshes;	// If true, bucket is sorted by distance to view location. Also always done for MAT_ALPHA buckets.
	bool			m_SortByMaterial;	// If true, bucket is sorted by shader and by texture 0
	Array<Mesh*>	m_Meshes;
	uint			m_ClearFlags;
	uint			m_ClearColor;
	float			m_ClearDepth;
	uint			m_ClearStencil;
	float			m_DepthMin;			// For scaling the depth buffer range
	float			m_DepthMax;			// For scaling the depth buffer range
	bool			m_Enabled;			// For selectively removing passes entirely
	bool			m_IsShadowMaster;	// Because the nested iteration for lights and shadow casters requires special code
	HashedString	m_Tag;				// For grouping buckets
#if BUILD_DEV
	HashedString	m_DEV_Name;
#endif

	struct SMaterialOverrideKey
	{
		HashedString	m_MaterialName;
		uint			m_VertexSignature;

		bool operator<( const SMaterialOverrideKey& Key ) const
		{
			if( m_MaterialName < Key.m_MaterialName )
			{
				return true;
			}

			if( m_MaterialName > Key.m_MaterialName )
			{
				return false;
			}

			if( m_VertexSignature < Key.m_VertexSignature )
			{
				return true;
			}

			return false;
		}

		bool operator==( const SMaterialOverrideKey& Key ) const
		{
			return
				m_MaterialName == Key.m_MaterialName &&
				m_VertexSignature == Key.m_VertexSignature;
		}
	};

	// Map from material definition names to the desired materials
	HashedString						m_MaterialOverridesDefinition;
	Map<SMaterialOverrideKey, Material>	m_MaterialOverrideMap;

	struct SSortHelper
	{
		Mesh*	m_Mesh;
		float	m_SortDistanceSq;

		bool operator<( const SSortHelper& Helper ) const { return m_SortDistanceSq > Helper.m_SortDistanceSq; }
	};

	struct SMatSortHelper
	{
		Mesh*			m_Mesh;
		IVertexShader*	m_VertexShader;
		IPixelShader*	m_PixelShader;
		ITexture*		m_BaseTexture;

		bool operator<( const SMatSortHelper& Helper ) const
		{
			if( m_VertexShader < Helper.m_VertexShader )
			{
				return true;
			}

			if( m_VertexShader > Helper.m_VertexShader )
			{
				return false;
			}

			if( m_PixelShader < Helper.m_PixelShader )
			{
				return true;
			}

			if( m_PixelShader > Helper.m_PixelShader )
			{
				return false;
			}

			if( m_BaseTexture < Helper.m_BaseTexture )
			{
				return true;
			}

			return false;
		}
	};

	Array<SSortHelper>		m_SortHelpers;
	Array<SMatSortHelper>	m_MatSortHelpers;
};

#endif // BUCKET_H
