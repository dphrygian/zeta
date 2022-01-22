#include "core.h"
#include "rosaroom.h"
#include "idatastream.h"

RosaRoom::RosaRoom()
:	m_Brushes()
,	m_Portals()
,	m_NavNodes()
,	m_NavEdges()
,	m_TilesX( 0 )
,	m_TilesY( 0 )
,	m_TilesZ( 0 )
,	m_MetersX( 0 )
,	m_MetersY( 0 )
,	m_MetersZ( 0 )
,	m_RenderBound()
,	m_CollisionBound()
{
}

RosaRoom::~RosaRoom()
{
}

// ROSANOTE: Despite this format being baked, it could be useful
// to have versioned (e.g., for updating room files after launch)
// ROSANOTE: Restarting from version 1 with new bake step
#define VERSION_EMPTY					0
#define VERSION_BASE					1
#define VERSION_BRUSHES					2
#define VERSION_BRUSH_TYPE				3
#define VERSION_ROOMSIZE				4
#define VERSION_BOUNDS					5
#define VERSION_PORTALS					6
#define VERSION_NAVMESH					7
#define VERSION_MESHNAMES_DEPR			8
#define VERSION_AMBIENTLIGHTS			9
#define VERSION_AMBIENTHULLS			10
#define VERSION_MESHES					11
#define VERSION_PORTAL_NOEXPAND			12
#define VERSION_PORTAL_NOJOIN			13
#define VERSION_PORTAL_EXPANDPRIORITY	14
#define VERSION_PORTAL_MUSTJOIN			15
#define VERSION_BRUSH_SCALE				16
#define VERSION_MESH_CASTSSHADOWS		17
#define VERSION_LINKEDBRUSHES			18
#define VERSION_NAVNODE_HEIGHT			19
#define VERSION_FOGMESHES				20
#define VERSION_PORTAL_NAMEHASHONLY		21
#define VERSION_CURRENT					21

void WriteHull( const IDataStream& Stream, const SConvexHull& Hull )
{
	Stream.Write<AABB>(			Hull.m_Bounds );
	Stream.WriteHashedString(	Hull.m_Surface );
	Stream.WriteUInt32(			Hull.m_CollisionFlags );
	Stream.WriteArray<Plane>(	Hull.m_Hull.GetPlanes() );
}

void ReadHull( const IDataStream& Stream, SConvexHull& Hull )
{
	Hull.m_Bounds			= Stream.Read<AABB>();
	Hull.m_Surface			= Stream.ReadHashedString();
	Hull.m_CollisionFlags	= Stream.ReadUInt32();

	Array<Plane> Planes;
	Stream.ReadArray<Plane>( Planes );
	Hull.m_Hull.AddPlanes( Planes );
}

void RosaRoom::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_TilesX );
	Stream.WriteUInt32( m_TilesY );
	Stream.WriteUInt32( m_TilesZ );
	Stream.WriteUInt32( m_MetersX );
	Stream.WriteUInt32( m_MetersY );
	Stream.WriteUInt32( m_MetersZ );

	Stream.Write<AABB>( m_RenderBound );
	Stream.Write<AABB>( m_CollisionBound );

	Stream.WriteUInt32( m_Brushes.Size() );
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& Brush = BrushIter.GetValue();
		Stream.WriteUInt32( Brush.m_Type );
		Stream.Write<Vector>( Brush.m_Location );
		Stream.Write<Angles>( Brush.m_Orientation );
		Stream.WriteFloat( Brush.m_Scale );
		Stream.WriteHashedString( Brush.m_Mat );
		Stream.WriteString( Brush.m_DefName );

		if( Brush.m_Type == EBT_Geo )
		{
			Stream.WriteUInt32( Brush.m_Meshes.Size() );
			FOR_EACH_ARRAY( MeshIter, Brush.m_Meshes, SMesh )
			{
				const SMesh& Mesh = MeshIter.GetValue();
				Stream.WriteString( Mesh.m_MeshName );
				Stream.WriteString( Mesh.m_MaterialName );
				Stream.WriteBool( Mesh.m_CastsShadows );
			}

			Stream.WriteUInt32( Brush.m_Hulls.Size() );
			FOR_EACH_ARRAY( HullIter, Brush.m_Hulls, SConvexHull )
			{
				const SConvexHull& Hull = HullIter.GetValue();
				WriteHull( Stream, Hull );
			}

			Stream.WriteUInt32( Brush.m_AmbientLights.Size() );
			FOR_EACH_ARRAY( AmbientLightIter, Brush.m_AmbientLights, SAmbientLight )
			{
				const SAmbientLight& AmbientLight = AmbientLightIter.GetValue();
				Stream.WriteString( AmbientLight.m_Mesh );
				Stream.WriteString( AmbientLight.m_Cubemap );
				WriteHull( Stream, AmbientLight.m_Hull );
			}

			Stream.WriteUInt32( Brush.m_FogMeshes.Size() );
			FOR_EACH_ARRAY( FogMeshIter, Brush.m_FogMeshes, SFogMesh )
			{
				const SFogMesh& FogMesh = FogMeshIter.GetValue();
				Stream.WriteString( FogMesh.m_Mesh );
				Stream.WriteHashedString( FogMesh.m_FogMeshDef );
			}
		}
		else if( Brush.m_Type == EBT_Spawner )
		{
			Stream.WriteArray( Brush.m_LinkedBrushes );
		}
	}

	ASSERT( m_Portals.Size() == m_TilesX * m_TilesY * m_TilesZ );
	FOR_EACH_ARRAY( PortalsIter, m_Portals, SPortals )
	{
		const SPortals& Portals = PortalsIter.GetValue();
		for( uint Index = 0; Index < 6; ++Index )
		{
			const SPortal& Portal = Portals.m_Portals[ Index ];
			Stream.WriteHashedString( Portal.m_DefNameHash );
		}
	}

	Stream.WriteUInt32( m_NavNodes.Size() );
	Stream.Write( m_NavNodes.MemorySize(), m_NavNodes.GetData() );

	Stream.WriteUInt32( m_NavEdges.Size() );
	Stream.Write( m_NavEdges.MemorySize(), m_NavEdges.GetData() );
}

void RosaRoom::Load( const IDataStream& Stream )
{
	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ROOMSIZE )
	{
		m_TilesX	= Stream.ReadUInt32();
		m_TilesY	= Stream.ReadUInt32();
		m_TilesZ	= Stream.ReadUInt32();
		m_MetersX	= Stream.ReadUInt32();
		m_MetersY	= Stream.ReadUInt32();
		m_MetersZ	= Stream.ReadUInt32();
	}
	else if( Version >= VERSION_BASE )
	{
		// Make assumptions
		m_TilesX	= 1;
		m_TilesY	= 1;
		m_TilesZ	= 1;
		m_MetersX	= Stream.ReadInt32();
		m_MetersY	= Stream.ReadInt32();
		m_MetersZ	= Stream.ReadInt32();
	}

	if( Version >= VERSION_BOUNDS )
	{
		m_RenderBound		= Stream.Read<AABB>();
		m_CollisionBound	= Stream.Read<AABB>();
	}

	if( Version >= VERSION_BRUSHES )
	{
		const uint NumBrushes = Stream.ReadUInt32();
		m_Brushes.Clear();
		m_Brushes.Reserve( NumBrushes );
		for( uint BrushIndex = 0; BrushIndex < NumBrushes; ++BrushIndex )
		{
			SBrush&		Brush		= m_Brushes.PushBack();

			if( Version >= VERSION_BRUSH_TYPE )
			{
				Brush.m_Type = static_cast<EBrushType>( Stream.ReadUInt32() );
			}

			if( Brush.m_Type == EBT_None )
			{
				// Assume it's a mesh type
				Brush.m_Type = EBT_Geo;
			}

			Brush.m_Location		= Stream.Read<Vector>();
			Brush.m_Orientation		= Stream.Read<Angles>();
			Brush.m_Scale			= ( Version >= VERSION_BRUSH_SCALE ) ? Stream.ReadFloat() : 1.0f;
			Brush.m_Mat				= Stream.ReadHashedString();
			Brush.m_DefName			= Stream.ReadString();

			if( Brush.m_Type == EBT_Geo )
			{
				if( Version >= VERSION_MESHES )
				{
					const uint	NumMeshNames = Stream.ReadUInt32();
					for( uint MeshIndex = 0; MeshIndex < NumMeshNames; ++MeshIndex )
					{
						SMesh& Mesh			= Brush.m_Meshes.PushBack();
						Mesh.m_MeshName		= Stream.ReadString();
						Mesh.m_MaterialName	= Stream.ReadString();
						Mesh.m_CastsShadows	= ( Version >= VERSION_MESH_CASTSSHADOWS ) ? Stream.ReadBool() : true;
					}
				}

				const uint	NumHulls	= Stream.ReadUInt32();
				for( uint HullIndex = 0; HullIndex < NumHulls; ++HullIndex )
				{
					SConvexHull& Hull = Brush.m_Hulls.PushBack();
					ReadHull( Stream, Hull );
				}

				if( Version >= VERSION_AMBIENTLIGHTS )
				{
					const uint NumAmbientLights = Stream.ReadUInt32();
					FOR_EACH_INDEX( AmbientLightIndex, NumAmbientLights )
					{
						SAmbientLight& AmbientLight	= Brush.m_AmbientLights.PushBack();
						AmbientLight.m_Mesh			= Stream.ReadString();
						AmbientLight.m_Cubemap		= Stream.ReadString();

						if( Version >= VERSION_AMBIENTHULLS )
						{
							ReadHull( Stream, AmbientLight.m_Hull );
						}
					}
				}

				if( Version >= VERSION_FOGMESHES )
				{
					const uint NumFogMeshes = Stream.ReadUInt32();
					FOR_EACH_INDEX( FogMeshIndex, NumFogMeshes )
					{
						SFogMesh& FogMesh		= Brush.m_FogMeshes.PushBack();
						FogMesh.m_Mesh			= Stream.ReadString();
						FogMesh.m_FogMeshDef	= Stream.ReadHashedString();
					}
				}
			}
			else if( Brush.m_Type == EBT_Spawner )
			{
				if( Version >= VERSION_LINKEDBRUSHES )
				{
					Stream.ReadArray( Brush.m_LinkedBrushes );
				}
			}
		}
	}

	const uint NumPortals = m_TilesX * m_TilesY * m_TilesZ;
	m_Portals.ResizeZero( NumPortals );
	if( Version >= VERSION_PORTALS )
	{
		for( uint PortalsIndex = 0; PortalsIndex < NumPortals; ++PortalsIndex )
		{
			SPortals& Portals = m_Portals[ PortalsIndex ];
			for( uint Index = 0; Index < 6; ++Index )
			{
				SPortal& Portal			= Portals.m_Portals[ Index ];
				if( Version >= VERSION_PORTAL_NAMEHASHONLY )
				{
					Portal.m_DefNameHash	= Stream.ReadHashedString();
					Portal.InitializeFromDefinition( Portal.m_DefNameHash );
				}
				else
				{
					Stream.ReadHashedString();												// FrontTag
					Stream.ReadHashedString();												// BackTag
					if( Version >= VERSION_PORTAL_NOEXPAND )		{ Stream.ReadBool(); }	// NoExpand
					if( Version >= VERSION_PORTAL_NOJOIN )			{ Stream.ReadBool(); }	// NoJoin
					if( Version >= VERSION_PORTAL_MUSTJOIN )		{ Stream.ReadBool(); }	// MustJoin
					if( Version >= VERSION_PORTAL_EXPANDPRIORITY )	{ Stream.ReadInt32(); }	// ExpandPriority
				}
			}
		}
	}

	if( Version >= VERSION_NAVMESH )
	{
		if( Version >= VERSION_NAVNODE_HEIGHT )
		{
			// HACKHACK: Assuming that the layout of SNavNode will either not change,
			// or that everything will be rebaked when it does.
			const uint NumNavNodes = Stream.ReadUInt32();
			m_NavNodes.Resize( NumNavNodes );
			Stream.Read( m_NavNodes.MemorySize(), m_NavNodes.GetData() );
		}
		else
		{
			// HACKHACK: Load the room without nav until I can resave everything
		}

		const uint NumNavEdges = Stream.ReadUInt32();
		m_NavEdges.Resize( NumNavEdges );
		Stream.Read( m_NavEdges.MemorySize(), m_NavEdges.GetData() );
	}
}
