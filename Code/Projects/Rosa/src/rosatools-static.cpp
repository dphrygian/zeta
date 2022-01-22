#include "core.h"
#include "rosatools.h"

#if BUILD_ROSA_TOOLS

// This file is some select static functions I pulled out of RosaTools to share with RoomBaker.

#include "mathcore.h"
#include "meshfactory.h"
#include "packstream.h"
#include "mesh.h"

// HACKHACK: Get planes from raw vertex/index buffer data while loading mesh. (Callback structure originally developed for Savage.)
/*static*/ void RosaTools::ReadMeshCallback( void* pVoid, const SReadMeshBuffers& Buffers )
{
	SConvexHull& Hull = *static_cast<SConvexHull*>( pVoid );

	const uint NumTris = Buffers.m_Header.m_NumIndices / 3;
	for( uint TriIndex = 0; TriIndex < NumTris; ++TriIndex )
	{
		const index_t	Index0		= Buffers.m_Indices[ TriIndex * 3 ];
		const index_t	Index1		= Buffers.m_Indices[ TriIndex * 3 + 1 ];
		const index_t	Index2		= Buffers.m_Indices[ TriIndex * 3 + 2 ];
		const Vector&	Position0	= Buffers.m_Positions[ Index0 ];
		const Vector&	Position1	= Buffers.m_Positions[ Index1 ];
		const Vector&	Position2	= Buffers.m_Positions[ Index2 ];
		const Vector	Edge0		= Position1 - Position0;
		const Vector	Edge1		= Position2 - Position0;
		const Vector	TriNormal	= Edge0.Cross( Edge1 ).GetNormalized();
		const Plane		TriPlane	= Plane( TriNormal, Position0 );

		Hull.m_Hull.TryAddPlane( TriPlane, EPSILON );
	}
}

struct SConvexHullTris
{
	SConvexHull			m_Hull;
	Array<Triangle>*	m_Tris;
};

/*static*/ void RosaTools::ReadMeshCallback_Tris( void* pVoid, const SReadMeshBuffers& Buffers )
{
	SConvexHullTris&	HullTris	= *static_cast<SConvexHullTris*>( pVoid );
	SConvexHull&		Hull		= HullTris.m_Hull;
	Array<Triangle>&	Tris		= *HullTris.m_Tris;

	const uint NumTris = Buffers.m_Header.m_NumIndices / 3;
	Tris.Resize( NumTris );
	for( uint TriIndex = 0; TriIndex < NumTris; ++TriIndex )
	{
		const index_t	Index0		= Buffers.m_Indices[ TriIndex * 3 ];
		const index_t	Index1		= Buffers.m_Indices[ TriIndex * 3 + 1 ];
		const index_t	Index2		= Buffers.m_Indices[ TriIndex * 3 + 2 ];
		const Vector&	Position0	= Buffers.m_Positions[ Index0 ];
		const Vector&	Position1	= Buffers.m_Positions[ Index1 ];
		const Vector&	Position2	= Buffers.m_Positions[ Index2 ];
		const Vector	Edge0		= Position1 - Position0;
		const Vector	Edge1		= Position2 - Position0;
		const Vector	TriNormal	= Edge0.Cross( Edge1 ).GetNormalized();
		const Plane		TriPlane	= Plane( TriNormal, Position0 );

		Hull.m_Hull.TryAddPlane( TriPlane, EPSILON );
		Tris[ TriIndex ]			= Triangle( Position0, Position1, Position2 );
	}
}

/*static*/ SConvexHull RosaTools::CreateHull( const SimpleString& HullName, const HashedString& Surface, MeshFactory* const pMeshFactory )
{
	ASSERT( HullName != "" );

	SConvexHull Hull;

	MeshFactory::SReadMeshCallback Callback;
	Callback.m_Callback	= &RosaTools::ReadMeshCallback;
	Callback.m_Void		= &Hull;

	Mesh* pHullMesh = pMeshFactory->Read( PackStream( HullName.CStr() ), NULL, NULL, NULL, Callback );

	Hull.m_Bounds	= pHullMesh->m_AABB;
	Hull.m_Surface	= Surface;

	// ROSANOTE: *Don't* add AABB expansion planes here; they need to be added after hull's orientation is applied

	// ROSANOTE: Optionally keep the mesh around in editor for rendering convex hulls
	SafeDelete( pHullMesh );

	return Hull;
}

/*static*/ SConvexHull RosaTools::CreateHullWithTris( const SimpleString& HullName, const HashedString& Surface, MeshFactory* const pMeshFactory, Array<Triangle>& OutTris )
{
	ASSERT( HullName != "" );

	SConvexHullTris	HullTris;
	HullTris.m_Tris		= &OutTris;

	MeshFactory::SReadMeshCallback Callback;
	Callback.m_Callback	= &RosaTools::ReadMeshCallback_Tris;
	Callback.m_Void		= &HullTris;

	Mesh* pHullMesh = pMeshFactory->Read( PackStream( HullName.CStr() ), NULL, NULL, NULL, Callback );

	HullTris.m_Hull.m_Bounds	= pHullMesh->m_AABB;
	HullTris.m_Hull.m_Surface	= Surface;

	// ROSANOTE: *Don't* add AABB expansion planes here; they need to be added after hull's orientation is applied

	// ROSANOTE: Optionally keep the mesh around in editor for rendering convex hulls
	SafeDelete( pHullMesh );

	return HullTris.m_Hull;
}

/*static*/ SConvexHull RosaTools::CreateHullFromAABB( const AABB& HullBound, const HashedString& Surface )
{
	SConvexHull Hull;

	Hull.m_Hull.TryAddPlane( Plane( Vector( -1.0f, 0.0f, 0.0f ), HullBound.m_Min.x ), EPSILON );
	Hull.m_Hull.TryAddPlane( Plane( Vector( 1.0f, 0.0f, 0.0f ), -HullBound.m_Max.x ), EPSILON );
	Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, -1.0f, 0.0f ), HullBound.m_Min.y ), EPSILON );
	Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, 1.0f, 0.0f ), -HullBound.m_Max.y ), EPSILON );
	Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, 0.0f, -1.0f ), HullBound.m_Min.z ), EPSILON );
	Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, 0.0f, 1.0f ), -HullBound.m_Max.z ), EPSILON );

	Hull.m_Bounds	= HullBound;
	Hull.m_Surface	= Surface;

	return Hull;
}

#endif // BUILD_ROSA_TOOLS
