#include "core.h"
#include "rosaroom-editor.h"
#include "idatastream.h"

#if BUILD_ROSA_TOOLS

RosaRoomEditor::RosaRoomEditor()
:	m_Brushes()
,	m_Portals()
,	m_NavVerts()
,	m_NavEdges()
,	m_NavFaces()
,	m_TilesX( 0 )
,	m_TilesY( 0 )
,	m_TilesZ( 0 )
,	m_MetersX( 0 )
,	m_MetersY( 0 )
,	m_MetersZ( 0 )
,	m_MapType( EMT_Room )
,	m_TOOLS_CameraLocation()
,	m_TOOLS_CameraOrientation()
{
}

RosaRoomEditor::~RosaRoomEditor()
{
}

// ROSANOTE: Restarting from version 1 with new bake step
#define VERSION_EMPTY					0
#define VERSION_BASE					1
#define VERSION_BRUSHES					2
#define VERSION_BRUSH_SELECTED			3
#define VERSION_BRUSH_TYPE				4
#define VERSION_ROOMSIZE				5
#define VERSION_PORTALS					6
#define VERSION_NAVMESH					7
#define VERSION_BRUSH_MAT				8
#define VERSION_PORTAL_NOEXPAND			9
#define VERSION_PORTAL_NOJOIN			10
#define VERSION_NAVVERT_TRANSFORM		11
#define VERSION_PORTAL_EXPANDPRIORITY	12
#define VERSION_PORTAL_MUSTJOIN			13
#define VERSION_BRUSH_SCALE				14
#define VERSION_MAPTYPE					15
#define VERSION_LINKEDBRUSHES			16
#define VERSION_NAVFACE_HEIGHT			17
#define VERSION_NAVFACE_PROPS			18
#define VERSION_PORTAL_NAMEONLY			19
#define VERSION_CURRENT					19
// ROSANOTE: When this format changes, I have to recompile RoomBaker too!

void RosaRoomEditor::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_MapType );

	Stream.WriteUInt32( m_TilesX );
	Stream.WriteUInt32( m_TilesY );
	Stream.WriteUInt32( m_TilesZ );
	Stream.WriteUInt32( m_MetersX );
	Stream.WriteUInt32( m_MetersY );
	Stream.WriteUInt32( m_MetersZ );

	Stream.WriteUInt32( m_Brushes.Size() );
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& Brush = BrushIter.GetValue();
		Stream.WriteUInt32( Brush.m_Type );
		Stream.WriteString( Brush.m_DefName );
		Stream.WriteBool( Brush.m_Selected );
		Stream.WriteBool( Brush.m_Hidden );
		Stream.Write<Vector>( Brush.m_Location );
		Stream.Write<Angles>( Brush.m_Orientation );
		Stream.WriteFloat( Brush.m_Scale );
		Stream.WriteHashedString( Brush.m_Mat );
		Stream.WriteArray( Brush.m_LinkedBrushes );
	}

	if( m_MapType == EMT_Room )
	{
		ASSERT( m_Portals.Size() == m_TilesX * m_TilesY * m_TilesZ );
		FOR_EACH_ARRAY( PortalsIter, m_Portals, SPortals )
		{
			const SPortals& Portals = PortalsIter.GetValue();
			for( uint Index = 0; Index < 6; ++Index )
			{
				const SPortal& Portal = Portals.m_Portals[ Index ];
				Stream.WriteString( Portal.m_DefName );
			}
		}

		Stream.WriteUInt32( m_NavVerts.Size() );
		Stream.WriteUInt32( m_NavEdges.Size() );
		Stream.WriteUInt32( m_NavFaces.Size() );
		Stream.Write( m_NavVerts.MemorySize(), m_NavVerts.GetData() );
		Stream.Write( m_NavEdges.MemorySize(), m_NavEdges.GetData() );
		Stream.Write( m_NavFaces.MemorySize(), m_NavFaces.GetData() );
	}

	Stream.Write( sizeof( Vector ), &m_TOOLS_CameraLocation );
	Stream.Write( sizeof( Angles ), &m_TOOLS_CameraOrientation );
}

void RosaRoomEditor::Load( const IDataStream& Stream )
{
	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_MAPTYPE )
	{
		m_MapType = static_cast<EMapType>( Stream.ReadUInt32() );
	}

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

	if( Version >= VERSION_BRUSHES )
	{
		const uint NumBrushes = Stream.ReadUInt32();
		m_Brushes.Clear();
		m_Brushes.Reserve( NumBrushes );
		for( uint BrushIndex = 0; BrushIndex < NumBrushes; ++BrushIndex )
		{
			SBrush& Brush = m_Brushes.PushBack();

			if( Version >= VERSION_BRUSH_TYPE )
			{
				Brush.m_Type = static_cast<EBrushType>( Stream.ReadUInt32() );
			}

			if( Brush.m_Type == EBT_None )
			{
				// Assume it's a mesh type
				Brush.m_Type = EBT_Geo;
			}

			Brush.m_DefName	= Stream.ReadString();

			if( Version >= VERSION_BRUSH_SELECTED )
			{
				Brush.m_Selected	= Stream.ReadBool();
				Brush.m_Hidden		= Stream.ReadBool();
			}

			Brush.m_Location	= Stream.Read<Vector>();
			Brush.m_Orientation	= Stream.Read<Angles>();

			Brush.m_Scale = ( Version >= VERSION_BRUSH_SCALE ) ? Stream.ReadFloat() : 1.0f;

			if( Version >= VERSION_BRUSH_MAT )
			{
				Brush.m_Mat	= Stream.ReadHashedString();
			}

			if( Version >= VERSION_LINKEDBRUSHES )
			{
				Stream.ReadArray( Brush.m_LinkedBrushes );
			}
		}
	}

	if( m_MapType == EMT_Room )
	{
		const uint NumPortals = m_TilesX * m_TilesY * m_TilesZ;
		m_Portals.Reserve( NumPortals );
		for( uint PortalsIndex = 0; PortalsIndex < NumPortals; ++PortalsIndex )
		{
			SPortals& Portals = m_Portals.PushBack();
			if( Version >= VERSION_PORTALS )
			{
				for( uint Index = 0; Index < 6; ++Index )
				{
					SPortal& Portal		= Portals.m_Portals[ Index ];
					Portal.m_DefName	= Stream.ReadString();
					if( Version < VERSION_PORTAL_NAMEONLY )
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
			const uint NumNavVerts = Stream.ReadUInt32();
			const uint NumNavEdges = Stream.ReadUInt32();
			const uint NumNavFaces = Stream.ReadUInt32();

			m_NavVerts.Resize( NumNavVerts );
			if( Version < VERSION_NAVVERT_TRANSFORM )
			{
				// HACKHACK because I changed this struct
				FOR_EACH_ARRAY( VertIter, m_NavVerts, RosaTools::SNavVert )
				{
					RosaTools::SNavVert&	Vert	= VertIter.GetValue();
					Vert.m_Vert						= Stream.Read<Vector>();
					Vert.m_Selected					= ( Stream.ReadUInt32() != 0 );	// ReadBool will only read 1 byte, but this is serialized according to the struct size
				}
			}
			else
			{
				Stream.Read( m_NavVerts.MemorySize(), m_NavVerts.GetData() );
			}

			m_NavEdges.Resize( NumNavEdges );
			Stream.Read( m_NavEdges.MemorySize(), m_NavEdges.GetData() );

			m_NavFaces.Resize( NumNavFaces );
			if( Version >= VERSION_NAVFACE_PROPS )
			{
				Stream.Read( m_NavFaces.MemorySize(), m_NavFaces.GetData() );
			}
			else
			{
				// HACKHACK: Trusting that the old size was just the new size without the props uint float
				const uint OldNavFaceSize = sizeof( RosaTools::SNavFace ) - sizeof( uint );
				FOR_EACH_INDEX( NavFaceIndex, NumNavFaces )
				{
					Stream.Read( OldNavFaceSize, &m_NavFaces[ NavFaceIndex ] );
					m_NavFaces[ NavFaceIndex ].m_Props = ENP_None;
				}
			}
		}
	}

	if( Version >= VERSION_BASE )
	{
		Stream.Read( sizeof( Vector ), &m_TOOLS_CameraLocation );
		Stream.Read( sizeof( Angles ), &m_TOOLS_CameraOrientation );
	}
}

#endif	// BUILD_ROSA_TOOLS
