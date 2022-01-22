#ifndef ROSAROOM_H
#define ROSAROOM_H

// ROSANOTE: The post-bake format for a room.

#include "rosaworld.h"	// For voxel types
#include "vector.h"
#include "angles.h"
#include "set.h"

class IDataStream;

class RosaRoom
{
public:
	RosaRoom();
	~RosaRoom();

	struct SMesh
	{
		SMesh()
		:	m_MeshName()
		,	m_MaterialName()
		,	m_CastsShadows( false )
		{
		}

		SimpleString	m_MeshName;
		SimpleString	m_MaterialName;
		bool			m_CastsShadows;
	};

	struct SAmbientLight
	{
		SAmbientLight()
		:	m_Mesh()
		,	m_Cubemap()
		,	m_Hull()
		{
		}

		SimpleString	m_Mesh;
		SimpleString	m_Cubemap;
		SConvexHull		m_Hull;		// For controlling exposure level
	};

	struct SFogMesh
	{
		SFogMesh()
		:	m_Mesh()
		,	m_FogMeshDef()
		{
		}

		SimpleString	m_Mesh;
		HashedString	m_FogMeshDef;
	};

	struct SBrush
	{
		SBrush()
		:	m_Type( EBT_None )
		,	m_Location()
		,	m_Orientation()
		,	m_Scale( 1.0f )
		,	m_Mat()
		,	m_LinkedBrushes()
		,	m_DefName()
		,	m_Meshes()
		,	m_Hulls()
		,	m_AmbientLights()
		,	m_FogMeshes()
		{
		};

		EBrushType				m_Type;
		Vector					m_Location;
		Angles					m_Orientation;
		float					m_Scale;
		HashedString			m_Mat;
		Array<uint>				m_LinkedBrushes;
		SimpleString			m_DefName;
		Array<SMesh>			m_Meshes;
		Array<SConvexHull>		m_Hulls;
		Array<SAmbientLight>	m_AmbientLights;
		Array<SFogMesh>			m_FogMeshes;
	};

	void	Save( const IDataStream& Stream );
	void	Load( const IDataStream& Stream );

	const Array<SBrush>&	GetBrushes()	const { return m_Brushes; }

private:
	friend class RoomBaker;
	friend class RosaWorldGen;

	Array<SBrush>	m_Brushes;
	Array<SPortals>	m_Portals;
	Array<SNavNode>	m_NavNodes;
	Array<SNavEdge>	m_NavEdges;

	// Module dimension in world tiles
	uint			m_TilesX;
	uint			m_TilesY;
	uint			m_TilesZ;

	// Full size of room, should match world meters/tile
	uint			m_MetersX;
	uint			m_MetersY;
	uint			m_MetersZ;

	AABB			m_RenderBound;		// Tight, tile-fitted bound, used in conjunction with sectors and portals
	AABB			m_CollisionBound;	// Expanded bound to contain collision hulls that overlap module edges
};

#endif // ROSAROOM_H
