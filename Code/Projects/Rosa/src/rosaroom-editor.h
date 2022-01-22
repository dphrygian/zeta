#ifndef ROSAROOMEDITOR_H
#define ROSAROOMEDITOR_H

// ROSANOTE: The pre-bake format for a room.

#include "rosatools.h"

#if BUILD_ROSA_TOOLS

#include "rosaworld.h"	// For voxel types
#include "vector.h"
#include "angles.h"
#include "set.h"
#include "rosaroom.h"

class IDataStream;

enum EMapType
{
	EMT_Room,
	EMT_World,
};

class RosaRoomEditor
{
public:
	RosaRoomEditor();
	~RosaRoomEditor();

	void	Save( const IDataStream& Stream );
	void	Load( const IDataStream& Stream );

	struct SBrush
	{
		SBrush()
		:	m_Type( EBT_None )
		,	m_DefName()
		,	m_Selected( false )
		,	m_Hidden( false )
		,	m_Location()
		,	m_Orientation()
		,	m_Scale( 1.0f )
		,	m_Mat()
		,	m_LinkedBrushes()
		{
		}

		EBrushType		m_Type;
		SimpleString	m_DefName;	// Mesh brush def, spawner def, etc.
		bool			m_Selected;
		bool			m_Hidden;
		Vector			m_Location;
		Angles			m_Orientation;
		float			m_Scale;
		HashedString	m_Mat;
		Array<uint>		m_LinkedBrushes;
	};

	struct SPortal
	{
		SPortal()
		:	m_DefName()
		{
		}

		SimpleString	m_DefName;
	};

	struct SPortals
	{
		SPortals()
		:	m_Portals()
		{
		}

		// Ordered X+ X- Y+ Y- Z+ Z-
		SPortal	m_Portals[6];
	};

private:
	friend class RosaTools;
	friend class RoomBaker;

	EMapType		m_MapType;

	Array<SBrush>	m_Brushes;
	Array<SPortals>	m_Portals;

	Array<RosaTools::SNavVert>	m_NavVerts;
	Array<RosaTools::SNavEdge>	m_NavEdges;
	Array<RosaTools::SNavFace>	m_NavFaces;

	// Module dimension in world tiles
	uint			m_TilesX;
	uint			m_TilesY;
	uint			m_TilesZ;

	// Full size of room, should match world meters/tile
	uint			m_MetersX;
	uint			m_MetersY;
	uint			m_MetersZ;

	// Editor-only variables that won't be baked.
	Vector			m_TOOLS_CameraLocation;
	Angles			m_TOOLS_CameraOrientation;
};

#endif // BUILD_ROSA_TOOLS

#endif // ROSAROOMEDITOR_H
