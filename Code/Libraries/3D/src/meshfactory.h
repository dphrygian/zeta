#ifndef MESHFACTORY_H
#define MESHFACTORY_H

#include "3d.h"
#include "iindexbuffer.h"

class Angles;

enum EPlane
{
	XY_PLANE,
	XZ_PLANE,
	YZ_PLANE,
};

struct SCompiledMeshHeader
{
	SCompiledMeshHeader();

	unsigned int	m_MagicID;			// 'SMCD'
	unsigned int	m_NumVertices;
	unsigned int	m_NumIndices;
	unsigned int	m_NumFrames;		// Ignored unless m_HasSkeleton
	unsigned char	m_NumBones;			// Ignored unless m_HasSkeleton
	unsigned char	m_NumAnims;			// Ignored unless m_HasSkeleton
	unsigned char	m_NumMaterials;
	unsigned char	m_PaddingA;			// Fix uninitialized memory in baked assets
	bool			m_LongIndices;
	bool			m_HasUVs;
	bool			m_HasColors;
	bool			m_HasNormals;
	bool			m_HasNormalsB;
	bool			m_HasTangents;
	bool			m_HasSkeleton;
	bool			m_PaddingB;
};

// For buffer callback
struct SReadMeshBuffers
{
	SReadMeshBuffers()
	:	m_Header()
	,	m_Positions( NULL )
	,	m_Colors( NULL )
	,	m_UVs( NULL )
	,	m_Normals( NULL )
	,	m_NormalsB( NULL )
	,	m_Tangents( NULL )
	,	m_BoneIndices( NULL )
	,	m_BoneWeights( NULL )
	,	m_Indices( NULL )
	,	m_BoneInfos( NULL )
	,	m_Bones( NULL )
	,	m_AnimData( NULL )
	,	m_Materials( NULL )
	{
	}

	SCompiledMeshHeader			m_Header;
	class Vector*				m_Positions;
	class Vector4*				m_Colors;
	class Vector2*				m_UVs;
	class Vector*				m_Normals;
	class Vector*				m_NormalsB;
	class Vector4*				m_Tangents;
	struct SBoneData*			m_BoneIndices;
	struct SBoneData*			m_BoneWeights;
	index_t*					m_Indices;
	struct SBoneInfo*			m_BoneInfos;
	struct SBone*				m_Bones;
	struct SAnimData*			m_AnimData;
	class SimpleString*			m_Materials;
};

class Mesh;
class IRenderer;
class Vector;
class Vector2;
class View;
class Frustum;
class IDataStream;
class SimpleString;

class MeshFactory
{
public:
	MeshFactory( IRenderer* Renderer );

	// TODO: Could add a UV reps parameter to these...
	Mesh* CreateQuad( float Length, EPlane Plane, bool TwoSided );
	Mesh* CreatePlane( float Length, float Width, int LengthSegments, int WidthSegments, EPlane Plane, bool TwoSided );
	Mesh* CreateSprite();		// Creates simple plane mesh in the XZ plane
	Mesh* CreateCircleSprite( const uint NumSides );	// Creates simple circle mesh in the XZ plane
	Mesh* CreateGrid( float Length, float Width, int LengthSegments, int WidthSegments, EPlane Plane );
	Mesh* CreateCylinder( float Radius, float Height, int RadialSegments, int HeightSegments );
	Mesh* CreateCube( float Length );
	Mesh* CreateBox( float Length, float Width, float Height, int LengthSegments, int WidthSegments, int HeightSegments );
	Mesh* CreateSphere( float Radius, int LongitudinalSegments, int LatitudinalSegments );
	Mesh* CreateGeosphere( float Radius, int Refinement );
	Mesh* CreateTorus( float OuterRadius, float InnerRadius, int OuterSegments, int InnerSegments );
	Mesh* CreateCapsule( float Radius, float InnerLength, int CylinderSegments, int LongitudinalSegments, int LatitudinalSegments );
	Mesh* CreateCone( float Radius, float Height, int RadialSegments, int HeightSegments );

	// These functions will work in Release mode, but the meshes are inefficient,
	// so they're generally to be avoided. They could be used in tools, though.
	Mesh* CreateDebugLine( const Vector& Start, const Vector& End, unsigned int Color );
	Mesh* CreateDebugTriangle( const Vector& V1, const Vector& V2, const Vector& V3, unsigned int Color );
	Mesh* CreateDebugQuad( const Vector& V1, const Vector& V2, const Vector& V3, const Vector& V4, unsigned int Color );
	Mesh* CreateDebugBox( const Vector& Min, const Vector& Max, unsigned int Color );
	Mesh* CreateDebugFrustum( const Frustum& rFrustum, unsigned int Color );
	Mesh* CreateDebugFrustum( const View& rView, unsigned int Color );
	Mesh* CreateDebugChar( float Width, float Height, const Vector2& UV1, const Vector2& UV2, EPlane Plane, bool TwoSided );
	Mesh* CreateDebugCircleXY( const Vector& Center, float Radius, unsigned int Color );
	Mesh* CreateDebugSphere( const Vector& Center, float Radius, unsigned int Color );
	Mesh* CreateDebugEllipsoid( const Vector& Center, const Vector& Extents, unsigned int Color );
	Mesh* CreateDebugCross( const Vector& Center, const float Length, unsigned int Color );
	Mesh* CreateDebugArrow( const Vector& Root, const Angles& Direction, const float Length, unsigned int Color );
	Mesh* CreateDebugCoords( const Vector& Location, const Angles& Orientation, const float Length );

	// Callback stuff in case I need to copy the buffers for use in game
	typedef void ( *ReadMeshCallback )( void*, const SReadMeshBuffers& );
	struct SReadMeshCallback
	{
		SReadMeshCallback()
			:	m_Callback( NULL )
			,	m_Void( NULL )
		{
		}

		ReadMeshCallback	m_Callback;
		void*				m_Void;
	};

	// If given pMesh, will not new up another mesh
	// HACK: If given pDiffuseMaterial, will fill it with the filename in the base material
	Mesh* Read(
		const IDataStream& Stream,
		const char* Filename,								// Used for initializing bone arrays for a particular mesh
		Mesh* pMesh = NULL,
		SimpleString* pDiffuseMaterial = NULL,
		SReadMeshCallback Callback = SReadMeshCallback() );

	void GetDynamicMesh( const char* Filename, Mesh* const pNewMesh, SReadMeshCallback Callback = SReadMeshCallback() );	// Read from the filename if not already in the DynamicMeshManager
	void WarmDynamicMesh( const char* Filename, SReadMeshCallback Callback = SReadMeshCallback() );							// Just request that the given mesh be loaded and cached if it is not already

private:
	IRenderer* m_Renderer;
};

#endif // MESHFACTORY_H
