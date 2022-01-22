#include "core.h"
#include "rosatools.h"

// Controls:
// Tab: toggle tools
//
// F1: toggle world/room mode
// F2: spawners
// F3: brushes/rooms
// F4: portals
// F5: navmesh
// F6: save as
// F7: clear
// F8: materials
// F9: load
// F10: prefabs
// F11: ?
// F12: unused?
//
// Num*: toggle grid
// Num+: double grid (Shift+Num+ to double rotation grid)
// Num-: halve grid (Shift+Num- to halve rotation grid)
// []: size X (Ctrl to change brush, Shift to change meters/tile, no mod to change tiles/room)
// ;': size Y ("")
// ./: size Z ("")
//
// LMB: select
// MMB: box select
// U: select all
// I: invert selection
// O: select none
//
// Normal mode:
// 	WASDQE: move camera
// 	C: clone
// 	V: place spawner (Ctrl+V to replace)
// 	B: place brush/room (Ctrl+B to replace)
// 	P: place prefab (Ctrl+P to replace)
// 	T: tag portal
//  Ctrl+T: untag portal
// 	G: translate (Alt+G to translate along Z; Ctrl+G to translate anchor/pivot, Ctrl+Alt+G to translate anchor/pivot on Z)
// 	R: rotate (about Z; Alt+R to rotate along prime camera axis)
// 	X: scale (Ctrl+X to reset scale)
//	F: link brushes
// 	L: load selected room (in world mode)
//  Ctrl+L: convert world to room (in world mode)
//  Shift+L: load world from worldgen stored modules (in world mode)
// 	K: print spawner manifest
// 	N: snap to grid
// 	Ctrl+N: reset rotation
// 	H: hide (Shift+H to hide all except selected; Alt+H to unhide all)
// 	M: apply material (Ctrl+M to reset to default)
//	J: save prefab
// 	RMB: eyedropper
// 	Ctrl+S: quicksave
// 	Ctrl+Y: redo
// 	Ctrl+Z: undo
//
// Navmesh mode:
// 	F: fill tri from selected elements and cursor
// 	G: translate
// 	R: rotate
//  T: tag navmesh face (should avoid / must avoid)
// 	Alt+G: translate selected vert to cursor
// 	RMB: place vert at cursor (connected to current selection)

#if BUILD_ROSA_TOOLS

#include "rosaframework.h"
#include "rosagame.h"
#include "irenderer.h"
#include "rosaworld.h"
#include "meshfactory.h"
#include "shadermanager.h"
#include "texturemanager.h"
#include "ivertexbuffer.h"
#include "ivertexdeclaration.h"
#include "iindexbuffer.h"
#include "mesh.h"
#include "mouse.h"
#include "keyboard.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "vector2.h"
#include "idatastream.h"
#include "filestream.h"
#include "memorystream.h"
#include "dynamicmemorystream.h"
#include "fileutil.h"
#include "windowwrapper.h"
#include "fontmanager.h"
#include "3d.h"
#include "segment.h"
#include "collisioninfo.h"
#include "rosaroom-editor.h"
#include "configmanager.h"
#include "view.h"
#include "Common/uimanagercommon.h"
#include "uiscreen.h"
#include "uistack.h"
#include "irendertarget.h"
#include "rosatargetmanager.h"
#include "reversehash.h"
#include "packstream.h"
#include "line.h"
#include "dynamicmeshmanager.h"
#include "wbeventmanager.h"

#include <Windows.h>	// For MessageBox

RosaTools::RosaTools()
:	m_Framework( NULL )
,	m_ToolMode( false )
,	m_SubTool( EST_RoomMode )
,	m_TilesX( 0 )
,	m_TilesY( 0 )
,	m_TilesZ( 0 )
,	m_MetersX( 0 )
,	m_MetersY( 0 )
,	m_MetersZ( 0 )
,	m_GridMeshes()
,	m_PortalMesh( NULL )
,	m_RoomElements()
,	m_SpawnerElements()
,	m_GeoElements()
,	m_PrefabElements()
,	m_PortalElements()
,	m_MatElements()
,	m_RoomDefs()
,	m_SpawnerDefs()
,	m_GeoDefs()
,	m_PortalDefs()
,	m_MatDefs()
,	m_GeoDefSizeMap()
,	m_HelpMesh( NULL )
,	m_CameraLocation()
,	m_CameraOrientation()
,	m_CameraSpeed( 0.0f )
,	m_CameraVelocity()
,	m_CameraRotationalVelocity()
,	m_BoundPlanes()
,	m_CurrentRoomName()
,	m_CurrentWorldName()
,	m_UndoStates()
,	m_UndoStateIndex( -1 )
,	m_SavedUndoStateIndex( -1 )
,	m_LoadingUndoState( false )
,	m_SavingUndoState( false )
,	m_Brushes()
,	m_SelectedBrushes()
,	m_GridActive( true )
,	m_GridSize( 0.5f )
,	m_RotateGridSize( 15.0f )
,	m_TransformStartCursorX( 0 )
,	m_TransformStartCursorY( 0 )
,	m_TransformAnchorScreenX( 0 )
,	m_TransformAnchorScreenY( 0 )
,	m_TransformAnchor()
,	m_TranslateAnchorStart()
,	m_IsTranslating( false )
,	m_IsTranslatingAnchor( false )
,	m_IsRotating( false )
,	m_IsScaling( false )
,	m_IsBoxSelecting( false )
,	m_NavSnapPoints()
,	m_NavSnapPoint()
,	m_NavVerts()
,	m_NavEdges()
,	m_NavFaces()
,	m_SelectedNavVerts()
,	m_SelectedNavEdges()
,	m_SelectedNavFaces()
,	m_NavMesh( NULL )
,	m_NavSnapMesh( NULL )
,	m_SpawnerNameColors()
{
}

RosaTools::~RosaTools()
{
	ShutDown();
}

void RosaTools::ShutDown()
{
	ClearBrushes();
	m_Portals.Clear();

	DeleteGridMeshes();
	SafeDelete( m_PortalMesh );
	SafeDelete( m_HelpMesh );
	DeleteElementButtonMeshes( m_RoomElements );
	DeleteElementButtonMeshes( m_SpawnerElements );
	DeleteElementButtonMeshes( m_GeoElements );
	DeleteElementButtonMeshes( m_PrefabElements );
	DeleteElementButtonMeshes( m_PortalElements );
	DeleteElementButtonMeshes( m_MatElements );
	SafeDelete( m_NavMesh );
	SafeDelete( m_NavSnapMesh );
}

void RosaTools::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumRoomCategories );
	const uint NumRoomCategories = ConfigManager::GetInt( sNumRoomCategories, 0, sDefinitionName );
	for( uint CategoryIndex = 0; CategoryIndex < NumRoomCategories; ++CategoryIndex )
	{
		const SimpleString Category = ConfigManager::GetSequenceString( "RoomCategory%d", CategoryIndex, "", sDefinitionName );
		AppendRoomCategory( Category );
	}

	STATICHASH( NumSpawnerCategories );
	const uint NumSpawnerCategories = ConfigManager::GetInt( sNumSpawnerCategories, 0, sDefinitionName );
	for( uint CategoryIndex = 0; CategoryIndex < NumSpawnerCategories; ++CategoryIndex )
	{
		const SimpleString Category = ConfigManager::GetSequenceString( "SpawnerCategory%d", CategoryIndex, "", sDefinitionName );
		AppendSpawnerCategory( Category );
	}

	STATICHASH( NumGeoCategories );
	const uint NumGeoCategories = ConfigManager::GetInt( sNumGeoCategories, 0, sDefinitionName );
	for( uint CategoryIndex = 0; CategoryIndex < NumGeoCategories; ++CategoryIndex )
	{
		const SimpleString Category = ConfigManager::GetSequenceString( "GeoCategory%d", CategoryIndex, "", sDefinitionName );
		AppendGeoCategory( Category );
	}

	STATICHASH( NumPrefabCategories );
	const uint NumPrefabCategories = ConfigManager::GetInt( sNumPrefabCategories, 0, sDefinitionName );
	for( uint CategoryIndex = 0; CategoryIndex < NumPrefabCategories; ++CategoryIndex )
	{
		const SimpleString Category = ConfigManager::GetSequenceString( "PrefabCategory%d", CategoryIndex, "", sDefinitionName );
		AppendPrefabCategory( Category );
	}

	STATICHASH( NumPortalCategories );
	const uint NumPortalCategories = ConfigManager::GetInt( sNumPortalCategories, 0, sDefinitionName );
	for( uint CategoryIndex = 0; CategoryIndex < NumPortalCategories; ++CategoryIndex )
	{
		const SimpleString Category = ConfigManager::GetSequenceString( "PortalCategory%d", CategoryIndex, "", sDefinitionName );
		AppendPortalCategory( Category );
	}

	STATICHASH( NumMatCategories );
	const uint NumMatCategories = ConfigManager::GetInt( sNumMatCategories, 0, sDefinitionName );
	for( uint CategoryIndex = 0; CategoryIndex < NumMatCategories; ++CategoryIndex )
	{
		const SimpleString Category = ConfigManager::GetSequenceString( "MatCategory%d", CategoryIndex, "", sDefinitionName );
		AppendMatCategory( Category );
	}

	STATICHASH( NumAppendLevels );
	const uint NumAppendLevels = ConfigManager::GetInt( sNumAppendLevels, 0, sDefinitionName );
	for( uint AppendLevelIndex = 0; AppendLevelIndex < NumAppendLevels; ++AppendLevelIndex )
	{
		const SimpleString LevelName = ConfigManager::GetSequenceString( "AppendLevel%d", AppendLevelIndex, "", sDefinitionName );
		AppendCategories( LevelName );
	}

	// All geo should be loaded now, sort the geo size map and build relationships
	BuildGeoSizeLinks();

	Reinitialize();
}

void RosaTools::AppendCategories( const SimpleString& LevelName )
{
	MAKEHASH( LevelName );

	STATICHASH( RosaTools_AppendRoomCategories );
	const SimpleString LevelRoomCategory	= ConfigManager::GetString( sLevelName, "", sRosaTools_AppendRoomCategories );
	AppendRoomCategory( LevelRoomCategory );

	STATICHASH( RosaTools_AppendSpawnerCategories );
	const SimpleString LevelSpawnerCategory	= ConfigManager::GetString( sLevelName, "", sRosaTools_AppendSpawnerCategories );
	AppendSpawnerCategory( LevelSpawnerCategory );

	STATICHASH( RosaTools_AppendGeoCategories );
	const SimpleString LevelGeoCategory	= ConfigManager::GetString( sLevelName, "", sRosaTools_AppendGeoCategories );
	AppendGeoCategory( LevelGeoCategory );

	STATICHASH( RosaTools_AppendPrefabCategories );
	const SimpleString LevelPrefabCategory	= ConfigManager::GetString( sLevelName, "", sRosaTools_AppendPrefabCategories );
	AppendPrefabCategory( LevelPrefabCategory );

	STATICHASH( RosaTools_AppendPortalCategories );
	const SimpleString LevelPortalCategory	= ConfigManager::GetString( sLevelName, "", sRosaTools_AppendPortalCategories );
	AppendPortalCategory( LevelPortalCategory );

	STATICHASH( RosaTools_AppendMatCategories );
	const SimpleString LevelMatCategory	= ConfigManager::GetString( sLevelName, "", sRosaTools_AppendMatCategories );
	AppendMatCategory( LevelMatCategory );
}

uint RosaTools::FindOrCreateRoomDef( const SimpleString& DefName )
{
	uint RetVal;

	if( FindRoomDef( DefName, &RetVal ) )
	{
		// Do nothing
	}
	else
	{
		RetVal = CreateRoomDef( DefName );
	}

	return RetVal;
}

uint RosaTools::CreateRoomDef( const SimpleString& RoomDefName )
{
	DEVASSERT( !FindRoomDef( RoomDefName ) );

	const uint	RoomDefIndex	= m_RoomDefs.Size();
	SRoomDef&	NewRoomDef		= m_RoomDefs.PushBack();
	NewRoomDef.m_DefName		= RoomDefName;

	// Open the world file to pull data from it
	RosaRoomEditor TempRoom;
	TempRoom.Load( FileStream( RoomDefName.CStr(), FileStream::EFM_Read ) );

	NewRoomDef.m_Extents.m_Max = Vector(
		static_cast<float>( TempRoom.m_TilesX ),
		static_cast<float>( TempRoom.m_TilesY ),
		static_cast<float>( TempRoom.m_TilesZ ) );
	NewRoomDef.m_Extents.MoveBy( Vector( -0.5f, -0.5f, -0.5f ) );	// Compensate for offset cursor

	return RoomDefIndex;
}

void RosaTools::AppendRoomCategory( const SimpleString& Category )
{
	if( Category == "" )
	{
		return;
	}

	// NOTE: This behaves differently than other ::Append functions because
	// I want to add new rooms whenever the tools are reinitialized.
	if( m_RoomElements.m_Categories.Search( Category ).IsNull() )
	{
		m_RoomElements.m_Categories.PushBack( Category );
	}

	const uint CategoryIndex = m_RoomElements.m_Categories.Search( Category ).GetIndex();

	MAKEHASH( Category );

	STATICHASH( RoomsDir );
	const SimpleString RoomsDir = ConfigManager::GetString( sRoomsDir, "", sCategory );
	if( RoomsDir != "" )
	{
		Array<SimpleString> RoomDefNames;
		FileUtil::GetFilesInFolder( RoomsDir, true, ".rosaroom", RoomDefNames );

		FOR_EACH_ARRAY( RoomDefNameIter, RoomDefNames, SimpleString )
		{
			const SimpleString& RoomDefName = RoomDefNameIter.GetValue();
			if( FindRoomDef( RoomDefName ) )
			{
				continue;
			}

			const uint RoomDefIndex = CreateRoomDef( RoomDefName );
			m_RoomElements.m_CategoryMap[ RoomDefIndex ] = CategoryIndex;
		}
	}

	STATICHASH( NumRoomDefs );
	const uint NumRoomDefs = ConfigManager::GetInt( sNumRoomDefs, 0, sCategory );

	for( uint RoomDefCategoryIndex = 0; RoomDefCategoryIndex < NumRoomDefs; ++RoomDefCategoryIndex )
	{
		const SimpleString RoomDefName = ConfigManager::GetSequenceString( "RoomDef%d", RoomDefCategoryIndex, "", sCategory );
		if( FindRoomDef( RoomDefName ) )
		{
			continue;
		}

		const uint RoomDefIndex = CreateRoomDef( RoomDefName );
		m_RoomElements.m_CategoryMap[ RoomDefIndex ] = CategoryIndex;
	}
}

void RosaTools::AppendSpawnerCategory( const SimpleString& Category )
{
	if( Category == "" )
	{
		return;
	}

	if( m_SpawnerElements.m_Categories.Search( Category ).IsValid() )
	{
		return;
	}

	const uint CategoryIndex = m_SpawnerElements.m_Categories.Size();
	m_SpawnerElements.m_Categories.PushBack( Category );

	MAKEHASH( Category );

	STATICHASH( NumSpawnerDefs );
	const uint NumSpawnerDefs = ConfigManager::GetInt( sNumSpawnerDefs, 0, sCategory );

	for( uint SpawnerDefCategoryIndex = 0; SpawnerDefCategoryIndex < NumSpawnerDefs; ++SpawnerDefCategoryIndex )
	{
		const SimpleString SpawnerDefName = ConfigManager::GetSequenceString( "SpawnerDef%d", SpawnerDefCategoryIndex, "", sCategory );
		if( FindSpawnerDef( SpawnerDefName ) )
		{
			continue;
		}

		const uint SpawnerDefIndex = CreateSpawnerDef( SpawnerDefName );
		m_SpawnerElements.m_CategoryMap[ SpawnerDefIndex ] = CategoryIndex;
	}
}

uint RosaTools::CreateSpawnerDef( const SimpleString& SpawnerDefName )
{
	DEVASSERT( !FindSpawnerDef( SpawnerDefName ) );

	const uint		SpawnerDefIndex	= m_SpawnerDefs.Size();
	SSpawnerDef&	NewSpawnerDef	= m_SpawnerDefs.PushBack();
	NewSpawnerDef.m_DefName			= SpawnerDefName;

	MAKEHASH( SpawnerDefName );

	{
		STATICHASH( OffsetX );
		NewSpawnerDef.m_Offset.x = ConfigManager::GetInheritedFloat( sOffsetX, 0.0f, sSpawnerDefName );

		STATICHASH( OffsetY );
		NewSpawnerDef.m_Offset.y = ConfigManager::GetInheritedFloat( sOffsetY, 0.0f, sSpawnerDefName );

		STATICHASH( OffsetZ );
		NewSpawnerDef.m_Offset.z = ConfigManager::GetInheritedFloat( sOffsetZ, 0.0f, sSpawnerDefName );
	}

	{
		STATICHASH( Mesh );
		NewSpawnerDef.m_MeshName = ConfigManager::GetInheritedString( sMesh, "Meshes/Brushes/spawner.cms", sSpawnerDefName );

		// DLP 6 Dec 2021: Only add nav snap points if we're using a custom mesh, else it's just noise.
		// (The snap points are derived from the visible mesh, not the spawned entity's collision, since
		// we can't know that in advance due to subspawners.)
		if( NewSpawnerDef.m_MeshName != "Meshes/Brushes/spawner.cms" )
		{
			NewSpawnerDef.m_AddNavMeshSnapPoints = true;
		}

		STATICHASH( MeshOffsetX );
		NewSpawnerDef.m_MeshOffset.x = ConfigManager::GetInheritedFloat(sMeshOffsetX, 0.0f, sSpawnerDefName );

		STATICHASH( MeshOffsetY );
		NewSpawnerDef.m_MeshOffset.y = ConfigManager::GetInheritedFloat(sMeshOffsetY, 0.0f, sSpawnerDefName );

		STATICHASH( MeshOffsetZ );
		NewSpawnerDef.m_MeshOffset.z = ConfigManager::GetInheritedFloat(sMeshOffsetZ, 0.0f, sSpawnerDefName );
	}

	{
		Vector HalfExtents;

		ASSERT( NewSpawnerDef.m_MeshName != "" );
		MeshFactory* const pMeshFactory = RosaFramework::GetInstance()->GetRenderer()->GetMeshFactory();
		Mesh* const pSpawnerMesh = DynamicMeshManager::GetInstance()->GetOrCreateMesh( NewSpawnerDef.m_MeshName.CStr(), pMeshFactory );
		HalfExtents = pSpawnerMesh->GetAABB().GetExtents();

		STATICHASH( HalfExtentsX );
		HalfExtents.x = ConfigManager::GetInheritedFloat( sHalfExtentsX, HalfExtents.x, sSpawnerDefName );

		STATICHASH( HalfExtentsY );
		HalfExtents.y = ConfigManager::GetInheritedFloat( sHalfExtentsY, HalfExtents.y, sSpawnerDefName );

		STATICHASH( HalfExtentsZ );
		HalfExtents.z = ConfigManager::GetInheritedFloat( sHalfExtentsZ, HalfExtents.z, sSpawnerDefName );

		NewSpawnerDef.m_TraceExtents = AABB::CreateFromCenterAndExtents( Vector(), HalfExtents );
	}

	return SpawnerDefIndex;
}

uint RosaTools::FindOrCreateSpawnerDef( const SimpleString& DefName )
{
	uint RetVal;

	if( FindSpawnerDef( DefName, &RetVal ) )
	{
		// Do nothing
	}
	else
	{
		RetVal = CreateSpawnerDef( DefName );
	}

	return RetVal;
}

uint RosaTools::FindOrCreateGeoDef( const SimpleString& DefName )
{
	uint RetVal;

	if( FindGeoDef( DefName, &RetVal ) )
	{
		// Do nothing
	}
	else
	{
		RetVal = CreateGeoDef( DefName );
	}

	return RetVal;
}

uint RosaTools::FindGeoSizeLink( const TGeoSizeIndexMap& TagGeo, const Vector& GeoDefSize, const uint& GeoDefIndex, const uint Axis, const bool Sign ) const
{
	const uint	AxisB			= ( Axis + 1 ) % 3;
	const uint	AxisC			= ( Axis + 2 ) % 3;
	float		MinDelta		= FLT_MAX;
	uint		MinGeoDefIndex	= 0xffffffff;

	FOR_EACH_MAP( FindLinkIter, TagGeo, Vector, uint )
	{
		const Vector&	FindGeoDefSize		= FindLinkIter.GetKey();
		const uint&		FindGeoDefIndex		= FindLinkIter.GetValue();
		if( FindGeoDefIndex == GeoDefIndex )
		{
			continue;
		}

		if( FindGeoDefSize.v[ AxisB ] != GeoDefSize.v[ AxisB ] ||
			FindGeoDefSize.v[ AxisC ] != GeoDefSize.v[ AxisC ] )
		{
			continue;
		}

		const float Delta = FindGeoDefSize.v[ Axis ] - GeoDefSize.v[ Axis ];
		if( ( Sign && Delta <= 0.0f ) || ( !Sign && Delta >= 0.0f ) )
		{
			continue;
		}

		const float AbsDelta = Abs( Delta );
		if( AbsDelta < MinDelta )
		{
			MinDelta		= AbsDelta;
			MinGeoDefIndex	= FindGeoDefIndex;
		}
	}

	return MinGeoDefIndex;
}

void RosaTools::BuildGeoSizeLinks()
{
	FOR_EACH_MAP( GeoDefTagIter, m_GeoDefSizeMap, HashedString, TGeoSizeIndexMap )
	{
		// Only link to matching tags
		const TGeoSizeIndexMap& TagGeo = GeoDefTagIter.GetValue();

		// Pseudocode:
		// for each item in the tag
		//   for each axis (in each direction)
		//     find the smallest delta, and add a link to its index
		// finally, do a validation pass to make sure all links are mutual
		FOR_EACH_MAP( GeoDefIndexIter, TagGeo, Vector, uint )
		{
			const Vector&	GeoDefSize		= GeoDefIndexIter.GetKey();
			const uint&		GeoDefIndex		= GeoDefIndexIter.GetValue();
			SGeoDef&		GeoDef			= m_GeoDefs[ GeoDefIndex ];

			GeoDef.m_LinkXNeg				= FindGeoSizeLink( TagGeo, GeoDefSize, GeoDefIndex, 0, false );
			GeoDef.m_LinkXPos				= FindGeoSizeLink( TagGeo, GeoDefSize, GeoDefIndex, 0, true );
			GeoDef.m_LinkYNeg				= FindGeoSizeLink( TagGeo, GeoDefSize, GeoDefIndex, 1, false );
			GeoDef.m_LinkYPos				= FindGeoSizeLink( TagGeo, GeoDefSize, GeoDefIndex, 1, true );
			GeoDef.m_LinkZNeg				= FindGeoSizeLink( TagGeo, GeoDefSize, GeoDefIndex, 2, false );
			GeoDef.m_LinkZPos				= FindGeoSizeLink( TagGeo, GeoDefSize, GeoDefIndex, 2, true );
		}

		// Validate
		FOR_EACH_ARRAY( GeoDefIter, m_GeoDefs, SGeoDef )
		{
			const uint		GeoDefIndex	= GeoDefIter.GetIndex();
			const SGeoDef&	GeoDef		= GeoDefIter.GetValue();

			if( GeoDef.m_LinkXNeg != 0xffffffff ) { DEVASSERT( m_GeoDefs[ GeoDef.m_LinkXNeg ].m_LinkXPos == GeoDefIndex ); }
			if( GeoDef.m_LinkXPos != 0xffffffff ) { DEVASSERT( m_GeoDefs[ GeoDef.m_LinkXPos ].m_LinkXNeg == GeoDefIndex ); }
			if( GeoDef.m_LinkYNeg != 0xffffffff ) { DEVASSERT( m_GeoDefs[ GeoDef.m_LinkYNeg ].m_LinkYPos == GeoDefIndex ); }
			if( GeoDef.m_LinkYPos != 0xffffffff ) { DEVASSERT( m_GeoDefs[ GeoDef.m_LinkYPos ].m_LinkYNeg == GeoDefIndex ); }
			if( GeoDef.m_LinkZNeg != 0xffffffff ) { DEVASSERT( m_GeoDefs[ GeoDef.m_LinkZNeg ].m_LinkZPos == GeoDefIndex ); }
			if( GeoDef.m_LinkZPos != 0xffffffff ) { DEVASSERT( m_GeoDefs[ GeoDef.m_LinkZPos ].m_LinkZNeg == GeoDefIndex ); }
		}
	}
}

inline bool IsNumeric( const char c )
{
	return c >= '0' && c <= '9';
}

bool RosaTools::ParseGeoDefSize( const SimpleString& GeoDefName, HashedString& OutTag, Vector& OutSize ) const
{
	if( !GeoDefName.EndsWith( ".cbr" ) )
	{
		return false;
	}

	// HACKHACK: Try to parse geodef name for relative size info
	// Expected format: "[path]/[tag]-[x]x[y]x[z].cbr"
	// e.g., "Brushes/Core/box-05x05x1.cbr", where "05" is a hack meaning 0.5

	const char*	pStringIter	= GeoDefName.CStr();
	const char*	pExtension	= NULL;
	const char*	pTagStart	= NULL;
	const char*	pTagEnd		= NULL;
	const char*	pSizeX		= NULL;
	const char*	pSizeY		= NULL;
	const char*	pSizeZ		= NULL;

	// Find the '.' preceding the file extension
	for( ; *pStringIter != '.'; ++pStringIter );
	pExtension = pStringIter;

	// Wind back to find the last '/' (this file should be in Brushes/, at least)
	for( ; *pStringIter != '/'; --pStringIter );
	pTagStart = pStringIter + 1;

	// Find the first numeral, if any
	for( ; pStringIter < pExtension && !IsNumeric( *pStringIter ); ++pStringIter );
	if( pStringIter == pExtension )
	{
		return false;
	}

	pTagEnd = pStringIter - 1;
	pSizeX = pStringIter;

	const SimpleString GeoDefTag = SimpleString( pTagStart, static_cast<uint>( pTagEnd - pTagStart ) );

	// Find the first 'x', if any
	for( ; pStringIter < pExtension && *pStringIter != 'x'; ++pStringIter );
	if( pStringIter == pExtension )
	{
		return false;
	}

	SimpleString SizeX = SimpleString( pSizeX, static_cast<uint>( pStringIter - pSizeX ) );
	if( SizeX.BeginsWith( "0" ) )
	{
		// HACKHACK: Parse as decimal
		*SizeX.MutableCStr() = '.';
	}

	pSizeY = pStringIter + 1;

	// Find the second 'x', if any
	for( ++pStringIter; pStringIter < pExtension && *pStringIter != 'x'; ++pStringIter );
	if( pStringIter == pExtension )
	{
		return false;
	}

	SimpleString SizeY = SimpleString( pSizeY, static_cast<uint>( pStringIter - pSizeY ) );
	if( SizeY.BeginsWith( "0" ) )
	{
		// HACKHACK: Parse as decimal
		*SizeY.MutableCStr() = '.';
	}

	pSizeZ = pStringIter + 1;

	// Assume the rest is Z
	SimpleString SizeZ = SimpleString( pSizeZ, static_cast<uint>( pExtension - pSizeZ ) );
	if( SizeZ.BeginsWith( "0" ) )
	{
		// HACKHACK: Parse as decimal
		*SizeZ.MutableCStr() = '.';
	}

	OutTag		= GeoDefTag;
	OutSize.x	= SizeX.AsFloat();
	OutSize.y	= SizeY.AsFloat();
	OutSize.z	= SizeZ.AsFloat();

	return true;
}

uint RosaTools::CreateGeoDef( const SimpleString& GeoDefName )
{
	DEVASSERT( !FindGeoDef( GeoDefName ) );

	MeshFactory* const pMeshFactory = RosaFramework::GetInstance()->GetRenderer()->GetMeshFactory();

	// HACKHACK: If the geodef is a .cbr file, load the config file now
	if( GeoDefName.EndsWith( ".cbr" ) )
	{
		ConfigManager::Load( PackStream( GeoDefName.CStr() ) );

		HashedString	GeoDefTag;
		Vector			GeoDefSize;
		if( ParseGeoDefSize( GeoDefName, GeoDefTag, GeoDefSize ) )
		{
			// Make sure we're not stomping another entry
			DEVASSERT( !m_GeoDefSizeMap.Contains( GeoDefTag ) || !m_GeoDefSizeMap[ GeoDefTag ].Contains( GeoDefSize ) );
			m_GeoDefSizeMap[ GeoDefTag ][ GeoDefSize ] = m_GeoDefs.Size();	// See GeoDefIndex below
		}
	}

	const uint	GeoDefIndex	= m_GeoDefs.Size();
	SGeoDef&	NewGeoDef	= m_GeoDefs.PushBack();
	NewGeoDef.m_DefName		= GeoDefName;

	MAKEHASH( GeoDefName );

	// Add meshes
	{
		STATICHASH( Mesh );
		const SimpleString MeshName = ConfigManager::GetInheritedString( sMesh, "", sGeoDefName );

		STATICHASH( EditorHidden );
		const bool EditorHidden = ConfigManager::GetInheritedBool( sEditorHidden, false, sGeoDefName );

		if( MeshName != "" && !EditorHidden )
		{
			NewGeoDef.m_MeshNames.PushBack( MeshName );
		}

		STATICHASH( NumMeshes );
		const uint NumMeshes = ConfigManager::GetInheritedInt( sNumMeshes, 0, sGeoDefName );
		for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
		{
			const SimpleString	IndexedMeshName		= ConfigManager::GetInheritedSequenceString( "Mesh%d", MeshIndex, "", sGeoDefName );
			const bool			IndexedEditorHidden	= ConfigManager::GetInheritedSequenceBool( "Mesh%dEditorHidden", MeshIndex, false, sGeoDefName );
			if( !IndexedEditorHidden )
			{
				NewGeoDef.m_MeshNames.PushBack( IndexedMeshName );
			}
		}
	}

	// Add hulls
	{
		STATICHASH( Hull );
		const SimpleString	HullName		= ConfigManager::GetInheritedString(	sHull,				"",		sGeoDefName );

		// We don't care about all hull properties in the editor, but this is useful for knowing what things should provide navmesh snap points
		STATICHASH( BlocksEntities );
		const bool			BlocksEntities	= ConfigManager::GetInheritedBool(		sBlocksEntities,	true,	sGeoDefName );

		STATICHASH( NavIgnore );
		const bool			NavIgnore		= ConfigManager::GetInheritedBool(		sNavIgnore,			false,	sGeoDefName );

		if( HullName != "" )
		{
			SConvexHull		NewHull			= CreateHull( HullName, HashedString::NullString, pMeshFactory );

			// We don't care about all hull properties in the editor, but this is useful for knowing what things should provide navmesh snap points
			if( BlocksEntities )	{ NewHull.m_CollisionFlags |= EECF_BlocksEntities;	}
			if( !NavIgnore )		{ NewHull.m_CollisionFlags |= EECF_BlocksNav;	}

			NewGeoDef.m_Hulls.PushBack( NewHull );
		}

		STATICHASH( NumHulls );
		const uint NumHulls = ConfigManager::GetInheritedInt( sNumHulls, 0, sGeoDefName );
		for( uint HullIndex = 0; HullIndex < NumHulls; ++HullIndex )
		{
			const SimpleString	IndexedHullName			= ConfigManager::GetInheritedSequenceString(	"Hull%d",				HullIndex,	"",		sGeoDefName );
			SConvexHull			NewHull					= CreateHull( IndexedHullName, HashedString::NullString, pMeshFactory );

			// We don't care about all hull properties in the editor, but this is useful for knowing what things should provide navmesh snap points
			const bool			IndexedBlocksEntities	= ConfigManager::GetInheritedSequenceBool(		"Hull%dBlocksEntities",	HullIndex,	true,	sGeoDefName );
			if( IndexedBlocksEntities )	{ NewHull.m_CollisionFlags |= EECF_BlocksEntities;	}

			const bool			IndexedNavIgnore		= ConfigManager::GetInheritedSequenceBool(		"Hull%dNavIgnore",		HullIndex,	false,	sGeoDefName );
			if( !IndexedNavIgnore )		{ NewHull.m_CollisionFlags |= EECF_BlocksNav;	}

			NewGeoDef.m_Hulls.PushBack( NewHull );
		}
	}

	// Editor meshes are just used so we can see invisible objects in editor; they are ignored in RoomBaker
	{
		STATICHASH( EditorMesh );
		const SimpleString EditorMeshName = ConfigManager::GetInheritedString( sEditorMesh, "", sGeoDefName );

		if( EditorMeshName != "" )
		{
			NewGeoDef.m_MeshNames.PushBack( EditorMeshName );
		}

		STATICHASH( NumEditorMeshes );
		const uint NumEditorMeshes = ConfigManager::GetInheritedInt( sNumEditorMeshes, 0, sGeoDefName );
		for( uint EditorMeshIndex = 0; EditorMeshIndex < NumEditorMeshes; ++EditorMeshIndex )
		{
			const SimpleString IndexedEditorMeshName = ConfigManager::GetInheritedSequenceString( "EditorMesh%d", EditorMeshIndex, "", sGeoDefName );
			NewGeoDef.m_MeshNames.PushBack( IndexedEditorMeshName );
		}
	}

	// Editor hulls are just used so we can select non-collidable objects in editor; they are ignored in RoomBaker
	{
		STATICHASH( EditorHull );
		const SimpleString EditorHull = ConfigManager::GetInheritedString( sEditorHull, "", sGeoDefName );

		if( EditorHull != "" )
		{
			NewGeoDef.m_Hulls.PushBack( CreateHull( EditorHull, HashedString::NullString, pMeshFactory ) );
		}

		STATICHASH( NumEditorHulls );
		const uint NumEditorHulls = ConfigManager::GetInheritedInt( sNumEditorHulls, 0, sGeoDefName );
		for( uint EditorHullIndex = 0; EditorHullIndex < NumEditorHulls; ++EditorHullIndex )
		{
			const SimpleString IndexedEditorHull = ConfigManager::GetInheritedSequenceString( "EditorHull%d", EditorHullIndex, "", sGeoDefName );
			NewGeoDef.m_Hulls.PushBack( CreateHull( IndexedEditorHull, HashedString::NullString, pMeshFactory ) );
		}
	}

	FOR_EACH_ARRAY( HullIter, NewGeoDef.m_Hulls, SConvexHull )
	{
		const SConvexHull& Hull = HullIter.GetValue();
		if( HullIter.GetIndex() == 0 )
		{
			NewGeoDef.m_Bounds = Hull.m_Bounds;
		}
		else
		{
			NewGeoDef.m_Bounds.ExpandTo( Hull.m_Bounds );
		}
	}

	return GeoDefIndex;
}

void RosaTools::AppendGeoCategory( const SimpleString& Category )
{
	if( Category == "" )
	{
		return;
	}

	// NOTE: This behaves differently than other ::Append functions because
	// I want to add new brushes whenever the tools are reinitialized.
	if( m_GeoElements.m_Categories.Search( Category ).IsNull() )
	{
		m_GeoElements.m_Categories.PushBack( Category );
	}

	const uint CategoryIndex = m_GeoElements.m_Categories.Search( Category ).GetIndex();

	MAKEHASH( Category );

	STATICHASH( GeoDir );
	const SimpleString GeoDir = ConfigManager::GetString( sGeoDir, "", sCategory );
	if( GeoDir != "" )
	{
		Array<SimpleString> GeoDefNames;
		FileUtil::GetFilesInFolder( GeoDir, true, ".cbr", GeoDefNames );

		FOR_EACH_ARRAY( GeoDefNameIter, GeoDefNames, SimpleString )
		{
			const SimpleString& GeoDefName = GeoDefNameIter.GetValue();
			if( FindGeoDef( GeoDefName ) )
			{
				continue;
			}

			const uint GeoDefIndex = CreateGeoDef( GeoDefName );
			m_GeoElements.m_CategoryMap[ GeoDefIndex ] = CategoryIndex;
		}
	}

	STATICHASH( NumGeoDefs );
	const uint NumGeoDefs = ConfigManager::GetInt( sNumGeoDefs, 0, sCategory );

	for( uint GeoDefCategoryIndex = 0; GeoDefCategoryIndex < NumGeoDefs; ++GeoDefCategoryIndex )
	{
		const SimpleString GeoDefName = ConfigManager::GetSequenceString( "GeoDef%d", GeoDefCategoryIndex, "", sCategory );
		if( FindGeoDef( GeoDefName ) )
		{
			continue;
		}

		const uint GeoDefIndex = CreateGeoDef( GeoDefName );
		m_GeoElements.m_CategoryMap[ GeoDefIndex ] = CategoryIndex;
	}
}

uint RosaTools::CreatePrefabDef( const SimpleString& PrefabDefName )
{
	DEVASSERT( !FindPrefabDef( PrefabDefName ) );

	const uint PrefabDefIndex = m_PrefabDefs.Size();
	SPrefabDef& NewPrefabDef = m_PrefabDefs.PushBack();
	NewPrefabDef.m_DefName = PrefabDefName;

	// HACKHACK: If the prefab def is a file, load it as a binary instead of from config
	if( PrefabDefName.EndsWith( ".rosaprefab" ) )
	{
		LoadPrefab( FileStream( PrefabDefName.CStr(), FileStream::EFM_Read ), NewPrefabDef );
	}
	else
	{
		MAKEHASH( PrefabDefName );

		STATICHASH( NumParts );
		const uint NumParts = ConfigManager::GetInheritedInt( sNumParts, 0, sPrefabDefName );
		FOR_EACH_INDEX( PartIndex, NumParts )
		{
			SPrefabPart& NewPrefabPart					= NewPrefabDef.m_Parts.PushBack();
			NewPrefabPart.m_TranslationOffsetLo.x		= ConfigManager::GetInheritedSequenceFloat(	"Part%dX",		PartIndex, 0.0f,	sPrefabDefName );
			NewPrefabPart.m_TranslationOffsetLo.y		= ConfigManager::GetInheritedSequenceFloat(	"Part%dY",		PartIndex, 0.0f,	sPrefabDefName );
			NewPrefabPart.m_TranslationOffsetLo.z		= ConfigManager::GetInheritedSequenceFloat(	"Part%dZ",		PartIndex, 0.0f,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetLo.Pitch	= ConfigManager::GetInheritedSequenceFloat(	"Part%dPitch",	PartIndex, 0.0f,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetLo.Roll	= ConfigManager::GetInheritedSequenceFloat(	"Part%dRoll",	PartIndex, 0.0f,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetLo.Yaw		= ConfigManager::GetInheritedSequenceFloat(	"Part%dYaw",	PartIndex, 0.0f,	sPrefabDefName );

			NewPrefabPart.m_TranslationOffsetLo.x		= ConfigManager::GetInheritedSequenceFloat(	"Part%dXLo",		PartIndex, NewPrefabPart.m_TranslationOffsetLo.x,		sPrefabDefName );
			NewPrefabPart.m_TranslationOffsetLo.y		= ConfigManager::GetInheritedSequenceFloat(	"Part%dYLo",		PartIndex, NewPrefabPart.m_TranslationOffsetLo.y,		sPrefabDefName );
			NewPrefabPart.m_TranslationOffsetLo.z		= ConfigManager::GetInheritedSequenceFloat(	"Part%dZLo",		PartIndex, NewPrefabPart.m_TranslationOffsetLo.z,		sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetLo.Pitch	= ConfigManager::GetInheritedSequenceFloat(	"Part%dPitchLo",	PartIndex, NewPrefabPart.m_OrientationOffsetLo.Pitch,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetLo.Roll	= ConfigManager::GetInheritedSequenceFloat(	"Part%dRollLo",		PartIndex, NewPrefabPart.m_OrientationOffsetLo.Roll,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetLo.Yaw		= ConfigManager::GetInheritedSequenceFloat(	"Part%dYawLo",		PartIndex, NewPrefabPart.m_OrientationOffsetLo.Yaw,		sPrefabDefName );

			NewPrefabPart.m_TranslationOffsetHi.x		= ConfigManager::GetInheritedSequenceFloat(	"Part%dXHi",		PartIndex, NewPrefabPart.m_TranslationOffsetLo.x,		sPrefabDefName );
			NewPrefabPart.m_TranslationOffsetHi.y		= ConfigManager::GetInheritedSequenceFloat(	"Part%dYHi",		PartIndex, NewPrefabPart.m_TranslationOffsetLo.y,		sPrefabDefName );
			NewPrefabPart.m_TranslationOffsetHi.z		= ConfigManager::GetInheritedSequenceFloat(	"Part%dZHi",		PartIndex, NewPrefabPart.m_TranslationOffsetLo.z,		sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetHi.Pitch	= ConfigManager::GetInheritedSequenceFloat(	"Part%dPitchHi",	PartIndex, NewPrefabPart.m_OrientationOffsetLo.Pitch,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetHi.Roll	= ConfigManager::GetInheritedSequenceFloat(	"Part%dRollHi",		PartIndex, NewPrefabPart.m_OrientationOffsetLo.Roll,	sPrefabDefName );
			NewPrefabPart.m_OrientationOffsetHi.Yaw		= ConfigManager::GetInheritedSequenceFloat(	"Part%dYawHi",		PartIndex, NewPrefabPart.m_OrientationOffsetLo.Yaw,		sPrefabDefName );

			NewPrefabPart.m_OrientationOffsetLo.Pitch	= D2R( NewPrefabPart.m_OrientationOffsetLo.Pitch );
			NewPrefabPart.m_OrientationOffsetLo.Roll	= D2R( NewPrefabPart.m_OrientationOffsetLo.Roll );
			NewPrefabPart.m_OrientationOffsetLo.Yaw		= D2R( NewPrefabPart.m_OrientationOffsetLo.Yaw );
			NewPrefabPart.m_OrientationOffsetHi.Pitch	= D2R( NewPrefabPart.m_OrientationOffsetHi.Pitch );
			NewPrefabPart.m_OrientationOffsetHi.Roll	= D2R( NewPrefabPart.m_OrientationOffsetHi.Roll );
			NewPrefabPart.m_OrientationOffsetHi.Yaw		= D2R( NewPrefabPart.m_OrientationOffsetHi.Yaw );

			const SimpleString SpawnerDefName			= ConfigManager::GetInheritedSequenceString(		"Part%dSpawner",	PartIndex, "",		sPrefabDefName );
			const SimpleString GeoDefName				= ConfigManager::GetInheritedSequenceString(		"Part%dGeo",		PartIndex, "",		sPrefabDefName );
			if( SpawnerDefName != "" )
			{
				NewPrefabPart.m_Type			= EBT_Spawner;
				NewPrefabPart.m_DefIndex		= FindOrCreateSpawnerDef( SpawnerDefName );
			}
			else if( GeoDefName != "" )
			{
				NewPrefabPart.m_Type			= EBT_Geo;
				NewPrefabPart.m_DefIndex		= FindOrCreateGeoDef( GeoDefName );
			}
			else
			{
				WARN;
			}
		}
	}

	return PrefabDefIndex;
}

void RosaTools::AppendPrefabCategory( const SimpleString& Category )
{
	if( Category == "" )
	{
		return;
	}

	// NOTE: This behaves differently than other ::Append functions because
	// I want to add new prefabs whenever the tools are reinitialized.
	if( m_PrefabElements.m_Categories.Search( Category ).IsNull() )
	{
		m_PrefabElements.m_Categories.PushBack( Category );
	}

	const uint CategoryIndex = m_PrefabElements.m_Categories.Search( Category ).GetIndex();

	MAKEHASH( Category );

	STATICHASH( PrefabsDir );
	const SimpleString PrefabsDir = ConfigManager::GetString( sPrefabsDir, "", sCategory );
	if( PrefabsDir != "" )
	{
		Array<SimpleString> PrefabDefNames;
		FileUtil::GetFilesInFolder( PrefabsDir, true, ".rosaprefab", PrefabDefNames );

		FOR_EACH_ARRAY( PrefabDefNameIter, PrefabDefNames, SimpleString )
		{
			const SimpleString& PrefabDefName = PrefabDefNameIter.GetValue();
			if( FindPrefabDef( PrefabDefName ) )
			{
				continue;
			}

			const uint PrefabDefIndex = CreatePrefabDef( PrefabDefName );
			m_PrefabElements.m_CategoryMap[ PrefabDefIndex ] = CategoryIndex;
		}
	}

	STATICHASH( NumPrefabDefs );
	const uint NumPrefabDefs = ConfigManager::GetInt( sNumPrefabDefs, 0, sCategory );

	for( uint PrefabDefCategoryIndex = 0; PrefabDefCategoryIndex < NumPrefabDefs; ++PrefabDefCategoryIndex )
	{
		const SimpleString PrefabDefName = ConfigManager::GetSequenceString( "PrefabDef%d", PrefabDefCategoryIndex, "", sCategory );
		if( FindPrefabDef( PrefabDefName ) )
		{
			continue;
		}

		const uint PrefabDefIndex = CreatePrefabDef( PrefabDefName );
		m_PrefabElements.m_CategoryMap[ PrefabDefIndex ] = CategoryIndex;
	}
}

void RosaTools::AppendPortalCategory( const SimpleString& Category )
{
	if( Category == "" )
	{
		return;
	}

	if( m_PortalElements.m_Categories.Search( Category ).IsValid() )
	{
		return;
	}

	const uint CategoryIndex = m_PortalElements.m_Categories.Size();
	m_PortalElements.m_Categories.PushBack( Category );

	MAKEHASH( Category );

	STATICHASH( NumPortalDefs );
	const uint NumPortalDefs = ConfigManager::GetInt( sNumPortalDefs, 0, sCategory );

	for( uint PortalDefCategoryIndex = 0; PortalDefCategoryIndex < NumPortalDefs; ++PortalDefCategoryIndex )
	{
		const SimpleString PortalDefName = ConfigManager::GetSequenceString( "PortalDef%d", PortalDefCategoryIndex, "", sCategory );
		if( FindPortalDef( PortalDefName ) )
		{
			continue;
		}

		const uint PortalDefIndex = m_PortalDefs.Size();

		SPortalDef& NewPortalDef = m_PortalDefs.PushBack();
		NewPortalDef.m_DefName = PortalDefName;
		NewPortalDef.m_Portal.InitializeFromDefinition( PortalDefName );

		m_PortalElements.m_CategoryMap[ PortalDefIndex ] = CategoryIndex;
	}
}

void RosaTools::AppendMatCategory( const SimpleString& Category )
{
	if( Category == "" )
	{
		return;
	}

	if( m_MatElements.m_Categories.Search( Category ).IsValid() )
	{
		return;
	}

	const uint CategoryIndex = m_MatElements.m_Categories.Size();
	m_MatElements.m_Categories.PushBack( Category );

	MAKEHASH( Category );

	STATICHASH( NumMatDefs );
	const uint NumMatDefs = ConfigManager::GetInt( sNumMatDefs, 0, sCategory );

	for( uint MatDefCategoryIndex = 0; MatDefCategoryIndex < NumMatDefs; ++MatDefCategoryIndex )
	{
		const SimpleString MatDefName = ConfigManager::GetSequenceString( "MatDef%d", MatDefCategoryIndex, "", sCategory );
		if( FindMatDef( MatDefName ) )
		{
			continue;
		}

		const uint MatDefIndex = m_MatDefs.Size();

		SMatDef& NewMatDef = m_MatDefs.PushBack();
		NewMatDef.m_DefName = MatDefName;

		MAKEHASH( MatDefName );

		STATICHASH( Albedo );
		NewMatDef.m_Albedo = ConfigManager::GetInheritedString( sAlbedo, "", sMatDefName );

		STATICHASH( Overlay );
		NewMatDef.m_Overlay = ConfigManager::GetInheritedString( sOverlay, "", sMatDefName );

		m_MatElements.m_CategoryMap[ MatDefIndex ] = CategoryIndex;
	}
}

void RosaTools::ClearBrushes()
{
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		SBrush& Brush = BrushIter.GetValue();
		FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
		{
			SBrushMesh& BrushMesh = BrushMeshIter.GetValue();
			SafeDelete( BrushMesh.m_Mesh );
		}
	}
	m_Brushes.Clear();
	m_SelectedBrushes.Clear();
}

void RosaTools::ResetPortals()
{
	ASSERT( m_PortalDefs.Size() > 0 );

	m_Portals.Clear();
	const uint NumPortals = m_TilesX * m_TilesY * m_TilesZ;
	for( uint PortalIndex = 0; PortalIndex < NumPortals; ++PortalIndex )
	{
		SPortals& Portals = m_Portals.PushBack();
		for( uint Index = 0; Index < 6; ++Index )
		{
			Portals.m_Portals[ Index ] = 0;
		}
	}

	// Also reset bound planes
	m_BoundPlanes.Clear();
	m_BoundPlanes.PushBack( Plane( Vector( 1.0f, 0.0f, 0.0f ),	Vector( 0.0f, 0.0f, 0.0f ) ) );
	m_BoundPlanes.PushBack( Plane( Vector( -1.0f, 0.0f, 0.0f ),	Vector( static_cast<float>( m_MetersX ), 0.0f, 0.0f ) ) );
	m_BoundPlanes.PushBack( Plane( Vector( 0.0f, 1.0f, 0.0f ),	Vector( 0.0f, 0.0f, 0.0f ) ) );
	m_BoundPlanes.PushBack( Plane( Vector( 0.0f, -1.0f, 0.0f ),	Vector( 0.0f, static_cast<float>( m_MetersY ), 0.0f ) ) );
	m_BoundPlanes.PushBack( Plane( Vector( 0.0f, 0.0f, 1.0f ),	Vector( 0.0f, 0.0f, 0.0f ) ) );
	m_BoundPlanes.PushBack( Plane( Vector( 0.0f, 0.0f, -1.0f ),	Vector( 0.0f, 0.0f, static_cast<float>( m_MetersZ ) ) ) );
}

void RosaTools::FixUpPortalsAfterResize( const uint OldTilesX, const uint OldTilesY, const uint OldTilesZ )
{
	ASSERT( m_PortalDefs.Size() > 0 );

	// Make a copy to reference
	Array<SPortals> OldPortals = m_Portals;

	ResetPortals();

	// Fix up, after either expanding or contracting the size.
	// Just copy what's there; don't bother trying to be clever about filling new
	// spaces, because it won't adapt well in all cases.

	for( uint TileX = 0; TileX < m_TilesX; ++TileX )
	{
		for( uint TileY = 0; TileY < m_TilesY; ++TileY )
		{
			if( TileX >= OldTilesX || TileY >= OldTilesY )
			{
				continue;
			}

			{
				const uint TileZ = 0;
				const uint OldTileIndex = TileX + TileY * OldTilesX + TileZ * OldTilesX * OldTilesY;
				const uint NewTileIndex = TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
				m_Portals[ NewTileIndex ].m_Portals[ EPI_Down ] = OldPortals[ OldTileIndex ].m_Portals[ EPI_Down ];
			}

			if( m_TilesZ == OldTilesZ )
			{
				const uint TileZ = m_TilesZ - 1;
				const uint OldTileIndex = TileX + TileY * OldTilesX + TileZ * OldTilesX * OldTilesY;
				const uint NewTileIndex = TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
				m_Portals[ NewTileIndex ].m_Portals[ EPI_Up ] = OldPortals[ OldTileIndex ].m_Portals[ EPI_Up ];
			}
		}
	}

	for( uint TileX = 0; TileX < m_TilesX; ++TileX )
	{
		for( uint TileZ = 0; TileZ < m_TilesZ; ++TileZ )
		{
			if( TileX >= OldTilesX || TileZ >= OldTilesZ )
			{
				continue;
			}

			{
				const uint TileY = 0;
				const uint OldTileIndex = TileX + TileY * OldTilesX + TileZ * OldTilesX * OldTilesY;
				const uint NewTileIndex = TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
				m_Portals[ NewTileIndex ].m_Portals[ EPI_South ] = OldPortals[ OldTileIndex ].m_Portals[ EPI_South ];
			}

			if( m_TilesY == OldTilesY )
			{
				const uint TileY = m_TilesY - 1;
				const uint OldTileIndex = TileX + TileY * OldTilesX + TileZ * OldTilesX * OldTilesY;
				const uint NewTileIndex = TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
				m_Portals[ NewTileIndex ].m_Portals[ EPI_North ] = OldPortals[ OldTileIndex ].m_Portals[ EPI_North ];
			}
		}
	}

	for( uint TileY = 0; TileY < m_TilesY; ++TileY )
	{
		for( uint TileZ = 0; TileZ < m_TilesZ; ++TileZ )
		{
			if( TileY >= OldTilesY || TileZ >= OldTilesZ )
			{
				continue;
			}

			{
				const uint TileX = 0;
				const uint OldTileIndex = TileX + TileY * OldTilesX + TileZ * OldTilesX * OldTilesY;
				const uint NewTileIndex = TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
				m_Portals[ NewTileIndex ].m_Portals[ EPI_West ] = OldPortals[ OldTileIndex ].m_Portals[ EPI_West ];
			}

			if( m_TilesX == OldTilesX )
			{
				const uint TileX = m_TilesX - 1;
				const uint OldTileIndex = TileX + TileY * OldTilesX + TileZ * OldTilesX * OldTilesY;
				const uint NewTileIndex = TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
				m_Portals[ NewTileIndex ].m_Portals[ EPI_East ] = OldPortals[ OldTileIndex ].m_Portals[ EPI_East ];
			}
		}
	}
}

bool RosaTools::FindSpawnerDef( const SimpleString& SearchDef, uint* const pOutIndex /*= NULL*/ ) const
{
	FOR_EACH_ARRAY( SpawnerDefIter, m_SpawnerDefs, SSpawnerDef )
	{
		const SSpawnerDef& SpawnerDef = SpawnerDefIter.GetValue();
		if( SpawnerDef.m_DefName == SearchDef )
		{
			if( pOutIndex )
			{
				*pOutIndex = SpawnerDefIter.GetIndex();
			}

			return true;
		}
	}

	return false;
}

bool RosaTools::FindRoomDef( const SimpleString& SearchDef, uint* const pOutIndex /*= NULL*/ ) const
{
	FOR_EACH_ARRAY( RoomDefIter, m_RoomDefs, SRoomDef )
	{
		const SRoomDef& RoomDef = RoomDefIter.GetValue();
		if( RoomDef.m_DefName == SearchDef )
		{
			if( pOutIndex )
			{
				*pOutIndex = RoomDefIter.GetIndex();
			}

			return true;
		}
	}

	return false;
}

bool RosaTools::FindGeoDef( const SimpleString& SearchDef, uint* const pOutIndex /*= NULL*/ ) const
{
	FOR_EACH_ARRAY( GeoDefIter, m_GeoDefs, SGeoDef )
	{
		const SGeoDef& GeoDef = GeoDefIter.GetValue();
		if( GeoDef.m_DefName == SearchDef )
		{
			if( pOutIndex )
			{
				*pOutIndex = GeoDefIter.GetIndex();
			}

			return true;
		}
	}

	return false;
}

bool RosaTools::FindPrefabDef( const SimpleString& SearchDef, uint* const pOutIndex /*= NULL*/ ) const
{
	FOR_EACH_ARRAY( PrefabDefIter, m_PrefabDefs, SPrefabDef )
	{
		const SPrefabDef& PrefabDef = PrefabDefIter.GetValue();
		if( PrefabDef.m_DefName == SearchDef )
		{
			if( pOutIndex )
			{
				*pOutIndex = PrefabDefIter.GetIndex();
			}

			return true;
		}
	}

	return false;
}

bool RosaTools::FindPortalDef( const SimpleString& SearchDef, uint* const pOutIndex /*= NULL*/ ) const
{
	FOR_EACH_ARRAY( PortalDefIter, m_PortalDefs, SPortalDef )
	{
		const SPortalDef& PortalDef = PortalDefIter.GetValue();
		if( PortalDef.m_DefName == SearchDef )
		{
			if( pOutIndex )
			{
				*pOutIndex = PortalDefIter.GetIndex();
			}

			return true;
		}
	}

	return false;
}

bool RosaTools::FindMatDef( const HashedString& SearchDef, uint* const pOutIndex /*= NULL*/ ) const
{
	FOR_EACH_ARRAY( MatDefIter, m_MatDefs, SMatDef )
	{
		const SMatDef& MatDef = MatDefIter.GetValue();
		if( HashedString( MatDef.m_DefName ) == SearchDef )
		{
			if( pOutIndex )
			{
				*pOutIndex = MatDefIter.GetIndex();
			}

			return true;
		}
	}

	return false;
}

RosaTools::SBrush& RosaTools::CreateBrush( const EBrushType Type, const uint DefIndex, const Vector& Location, const Angles& Orientation, const float Scale, const Array<uint>& LinkedBrushes )
{
	// Prefabs must be spawned with CreateBrushes
	DEVASSERT( Type != EBT_Prefab );

	IRenderer* const	pRenderer	= RosaFramework::GetInstance()->GetRenderer();
	RosaWorld* const	pWorld		= RosaFramework::GetInstance()->GetWorld();

	SBrush&				NewBrush	= m_Brushes.PushBack();
	NewBrush.m_Type					= Type;
	NewBrush.m_Location				= Location;
	NewBrush.m_Orientation			= Orientation;
	NewBrush.m_Scale				= Scale;
	NewBrush.m_DefIndex				= DefIndex;
	NewBrush.m_LinkedBrushes		= LinkedBrushes;

	if( Type == EBT_Geo )
	{
		const SGeoDef&	GeoDef		= m_GeoDefs[ DefIndex ];
		FOR_EACH_ARRAY( MeshIter, GeoDef.m_MeshNames, SimpleString )
		{
			const SimpleString&	MeshName	= MeshIter.GetValue();

			// Use the save buffers callback so we don't lose track of these meshes for the game when they're cached
			MeshFactory::SReadMeshCallback Callback;
			Callback.m_Callback		= &RosaWorld::ReadMeshCallback_SaveBuffers;
			Callback.m_Void			= pWorld;

			// Push this before accessing the mesh, it's used in the callback
			pWorld->m_GeoMeshNames.PushBack( MeshName );

			SBrushMesh&			NewBrushMesh	= NewBrush.m_Meshes.PushBack();
			NewBrushMesh.m_Mesh					= new Mesh;
			pRenderer->GetMeshFactory()->GetDynamicMesh( MeshName.CStr(), NewBrushMesh.m_Mesh, Callback );

			// And then pop because we're not merging meshes here
			pWorld->m_GeoMeshNames.PopBack();
		}
	}
	else if( Type == EBT_Spawner )
	{
		const SSpawnerDef&	SpawnerDef		= m_SpawnerDefs[ DefIndex ];

		SBrushMesh&			NewBrushMesh	= NewBrush.m_Meshes.PushBack();
		NewBrushMesh.m_Mesh					= new Mesh;
		pRenderer->GetMeshFactory()->GetDynamicMesh( SpawnerDef.m_MeshName.CStr(), NewBrushMesh.m_Mesh );

		NewBrushMesh.m_Offset				= SpawnerDef.m_Offset + SpawnerDef.m_MeshOffset;
	}
	else if( Type == EBT_Room )
	{
		const SRoomDef&	RoomDef	= m_RoomDefs[ DefIndex ];

		// Open the world file to pull data from it
		RosaRoomEditor TempRoom;
		TempRoom.Load( FileStream( RoomDef.m_DefName.CStr(), FileStream::EFM_Read ) );

		const float RoomScaleXY = 1.0f / ( static_cast<float>( m_MetersX ) / static_cast<float>( m_TilesX ) );
		const float RoomScaleZ = 1.0f / ( static_cast<float>( m_MetersZ ) / static_cast<float>( m_TilesZ ) );
		const Vector RoomScale = Vector( RoomScaleXY, RoomScaleXY, RoomScaleZ );

		bool HasSetHullsBound = false;
		AABB HullsBound;
		bool HasGroundMatDefIndex = false;
		uint GroundMatDefIndex = 0;
		float GroundGeoZ = FLT_MAX;
		Array<Vector> Positions;
		Array<Vector2> UVs;
		Array<Vector> Normals;
		Array<index_t> Indices;
		FOR_EACH_ARRAY( RoomBrushIter, TempRoom.m_Brushes, RosaRoomEditor::SBrush )
		{
			const RosaRoomEditor::SBrush& RoomBrush = RoomBrushIter.GetValue();
			if( RoomBrush.m_Type == EBT_Geo )
			{
				if( RoomBrush.m_Location.z < GroundGeoZ )
				{
					if( FindMatDef( RoomBrush.m_Mat, &GroundMatDefIndex ) )
					{
						HasGroundMatDefIndex = true;
						GroundGeoZ = RoomBrush.m_Location.z;
					}
				}

				uint RoomBrushGeoDefIndex = 0;
				if( FindGeoDef( RoomBrush.m_DefName, &RoomBrushGeoDefIndex ) )
				{
					const SGeoDef&	GeoDef	= m_GeoDefs[ RoomBrushGeoDefIndex ];
					FOR_EACH_ARRAY( HullIter, GeoDef.m_Hulls, SConvexHull )
					{
						const SConvexHull& Hull = HullIter.GetValue();
						const AABB TransformedBounds = Hull.m_Bounds.GetTransformedBound( RoomBrush.m_Location, RoomBrush.m_Orientation, Vector( RoomBrush.m_Scale, RoomBrush.m_Scale, RoomBrush.m_Scale ) );
						const Vector MeshBase = ( RoomScale * TransformedBounds.m_Min ) - Vector( 0.5f, 0.5f, 0.5f );	// Offset by 1/2 so origin is in center of low tile
						const Vector MeshDims = RoomScale * ( TransformedBounds.m_Max - TransformedBounds.m_Min );
						AppendHullMesh( MeshBase, MeshDims, Positions, UVs, Normals, Indices );

						if( HasSetHullsBound )
						{
							HullsBound.ExpandTo( MeshBase );
							HullsBound.ExpandTo( MeshBase + MeshDims );
						}
						else
						{
							HasSetHullsBound = true;
							HullsBound.m_Min = MeshBase;
							HullsBound.m_Max = MeshBase + MeshDims;
						}
					}
				}
			}
		}
		Mesh* const	pHullsMesh			= CreateHullsMesh( Positions, UVs, Normals, Indices );
		pHullsMesh->SetAABB( HullsBound );

		SBrushMesh&	HullsBrushMesh		= NewBrush.m_Meshes.PushBack();
		HullsBrushMesh.m_Mesh			= pHullsMesh;

		const Vector	HullsExtents	= HullsBound.GetExtents();
		Mesh* const		pGroundMesh		= pRenderer->GetMeshFactory()->CreatePlane( 2.0f * HullsExtents.x, 2.0f * HullsExtents.y, 1, 1, XY_PLANE, false );
		const char*		pGroundAlbedo	= HasGroundMatDefIndex ? m_MatDefs[ GroundMatDefIndex ].m_Albedo.CStr() : DEFAULT_TEXTURE;
		pGroundMesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( pGroundAlbedo ) );
		pGroundMesh->SetAABB( AABB::CreateFromCenterAndExtents( Vector( 0.0f, 0.0f, HullsExtents.z ), HullsExtents ) );

		SBrushMesh&	GroundBrushMesh		= NewBrush.m_Meshes.PushBack();
		GroundBrushMesh.m_Mesh			= pGroundMesh;
		GroundBrushMesh.m_Offset		= Vector( HullsExtents.x - 0.5f, HullsExtents.y - 0.5f, -0.5f );
	}

	FOR_EACH_ARRAY( BrushMeshIter, NewBrush.m_Meshes, SBrushMesh )
	{
		SBrushMesh&		BrushMesh		= BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->SetMaterialDefinition( "Material_ToolsBrush", pRenderer );
		BrushMesh.m_Mesh->SetTexture( 1, pRenderer->GetTextureManager()->GetTexture( DEFAULT_TEXTURE ) );	// Stomp the normal map with a default overlay
		BrushMesh.m_Mesh->SetBucket( "DevToolsFwd" );
		BrushMesh.m_Mesh->m_Location	= Location + ( ( BrushMesh.m_Offset * Scale ) * Orientation.ToMatrix() );
		BrushMesh.m_Mesh->m_Rotation	= Orientation;
		BrushMesh.m_Mesh->m_Scale		= Vector( Scale, Scale, Scale );
		BrushMesh.m_Mesh->RecomputeAABB();
	}

	return NewBrush;
}

void RosaTools::AppendHullMesh( const Vector& Base, const Vector& Dims, Array<Vector>& InOutPositions, Array<Vector2>& InOutUVs, Array<Vector>& InOutNormals, Array<index_t>& InOutIndices ) const
{
	// If this fails, I'll need to split into multiple meshes.
	// That would be 2730 hulls in a single room, hopefully unlikely?
	ASSERT( InOutPositions.Size() + 24 < 65536 );

	const index_t	IndexBase	= static_cast<index_t>( InOutPositions.Size() );

	const Vector	DimsX		= Vector( Dims.x, 0.0f, 0.0f );
	const Vector	DimsY		= Vector( 0.0f, Dims.y, 0.0f );
	const Vector	DimsZ		= Vector( 0.0f, 0.0f, Dims.z );

	const Vector	X			= Vector( 1.0f, 0.0f, 0.0f );
	const Vector	Y			= Vector( 0.0f, 1.0f, 0.0f );
	const Vector	Z			= Vector( 0.0f, 0.0f, 1.0f );

	Array<Vector>	Positions;
	Positions.Resize( 24 );
	Positions[0]	= Base + DimsX;			Positions[1]	= Base + DimsX + DimsY;	Positions[2]	= Base + DimsX + DimsZ;			Positions[3]	= Base + DimsX + DimsY + DimsZ;	// X+
	Positions[4]	= Base + DimsY;			Positions[5]	= Base;					Positions[6]	= Base + DimsY + DimsZ;			Positions[7]	= Base + DimsZ;					// X-
	Positions[8]	= Base + DimsX + DimsY;	Positions[9]	= Base + DimsY;			Positions[10]	= Base + DimsX + DimsY + DimsZ;	Positions[11]	= Base + DimsY + DimsZ;			// Y+
	Positions[12]	= Base;					Positions[13]	= Base + DimsX;			Positions[14]	= Base + DimsZ;					Positions[15]	= Base + DimsX + DimsZ;			// Y-
	Positions[16]	= Base + DimsZ;			Positions[17]	= Base + DimsX + DimsZ;	Positions[18]	= Base + DimsY + DimsZ;			Positions[19]	= Base + DimsX + DimsY + DimsZ;	// Z+
	Positions[20]	= Base + DimsY;			Positions[21]	= Base + DimsX + DimsY;	Positions[22]	= Base;							Positions[23]	= Base + DimsX;					// Z-

	// Hulls aren't textured so UVs don't matter
	Array<Vector2>	UVs;
	UVs.ResizeZero( 24 );

	Array<Vector>	Normals;
	Normals.Resize( 24 );
	Normals[0]	= X;	Normals[1]	= X;	Normals[2]	= X;	Normals[3]	= X;
	Normals[4]	= -X;	Normals[5]	= -X;	Normals[6]	= -X;	Normals[7]	= -X;
	Normals[8]	= Y;	Normals[9]	= Y;	Normals[10]	= Y;	Normals[11]	= Y;
	Normals[12]	= -Y;	Normals[13]	= -Y;	Normals[14]	= -Y;	Normals[15]	= -Y;
	Normals[16]	= Z;	Normals[17]	= Z;	Normals[18]	= Z;	Normals[19]	= Z;
	Normals[20]	= -Z;	Normals[21]	= -Z;	Normals[22]	= -Z;	Normals[23]	= -Z;

#if 0
	// For triangle list
	Array<index_t>	Indices;
	Indices.Resize( 36 );
	Indices[0]	= IndexBase + 0;	Indices[1]	= IndexBase + 1;	Indices[2]	= IndexBase + 2;	Indices[3]	= IndexBase + 1;	Indices[4]	= IndexBase + 3;	Indices[5]	= IndexBase + 2;
	Indices[6]	= IndexBase + 4;	Indices[7]	= IndexBase + 5;	Indices[8]	= IndexBase + 6;	Indices[9]	= IndexBase + 5;	Indices[10]	= IndexBase + 7;	Indices[11]	= IndexBase + 6;
	Indices[12]	= IndexBase + 8;	Indices[13]	= IndexBase + 9;	Indices[14]	= IndexBase + 10;	Indices[15]	= IndexBase + 9;	Indices[16]	= IndexBase + 11;	Indices[17]	= IndexBase + 10;
	Indices[18]	= IndexBase + 12;	Indices[19]	= IndexBase + 13;	Indices[20]	= IndexBase + 14;	Indices[21]	= IndexBase + 13;	Indices[22]	= IndexBase + 15;	Indices[23]	= IndexBase + 14;
	Indices[24]	= IndexBase + 16;	Indices[25]	= IndexBase + 17;	Indices[26]	= IndexBase + 18;	Indices[27]	= IndexBase + 17;	Indices[28]	= IndexBase + 19;	Indices[29]	= IndexBase + 18;
	Indices[30]	= IndexBase + 20;	Indices[31]	= IndexBase + 21;	Indices[32]	= IndexBase + 22;	Indices[33]	= IndexBase + 21;	Indices[34]	= IndexBase + 23;	Indices[35]	= IndexBase + 22;
#elif 0
	// For basic line list
	Array<index_t>	Indices;
	Indices.Resize( 36 );
	Indices[0]	= IndexBase + 0;	Indices[1]	= IndexBase + 1;	Indices[2]	= IndexBase + 1;	Indices[3]	= IndexBase + 3;
	Indices[4]	= IndexBase + 4;	Indices[5]	= IndexBase + 5;	Indices[6]	= IndexBase + 5;	Indices[7]	= IndexBase + 7;
	Indices[8]	= IndexBase + 8;	Indices[9]	= IndexBase + 9;	Indices[10]	= IndexBase + 9;	Indices[11]	= IndexBase + 11;
	Indices[12]	= IndexBase + 12;	Indices[13]	= IndexBase + 13;	Indices[14]	= IndexBase + 13;	Indices[15]	= IndexBase + 15;
	Indices[16]	= IndexBase + 16;	Indices[17]	= IndexBase + 17;	Indices[18]	= IndexBase + 17;	Indices[19]	= IndexBase + 19;
	Indices[20]	= IndexBase + 19;	Indices[21]	= IndexBase + 18;	Indices[22]	= IndexBase + 18;	Indices[23]	= IndexBase + 16;	// These indexes look weird but it's because we've already drawn the whole bottom
#else
	// For crossed line list
	Array<index_t>	Indices;
	Indices.Resize( 36 );
	Indices[0]	= IndexBase + 0;	Indices[1]	= IndexBase + 1;	Indices[2]	= IndexBase + 1;	Indices[3]	= IndexBase + 3;	Indices[4]	= IndexBase + 3;	Indices[5]	= IndexBase + 0;
	Indices[6]	= IndexBase + 4;	Indices[7]	= IndexBase + 5;	Indices[8]	= IndexBase + 5;	Indices[9]	= IndexBase + 7;	Indices[10]	= IndexBase + 7;	Indices[11]	= IndexBase + 4;
	Indices[12]	= IndexBase + 8;	Indices[13]	= IndexBase + 9;	Indices[14]	= IndexBase + 9;	Indices[15]	= IndexBase + 11;	Indices[16]	= IndexBase + 11;	Indices[17]	= IndexBase + 8;
	Indices[18]	= IndexBase + 12;	Indices[19]	= IndexBase + 13;	Indices[20]	= IndexBase + 13;	Indices[21]	= IndexBase + 15;	Indices[22]	= IndexBase + 15;	Indices[23]	= IndexBase + 12;
	Indices[24]	= IndexBase + 16;	Indices[25]	= IndexBase + 17;	Indices[26]	= IndexBase + 17;	Indices[27]	= IndexBase + 19;	Indices[28]	= IndexBase + 19;	Indices[29]	= IndexBase + 16;
	Indices[30]	= IndexBase + 19;	Indices[31]	= IndexBase + 18;	Indices[32]	= IndexBase + 18;	Indices[33]	= IndexBase + 16;	Indices[34]	= IndexBase + 23;	Indices[35]	= IndexBase + 20;
#endif

	InOutPositions.Append( Positions );
	InOutUVs.Append( UVs );
	InOutNormals.Append( Normals );
	InOutIndices.Append( Indices );
}

Mesh* RosaTools::CreateHullsMesh( const Array<Vector>& Positions, const Array<Vector2>& UVs, const Array<Vector>& Normals, const Array<index_t>& Indices ) const
{
	IRenderer* const			pRenderer			= RosaFramework::GetInstance()->GetRenderer();
	IVertexBuffer* const		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration* const	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS | VD_NORMALS );
	IIndexBuffer* const			pIndexBuffer		= pRenderer->CreateIndexBuffer();

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= Positions.Size();
	InitStruct.Positions	= Positions.GetData();
	InitStruct.UVs			= UVs.GetData();
	InitStruct.Normals		= Normals.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( Indices.Size(), Indices.GetData() );
	pIndexBuffer->SetPrimitiveType( EPT_LINELIST );
	Mesh* const pHullsMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	pHullsMesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( DEFAULT_TEXTURE ) );

#if BUILD_DEBUG
	pHullsMesh->m_DEBUG_Name = "RosaTools HullsMesh";
#endif

	return pHullsMesh;
}

void RosaTools::CreateBrushes( const EBrushType Type, const uint DefIndex, const Vector& Location, const Angles& Orientation, const float Scale, Array<uint>& OutBrushes )
{
	if( Type == EBT_Prefab )
	{
		// Create sub-brushes, applying appropriate offsets relative to Orientation
		const Matrix		BrushOrientationMatrix	= Orientation.ToMatrix();
		const SPrefabDef&	PrefabDef				= m_PrefabDefs[ DefIndex ];
		FOR_EACH_ARRAY( PartIter, PrefabDef.m_Parts, SPrefabPart )
		{
			const SPrefabPart&	Part					= PartIter.GetValue();
			const Vector		RandomTranslation		= Math::Random( Part.m_TranslationOffsetLo, Part.m_TranslationOffsetHi );
			const Angles		RandomOrientation		= Math::Random( Part.m_OrientationOffsetLo, Part.m_OrientationOffsetHi );
			const Matrix		PartOrientationMatrix	= RandomOrientation.ToMatrix();
			const Vector		PartLocation			= Location + ( ( RandomTranslation * BrushOrientationMatrix ) * Scale );
			const Angles		PartOrientation			= ( PartOrientationMatrix * BrushOrientationMatrix ).ToAngles();	// Not sure about the order here
			const float			PartScale				= Scale;
			SBrush&				NewBrush				= CreateBrush( Part.m_Type, Part.m_DefIndex, PartLocation, PartOrientation, PartScale, Array<uint>() );
			OutBrushes.PushBack( GetIndexOfBrush( NewBrush ) );

			uint MatDefIndex;
			if( FindMatDef( Part.m_Mat, &MatDefIndex ) )
			{
				ApplyMatToBrush( MatDefIndex, NewBrush );
			}
		}
	}
	else
	{
		OutBrushes.PushBack( GetIndexOfBrush( CreateBrush( Type, DefIndex, Location, Orientation, Scale, Array<uint>() ) ) );
	}
}

void RosaTools::Reinitialize()
{
	ShutDown();

	SimpleString& CurrentMapName = IsInRoomMode() ? m_CurrentRoomName : m_CurrentWorldName;
	CurrentMapName = "";

	m_Framework = RosaFramework::GetInstance();
	ASSERT( m_Framework );

	if( !m_LoadingUndoState )
	{
		m_UndoStates.Clear();
		m_UndoStateIndex = -1;
		m_SavedUndoStateIndex = -1;
	}

	m_TilesX	= 1;
	m_TilesY	= 1;
	m_TilesZ	= 1;

	// ROSATODO: Configurate
	m_MetersX	= 12;
	m_MetersY	= 12;
	m_MetersZ	= 8;

	ResetPortals();
	m_NavSnapPoints.Clear();

	if( !m_LoadingUndoState )
	{
		m_CameraLocation.x = static_cast<float>( m_MetersX ) * 0.5f;
		m_CameraLocation.y = static_cast<float>( m_MetersY ) * 0.25f;
		m_CameraLocation.z = static_cast<float>( m_MetersZ ) * 0.75f;
		m_CameraOrientation = Angles();
	}

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	RosaGame* const			pGame		= pFramework->GetGame();
	const SimpleString		LevelName	= pGame->GetCurrentLevelName();

	AppendCategories( LevelName );

	InitializeGridMeshes();
	InitializePortalMesh();
	InitializeHelpMesh();

	Array<SimpleString> RoomDefNames;
	FOR_EACH_ARRAY( RoomDefIter, m_RoomDefs, SRoomDef ) { RoomDefNames.PushBack( RoomDefIter.GetValue().m_DefName ); }
	InitializeElementButtonMeshes( m_RoomElements, RoomDefNames );

	Array<SimpleString> SpawnerDefNames;
	FOR_EACH_ARRAY( SpawnerDefIter, m_SpawnerDefs, SSpawnerDef ) { SpawnerDefNames.PushBack( SpawnerDefIter.GetValue().m_DefName ); }
	InitializeElementButtonMeshes( m_SpawnerElements, SpawnerDefNames );

	Array<SimpleString> GeoDefNames;
	FOR_EACH_ARRAY( GeoDefIter, m_GeoDefs, SGeoDef ) { GeoDefNames.PushBack( GeoDefIter.GetValue().m_DefName ); }
	InitializeElementButtonMeshes( m_GeoElements, GeoDefNames );

	Array<SimpleString> PrefabDefNames;
	FOR_EACH_ARRAY( PrefabDefIter, m_PrefabDefs, SPrefabDef ) { PrefabDefNames.PushBack( PrefabDefIter.GetValue().m_DefName ); }
	InitializeElementButtonMeshes( m_PrefabElements, PrefabDefNames );

	Array<SimpleString> PortalDefNames;
	FOR_EACH_ARRAY( PortalDefIter, m_PortalDefs, SPortalDef ) { PortalDefNames.PushBack( PortalDefIter.GetValue().m_DefName ); }
	InitializeElementButtonMeshes( m_PortalElements, PortalDefNames );

	Array<SimpleString> MatDefNames;
	FOR_EACH_ARRAY( MatDefIter, m_MatDefs, SMatDef ) { MatDefNames.PushBack( MatDefIter.GetValue().m_DefName ); }
	InitializeElementButtonMeshes( m_MatElements, MatDefNames );

	Clear();
}

void RosaTools::InitializeHelpMesh()
{
	// This is (for the moment) easier than using the UI stack for tools work.
	IRenderer* const pRenderer = m_Framework->GetRenderer();

	const SimpleString CurrentMode = IsInRoomMode() ? "ROOM MODE (F1 for world mode)" : "WORLD MODE (F1 for room mode)";
	const SimpleString SubModes = IsInRoomMode() ? "F2: Spawners\tF3: Brushes\tF4: Portals\tF5: Navmesh\tF8: Materials\tF10: Prefabs" : "F3: Rooms\tF10: Prefabs";

	SafeDelete( m_HelpMesh );
	const uint MetersPerTileX = m_MetersX / m_TilesX;
	const uint MetersPerTileY = m_MetersY / m_TilesY;
	const uint MetersPerTileZ = m_MetersZ / m_TilesZ;
	const SimpleString HelpText =
		SimpleString::PrintF(
			"%s\n"
			"%s\n"
			"\n"
			"F6: Save\tF7: Clear\tF9: Load\n"
			"LMB: select\tMMB: box select\tRMB: eyedropper\n"
			"U: select all\tI: invert selection\tO:select none\n"
			"H: hide selected\tShift+H: hide all but selected\tAlt+H: unhide all\n"
			"V: place spawner\tB: place brush\tP: place prefab (Ctrl+ to replace selected)\n"
			"M: apply material\tCtrl+M: reset material\n"
			"N: snap to grid\tCtrl+N: reset orientation\n"
			"F: link brushes\n"
			"J: save selection as prefab\n"
			"K: print spawner manifest\n"
			"L: load selected room (WORLD MODE)\n"
			"Ctrl+L: convert world to room (WORLD MODE)\n"
			"Shift+L: load world from worldgen (WORLD MODE)\n"
			"RMB: place vert at cursor (NAVMESH MODE)\n"
			"Ctrl+G: move anchor/pivot\n"
			"Alt+G: move selected vert to cursor (NAVMESH MODE)\n"
			"\n"
			"Grid size: %f\n"
			"Rotate grid size: %f\n"
			"Tiles: %d\xc3\x97%d\xc3\x97%d\n"
			"Meters/tile: %d\xc3\x97%d\xc3\x97%d",
			CurrentMode.CStr(),
			SubModes.CStr(),
			m_GridActive ? m_GridSize : 0.0f,
			m_GridActive ? m_RotateGridSize : 0.0f,
			m_TilesX, m_TilesY, m_TilesZ,
			MetersPerTileX, MetersPerTileY, MetersPerTileZ );
	m_HelpMesh = pRenderer->Print( HelpText.CStr(), pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG ), SRect(), 0 );
}

void RosaTools::CreateGridMesh( const Array<Vector>& Corners, const uint Color, Array<Vector>& OutPositions, Array<uint>& OutColors, Array<index_t>& OutIndices )
{
	ASSERT( Corners.Size() == 4 );

	ASSERT( OutPositions.Size() < 65536 );	// Assuming short index_t
	const index_t IndexBase = static_cast<index_t>( OutPositions.Size() );

	OutPositions.Append( Corners );

	for( uint i = 0; i < 4; ++i )
	{
		OutColors.PushBack( Color );
	}

	OutIndices.PushBack( IndexBase );
	OutIndices.PushBack( IndexBase + 1 );
	OutIndices.PushBack( IndexBase + 1);
	OutIndices.PushBack( IndexBase + 3 );
	OutIndices.PushBack( IndexBase + 3);
	OutIndices.PushBack( IndexBase + 2 );
	OutIndices.PushBack( IndexBase + 2 );
	OutIndices.PushBack( IndexBase );
}

void RosaTools::DeleteGridMeshes()
{
	const uint NumGridMeshes = m_GridMeshes.Size();
	for( uint GridMeshIndex = 0; GridMeshIndex < NumGridMeshes; ++GridMeshIndex )
	{
		SafeDelete( m_GridMeshes[ GridMeshIndex ] );
	}
	m_GridMeshes.Clear();
}

void RosaTools::InitializeGridMeshes()
{
	DeleteGridMeshes();

	static const float	kBuffer			= 0.01f;
	static const float	kMin			= kBuffer;
	static const float	kMax			= 1.0f - kBuffer;
	static const uint	kGridRightColor	= ARGB_TO_COLOR( 255, 255, 128, 128 );
	static const uint	kGridLeftColor	= ARGB_TO_COLOR( 255, 192,  48,  48 );
	static const uint	kGridFrontColor	= ARGB_TO_COLOR( 255, 128, 255, 128 );
	static const uint	kGridBackColor	= ARGB_TO_COLOR( 255,  48, 192,  48 );
	static const uint	kGridUpColor	= ARGB_TO_COLOR( 255, 128, 128, 255 );
	static const uint	kGridDownColor	= ARGB_TO_COLOR( 255,  48,  48, 192 );

	const uint	MapSizeX	= IsInRoomMode() ? m_MetersX : m_TilesX;
	const uint	MapSizeY	= IsInRoomMode() ? m_MetersY : m_TilesY;
	const uint	MapSizeZ	= IsInRoomMode() ? m_MetersZ : m_TilesZ;
	const float	fMapSizeX	= static_cast<float>( MapSizeX );
	const float	fMapSizeY	= static_cast<float>( MapSizeY );
	const float	fMapSizeZ	= static_cast<float>( MapSizeZ );

	Array<Vector>	Positions[6];
	Array<uint>		Colors[6];
	Array<index_t>	Indices[6];

	Array<Vector> Corners;
	Corners.Resize( 4 );

	for( uint Y = 0; Y < MapSizeY; ++Y )
	{
		for( uint X = 0; X < MapSizeX; ++X )
		{
			const float fX = static_cast<float>( X );
			const float fY = static_cast<float>( Y );

			Corners[0] = Vector( fX + kMin, fY + kMin, -kBuffer );
			Corners[1] = Vector( fX + kMax, fY + kMin, -kBuffer );
			Corners[2] = Vector( fX + kMin, fY + kMax, -kBuffer );
			Corners[3] = Vector( fX + kMax, fY + kMax, -kBuffer );
			CreateGridMesh( Corners, kGridDownColor, Positions[0], Colors[0], Indices[0] );

			Corners[0] = Vector( fX + kMin, fY + kMin, fMapSizeZ + kBuffer );
			Corners[1] = Vector( fX + kMax, fY + kMin, fMapSizeZ + kBuffer );
			Corners[2] = Vector( fX + kMin, fY + kMax, fMapSizeZ + kBuffer );
			Corners[3] = Vector( fX + kMax, fY + kMax, fMapSizeZ + kBuffer );
			CreateGridMesh( Corners, kGridUpColor, Positions[1], Colors[1], Indices[1] );
		}
	}

	for( uint Z = 0; Z < MapSizeZ; ++Z )
	{
		for( uint X = 0; X < MapSizeX; ++X )
		{
			const float fX = static_cast<float>( X );
			const float fZ = static_cast<float>( Z );

			Corners[0] = Vector( fX + kMin, -kBuffer, fZ + kMin );
			Corners[1] = Vector( fX + kMax, -kBuffer, fZ + kMin );
			Corners[2] = Vector( fX + kMin, -kBuffer, fZ + kMax );
			Corners[3] = Vector( fX + kMax, -kBuffer, fZ + kMax );
			CreateGridMesh( Corners, kGridBackColor, Positions[2], Colors[2], Indices[2] );

			Corners[0] = Vector( fX + kMin, fMapSizeY + kBuffer, fZ + kMin );
			Corners[1] = Vector( fX + kMax, fMapSizeY + kBuffer, fZ + kMin );
			Corners[2] = Vector( fX + kMin, fMapSizeY + kBuffer, fZ + kMax );
			Corners[3] = Vector( fX + kMax, fMapSizeY + kBuffer, fZ + kMax );
			CreateGridMesh( Corners, kGridFrontColor, Positions[3], Colors[3], Indices[3] );
		}
	}

	for( uint Z = 0; Z < MapSizeZ; ++Z )
	{
		for( uint Y = 0; Y < MapSizeY; ++Y )
		{
			const float fY = static_cast<float>( Y );
			const float fZ = static_cast<float>( Z );

			Corners[0] = Vector( -kBuffer, fY + kMin, fZ + kMin );
			Corners[1] = Vector( -kBuffer, fY + kMax, fZ + kMin );
			Corners[2] = Vector( -kBuffer, fY + kMin, fZ + kMax );
			Corners[3] = Vector( -kBuffer, fY + kMax, fZ + kMax );
			CreateGridMesh( Corners, kGridLeftColor, Positions[4], Colors[4], Indices[4] );

			Corners[0] = Vector( fMapSizeX + kBuffer, fY + kMin, fZ + kMin );
			Corners[1] = Vector( fMapSizeX + kBuffer, fY + kMax, fZ + kMin );
			Corners[2] = Vector( fMapSizeX + kBuffer, fY + kMin, fZ + kMax );
			Corners[3] = Vector( fMapSizeX + kBuffer, fY + kMax, fZ + kMax );
			CreateGridMesh( Corners, kGridRightColor, Positions[5], Colors[5], Indices[5] );
		}
	}

	IRenderer* const pRenderer = m_Framework->GetRenderer();
	for( uint i = 0; i < 6; ++i )
	{
		IVertexBuffer*		VertexBuffer		= pRenderer->CreateVertexBuffer();
		IVertexDeclaration*	VertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_COLORS );
		IIndexBuffer*		IndexBuffer			= pRenderer->CreateIndexBuffer();

		IVertexBuffer::SInit InitStruct;
		InitStruct.NumVertices	= Positions[i].Size();
		InitStruct.Positions	= Positions[i].GetData();
		InitStruct.Colors		= Colors[i].GetData();
		VertexBuffer->Init( InitStruct );
		IndexBuffer->Init( Indices[i].Size(), Indices[i].GetData() );
		IndexBuffer->SetPrimitiveType( EPT_LINELIST );
		Mesh* const pQuadMesh = new Mesh( VertexBuffer, VertexDeclaration, IndexBuffer );

		const Vector MinV = Vector( 0.0f, 0.0f, 0.0f );
		const Vector MaxV = Vector( fMapSizeX, fMapSizeY, fMapSizeZ );
		pQuadMesh->SetAABB( AABB( MinV, MaxV ) );
		pQuadMesh->SetMaterialDefinition( DEFAULT_MATERIAL, pRenderer );
		pQuadMesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( DEFAULT_TEXTURE ) );
		pQuadMesh->SetBucket( "DevToolsFwd" );
#if BUILD_DEBUG
		pQuadMesh->m_DEBUG_Name = "ToolsGrid";
#endif

		m_GridMeshes.PushBack( pQuadMesh );
	}
}

void RosaTools::CreatePortalMesh( const Array<Vector>& Corners, const uint Color, Array<Vector>& OutPositions, Array<uint>& OutColors, Array<index_t>& OutIndices )
{
	ASSERT( Corners.Size() == 4 );
	ASSERT( OutPositions.Size() < 65536 );	// Assuming short index_t

	const index_t IndexBase = static_cast<index_t>( OutPositions.Size() );

	OutPositions.Append( Corners );

	for( uint Index = 0; Index < 4; ++Index )
	{
		OutColors.PushBack( Color );
	}

	OutIndices.PushBack( IndexBase );
	OutIndices.PushBack( IndexBase + 1 );
	OutIndices.PushBack( IndexBase + 2 );
	OutIndices.PushBack( IndexBase + 1 );
	OutIndices.PushBack( IndexBase + 3 );
	OutIndices.PushBack( IndexBase + 2 );
}

uint RosaTools::GetTileIndex( const uint TileX, const uint TileY, const uint TileZ ) const
{
	return TileX + TileY * m_TilesX + TileZ * m_TilesX * m_TilesY;
}

uint RosaTools::GetPortalIndex( const Vector& Facing ) const
{
	const float Threshold	= .0707107f;
	const float DotRight	= Facing.Dot( Vector(  1,  0,  0 ) );
	const float DotLeft		= Facing.Dot( Vector( -1,  0,  0 ) );
	const float DotFront	= Facing.Dot( Vector(  0,  1,  0 ) );
	const float DotBack		= Facing.Dot( Vector(  0, -1,  0 ) );
	const float DotUp		= Facing.Dot( Vector(  0,  0,  1 ) );
	const float DotDown		= Facing.Dot( Vector(  0,  0, -1 ) );

	if( DotRight > Threshold )	{ return 0; }
	if( DotLeft > Threshold )	{ return 1; }
	if( DotFront > Threshold )	{ return 2; }
	if( DotBack > Threshold )	{ return 3; }
	if( DotUp > Threshold )		{ return 4; }
	if( DotDown > Threshold )	{ return 5; }

	// Else, it was an odd angle.
	return 0;
}

uint RosaTools::GetPortalColor( const SPortalDef& Portal ) const
{
	const uint	AlphaBits	= 0xff000000;

	const uint	FrontMask	= 0x002aaa2a;
	const uint	FrontBits	= FrontMask & Portal.m_Portal.m_FrontTag.GetHash();

	const uint	BackMask	= 0x00551555;
	uint		BackBits	= 0;
	FOR_EACH_ARRAY( BackTagIter, Portal.m_Portal.m_BackTags, HashedString )
	{
		const HashedString	BackTag	= BackTagIter.GetValue();
		BackBits ^= BackTag.GetHash();
	}
	BackBits				= BackMask & BackBits;

	const uint	NoExpandBit	= Portal.m_Portal.m_NoExpand	? 0x00800000 : 0x00000000;
	const uint	NoJoinBit	= Portal.m_Portal.m_NoJoin		? 0x00004000 : 0x00000000;
	const uint	MustJoinBit	= Portal.m_Portal.m_MustJoin	? 0x00000080 : 0x00000000;

	const uint	Color		= AlphaBits | FrontBits | BackBits | NoExpandBit | NoJoinBit | MustJoinBit;

	return Color;
}

void RosaTools::InitializePortalMesh()
{
	SafeDelete( m_PortalMesh );

	if( IsInWorldMode() )
	{
		return;
	}

	static const float	kBuffer			= 0.02f;
	const uint			MetersPerTileX	= m_MetersX / m_TilesX;
	const uint			MetersPerTileY	= m_MetersY / m_TilesY;
	const uint			MetersPerTileZ	= m_MetersZ / m_TilesZ;
	const float			fRoomSizeX		= static_cast<float>( m_MetersX );
	const float			fRoomSizeY		= static_cast<float>( m_MetersY );
	const float			fRoomSizeZ		= static_cast<float>( m_MetersZ );

	Array<Vector>	Positions;
	Array<uint>		Colors;
	Array<index_t>	Indices;

	Array<Vector>	Corners;
	Corners.Resize( 4 );

	for( uint Y = 0; Y < m_TilesY; ++Y )
	{
		for( uint Z = 0; Z < m_TilesZ; ++Z )
		{
			const float	LoY	= static_cast<float>( Y * MetersPerTileY );
			const float	HiY	= static_cast<float>( ( Y + 1 ) * MetersPerTileY );
			const float	LoZ	= static_cast<float>( Z * MetersPerTileZ );
			const float	HiZ	= static_cast<float>( ( Z + 1 ) * MetersPerTileZ );

			// Build left faces
			{
				const uint			X			= 0;
				const float			fX			= 0.0f;
				const SPortalDef&	Portal		= m_PortalDefs[ m_Portals[ GetTileIndex( X, Y, Z ) ].m_Portals[1] ];	// Left
				const uint			Color		= GetPortalColor( Portal );

				Corners[0]	= Vector( fX - kBuffer, LoY, LoZ );
				Corners[1]	= Vector( fX - kBuffer, HiY, LoZ );
				Corners[2]	= Vector( fX - kBuffer, LoY, HiZ );
				Corners[3]	= Vector( fX - kBuffer, HiY, HiZ );
				CreatePortalMesh( Corners, Color, Positions, Colors, Indices );
			}

			// Build right faces
			{
				const uint			X			= m_TilesX - 1;
				const float			fX			= static_cast<float>( m_MetersX );
				const SPortalDef&	Portal		= m_PortalDefs[ m_Portals[ GetTileIndex( X, Y, Z ) ].m_Portals[0] ];	// Right
				const uint			Color		= GetPortalColor( Portal );

				Corners[0]	= Vector( fX + kBuffer, HiY, LoZ );
				Corners[1]	= Vector( fX + kBuffer, LoY, LoZ );
				Corners[2]	= Vector( fX + kBuffer, HiY, HiZ );
				Corners[3]	= Vector( fX + kBuffer, LoY, HiZ );
				CreatePortalMesh( Corners, Color, Positions, Colors, Indices );
			}
		}
	}

	for( uint X = 0; X < m_TilesX; ++X )
	{
		for( uint Z = 0; Z < m_TilesZ; ++Z )
		{
			const float	LoX	= static_cast<float>( X * MetersPerTileX );
			const float	HiX	= static_cast<float>( ( X + 1 ) * MetersPerTileX );
			const float	LoZ	= static_cast<float>( Z * MetersPerTileZ );
			const float	HiZ	= static_cast<float>( ( Z + 1 ) * MetersPerTileZ );

			// Build back faces
			{
				const uint			Y			= 0;
				const float			fY			= 0.0f;
				const SPortalDef&	Portal		= m_PortalDefs[ m_Portals[ GetTileIndex( X, Y, Z ) ].m_Portals[3] ];	// Back
				const uint			Color		= GetPortalColor( Portal );

				Corners[0]	= Vector( HiX, fY - kBuffer, LoZ );
				Corners[1]	= Vector( LoX, fY - kBuffer, LoZ );
				Corners[2]	= Vector( HiX, fY - kBuffer, HiZ );
				Corners[3]	= Vector( LoX, fY - kBuffer, HiZ );
				CreatePortalMesh( Corners, Color, Positions, Colors, Indices );
			}

			// Build front faces
			{
				const uint			Y			= m_TilesY - 1;
				const float			fY			= static_cast<float>( m_MetersY );
				const SPortalDef&	Portal		= m_PortalDefs[ m_Portals[ GetTileIndex( X, Y, Z ) ].m_Portals[2] ];	// Front
				const uint			Color		= GetPortalColor( Portal );

				Corners[0]	= Vector( LoX, fY + kBuffer, LoZ );
				Corners[1]	= Vector( HiX, fY + kBuffer, LoZ );
				Corners[2]	= Vector( LoX, fY + kBuffer, HiZ );
				Corners[3]	= Vector( HiX, fY + kBuffer, HiZ );
				CreatePortalMesh( Corners, Color, Positions, Colors, Indices );
			}
		}
	}

	for( uint X = 0; X < m_TilesX; ++X )
	{
		for( uint Y = 0; Y < m_TilesY; ++Y )
		{
			const float	LoX	= static_cast<float>( X * MetersPerTileX );
			const float	HiX	= static_cast<float>( ( X + 1 ) * MetersPerTileX );
			const float	LoY	= static_cast<float>( Y * MetersPerTileY );
			const float	HiY	= static_cast<float>( ( Y + 1 ) * MetersPerTileY );

			// Build bottom faces
			{
				const uint			Z			= 0;
				const float			fZ			= 0.0f;
				const SPortalDef&	Portal		= m_PortalDefs[ m_Portals[ GetTileIndex( X, Y, Z ) ].m_Portals[5] ];	// Bottom
				const uint			Color		= GetPortalColor( Portal );

				Corners[0]	= Vector( LoX, LoY, fZ - kBuffer );
				Corners[1]	= Vector( HiX, LoY, fZ - kBuffer );
				Corners[2]	= Vector( LoX, HiY, fZ - kBuffer );
				Corners[3]	= Vector( HiX, HiY, fZ - kBuffer );
				CreatePortalMesh( Corners, Color, Positions, Colors, Indices );
			}

			// Build top faces
			{
				const uint			Z			= m_TilesZ - 1;
				const float			fZ			= static_cast<float>( m_MetersZ );
				const SPortalDef&	Portal		= m_PortalDefs[ m_Portals[ GetTileIndex( X, Y, Z ) ].m_Portals[4] ];	// Top
				const uint			Color		= GetPortalColor( Portal );

				Corners[0]	= Vector( LoX, HiY, fZ + kBuffer );
				Corners[1]	= Vector( HiX, HiY, fZ + kBuffer );
				Corners[2]	= Vector( LoX, LoY, fZ + kBuffer );
				Corners[3]	= Vector( HiX, LoY, fZ + kBuffer );
				CreatePortalMesh( Corners, Color, Positions, Colors, Indices );
			}
		}
	}

	IRenderer* const			pRenderer			= m_Framework->GetRenderer();
	IVertexBuffer* const		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration* const	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_COLORS );
	IIndexBuffer* const			pIndexBuffer		= pRenderer->CreateIndexBuffer();

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= Positions.Size();
	InitStruct.Positions	= Positions.GetData();
	InitStruct.Colors		= Colors.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( Indices.Size(), Indices.GetData() );
	m_PortalMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	m_PortalMesh->SetAABB( AABB( Vector(), Vector( fRoomSizeX, fRoomSizeY, fRoomSizeZ ) ) );
	m_PortalMesh->SetMaterialDefinition( DEFAULT_MATERIAL, pRenderer );
	m_PortalMesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( DEFAULT_TEXTURE ) );
	m_PortalMesh->SetBucket( "DevToolsFwd" );
#if BUILD_DEBUG
	m_PortalMesh->m_DEBUG_Name = "ToolsPortals";
#endif
}

void RosaTools::InitializeElementButtonMeshes( SElements& Elements, const Array<SimpleString>& DefNames )
{
	static const float	skBase					= 16.0f;
	static const float	skSpacing				= 16.0f;

	IRenderer* const	pRenderer				= m_Framework->GetRenderer();
	const float			kDisplayHeight			= static_cast<float>( m_Framework->GetDisplay()->m_Height );
	const uint			kMaxVerticalElements	= static_cast<uint>( Floor( ( kDisplayHeight - ( skBase * 2.0f ) ) / skSpacing ) );

	float				MaxCategoryWidth		= 0.0f;
	float				MaxDefWidth				= 0.0f;
	float				LeftSpacing				= 0.0f;
	Map<uint, uint>		CategoryCountMap;

	const uint NumCategories = Elements.m_Categories.Size();
	for( uint CategoryIndex = 0; CategoryIndex < NumCategories; ++CategoryIndex )
	{
		const SimpleString& Category = Elements.m_Categories[ CategoryIndex ];
		SRect Rect = SRect( skBase, skBase + CategoryIndex * skSpacing, 0.0f, 0.0f );
		Mesh* const pCategoryMesh = pRenderer->Print( Category.CStr(), pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG ), Rect, 0 );

		SElementCategoryButton CategoryButton;
		CategoryButton.m_Mesh			= pCategoryMesh;
		CategoryButton.m_CategoryIndex	= CategoryIndex;
		CategoryButton.m_BoxMin			= Vector2( pCategoryMesh->m_AABB.m_Min.x, pCategoryMesh->m_AABB.m_Min.z );
		CategoryButton.m_BoxMax			= Vector2( pCategoryMesh->m_AABB.m_Max.x, pCategoryMesh->m_AABB.m_Max.z );

		const float CategoryWidth = pCategoryMesh->m_AABB.m_Max.x - pCategoryMesh->m_AABB.m_Min.x;
		MaxCategoryWidth = Max( CategoryWidth, MaxCategoryWidth );

		Elements.m_CategoryButtons.PushBack( CategoryButton );

		CategoryCountMap[ CategoryIndex ] = 0;
	}

	// Iterate through all elements to find max column width
	const uint NumDefs = DefNames.Size();
	for( uint DefIndex = 0; DefIndex < NumDefs; ++DefIndex )
	{
		Vector2 DefExtents;
		pRenderer->Arrange( DefNames[ DefIndex ].CStr(), pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG ), SRect(), 0, DefExtents );
		MaxDefWidth = Max( DefExtents.x, MaxDefWidth );
	}

	LeftSpacing = skBase + MaxCategoryWidth + skSpacing;
	for( uint DefIndex = 0; DefIndex < NumDefs; ++DefIndex )
	{
		const uint CategoryIndex = Elements.m_CategoryMap[ DefIndex ];
		const uint CategoryCount = CategoryCountMap[ CategoryIndex ]++;

		uint X = 0;
		uint Y = CategoryCount;
		while( Y >= kMaxVerticalElements )
		{
			X += 1;
			Y -= kMaxVerticalElements;
		}

		const SimpleString&	DefName		= DefNames[ DefIndex ];
		SRect				Rect		= SRect( LeftSpacing + X * MaxDefWidth + skSpacing, skBase + Y * skSpacing, 0.0f, 0.0f );
		Mesh* const			pDefMesh	= pRenderer->Print( DefName.CStr(), pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG ), Rect, 0 );

		SElementButton Button;
		Button.m_Mesh		= pDefMesh;
		Button.m_DefIndex	= DefIndex;
		Button.m_BoxMin	= Vector2( pDefMesh->m_AABB.m_Min.x, pDefMesh->m_AABB.m_Min.z );
		Button.m_BoxMax	= Vector2( pDefMesh->m_AABB.m_Max.x, pDefMesh->m_AABB.m_Max.z );

		Elements.m_Buttons.PushBack( Button );
	}
}

void RosaTools::DeleteElementButtonMeshes( SElements& Elements )
{
	const uint NumCategoryButtons = Elements.m_CategoryButtons.Size();
	for( uint CategoryButtonIndex = 0; CategoryButtonIndex < NumCategoryButtons; ++CategoryButtonIndex )
	{
		SElementCategoryButton& CategoryButton = Elements.m_CategoryButtons[ CategoryButtonIndex ];
		SafeDelete( CategoryButton.m_Mesh );
	}
	Elements.m_CategoryButtons.Clear();

	const uint NumButtons = Elements.m_Buttons.Size();
	for( uint ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex )
	{
		SElementButton& Button = Elements.m_Buttons[ ButtonIndex ];
		SafeDelete( Button.m_Mesh );
	}
	Elements.m_Buttons.Clear();
}

bool RosaTools::IsInToolMode() const
{
	return m_ToolMode;
}

void RosaTools::ToggleToolMode()
{
	m_ToolMode = !m_ToolMode;

	STATIC_HASHED_STRING( HUD );
	STATIC_HASHED_STRING( ToolsHUD );
	STATIC_HASHED_STRING( Main );
	STATIC_HASHED_STRING( Tools );
	IRenderer* const	pRenderer	= m_Framework->GetRenderer();
	UIManager* const	pUIManager	= m_Framework->GetUIManager();

	pRenderer->SetBucketsEnabled( sTools, m_ToolMode );
	pRenderer->SetBucketsEnabled( sMain, !m_ToolMode );

	UIScreen* const		pOldHUD		= pUIManager->GetScreen( m_ToolMode ? sHUD : sToolsHUD );
	UIScreen* const		pNewHUD		= pUIManager->GetScreen( m_ToolMode ? sToolsHUD : sHUD );
	pUIManager->GetUIStack()->Replace( pOldHUD, pNewHUD );

	m_Framework->GetMouse()->SetActive( !m_ToolMode );

	if( !m_ToolMode )
	{
		SetSubTool( IsInRoomMode() ? EST_RoomMode : EST_WorldMode, false );
	}
}

void RosaTools::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	static const float kVelocity			= 2.0f;
	static const float kRotationVelocity	= 0.5f;

	m_CameraLocation			+= m_CameraVelocity				* m_CameraSpeed		* DeltaTime;
	m_CameraOrientation			+= m_CameraRotationalVelocity	* kRotationVelocity	* DeltaTime;

	m_CameraOrientation.Pitch	= Clamp( m_CameraOrientation.Pitch, -PI * 0.49f, PI * 0.49f );

	m_Framework->SetMainViewTransform( m_CameraLocation, m_CameraOrientation );

	// Zero these after we're done with them.
	m_CameraVelocity.Zero();
	m_CameraRotationalVelocity.Zero();
}

int RosaTools::ConditionalMessageBox( const SimpleString& Message ) const
{
	return HasUnsavedChanges() ? MessageBox( NULL, Message.CStr(), Message.CStr(), MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND ) : IDYES;
}

void RosaTools::TickInput( const float DeltaTime )
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();

	// F1: toggle room mode and world mode
	if( pKeyboard->OnRise( Keyboard::EB_F1 ) )
	{
		if( IsInNormalMode() )
		{
			// Need to make a copy of the string so it doesn't get cleared by reiniting the world
			const SimpleString	TargetMapName	= IsInRoomMode() ? m_CurrentWorldName : m_CurrentRoomName;
			const bool			ShouldClear		= ( TargetMapName == "" );
			const SimpleString	Message			= ShouldClear ? SimpleString( "Clear world?" ) : SimpleString::PrintF( "Load %s?", TargetMapName.CStr() );
			const int			Response		= ConditionalMessageBox( Message );
			if( Response == IDYES )
			{
				SetSubTool( IsInRoomMode() ? EST_WorldMode : EST_RoomMode, ShouldClear );
				if( ShouldClear )
				{
					Clear();
				}
				else
				{
					Load( TargetMapName );
				}
			}
		}
		else
		{
			SetSubTool( IsInRoomMode() ? EST_RoomMode : EST_WorldMode, true );
		}
	}

	// F2: Spawners (room mode)
	if( IsInRoomMode() && pKeyboard->OnRise( Keyboard::EB_F2 ) )
	{
		SetSubTool( EST_Spawners, false );
	}

	// F3: Geo (room mode) or rooms (world mode)
	if( pKeyboard->OnRise( Keyboard::EB_F3 ) )
	{
		SetSubTool( IsInRoomMode() ? EST_Geo : EST_Rooms, false );
	}

	// F10: Prefabs (room mode)
	if( pKeyboard->OnRise( Keyboard::EB_F10 ) )
	{
		SetSubTool( IsInRoomMode() ? EST_Prefabs : EST_WorldPrefabs, false );
	}

	// F4: Portals (room mode)
	if( IsInRoomMode() && pKeyboard->OnRise( Keyboard::EB_F4 ) )
	{
		SetSubTool( EST_Portals, false );
	}

	// F5: Navmesh (room mode)
	if( IsInRoomMode() && pKeyboard->OnRise( Keyboard::EB_F5 ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			SetMeshSubTool_NoCache();
		}
		else
		{
			SetSubTool( EST_NavMesh, false );
		}
	}

	// F8: Mats (room mode)
	if( IsInRoomMode() && pKeyboard->OnRise( Keyboard::EB_F8 ) )
	{
		SetSubTool( EST_Mats, false );
	}

	if( m_SubTool == EST_Rooms )
	{
		TickElementsInput( m_RoomElements );
	}
	else if( m_SubTool == EST_Spawners )
	{
		TickElementsInput( m_SpawnerElements );
	}
	else if( m_SubTool == EST_Geo )
	{
		TickElementsInput( m_GeoElements );
	}
	else if( m_SubTool == EST_Prefabs || m_SubTool == EST_WorldPrefabs )
	{
		TickElementsInput( m_PrefabElements );
	}
	else if( m_SubTool == EST_Portals )
	{
		TickElementsInput( m_PortalElements );
	}
	else if( m_SubTool == EST_Mats )
	{
		TickElementsInput( m_MatElements );
	}
	else if( m_SubTool == EST_NavMesh )
	{
		TickNavMeshInput( DeltaTime );
	}
	else
	{
		TickNormalInput( DeltaTime );
	}
}

const float		kNavMeshLineSize			= 0.05f;
const Vector	kNavMeshLineUp				= Vector( 0.0f, 0.0f, kNavMeshLineSize );
const Vector	kNavMeshLineRight			= Vector( kNavMeshLineSize, 0.0f, 0.0f );
const Vector	kNavMeshLineFront			= Vector( 0.0f, kNavMeshLineSize, 0.0f );
const float		kNavSnapSize				= 0.02f;
const Vector	kNavSnapSizeX				= Vector( kNavSnapSize, 0.0f, 0.0f );
const Vector	kNavSnapSizeY				= Vector( 0.0f, kNavSnapSize, 0.0f );
const Vector	kNavSnapSizeZ				= Vector( 0.0f, 0.0f, kNavSnapSize );
const uint		kNavUnselectedColor			= ARGB_TO_COLOR( 255, 32, 32, 32 );
const uint		kNavVertSelectedColor		= ARGB_TO_COLOR( 255, 255, 255, 0 );
const uint		kNavEdgeSelectedColor		= ARGB_TO_COLOR( 255, 0, 255, 255 );
const uint		kNavFaceSelectedColor		= ARGB_TO_COLOR( 255, 255, 0, 255 );
const uint		kNavFaceMustAvoidColor		= ARGB_TO_COLOR( 255, 255, 0, 0 );
const uint		kNavFaceShouldAvoidColor	= ARGB_TO_COLOR( 255, 255, 255, 0 );
const uint		kNavSnapColor				= ARGB_TO_COLOR( 255, 255, 40, 40 );

void RosaTools::AddNavVertToMesh( const SNavVert& Vert, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const
{
	const Vector	VertLoc		= Vert.m_Vert;
	const uint		Color		= Vert.m_Selected ? kNavVertSelectedColor : kNavUnselectedColor;
	const index_t	BaseIndex	= static_cast<index_t>( Positions.Size() );

	Positions.PushBack( VertLoc - kNavMeshLineRight - kNavMeshLineFront - kNavMeshLineUp );
	Positions.PushBack( VertLoc + kNavMeshLineRight - kNavMeshLineFront - kNavMeshLineUp );
	Positions.PushBack( VertLoc - kNavMeshLineRight + kNavMeshLineFront - kNavMeshLineUp );
	Positions.PushBack( VertLoc + kNavMeshLineRight + kNavMeshLineFront - kNavMeshLineUp );
	Positions.PushBack( VertLoc - kNavMeshLineRight - kNavMeshLineFront + kNavMeshLineUp );
	Positions.PushBack( VertLoc + kNavMeshLineRight - kNavMeshLineFront + kNavMeshLineUp );
	Positions.PushBack( VertLoc - kNavMeshLineRight + kNavMeshLineFront + kNavMeshLineUp );
	Positions.PushBack( VertLoc + kNavMeshLineRight + kNavMeshLineFront + kNavMeshLineUp );

	for( uint Index = 0; Index < 8; ++Index )
	{
		Colors.PushBack( Color );
	}

#define PUSH_INDICES( a, b, c, d ) \
	Indices.PushBack( BaseIndex + a );	Indices.PushBack( BaseIndex + b );	Indices.PushBack( BaseIndex + c ); \
	Indices.PushBack( BaseIndex + b );	Indices.PushBack( BaseIndex + d );	Indices.PushBack( BaseIndex + c )
	PUSH_INDICES( 2, 0, 6, 4 );	// Left face
	PUSH_INDICES( 1, 3, 5, 7 );	// Right face
	PUSH_INDICES( 0, 1, 4, 5 );	// Back face
	PUSH_INDICES( 3, 2, 7, 6 );	// Front face
	PUSH_INDICES( 2, 3, 0, 1 );	// Bottom face
	PUSH_INDICES( 4, 5, 6, 7 );	// Top face
#undef PUSH_INDICES
}

void RosaTools::AddNavEdgeToMesh( const SNavEdge& Edge, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const
{
	const Vector&	VertA		= m_NavVerts[ Edge.m_VertA ].m_Vert;
	const Vector&	VertB		= m_NavVerts[ Edge.m_VertB ].m_Vert;
	const Vector	Direction	= ( VertB - VertA ).GetNormalized();
	const Vector	UpVector	= Vector( 0.0f, 0.0f, 1.0f );
	const Vector	Offset		= kNavMeshLineSize * UpVector.Cross( Direction ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
	const uint		Color		= Edge.m_Selected ? kNavEdgeSelectedColor : kNavUnselectedColor;
	const index_t	BaseIndex	= static_cast<index_t>( Positions.Size() );

	Positions.PushBack( VertA - kNavMeshLineUp - Offset );
	Positions.PushBack( VertB - kNavMeshLineUp - Offset );
	Positions.PushBack( VertA + kNavMeshLineUp + Offset );
	Positions.PushBack( VertB + kNavMeshLineUp + Offset );
	Positions.PushBack( VertA - kNavMeshLineUp + Offset );
	Positions.PushBack( VertB - kNavMeshLineUp + Offset );
	Positions.PushBack( VertA + kNavMeshLineUp - Offset );
	Positions.PushBack( VertB + kNavMeshLineUp - Offset );

	for( uint Index = 0; Index < 8; ++Index )
	{
		Colors.PushBack( Color );
	}

#define PUSH_INDICES( a, b, c, d ) \
	Indices.PushBack( BaseIndex + a );	Indices.PushBack( BaseIndex + b );	Indices.PushBack( BaseIndex + c ); \
	Indices.PushBack( BaseIndex + b );	Indices.PushBack( BaseIndex + d );	Indices.PushBack( BaseIndex + c )
	PUSH_INDICES( 0, 1, 2, 3 );
	PUSH_INDICES( 1, 0, 3, 2 );
	PUSH_INDICES( 4, 5, 6, 7 );
	PUSH_INDICES( 5, 4, 7, 6 );
#undef PUSH_INDICES
}

void RosaTools::AddNavFaceToMesh( const SNavFace& Face, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const
{
	// Draw the actual triangle in red or yellow if it's tagged
	if( ( Face.m_Props & ENP_CautiousMustAvoid ) || ( Face.m_Props & ENP_CautiousShouldAvoid ) )
	{
		const uint		FaceColor		= ( Face.m_Props & ENP_CautiousMustAvoid ) ? kNavFaceMustAvoidColor : kNavFaceShouldAvoidColor;
		const index_t	FaceBaseIndex	= static_cast<index_t>( Positions.Size() );

		// Get the three unique verts for the triangle
		Array<Vector> NavVerts;
		NavVerts.PushBackUnique( m_NavVerts[ m_NavEdges[ Face.m_EdgeA ].m_VertA ].m_Vert );
		NavVerts.PushBackUnique( m_NavVerts[ m_NavEdges[ Face.m_EdgeA ].m_VertB ].m_Vert );
		NavVerts.PushBackUnique( m_NavVerts[ m_NavEdges[ Face.m_EdgeB ].m_VertA ].m_Vert );
		NavVerts.PushBackUnique( m_NavVerts[ m_NavEdges[ Face.m_EdgeB ].m_VertB ].m_Vert );
		NavVerts.PushBackUnique( m_NavVerts[ m_NavEdges[ Face.m_EdgeC ].m_VertA ].m_Vert );
		NavVerts.PushBackUnique( m_NavVerts[ m_NavEdges[ Face.m_EdgeC ].m_VertB ].m_Vert );
		ASSERT( NavVerts.Size() == 3 );

		// We'll draw both sides, so we don't care about winding order
		Positions.PushBack( NavVerts[0] );
		Positions.PushBack( NavVerts[1] );
		Positions.PushBack( NavVerts[2] );

		for( uint Index = 0; Index < 3; ++Index )
		{
			Colors.PushBack( FaceColor );
		}

#define PUSH_INDICES( a, b, c ) \
		Indices.PushBack( FaceBaseIndex + a );	Indices.PushBack( FaceBaseIndex + b );	Indices.PushBack( FaceBaseIndex + c )
		PUSH_INDICES( 0, 1, 2 );
		PUSH_INDICES( 0, 2, 1 );
#undef PUSH_INDICES
	}

	// Draw the centroid with nav height in all other cases
	{
		const Vector	Centroid	= GetNavFaceCentroid( Face );
		const Vector	CentroidTop	= Centroid + Vector( 0.0f, 0.0f, Face.m_Height );
		const uint		Color		= Face.m_Selected ? kNavFaceSelectedColor : kNavUnselectedColor;
		const index_t	BaseIndex	= static_cast<index_t>( Positions.Size() );

		Positions.PushBack( Centroid	- kNavMeshLineRight - kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( Centroid	+ kNavMeshLineRight - kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( Centroid	- kNavMeshLineRight + kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( Centroid	+ kNavMeshLineRight + kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( Centroid	- kNavMeshLineRight - kNavMeshLineFront + kNavMeshLineUp );
		Positions.PushBack( Centroid	+ kNavMeshLineRight - kNavMeshLineFront + kNavMeshLineUp );
		Positions.PushBack( Centroid	- kNavMeshLineRight + kNavMeshLineFront + kNavMeshLineUp );
		Positions.PushBack( Centroid	+ kNavMeshLineRight + kNavMeshLineFront + kNavMeshLineUp );

		const index_t	TopBaseIndex	= static_cast<index_t>( Positions.Size() );

		Positions.PushBack( CentroidTop	- kNavMeshLineRight - kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( CentroidTop	+ kNavMeshLineRight - kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( CentroidTop	- kNavMeshLineRight + kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( CentroidTop	+ kNavMeshLineRight + kNavMeshLineFront - kNavMeshLineUp );
		Positions.PushBack( CentroidTop	- kNavMeshLineRight - kNavMeshLineFront + kNavMeshLineUp );
		Positions.PushBack( CentroidTop	+ kNavMeshLineRight - kNavMeshLineFront + kNavMeshLineUp );
		Positions.PushBack( CentroidTop	- kNavMeshLineRight + kNavMeshLineFront + kNavMeshLineUp );
		Positions.PushBack( CentroidTop	+ kNavMeshLineRight + kNavMeshLineFront + kNavMeshLineUp );

		for( uint Index = 0; Index < 16; ++Index )
		{
			Colors.PushBack( Color );
		}

#define PUSH_INDICES( baseIndex, a, b, c, d ) \
		Indices.PushBack( baseIndex + a );	Indices.PushBack( baseIndex + b );	Indices.PushBack( baseIndex + c ); \
		Indices.PushBack( baseIndex + b );	Indices.PushBack( baseIndex + d );	Indices.PushBack( baseIndex + c )

		// Centroid
		PUSH_INDICES( BaseIndex, 2, 0, 6, 4 );	// Left face
		PUSH_INDICES( BaseIndex, 1, 3, 5, 7 );	// Right face
		PUSH_INDICES( BaseIndex, 0, 1, 4, 5 );	// Back face
		PUSH_INDICES( BaseIndex, 3, 2, 7, 6 );	// Front face
		PUSH_INDICES( BaseIndex, 2, 3, 0, 1 );	// Bottom face
		PUSH_INDICES( BaseIndex, 4, 5, 6, 7 );	// Top face

		// Centroid top
		PUSH_INDICES( TopBaseIndex, 2, 0, 6, 4 );	// Left face
		PUSH_INDICES( TopBaseIndex, 1, 3, 5, 7 );	// Right face
		PUSH_INDICES( TopBaseIndex, 0, 1, 4, 5 );	// Back face
		PUSH_INDICES( TopBaseIndex, 3, 2, 7, 6 );	// Front face
		PUSH_INDICES( TopBaseIndex, 2, 3, 0, 1 );	// Bottom face
		PUSH_INDICES( TopBaseIndex, 4, 5, 6, 7 );	// Top face
#undef PUSH_INDICES
	}
}

void RosaTools::AddNavSnapPointToMesh( const Vector& SnapPoint, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const
{
	const uint		Color		= kNavSnapColor;
	const index_t	BaseIndex	= static_cast<index_t>( Positions.Size() );

	Positions.PushBack( SnapPoint - kNavSnapSizeX - kNavSnapSizeY - kNavSnapSizeZ );
	Positions.PushBack( SnapPoint + kNavSnapSizeX - kNavSnapSizeY - kNavSnapSizeZ );
	Positions.PushBack( SnapPoint - kNavSnapSizeX + kNavSnapSizeY - kNavSnapSizeZ );
	Positions.PushBack( SnapPoint + kNavSnapSizeX + kNavSnapSizeY - kNavSnapSizeZ );
	Positions.PushBack( SnapPoint - kNavSnapSizeX - kNavSnapSizeY + kNavSnapSizeZ );
	Positions.PushBack( SnapPoint + kNavSnapSizeX - kNavSnapSizeY + kNavSnapSizeZ );
	Positions.PushBack( SnapPoint - kNavSnapSizeX + kNavSnapSizeY + kNavSnapSizeZ );
	Positions.PushBack( SnapPoint + kNavSnapSizeX + kNavSnapSizeY + kNavSnapSizeZ );

	for( uint Index = 0; Index < 8; ++Index )
	{
		Colors.PushBack( Color );
	}

#define PUSH_INDICES( a, b, c, d ) \
	Indices.PushBack( BaseIndex + a );	Indices.PushBack( BaseIndex + b );	Indices.PushBack( BaseIndex + c ); \
	Indices.PushBack( BaseIndex + b );	Indices.PushBack( BaseIndex + d );	Indices.PushBack( BaseIndex + c )
	PUSH_INDICES( 2, 0, 6, 4 );	// Left face
	PUSH_INDICES( 1, 3, 5, 7 );	// Right face
	PUSH_INDICES( 0, 1, 4, 5 );	// Back face
	PUSH_INDICES( 3, 2, 7, 6 );	// Front face
	PUSH_INDICES( 2, 3, 0, 1 );	// Bottom face
	PUSH_INDICES( 4, 5, 6, 7 );	// Top face
#undef PUSH_INDICES
}

void RosaTools::CreateNavMesh()
{
	SafeDelete( m_NavMesh );

	const float	fRoomSizeX	= static_cast<float>( m_MetersX );
	const float	fRoomSizeY	= static_cast<float>( m_MetersY );
	const float	fRoomSizeZ	= static_cast<float>( m_MetersZ );

	Array<Vector>	Positions;
	Array<uint>		Colors;
	Array<index_t>	Indices;

	FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
	{
		const SNavFace& Face = FaceIter.GetValue();
		AddNavFaceToMesh( Face, Positions, Colors, Indices );
	}

	FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
	{
		const SNavEdge& Edge = EdgeIter.GetValue();
		AddNavEdgeToMesh( Edge, Positions, Colors, Indices );
	}

	FOR_EACH_ARRAY( VertIter, m_NavVerts, SNavVert )
	{
		const SNavVert& Vert = VertIter.GetValue();
		AddNavVertToMesh( Vert, Positions, Colors, Indices );
	}

	IRenderer* const			pRenderer			= m_Framework->GetRenderer();
	IVertexBuffer* const		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration* const	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_COLORS );
	IIndexBuffer* const			pIndexBuffer		= pRenderer->CreateIndexBuffer();

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= Positions.Size();
	InitStruct.Positions	= Positions.GetData();
	InitStruct.Colors		= Colors.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( Indices.Size(), Indices.GetData() );
	m_NavMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	const Vector MinV = Vector( 0.0f, 0.0f, 0.0f );
	const Vector MaxV = Vector( fRoomSizeX, fRoomSizeY, fRoomSizeZ );
	m_NavMesh->SetAABB( AABB( MinV, MaxV ) );
	m_NavMesh->SetMaterialDefinition( "Material_ToolsNavMesh", pRenderer );
	m_NavMesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( DEFAULT_TEXTURE ) );
	m_NavMesh->SetBucket( "DevToolsNoZ" );
#if BUILD_DEBUG
	m_NavMesh->m_DEBUG_Name = "ToolsNavMesh";
#endif
}

void RosaTools::CreateNavSnapMesh()
{
	SafeDelete( m_NavSnapMesh );

	const float	fRoomSizeX	= static_cast<float>( m_MetersX );
	const float	fRoomSizeY	= static_cast<float>( m_MetersY );
	const float	fRoomSizeZ	= static_cast<float>( m_MetersZ );

	Array<Vector>	Positions;
	Array<uint>		Colors;
	Array<index_t>	Indices;

	FOR_EACH_ARRAY( SnapIter, m_NavSnapPoints, Vector )
	{
		const Vector& SnapPoint = SnapIter.GetValue();
		AddNavSnapPointToMesh( SnapPoint, Positions, Colors, Indices );
	}

	IRenderer* const			pRenderer			= m_Framework->GetRenderer();
	IVertexBuffer* const		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration* const	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_COLORS );
	IIndexBuffer* const			pIndexBuffer		= pRenderer->CreateIndexBuffer();

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= Positions.Size();
	InitStruct.Positions	= Positions.GetData();
	InitStruct.Colors		= Colors.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( Indices.Size(), Indices.GetData() );
	m_NavSnapMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	const Vector MinV = Vector( 0.0f, 0.0f, 0.0f );
	const Vector MaxV = Vector( fRoomSizeX, fRoomSizeY, fRoomSizeZ );
	m_NavSnapMesh->SetAABB( AABB( MinV, MaxV ) );
	m_NavSnapMesh->SetMaterialDefinition( "Material_ToolsNavMesh", pRenderer );
	m_NavSnapMesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( DEFAULT_TEXTURE ) );
	m_NavSnapMesh->SetBucket( "DevToolsNoZ" );
#if BUILD_DEBUG
	m_NavSnapMesh->m_DEBUG_Name = "ToolsNavMeshCache";
#endif
}

void RosaTools::SelectAllNav()
{
	FOR_EACH_ARRAY( VertIter, m_NavVerts, SNavVert )
	{
		SelectNavVert( VertIter.GetIndex() );
	}

	FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
	{
		SelectNavEdge( EdgeIter.GetIndex() );
	}

	FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
	{
		SelectNavFace( FaceIter.GetIndex() );
	}
}

void RosaTools::DeselectAllNav()
{
	FOR_EACH_ARRAY( VertIter, m_NavVerts, SNavVert )
	{
		DeselectNavVert( VertIter.GetIndex() );
	}

	FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
	{
		DeselectNavEdge( EdgeIter.GetIndex() );
	}

	FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
	{
		DeselectNavFace( FaceIter.GetIndex() );
	}
}

void RosaTools::SetNavVertSelected( const uint VertIndex, const bool Selected )
{
	Selected ? SelectNavVert( VertIndex ) : DeselectNavVert( VertIndex );
}

void RosaTools::SelectNavVert( const uint VertIndex )
{
	SNavVert& Vert	= m_NavVerts[ VertIndex ];
	Vert.m_Selected	= true;

	m_SelectedNavVerts.PushBackUnique( VertIndex );

	if( m_SelectedNavVerts.Size() == 1 && m_SelectedNavEdges.Empty() && m_SelectedNavFaces.Empty() )
	{
		m_TransformAnchor = Vert.m_Vert;
	}
}

void RosaTools::DeselectNavVert( const uint VertIndex )
{
	SNavVert& Vert	= m_NavVerts[ VertIndex ];
	Vert.m_Selected	= false;

	m_SelectedNavVerts.RemoveItem( VertIndex );
}

void RosaTools::DeselectNavVerts( const Array<uint>& Verts )
{
	FOR_EACH_ARRAY( VertIter, Verts, uint )
	{
		DeselectNavVert( VertIter.GetValue() );
	}
}

void RosaTools::ToggleNavVert( const uint VertIndex )
{
	SNavVert& Vert	= m_NavVerts[ VertIndex ];
	Vert.m_Selected	? DeselectNavVert( VertIndex ) : SelectNavVert( VertIndex );
}

void RosaTools::SetNavEdgeSelected( const uint EdgeIndex, const bool Selected )
{
	Selected ? SelectNavEdge( EdgeIndex ) : DeselectNavEdge( EdgeIndex );
}

void RosaTools::SelectNavEdge( const uint EdgeIndex )
{
	SNavEdge& Edge	= m_NavEdges[ EdgeIndex ];
	Edge.m_Selected	= true;

	m_SelectedNavEdges.PushBackUnique( EdgeIndex );

	if( m_SelectedNavEdges.Size() == 1 && m_SelectedNavVerts.Empty() && m_SelectedNavFaces.Empty() )
	{
		m_TransformAnchor = m_NavVerts[ Edge.m_VertA ].m_Vert;
	}
}

void RosaTools::DeselectNavEdge( const uint EdgeIndex )
{
	SNavEdge& Edge	= m_NavEdges[ EdgeIndex ];
	Edge.m_Selected	= false;

	m_SelectedNavEdges.RemoveItem( EdgeIndex );
}

void RosaTools::DeselectNavEdges( const Array<uint>& Edges )
{
	FOR_EACH_ARRAY( EdgeIter, Edges, uint )
	{
		DeselectNavEdge( EdgeIter.GetValue() );
	}
}

void RosaTools::ToggleNavEdge( const uint EdgeIndex )
{
	SNavEdge& Edge	= m_NavEdges[ EdgeIndex ];
	Edge.m_Selected	? DeselectNavEdge( EdgeIndex ) : SelectNavEdge( EdgeIndex );
}

void RosaTools::SetNavFaceSelected( const uint FaceIndex, const bool Selected )
{
	Selected ? SelectNavFace( FaceIndex ) : DeselectNavFace( FaceIndex );
}

void RosaTools::SelectNavFace( const uint FaceIndex )
{
	SNavFace& Face	= m_NavFaces[ FaceIndex ];
	Face.m_Selected	= true;

	m_SelectedNavFaces.PushBackUnique( FaceIndex );

	if( m_SelectedNavFaces.Size() == 1 && m_SelectedNavVerts.Empty() && m_SelectedNavEdges.Empty() )
	{
		m_TransformAnchor = m_NavVerts[ m_NavEdges[ Face.m_EdgeA ].m_VertA ].m_Vert;
	}
}

void RosaTools::DeselectNavFace( const uint FaceIndex )
{
	SNavFace& Face	= m_NavFaces[ FaceIndex ];
	Face.m_Selected	= false;

	m_SelectedNavFaces.RemoveItem( FaceIndex );
}

void RosaTools::DeselectNavFaces( const Array<uint>& Faces )
{
	FOR_EACH_ARRAY( FaceIter, Faces, uint )
	{
		DeselectNavFace( FaceIter.GetValue() );
	}
}

void RosaTools::ToggleNavFace( const uint FaceIndex )
{
	SNavFace& Face	= m_NavFaces[ FaceIndex ];
	Face.m_Selected	? DeselectNavFace( FaceIndex ) : SelectNavFace( FaceIndex );
}

bool RosaTools::DoesBrushProvideNavMeshSnapPoints( const SBrush& Brush ) const
{
	if( Brush.m_Hidden )
	{
		return false;
	}

	if( Brush.m_Type == EBT_Geo )
	{
		return true;
	}
	else if( Brush.m_Type == EBT_Spawner )
	{
		const SSpawnerDef&	SpawnerDef	= m_SpawnerDefs[ Brush.m_DefIndex ];
		return SpawnerDef.m_AddNavMeshSnapPoints;
	}
	else
	{
		return false;
	}
}

AABB RosaTools::GetBoundsForBrush( const SBrush& Brush ) const
{
	if( Brush.m_Type == EBT_Geo )
	{
		const SGeoDef&		GeoDef		= m_GeoDefs[ Brush.m_DefIndex ];
		const AABB			BrushBound	= GeoDef.m_Bounds.GetTransformedBound( Brush.m_Location, Brush.m_Orientation, Vector( Brush.m_Scale, Brush.m_Scale, Brush.m_Scale ) );
		return BrushBound;
	}
	else if( Brush.m_Type == EBT_Spawner )
	{
		const SSpawnerDef&	SpawnerDef	= m_SpawnerDefs[ Brush.m_DefIndex ];
		const AABB			BrushBound	= SpawnerDef.m_TraceExtents.GetTransformedBound( Brush.m_Location + SpawnerDef.m_Offset, Brush.m_Orientation, Vector( Brush.m_Scale, Brush.m_Scale, Brush.m_Scale ) );
		return BrushBound;
	}
	else
	{
		WARN;
		return AABB();
	}
}

void RosaTools::CacheNavMeshSnapPoints()
{
	DEVPRINTF( "Caching navmesh snap points...\n" );

	m_NavSnapPoints.Clear();

	Array<Plane> HullPlanes;
	HullPlanes.SetDeflate( false );

	static const Vector skUpVector = Vector( 0.0f, 0.0f, 1.0f );

	// ROSANOTE: We *do* want to intersect each brush with itself
	for( uint Brush0Index = 0; Brush0Index < m_Brushes.Size(); ++Brush0Index )
	{
		for( uint Brush1Index = Brush0Index; Brush1Index < m_Brushes.Size(); ++Brush1Index )
		{
			const uint Brush2Lo = ( Brush0Index == Brush1Index ) ? Brush1Index : ( Brush1Index + 1 );
			for( uint Brush2Index = Brush2Lo; Brush2Index < m_Brushes.Size(); ++Brush2Index )
			{
				const SBrush& Brush0 = m_Brushes[ Brush0Index ];
				const SBrush& Brush1 = m_Brushes[ Brush1Index ];
				const SBrush& Brush2 = m_Brushes[ Brush2Index ];

				// Skip anything that shouldn't provide snap points, including hidden brushes
				if( !DoesBrushProvideNavMeshSnapPoints( Brush0 ) ||
					!DoesBrushProvideNavMeshSnapPoints( Brush1 ) ||
					!DoesBrushProvideNavMeshSnapPoints( Brush2 ) )
				{
					continue;
				}

				const AABB Brush0Bound = GetBoundsForBrush( Brush0 );
				const AABB Brush1Bound = GetBoundsForBrush( Brush1 );
				const AABB Brush2Bound = GetBoundsForBrush( Brush2 );

				// Skip brushes if any 1 of the 3 doesn't intersect the other 2
				if( !Brush0Bound.Intersects( Brush1Bound ) ||
					!Brush0Bound.Intersects( Brush2Bound ) ||
					!Brush1Bound.Intersects( Brush2Bound ) )
				{
					continue;
				}

				HullPlanes.Clear();
				GatherHullPlanes( Brush0, HullPlanes );
				GatherHullPlanes( Brush1, HullPlanes );
				GatherHullPlanes( Brush2, HullPlanes );

				const Vector Epsilon( EPSILON, EPSILON, EPSILON );
				for( uint Plane0Index = 0; Plane0Index < HullPlanes.Size(); ++Plane0Index )
				{
					for( uint Plane1Index = Plane0Index + 1; Plane1Index < HullPlanes.Size(); ++Plane1Index )
					{
						for( uint Plane2Index = Plane0Index + 2; Plane2Index < HullPlanes.Size(); ++Plane2Index )
						{
							const Plane& Plane0 = HullPlanes[ Plane0Index ];
							const Plane& Plane1 = HullPlanes[ Plane1Index ];
							const Plane& Plane2 = HullPlanes[ Plane2Index ];

							if( Plane0.m_Normal.Dot( skUpVector ) < 0.5f &&
								Plane1.m_Normal.Dot( skUpVector ) < 0.5f &&
								Plane2.m_Normal.Dot( skUpVector ) < 0.5f )
							{
								// None of the planes are walkable, this point can be ignored for nav
								continue;
							}

							if( Equal( Square( Plane0.m_Normal.Dot( Plane1.m_Normal ) ), 1.0f, EPSILON ) ||
								Equal( Square( Plane0.m_Normal.Dot( Plane2.m_Normal ) ), 1.0f, EPSILON ) )
							{
								// Planes are parallel, no 3-way intersection
								continue;
							}

							const Line Intersection01 = Plane0.GetIntersection( Plane1 );
							if( Equal( Intersection01.m_Direction.Dot( Plane2.m_Normal ), 0.0f, EPSILON ) )
							{
								// Line is parallel with third plane, no 3-way intersection
								continue;
							}

							const Vector Intersection012 = Plane2.GetIntersection( Intersection01 );

							// Expand brushes a bit to deal with rounding errors
							AABB ExpBrush0Bound = Brush0Bound; ExpBrush0Bound.ExpandBy( Epsilon );
							AABB ExpBrush1Bound = Brush1Bound; ExpBrush1Bound.ExpandBy( Epsilon );
							AABB ExpBrush2Bound = Brush2Bound; ExpBrush2Bound.ExpandBy( Epsilon );
							if( !ExpBrush0Bound.Intersects( Intersection012 ) ||
								!ExpBrush1Bound.Intersects( Intersection012 ) ||
								!ExpBrush2Bound.Intersects( Intersection012 ) )
							{
								// Intersection point is outside bounds of a brush, no intersection
								continue;
							}

							bool AddIntersection = true;
							FOR_EACH_ARRAY( IntersectionIter, m_NavSnapPoints, Vector )
							{
								const Vector& Intersection = IntersectionIter.GetValue();
								if( Intersection.Equals( Intersection012, EPSILON ) )
								{
									AddIntersection = false;
									break;
								}
							}
							if( !AddIntersection )
							{
								// Point is already in snap points cache
								continue;
							}

							m_NavSnapPoints.PushBack( Intersection012 );
						}
					}
				}
			}
		}
	}

	DEVPRINTF( "%d nav snap points\n", m_NavSnapPoints.Size() );
}

void RosaTools::GatherHullPlanes( const SBrush& Brush, Array<Plane>& Planes )
{
	if( Brush.m_Type == EBT_Geo )
	{
		const SGeoDef& BrushDef = m_GeoDefs[ Brush.m_DefIndex ];

		FOR_EACH_ARRAY( HullIter, BrushDef.m_Hulls, SConvexHull )
		{
			const SConvexHull&	Hull			= HullIter.GetValue();

			// Skip non-entity-collidable hulls (since we're only currently
			// using this to gather planes for navmesh snap points)
			if( 0 == ( Hull.m_CollisionFlags & EECF_BlocksEntities ) ||
				0 == ( Hull.m_CollisionFlags & EECF_BlocksNav ) )
			{
				continue;
			}

			ConvexHull			TransformedHull	= Hull.m_Hull;
			TransformedHull.MoveBy( Brush.m_Location, Brush.m_Orientation, Brush.m_Scale );

			FOR_EACH_ARRAY( HullPlaneIter, TransformedHull.GetPlanes(), Plane )
			{
				const Plane& HullPlane = HullPlaneIter.GetValue();

				bool AddPlane = true;
				FOR_EACH_ARRAY( ExistingPlaneIter, Planes, Plane )
				{
					const Plane& ExistingPlane = ExistingPlaneIter.GetValue();
					if( ExistingPlane.Equals( HullPlane, EPSILON ) )
					{
						AddPlane = false;
						break;
					}
				}

				if( AddPlane )
				{
					Planes.PushBack( HullPlane );
				}
			}
		}
	}
	else if( Brush.m_Type == EBT_Spawner )
	{
		const SSpawnerDef&	SpawnerDef	= m_SpawnerDefs[ Brush.m_DefIndex ];
		const AABB			BrushBound	= SpawnerDef.m_TraceExtents.GetTransformedBound( Brush.m_Location + SpawnerDef.m_Offset, Brush.m_Orientation, Vector( Brush.m_Scale, Brush.m_Scale, Brush.m_Scale ) );
		Planes.PushBack( Plane( Vector(  1.0f,  0.0f,  0.0f ), -BrushBound.m_Max.x ) );
		Planes.PushBack( Plane( Vector( -1.0f,  0.0f,  0.0f ),  BrushBound.m_Min.x ) );
		Planes.PushBack( Plane( Vector(  0.0f,  1.0f,  0.0f ), -BrushBound.m_Max.y ) );
		Planes.PushBack( Plane( Vector(  0.0f, -1.0f,  0.0f ),  BrushBound.m_Min.y ) );
		Planes.PushBack( Plane( Vector(  0.0f,  0.0f,  1.0f ), -BrushBound.m_Max.z ) );
		Planes.PushBack( Plane( Vector(  0.0f,  0.0f, -1.0f ),  BrushBound.m_Min.z ) );
	}
	else
	{
		WARN;
	}
}

void RosaTools::TickElementsInput( SElements& Elements )
{
	Keyboard* const pKeyboard = m_Framework->GetKeyboard();
	Mouse* const pMouse = m_Framework->GetMouse();

	if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
	{
		const uint NumCategoryButtons = Elements.m_CategoryButtons.Size();
		for( uint CategoryIndex = 0; CategoryIndex < NumCategoryButtons; ++CategoryIndex )
		{
			const SElementCategoryButton& CategoryButton = Elements.m_CategoryButtons[ CategoryIndex ];

			int X, Y;
			pMouse->GetPosition( X, Y, m_Framework->GetWindow() );

			const float fX = static_cast<float>( X );
			const float fY = static_cast<float>( Y );

			if( fX >= CategoryButton.m_BoxMin.x &&
				fY >= CategoryButton.m_BoxMin.y &&
				fX <= CategoryButton.m_BoxMax.x &&
				fY <= CategoryButton.m_BoxMax.y )
			{
				// Clicked
				Elements.m_CurrentCategoryIndex = CategoryButton.m_CategoryIndex;
			}
		}

		const uint NumButtons = Elements.m_Buttons.Size();
		for( uint ButtonIndex = 0; ButtonIndex < NumButtons; ++ButtonIndex )
		{
			const SElementButton& Button = Elements.m_Buttons[ ButtonIndex ];

			const uint DefIndex = Button.m_DefIndex;
			const uint CategoryIndex = Elements.m_CategoryMap[ DefIndex ];
			if( CategoryIndex != Elements.m_CurrentCategoryIndex )
			{
				continue;
			}

			int X, Y;
			pMouse->GetPosition( X, Y, m_Framework->GetWindow() );

			const float fX = static_cast<float>( X );
			const float fY = static_cast<float>( Y );

			if( fX >= Button.m_BoxMin.x &&
				fY >= Button.m_BoxMin.y &&
				fX <= Button.m_BoxMax.x &&
				fY <= Button.m_BoxMax.y )
			{
				// Clicked
				Elements.m_CurrentDefIndex = DefIndex;
				SetSubTool( IsInRoomMode() ? EST_RoomMode : EST_WorldMode, false );
			}
		}
	}
}

void RosaTools::SetSubTool( const ESubTool SubTool, const bool ReinitGridMeshes )
{
	m_SubTool = SubTool;

	if( m_SubTool == EST_NavMesh )
	{
		CacheNavMeshSnapPoints();
		CreateNavSnapMesh();

		ReduceNavElements();
		CreateNavMesh();
	}

	if( ReinitGridMeshes )
	{
		InitializeGridMeshes();
	}

	InitializePortalMesh();
	InitializeHelpMesh();
}

void RosaTools::SetMeshSubTool_NoCache()
{
	m_SubTool = EST_NavMesh;

	ReduceNavElements();
	CreateNavMesh();
}

void RosaTools::SetCameraTransform( const Vector& Location, const Angles& Orientation )
{
	m_CameraLocation	= Location;
	m_CameraOrientation	= Orientation;
}

void RosaTools::TickCameraInput( const float DeltaTime )
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();
	Mouse* const	pMouse		= m_Framework->GetMouse();

	Vector X, Y, Z;
	m_CameraOrientation.GetAxes( X, Y, Z );

	Z = Vector( 0.0f, 0.0f, 1.0f );

	static const float kNormalVelocity	= 6.0f;
	static const float kFastVelocity	= 16.0f;
	m_CameraSpeed = pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) ? kFastVelocity : kNormalVelocity;

	if( pKeyboard->IsHigh( Keyboard::EB_W ) )
	{
		m_CameraVelocity += Y;
	}

	if( pKeyboard->IsHigh( Keyboard::EB_S ) && !pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
	{
		m_CameraVelocity -= Y;
	}

	if( pKeyboard->IsHigh( Keyboard::EB_A ) )
	{
		m_CameraVelocity -= X;
	}

	if( pKeyboard->IsHigh( Keyboard::EB_D ) )
	{
		m_CameraVelocity += X;
	}

	if( pKeyboard->IsHigh( Keyboard::EB_Q ) )
	{
		m_CameraVelocity += Z;
	}

	if( pKeyboard->IsHigh( Keyboard::EB_E ) )
	{
		m_CameraVelocity -= Z;
	}

	// Enable mouse while moving or holding Shift. Feels surprisingly natural so far.
	const bool AllowTurning = pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) || !m_CameraVelocity.IsZero();
	pMouse->SetActive( AllowTurning );

	// Normalize to 60fps delta a la InputSystem
	static const float	skBaseFPS			= 60.0f;
	const float			kMouseAdjustment	= ( DeltaTime > 0.0f ) ? ( 1.0f / ( skBaseFPS * DeltaTime ) ) : 1.0f;
	STATICHASH( MouseSpeed );
	const float MouseSpeed = ConfigManager::GetFloat( sMouseSpeed );
	m_CameraRotationalVelocity.Yaw		-= MouseSpeed * kMouseAdjustment * pMouse->GetPosition( Mouse::EA_X );
	m_CameraRotationalVelocity.Pitch	-= MouseSpeed * kMouseAdjustment * pMouse->GetPosition( Mouse::EA_Y );
}

void RosaTools::TickGridInput()
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();

	// Numpad *: toggle grid
	if( pKeyboard->OnRise( Keyboard::EB_NumMultiply ) )
	{
		m_GridActive = !m_GridActive;

		InitializeHelpMesh();	// Reprint meshes with new grid size
	}

	// Numpad +/-: Adjust grid size (hold Shift to adjust rotate grid)
	if( pKeyboard->OnRise( Keyboard::EB_NumAdd ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			m_RotateGridSize *= 2.0f;
		}
		else
		{
			m_GridSize *= 2.0f;
		}

		InitializeHelpMesh();	// Reprint meshes with new grid size
	}
	if( pKeyboard->OnRise( Keyboard::EB_NumSubtract ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			m_RotateGridSize /= 2.0f;
		}
		else
		{
			m_GridSize /= 2.0f;
		}

		InitializeHelpMesh();	// Reprint meshes with new grid size
	}
}

void RosaTools::TickUndoInput()
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();

	// Ctrl+Z: Undo
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) && pKeyboard->OnRise( Keyboard::EB_Z ) )
	{
		TryUndo();
	}

	// Ctrl+Y: Redo
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) && pKeyboard->OnRise( Keyboard::EB_Y ) )
	{
		TryRedo();
	}
}

void RosaTools::TickSaveLoadInput()
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();

	if( pKeyboard->OnRise( Keyboard::EB_S ) && pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
	{
		TryQuickSave();
	}

	if( pKeyboard->OnRise( Keyboard::EB_F6 ) )
	{
		TrySave();
	}

	if( pKeyboard->OnRise( Keyboard::EB_F9 ) )
	{
		TryLoad();
	}

	if( pKeyboard->OnRise( Keyboard::EB_F7 ) )
	{
		TryClear();
	}

	if( IsInWorldMode() && pKeyboard->OnRise( Keyboard::EB_L ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			const int Response = ConditionalMessageBox( "Convert world to room?" );
			if( Response == IDYES )
			{
				ConvertWorldToRoom();
			}
		}
		else if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			const int Response = ConditionalMessageBox( "Load world from worldgen?" );
			if( Response == IDYES )
			{
				LoadWorldFromStoredModules();
			}
		}
		else
		{
			if( !m_SelectedBrushes.Empty() )
			{
				const SBrush&		Brush		= m_Brushes[ m_SelectedBrushes[0] ];
				DEVASSERT( Brush.m_Type == EBT_Room );
				const SRoomDef&		RoomDef		= m_RoomDefs[ Brush.m_DefIndex ];

				const SimpleString	Message		= SimpleString::PrintF( "Load %s?", RoomDef.m_DefName.CStr() );
				const int			Response	= ConditionalMessageBox( Message );
				if( Response == IDYES )
				{
					SetSubTool( EST_RoomMode, false );
					Load( RoomDef.m_DefName );
				}
			}
		}
	}
}

void RosaTools::FindNavSnapPoint( const bool ShouldSnapToGrid )
{
	Mouse* const pMouse = m_Framework->GetMouse();

	int X, Y;
	pMouse->GetPosition( X, Y, m_Framework->GetWindow() );

	if( ShouldSnapToGrid )
	{
		const Segment TraceSegment = Unproject( X, Y );

		CollisionInfo Info;
		if( TraceGeo( TraceSegment, Info, true ) && !Info.m_Out_Plane.m_Normal.IsZero() )
		{
			const float		AbsX			= Abs( Info.m_Out_Plane.m_Normal.x );
			const float		AbsY			= Abs( Info.m_Out_Plane.m_Normal.y );
			const float		AbsZ			= Abs( Info.m_Out_Plane.m_Normal.z );
			Vector			PrimaryAxis;
			if( AbsX > AbsY && AbsX > AbsZ )
			{
				PrimaryAxis = Vector( Sign( Info.m_Out_Plane.m_Normal.x ), 0.0f, 0.0f );
			}
			else if( AbsY > AbsZ )
			{
				PrimaryAxis = Vector( 0.0f, Sign( Info.m_Out_Plane.m_Normal.y ), 0.0f );
			}
			else
			{
				PrimaryAxis = Vector( 0.0f, 0.0f, Sign( Info.m_Out_Plane.m_Normal.z ) );
			}

			const Vector	SnappedToGrid	= SnapToGrid( Info.m_Out_Intersection );
			const Vector	OnPlane			= Line( SnappedToGrid, PrimaryAxis ).GetIntersection( Info.m_Out_Plane );

			m_NavSnapPoint = OnPlane;
		}
	}
	else
	{
		float Nearest = FLT_MAX;
		FOR_EACH_ARRAY( IntIter, m_NavSnapPoints, Vector )
		{
			const Vector	Intersection	= IntIter.GetValue();
			int				NavScreenX		= 0;
			int				NavScreenY		= 0;
			if( !Project( Intersection, NavScreenX, NavScreenY ) )
			{
				continue;
			}
			const float		DistSq			= GetScreenDistSq( X, Y, NavScreenX, NavScreenY );
			if( DistSq < Nearest )
			{
				Nearest = DistSq;
				m_NavSnapPoint = Intersection;
			}
		}
	}
}

uint RosaTools::GetNearestEdge( const SNavFace& Face, const SNavVert& Vert ) const
{
	const float DistanceSqA = GetNavEdgeSegment( m_NavEdges[ Face.m_EdgeA ] ).DistanceSqTo( Vert.m_Vert );
	const float DistanceSqB = GetNavEdgeSegment( m_NavEdges[ Face.m_EdgeB ] ).DistanceSqTo( Vert.m_Vert );
	const float DistanceSqC = GetNavEdgeSegment( m_NavEdges[ Face.m_EdgeC ] ).DistanceSqTo( Vert.m_Vert );

	if( DistanceSqA < DistanceSqB && DistanceSqA < DistanceSqC )
	{
		return Face.m_EdgeA;
	}
	else if( DistanceSqB < DistanceSqC )
	{
		return Face.m_EdgeB;
	}
	else
	{
		return Face.m_EdgeC;
	}
}

void RosaTools::ConnectVertToVert( const uint VertAIndex, const uint VertBIndex )
{
	ASSERT( VertAIndex < m_NavVerts.Size() );
	ASSERT( VertBIndex < m_NavVerts.Size() );

	const uint	EdgeIndex	= m_NavEdges.Size();
	SNavEdge&	Edge		= m_NavEdges.PushBack();

	Edge.m_VertA			= VertAIndex;
	Edge.m_VertB			= VertBIndex;

	DeselectAllNav();
	SelectNavVert( VertAIndex );
	SelectNavVert( VertBIndex );
	SelectNavEdge( EdgeIndex );
}

void RosaTools::ConnectVerts( const uint VertAIndex, const uint VertBIndex, const uint VertCIndex )
{
	ASSERT( VertAIndex < m_NavVerts.Size() );
	ASSERT( VertBIndex < m_NavVerts.Size() );
	ASSERT( VertCIndex < m_NavVerts.Size() );

	const uint	EdgeAIndex	= m_NavEdges.Size();
	const uint	EdgeBIndex	= EdgeAIndex + 1;
	const uint	EdgeCIndex	= EdgeAIndex + 2;
	const uint	FaceIndex	= m_NavFaces.Size();

	// Push, *then* get references so resizing the array doesn't reseat references
	m_NavEdges.PushBack();
	m_NavEdges.PushBack();
	m_NavEdges.PushBack();

	SNavEdge&	EdgeA		= m_NavEdges[ EdgeAIndex ];
	SNavEdge&	EdgeB		= m_NavEdges[ EdgeBIndex ];
	SNavEdge&	EdgeC		= m_NavEdges[ EdgeCIndex ];
	SNavFace&	Face		= m_NavFaces.PushBack();

	EdgeA.m_VertA			= VertAIndex;
	EdgeA.m_VertB			= VertBIndex;
	EdgeB.m_VertA			= VertBIndex;
	EdgeB.m_VertB			= VertCIndex;
	EdgeC.m_VertA			= VertCIndex;
	EdgeC.m_VertB			= VertAIndex;
	Face.m_EdgeA			= EdgeAIndex;
	Face.m_EdgeB			= EdgeBIndex;
	Face.m_EdgeC			= EdgeCIndex;

	DeselectAllNav();
	SelectNavVert( VertAIndex );
	SelectNavVert( VertBIndex );
	SelectNavVert( VertAIndex );
	SelectNavEdge( EdgeAIndex );
	SelectNavEdge( EdgeBIndex );
	SelectNavEdge( EdgeCIndex );
	SelectNavFace( FaceIndex );
}

void RosaTools::ConnectVertToEdge( const uint VertIndex, const uint EdgeIndex )
{
	ASSERT( VertIndex < m_NavVerts.Size() );
	ASSERT( EdgeIndex < m_NavEdges.Size() );

	const uint	EdgeBIndex	= m_NavEdges.Size();
	const uint	EdgeCIndex	= EdgeBIndex + 1;
	const uint	FaceIndex	= m_NavFaces.Size();

	// Push, *then* get references so resizing the array doesn't reseat references
	m_NavEdges.PushBack();
	m_NavEdges.PushBack();

	SNavEdge&	EdgeA		= m_NavEdges[ EdgeIndex ];
	SNavEdge&	EdgeB		= m_NavEdges[ EdgeBIndex ];
	SNavEdge&	EdgeC		= m_NavEdges[ EdgeCIndex ];
	SNavFace&	Face		= m_NavFaces.PushBack();

	EdgeB.m_VertA			= EdgeA.m_VertA;
	EdgeB.m_VertB			= VertIndex;
	EdgeC.m_VertA			= EdgeA.m_VertB;
	EdgeC.m_VertB			= VertIndex;
	Face.m_EdgeA			= EdgeIndex;
	Face.m_EdgeB			= EdgeBIndex;
	Face.m_EdgeC			= EdgeCIndex;

	DeselectAllNav();
	SelectNavVert( EdgeA.m_VertA );
	SelectNavVert( EdgeA.m_VertB );
	SelectNavVert( VertIndex );
	SelectNavEdge( EdgeIndex );
	SelectNavEdge( EdgeBIndex );
	SelectNavEdge( EdgeCIndex );
	SelectNavFace( FaceIndex );
}

void RosaTools::ConnectEdges( const uint EdgeAIndex, const uint EdgeBIndex, const uint EdgeCIndex )
{
	ASSERT( EdgeAIndex < m_NavEdges.Size() );
	ASSERT( EdgeBIndex < m_NavEdges.Size() );
	ASSERT( EdgeCIndex < m_NavEdges.Size() );

	const uint	FaceIndex	= m_NavFaces.Size();
	SNavFace&	Face		= m_NavFaces.PushBack();

	Face.m_EdgeA			= EdgeAIndex;
	Face.m_EdgeB			= EdgeBIndex;
	Face.m_EdgeC			= EdgeCIndex;

	DeselectAllNav();
	SelectNavEdge( EdgeAIndex );
	SelectNavEdge( EdgeBIndex );
	SelectNavEdge( EdgeCIndex );
	SelectNavFace( FaceIndex );
}

void RosaTools::ConnectEdges( const uint EdgeAIndex, const uint EdgeBIndex )
{
	ASSERT( EdgeAIndex < m_NavEdges.Size() );
	ASSERT( EdgeBIndex < m_NavEdges.Size() );

	uint UnsharedVertA, UnsharedVertB;
	if( !GetUnsharedVerts( m_NavEdges[ EdgeAIndex ], m_NavEdges[ EdgeBIndex ], UnsharedVertA, UnsharedVertB ) )
	{
		return;
	}

	const uint	EdgeCIndex	= m_NavEdges.Size();
	const uint	FaceIndex	= m_NavFaces.Size();

	SNavEdge&	EdgeC		= m_NavEdges.PushBack();
	SNavFace&	Face		= m_NavFaces.PushBack();

	EdgeC.m_VertA			= UnsharedVertA;
	EdgeC.m_VertB			= UnsharedVertB;
	Face.m_EdgeA			= EdgeAIndex;
	Face.m_EdgeB			= EdgeBIndex;
	Face.m_EdgeC			= EdgeCIndex;

	DeselectAllNav();
	SelectNavVert( UnsharedVertA );
	SelectNavVert( UnsharedVertB );
	SelectNavEdge( EdgeCIndex );
	SelectNavFace( FaceIndex );
}

bool RosaTools::GetUnsharedVerts( const SNavEdge& EdgeA, const SNavEdge& EdgeB, uint& OutVertA, uint& OutVertB )
{
	if( EdgeA.m_VertA == EdgeB.m_VertA )
	{
		OutVertA	= EdgeA.m_VertB;
		OutVertB	= EdgeB.m_VertB;
		return true;
	}

	if( EdgeA.m_VertA == EdgeB.m_VertB )
	{
		OutVertA	= EdgeA.m_VertB;
		OutVertB	= EdgeB.m_VertA;
		return true;
	}

	if( EdgeA.m_VertB == EdgeB.m_VertA )
	{
		OutVertA	= EdgeA.m_VertA;
		OutVertB	= EdgeB.m_VertB;
		return true;
	}

	if( EdgeA.m_VertB == EdgeB.m_VertB )
	{
		OutVertA	= EdgeA.m_VertA;
		OutVertB	= EdgeB.m_VertA;
		return true;
	}

	return false;
}

bool RosaTools::GetNearestNavElement( const int ScreenX, const int ScreenY, uint& OutNavIndex, uint& OutNavType ) const
{
	float MinDistSq = FLT_MAX;

	FOR_EACH_ARRAY( VertIter, m_NavVerts, SNavVert )
	{
		const SNavVert&	Vert		= VertIter.GetValue();

		int				NavScreenX	= 0;
		int				NavScreenY	= 0;
		if( !Project( Vert.m_Vert, NavScreenX, NavScreenY ) )
		{
			continue;
		}

		const float		DistSq		= GetScreenDistSq( ScreenX, ScreenY, NavScreenX, NavScreenY );
		if( DistSq < MinDistSq )
		{
			MinDistSq	= DistSq;
			OutNavIndex	= VertIter.GetIndex();
			OutNavType	= ENT_Vert;
		}
	}

	const Vector ScreenLocation = Vector( static_cast<float>( ScreenX ), static_cast<float>( ScreenY ), 0.0f );
	FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
	{
		const SNavEdge&	Edge			= EdgeIter.GetValue();
		const Segment	Span			= GetNavEdgeSegment( Edge );
		// HACKHACK: Test against a span half the size of the edge segment, so we can still select verts.
		const Vector	SpanOffset		= Span.m_Point2 - Span.m_Point1;
		const Segment	ModifiedSpan	= Segment( Span.m_Point1 + SpanOffset * 0.25f, Span.m_Point1 + SpanOffset * 0.75f );

		// Both parts of the segment need to be in view, this should be fine
		int				SpanScreenXA	= 0;
		int				SpanScreenYA	= 0;
		if( !Project( ModifiedSpan.m_Point1, SpanScreenXA, SpanScreenYA ) )
		{
			continue;
		}

		int				SpanScreenXB	= 0;
		int				SpanScreenYB	= 0;
		if( !Project( ModifiedSpan.m_Point2, SpanScreenXB, SpanScreenYB ) )
		{
			continue;
		}

		const Vector	SpanScreenA		= Vector( static_cast<float>( SpanScreenXA ), static_cast<float>( SpanScreenYA ), 0.0f );
		const Vector	SpanScreenB		= Vector( static_cast<float>( SpanScreenXB ), static_cast<float>( SpanScreenYB ), 0.0f );
		const Segment	ScreenSpan		= Segment( SpanScreenA, SpanScreenB );
		const float		DistSq			= ScreenSpan.DistanceSqTo( ScreenLocation );
		if( DistSq < MinDistSq )
		{
			MinDistSq	= DistSq;
			OutNavIndex	= EdgeIter.GetIndex();
			OutNavType	= ENT_Edge;
		}
	}

	FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
	{
		const SNavFace&	Face		= FaceIter.GetValue();
		const Vector	Centroid	= GetNavFaceCentroid( Face );

		int				NavScreenX	= 0;
		int				NavScreenY	= 0;
		if( !Project( Centroid, NavScreenX, NavScreenY ) )
		{
			continue;
		}

		const float		DistSq		= GetScreenDistSq( ScreenX, ScreenY, NavScreenX, NavScreenY );
		if( DistSq < MinDistSq )
		{
			MinDistSq	= DistSq;
			OutNavIndex	= FaceIter.GetIndex();
			OutNavType	= ENT_Face;
		}
	}

	return MinDistSq < FLT_MAX;
}

Vector RosaTools::GetNavEdgeCentroid( const SNavEdge& Edge ) const
{
	const Vector&	VertA		= m_NavVerts[ Edge.m_VertA ].m_Vert;
	const Vector&	VertB		= m_NavVerts[ Edge.m_VertB ].m_Vert;
	const Vector	Centroid	= ( VertA + VertB ) / 2.0f;
	return Centroid;
}

Segment RosaTools::GetNavEdgeSegment( const SNavEdge& Edge ) const
{
	const Vector&	VertA		= m_NavVerts[ Edge.m_VertA ].m_Vert;
	const Vector&	VertB		= m_NavVerts[ Edge.m_VertB ].m_Vert;
	return Segment( VertA, VertB );
}

Vector RosaTools::GetNavFaceCentroid( const SNavFace& Face ) const
{
	// Because we don't know the direction of edges, add all vertices and divide by 6 instead of 3
	const SNavEdge&	EdgeA		= m_NavEdges[ Face.m_EdgeA ];
	const SNavEdge&	EdgeB		= m_NavEdges[ Face.m_EdgeB ];
	const SNavEdge&	EdgeC		= m_NavEdges[ Face.m_EdgeC ];
	const Vector&	VertAA		= m_NavVerts[ EdgeA.m_VertA ].m_Vert;
	const Vector&	VertAB		= m_NavVerts[ EdgeA.m_VertB ].m_Vert;
	const Vector&	VertBA		= m_NavVerts[ EdgeB.m_VertA ].m_Vert;
	const Vector&	VertBB		= m_NavVerts[ EdgeB.m_VertB ].m_Vert;
	const Vector&	VertCA		= m_NavVerts[ EdgeC.m_VertA ].m_Vert;
	const Vector&	VertCB		= m_NavVerts[ EdgeC.m_VertB ].m_Vert;
	const Vector	Centroid	= ( VertAA + VertAB + VertBA + VertBB + VertCA + VertCB ) / 6.0f;
	return Centroid;
}

void RosaTools::DeleteSelectedNavElements()
{
	Array<uint>	VertsToDelete	= m_SelectedNavVerts;
	Array<uint>	EdgesToDelete	= m_SelectedNavEdges;
	Array<uint> FacesToDelete	= m_SelectedNavFaces;

	m_SelectedNavVerts.Clear();
	m_SelectedNavEdges.Clear();
	m_SelectedNavFaces.Clear();

	// Delete any edges that reference deleted verts
	FOR_EACH_ARRAY( VertIter, VertsToDelete, uint )
	{
		const uint DeletedVert = VertIter.GetValue();
		FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
		{
			const SNavEdge& Edge = EdgeIter.GetValue();
			if( Edge.m_VertA == DeletedVert ||
				Edge.m_VertB == DeletedVert )
			{
				EdgesToDelete.PushBackUnique( EdgeIter.GetIndex() );
			}
		}
	}

	// Delete any tris that reference deleted edges
	FOR_EACH_ARRAY( EdgeIter, EdgesToDelete, uint )
	{
		const uint DeletedEdge = EdgeIter.GetValue();
		FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
		{
			const SNavFace& Face = FaceIter.GetValue();
			if( Face.m_EdgeA == DeletedEdge ||
				Face.m_EdgeB == DeletedEdge ||
				Face.m_EdgeC == DeletedEdge )
			{
				FacesToDelete.PushBackUnique( FaceIter.GetIndex() );
			}
		}
	}

	// Delete any edges referenced by a deleted tri that are not referenced by any undeleted tri
	FOR_EACH_ARRAY( FaceIter, FacesToDelete, uint )
	{
		const SNavFace&	DeletedFace	= m_NavFaces[ FaceIter.GetValue() ];
		bool			HeldRefs	= false;
		FOR_EACH_ARRAY( NavFaceIter, m_NavFaces, SNavFace )
		{
			const SNavFace& NavFace = NavFaceIter.GetValue();
			if( FacesToDelete.Find( NavFaceIter.GetIndex() ) )
			{
				continue;
			}

			if( FaceHasEdge( NavFace, DeletedFace.m_EdgeA ) ||
				FaceHasEdge( NavFace, DeletedFace.m_EdgeB ) ||
				FaceHasEdge( NavFace, DeletedFace.m_EdgeC ) )
			{
				HeldRefs = true;
				break;
			}
		}

		if( !HeldRefs )
		{
			EdgesToDelete.PushBackUnique( DeletedFace.m_EdgeA );
			EdgesToDelete.PushBackUnique( DeletedFace.m_EdgeB );
			EdgesToDelete.PushBackUnique( DeletedFace.m_EdgeC );
		}
	}

	// Delete any verts referenced by a deleted edge that are not referenced by any undeleted edge
	FOR_EACH_ARRAY( EdgeIter, EdgesToDelete, uint )
	{
		const SNavEdge&	DeletedEdge	= m_NavEdges[ EdgeIter.GetValue() ];
		bool			HeldRefs	= false;
		FOR_EACH_ARRAY( NavEdgeIter, m_NavEdges, SNavEdge )
		{
			const SNavEdge& NavEdge = NavEdgeIter.GetValue();
			if( EdgesToDelete.Find( NavEdgeIter.GetIndex() ) )
			{
				continue;
			}

			if( EdgeHasVert( NavEdge, DeletedEdge.m_VertA ) ||
				EdgeHasVert( NavEdge, DeletedEdge.m_VertB ) )
			{
				HeldRefs = true;
				break;
			}
		}

		if( !HeldRefs )
		{
			VertsToDelete.PushBackUnique( DeletedEdge.m_VertA );
			VertsToDelete.PushBackUnique( DeletedEdge.m_VertB );
		}
	}

	// Finally, delete all gathered elements, in faces-edges-verts order
	// so references get fixed up.
	FacesToDelete.QuickSort();
	FOR_EACH_ARRAY_REVERSE( FaceIter, FacesToDelete, uint )
	{
		DeleteNavFace( FaceIter.GetValue() );
	}

	EdgesToDelete.QuickSort();
	FOR_EACH_ARRAY_REVERSE( EdgeIter, EdgesToDelete, uint )
	{
		DeleteNavEdge( EdgeIter.GetValue() );
	}

	VertsToDelete.QuickSort();
	FOR_EACH_ARRAY_REVERSE( VertIter, VertsToDelete, uint )
	{
		DeleteNavVert( VertIter.GetValue() );
	}
}

void RosaTools::DeleteNavVert( const uint VertIndex )
{
	ASSERT( VertIndex < m_NavVerts.Size() );

	m_NavVerts.Remove( VertIndex );

	FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
	{
		SNavEdge& Edge = EdgeIter.GetValue();
		DEBUGASSERT( Edge.m_VertA != VertIndex );
		DEBUGASSERT( Edge.m_VertB != VertIndex );
		if( Edge.m_VertA > VertIndex ) { Edge.m_VertA--; }
		if( Edge.m_VertB > VertIndex ) { Edge.m_VertB--; }
	}

	FOR_EACH_ARRAY( SelectedVertIter, m_SelectedNavVerts, uint )
	{
		uint& SelectedVertIndex = SelectedVertIter.GetValue();
		DEBUGASSERT( SelectedVertIndex != VertIndex );
		if( SelectedVertIndex > VertIndex )
		{
			SelectedVertIndex--;
		}
	}
}

void RosaTools::DeleteNavEdge( const uint EdgeIndex )
{
	ASSERT( EdgeIndex < m_NavEdges.Size() );

	m_NavEdges.Remove( EdgeIndex );

	FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
	{
		SNavFace& Face = FaceIter.GetValue();
		DEBUGASSERT( Face.m_EdgeA != EdgeIndex );
		DEBUGASSERT( Face.m_EdgeB != EdgeIndex );
		DEBUGASSERT( Face.m_EdgeC != EdgeIndex );
		if( Face.m_EdgeA > EdgeIndex ) { Face.m_EdgeA--; }
		if( Face.m_EdgeB > EdgeIndex ) { Face.m_EdgeB--; }
		if( Face.m_EdgeC > EdgeIndex ) { Face.m_EdgeC--; }
	}

	FOR_EACH_ARRAY( SelectedEdgeIter, m_SelectedNavEdges, uint )
	{
		uint& SelectedEdgeIndex = SelectedEdgeIter.GetValue();
		DEBUGASSERT( SelectedEdgeIndex != EdgeIndex );
		if( SelectedEdgeIndex > EdgeIndex )
		{
			SelectedEdgeIndex--;
		}
	}
}

void RosaTools::DeleteNavFace( const uint FaceIndex )
{
	ASSERT( FaceIndex < m_NavFaces.Size() );

	m_NavFaces.Remove( FaceIndex );

	FOR_EACH_ARRAY( SelectedFaceIter, m_SelectedNavFaces, uint )
	{
		uint& SelectedFaceIndex = SelectedFaceIter.GetValue();
		DEBUGASSERT( SelectedFaceIndex != FaceIndex );
		if( SelectedFaceIndex > FaceIndex )
		{
			SelectedFaceIndex--;
		}
	}
}

bool RosaTools::EdgeHasVert( const SNavEdge& Edge, const uint VertIndex ) const
{
	ASSERT( VertIndex < m_NavVerts.Size() );

	return
		Edge.m_VertA == VertIndex ||
		Edge.m_VertB == VertIndex;
}

bool RosaTools::FaceHasEdge( const SNavFace& Face, const uint EdgeIndex ) const
{
	ASSERT( EdgeIndex < m_NavEdges.Size() );

	return
		Face.m_EdgeA == EdgeIndex ||
		Face.m_EdgeB == EdgeIndex ||
		Face.m_EdgeC == EdgeIndex;
}

void RosaTools::ReduceNavElements()
{
	Array<uint> VertsToDelete;
	Array<uint> EdgesToDelete;
	Array<uint> FacesToDelete;

	for( uint VertIndexA = 0; VertIndexA < m_NavVerts.Size(); ++VertIndexA )
	{
		for( uint VertIndexB = VertIndexA + 1; VertIndexB < m_NavVerts.Size(); ++VertIndexB )
		{
			const Vector& VertA = m_NavVerts[ VertIndexA ].m_Vert;
			const Vector& VertB = m_NavVerts[ VertIndexB ].m_Vert;

			// DLP 2 Aug 2020: This used to be a != check, I'm switching to an epsilon
			// for reducing nav elements after converting a world to room, which introduces
			// some small amount of error. Using the same epsilon as RosaWorldGen::LinkNavMesh().
			static const float skMinVertDist = 0.05f;
			if( !( VertA.Equals( VertB, skMinVertDist ) ) )
			{
				continue;
			}

			VertsToDelete.PushBackUnique( VertIndexB );

			if( m_SelectedNavVerts.Find( VertIndexB ) )
			{
				m_SelectedNavVerts.FastRemoveItem( VertIndexB );
				m_SelectedNavVerts.PushBackUnique( VertIndexA );
			}

			FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge )
			{
				SNavEdge& Edge = EdgeIter.GetValue();
				if( Edge.m_VertA == VertIndexB ) { Edge.m_VertA = VertIndexA; }
				if( Edge.m_VertB == VertIndexB ) { Edge.m_VertB = VertIndexA; }
			}
		}
	}

	for( uint EdgeIndexA = 0; EdgeIndexA < m_NavEdges.Size(); ++EdgeIndexA )
	{
		for( uint EdgeIndexB = EdgeIndexA + 1; EdgeIndexB < m_NavEdges.Size(); ++EdgeIndexB )
		{
			const SNavEdge& EdgeA = m_NavEdges[ EdgeIndexA ];
			const SNavEdge& EdgeB = m_NavEdges[ EdgeIndexB ];

			if( !EdgeHasVert( EdgeB, EdgeA.m_VertA ) ||
				!EdgeHasVert( EdgeB, EdgeA.m_VertB ) )
			{
				continue;
			}

			EdgesToDelete.PushBackUnique( EdgeIndexB );

			if( m_SelectedNavEdges.Find( EdgeIndexB ) )
			{
				m_SelectedNavEdges.FastRemoveItem( EdgeIndexB );
				m_SelectedNavEdges.PushBackUnique( EdgeIndexA );
			}

			FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace )
			{
				SNavFace& Face = FaceIter.GetValue();
				if( Face.m_EdgeA == EdgeIndexB ) { Face.m_EdgeA = EdgeIndexA; }
				if( Face.m_EdgeB == EdgeIndexB ) { Face.m_EdgeB = EdgeIndexA; }
				if( Face.m_EdgeC == EdgeIndexB ) { Face.m_EdgeC = EdgeIndexA; }
			}
		}
	}

	for( uint FaceIndexA = 0; FaceIndexA < m_NavFaces.Size(); ++FaceIndexA )
	{
		for( uint FaceIndexB = FaceIndexA + 1; FaceIndexB < m_NavFaces.Size(); ++FaceIndexB )
		{
			const SNavFace& FaceA = m_NavFaces[ FaceIndexA ];
			const SNavFace& FaceB = m_NavFaces[ FaceIndexB ];

			if( !FaceHasEdge( FaceB, FaceA.m_EdgeA ) ||
				!FaceHasEdge( FaceB, FaceA.m_EdgeB ) ||
				!FaceHasEdge( FaceB, FaceA.m_EdgeC ) )
			{
				continue;
			}

			FacesToDelete.PushBackUnique( FaceIndexB );

			if( m_SelectedNavFaces.Find( FaceIndexB ) )
			{
				m_SelectedNavFaces.FastRemoveItem( FaceIndexB );
				m_SelectedNavFaces.PushBackUnique( FaceIndexA );
			}
		}
	}

	FacesToDelete.QuickSort();
	FOR_EACH_ARRAY_REVERSE( FaceIter, FacesToDelete, uint )
	{
		DeleteNavFace( FaceIter.GetValue() );
	}

	EdgesToDelete.QuickSort();
	FOR_EACH_ARRAY_REVERSE( EdgeIter, EdgesToDelete, uint )
	{
		DeleteNavEdge( EdgeIter.GetValue() );
	}

	VertsToDelete.QuickSort();
	FOR_EACH_ARRAY_REVERSE( VertIter, VertsToDelete, uint )
	{
		DeleteNavVert( VertIter.GetValue() );
	}
}

void RosaTools::TickNavMeshInput( const float DeltaTime )
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();
	Mouse* const	pMouse		= m_Framework->GetMouse();

	// ROSATODO: Organize input code better.
	if( m_IsTranslating )
	{
		const bool Vertical = pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt );
		TickTranslateNav( Vertical );

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndTranslateNav( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndTranslateNav( true );
		}

		CreateNavMesh();
		return;
	}

	if( m_IsTranslatingAnchor )
	{
		const bool Vertical = pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt );
		TickTranslateAnchor( Vertical );

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndTranslateAnchor( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndTranslateAnchor( true );
		}

		return;
	}

	if( m_IsRotating )
	{
		Vector RotateAxis = Vector( 0.0f, 0.0f, 1.0f );
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) )
		{
			// Alt: Rotate about nearer of X and Y axes.
			const Vector	Facing	= m_CameraOrientation.ToVector();
			const Vector	XAxis	= Vector( 1.0f, 0.0f, 0.0f );
			const Vector	YAxis	= Vector( 0.0f, 1.0f, 0.0f );
			const float		DotX	= Facing.Dot( XAxis );
			const float		DotY	= Facing.Dot( YAxis );
			RotateAxis				= ( Abs( DotX ) > Abs( DotY ) ) ? XAxis : YAxis;
		}

		TickRotateNav( RotateAxis );

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndRotateNav( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndRotateNav( true );
		}

		CreateNavMesh();
		return;
	}

	TickCameraInput( DeltaTime );
	TickGridInput();
	TickUndoInput();
	TickSaveLoadInput();

	if( m_NavSnapPoints.Empty() )
	{
		CacheNavMeshSnapPoints();
		CreateNavSnapMesh();
	}

	const bool ShouldSnapToGrid = pKeyboard->IsHigh( Keyboard::EB_Virtual_Control );
	FindNavSnapPoint( ShouldSnapToGrid );

	if( pKeyboard->OnRise( Keyboard::EB_G ) && !pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			TryStartTranslateAnchor();
		}
		else
		{
			TryStartTranslateNav();
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_R ) )
	{
		TryStartRotateNav();
	}

	// Alt+G: translate selected vert to cursor
	if( pKeyboard->OnRise( Keyboard::EB_G ) &&
		pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) &&
		m_SelectedNavVerts.Size() == 1 &&
		m_SelectedNavEdges.Empty() &&
		m_SelectedNavFaces.Empty() )
	{
		SNavVert&	Vert	= m_NavVerts[ m_SelectedNavVerts[0] ];
		Vert.m_Vert			= m_NavSnapPoint;
		CreateNavMesh();
		StoreUndoState();
	}

	if( pKeyboard->OnRise( Keyboard::EB_F ) )
	{
		// Fill
		if( m_SelectedNavEdges.Size() == 3 )
		{
			// Create tri from three edges
			ConnectEdges( m_SelectedNavEdges[0], m_SelectedNavEdges[1], m_SelectedNavEdges[2] );
		}
		else if( m_SelectedNavEdges.Size() == 2 )
		{
			// Create tri from two edges
			ConnectEdges( m_SelectedNavEdges[0], m_SelectedNavEdges[1] );
		}
		else if( m_SelectedNavVerts.Size() == 1 &&
			m_SelectedNavEdges.Size() == 1 )
		{
			// Connect vert to edge, creating tri
			ConnectVertToEdge( m_SelectedNavVerts[0], m_SelectedNavEdges[0] );
		}
		else if( m_SelectedNavVerts.Size() == 2 )
		{
			// Connect verts, creating edge
			ConnectVertToVert( m_SelectedNavVerts[0], m_SelectedNavVerts[1] );
		}
		else if( m_SelectedNavVerts.Size() == 3 )
		{
			// Connect verts, creating edges and tri
			ConnectVerts( m_SelectedNavVerts[0], m_SelectedNavVerts[1], m_SelectedNavVerts[2] );
		}

		ReduceNavElements();
		CreateNavMesh();
		StoreUndoState();
	}

	if( pKeyboard->OnRise( Keyboard::EB_Delete ) )
	{
		DeleteSelectedNavElements();
		CreateNavMesh();
		StoreUndoState();
	}

	// U: select all
	if( pKeyboard->OnRise( Keyboard::EB_U ) )
	{
		SelectAllNav();
		CreateNavMesh();
	}

	// I: invert selection
	if( pKeyboard->OnRise( Keyboard::EB_I ) )
	{
		Array<uint> SelectedVerts = m_SelectedNavVerts;
		Array<uint> SelectedEdges = m_SelectedNavEdges;
		Array<uint> SelectedFaces = m_SelectedNavFaces;
		SelectAllNav();
		DeselectNavVerts( SelectedVerts );
		DeselectNavEdges( SelectedEdges );
		DeselectNavFaces( SelectedFaces );
		CreateNavMesh();
	}

	// O: select none
	if( pKeyboard->OnRise( Keyboard::EB_O ) )
	{
		DeselectAllNav();
		CreateNavMesh();
	}

	if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
	{
		if( !pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			DeselectAllNav();
		}

		// Trace scene to find a thing to select
		int X, Y;
		pMouse->GetPosition( X, Y, m_Framework->GetWindow() );

		// Find and toggle selection of the nearest nav element (vert, edge, or tri) to the intersection
		uint NavIndex, NavType;
		if( GetNearestNavElement( X, Y, NavIndex, NavType ) )
		{
			if( NavType == ENT_Vert )
			{
				ToggleNavVert( NavIndex );
			}
			else if( NavType == ENT_Edge )
			{
				ToggleNavEdge( NavIndex );
			}
			else if( NavType == ENT_Face )
			{
				ToggleNavFace( NavIndex );
			}
		}

		CreateNavMesh();
	}

	if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
	{
		const uint	VertIndex	= m_NavVerts.Size();
		SNavVert&	Vert		= m_NavVerts.PushBack();
		Vert.m_Vert				= m_NavSnapPoint;

		if( m_SelectedNavFaces.Size() == 1 )
		{
			// Connect to nearest side of tri
			const uint EdgeIndex = GetNearestEdge( m_NavFaces[ m_SelectedNavFaces[0] ], Vert );
			ConnectVertToEdge( VertIndex, EdgeIndex );
		}
		else if( m_SelectedNavEdges.Size() == 1 )
		{
			// Connect to edge, creating tri
			ConnectVertToEdge( VertIndex, m_SelectedNavEdges[0] );
		}
		else if( m_SelectedNavVerts.Size() == 1 )
		{
			// Connect to vert, creating edge
			ConnectVertToVert( VertIndex, m_SelectedNavVerts[0] );
		}
		else
		{
			DeselectAllNav();
			SelectNavVert( VertIndex );
		}

		ReduceNavElements();
		CreateNavMesh();
		StoreUndoState();
	}

	// T: Tag navmesh faces
	if( pKeyboard->OnRise( Keyboard::EB_T ) )
	{
		for( uint SelectedFaceIndexIndex = 0; SelectedFaceIndexIndex < m_SelectedNavFaces.Size(); ++SelectedFaceIndexIndex )
		{
			const uint	SelectedFaceIndex	= m_SelectedNavFaces[ SelectedFaceIndexIndex ];
			SNavFace&	NavFace				= m_NavFaces[ SelectedFaceIndex ];

			// HACKHACK: For now, just cycle through these three props
			if( NavFace.m_Props == ENP_None )
			{
				NavFace.m_Props = ENP_CautiousShouldAvoid;
			}
			else if( NavFace.m_Props == ENP_CautiousShouldAvoid )
			{
				NavFace.m_Props = ENP_CautiousMustAvoid;
			}
			else
			{
				ASSERT( NavFace.m_Props == ENP_CautiousMustAvoid );
				NavFace.m_Props = ENP_None;
			}
		}

		CreateNavMesh();
		StoreUndoState();
	}
}

void RosaTools::TickNormalInput( const float DeltaTime )
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();
	Mouse* const	pMouse		= m_Framework->GetMouse();

	// ROSATODO: Organize input code better.
	if( m_IsTranslating )
	{
		const bool Vertical = pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt );
		TickTranslate( Vertical );

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndTranslate( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndTranslate( true );
		}

		return;
	}

	if( m_IsTranslatingAnchor )
	{
		const bool Vertical = pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt );
		TickTranslateAnchor( Vertical );

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndTranslateAnchor( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndTranslateAnchor( true );
		}

		return;
	}

	if( m_IsRotating )
	{
		Vector RotateAxis = Vector( 0.0f, 0.0f, 1.0f );
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) )
		{
			// Alt: Rotate about nearer of X and Y axes.
			const Vector	Facing	= m_CameraOrientation.ToVector();
			const Vector	XAxis	= Vector( 1.0f, 0.0f, 0.0f );
			const Vector	YAxis	= Vector( 0.0f, 1.0f, 0.0f );
			const float		DotX	= Facing.Dot( XAxis );
			const float		DotY	= Facing.Dot( YAxis );
			RotateAxis				= ( Abs( DotX ) > Abs( DotY ) ) ? XAxis : YAxis;
		}

		TickRotate( RotateAxis );

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndRotate( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndRotate( true );
		}

		return;
	}

	if( m_IsScaling )
	{
		TickScale();

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
		{
			EndScale( false );
		}

		if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
		{
			EndScale( true );
		}

		return;
	}

	if( m_IsBoxSelecting )
	{
		if( pKeyboard->OnFall( Keyboard::EB_Mouse_Middle ) )
		{
			EndBoxSelect();
		}
	}

	TickCameraInput( DeltaTime );
	TickGridInput();
	TickUndoInput();
	TickSaveLoadInput();

	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
	{
		if( m_SelectedBrushes.Size() == 1 && m_Brushes[ m_SelectedBrushes[0] ].m_Type == EBT_Geo )
		{
			const SBrush&	AnchorBrush		= m_Brushes[ m_SelectedBrushes[0] ];
			const SGeoDef&	AnchorGeoDef	= m_GeoDefs[ AnchorBrush.m_DefIndex ];

			// Ctrl + [/], ;/', ./?: adjust selected geo brush size to nearest linked size
			uint ConvertToLinkedGeoDef = 0xffffffff;
			if( pKeyboard->OnRise( Keyboard::EB_LeftBrace ) )
			{
				ConvertToLinkedGeoDef = AnchorGeoDef.m_LinkXNeg;
			}
			else if( pKeyboard->OnRise( Keyboard::EB_RightBrace ) )
			{
				ConvertToLinkedGeoDef = AnchorGeoDef.m_LinkXPos;
			}
			else if( pKeyboard->OnRise( Keyboard::EB_Semicolon ) )
			{
				ConvertToLinkedGeoDef = AnchorGeoDef.m_LinkYNeg;
			}
			else if( pKeyboard->OnRise( Keyboard::EB_Apostrophe ) )
			{
				ConvertToLinkedGeoDef = AnchorGeoDef.m_LinkYPos;
			}
			else if( pKeyboard->OnRise( Keyboard::EB_Period ) )
			{
				ConvertToLinkedGeoDef = AnchorGeoDef.m_LinkZNeg;
			}
			else if( pKeyboard->OnRise( Keyboard::EB_Slash ) )
			{
				ConvertToLinkedGeoDef = AnchorGeoDef.m_LinkZPos;
			}

			if( ConvertToLinkedGeoDef != 0xffffffff )
			{
				// Copied from ::TickCreateBrushInput (TODO: Consolidate)
				uint MatDefIndex;
				const bool HasMat = FindMatDef( AnchorBrush.m_Mat, &MatDefIndex );

				const Vector	Location	= AnchorBrush.m_Location;
				const Angles	Orientation	= AnchorBrush.m_Orientation;
				const float		Scale		= AnchorBrush.m_Scale;
				DeleteSelectedBrushes();
				Array<uint>		NewBrushes;
				CreateBrushes( EBT_Geo, ConvertToLinkedGeoDef, Location, Orientation, Scale, NewBrushes );
				ASSERT( NewBrushes.Size() == 1 );

				if( HasMat )
				{
					for( uint BrushIndex = 0; BrushIndex < NewBrushes.Size(); ++BrushIndex )
					{
						SBrush& NewBrush = GetBrush( NewBrushes[ BrushIndex ] );
						ApplyMatToBrush( MatDefIndex, NewBrush );
					}
				}

				SelectBrushes( NewBrushes );
				StoreUndoState();
			}
		}
	}
	else
	{
		// [/], ;/', ./?: adjust room dimensions in tiles (hold Shift to adjust meters/tile dimensions)
		const uint	MetersPerTileX	= m_MetersX / m_TilesX;
		const uint	MetersPerTileY	= m_MetersY / m_TilesY;
		const uint	MetersPerTileZ	= m_MetersZ / m_TilesZ;
		const bool	IsShiftHigh		= pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift );
		const uint	OldTilesX		= m_TilesX;
		const uint	OldTilesY		= m_TilesY;
		const uint	OldTilesZ		= m_TilesZ;
		bool		Resized			= false;
		if( pKeyboard->OnRise( Keyboard::EB_LeftBrace ) )
		{
			if( IsShiftHigh ) { if( m_MetersX > m_TilesX ) { m_MetersX -= m_TilesX; Resized = true; } }
			else { if( m_TilesX > 1 ) { m_TilesX--; m_MetersX -= MetersPerTileX; Resized = true; } }
		}
		if( pKeyboard->OnRise( Keyboard::EB_RightBrace ) )
		{
			if( IsShiftHigh ) { m_MetersX += m_TilesX; Resized = true; }
			else { m_TilesX++; m_MetersX += MetersPerTileX; Resized = true; }
		}
		if( pKeyboard->OnRise( Keyboard::EB_Semicolon ) )
		{
			if( IsShiftHigh ) { if( m_MetersY > m_TilesY ) { m_MetersY -= m_TilesY; Resized = true; } }
			else { if( m_TilesY > 1 ) { m_TilesY--; m_MetersY -= MetersPerTileY; Resized = true; } }
		}
		if( pKeyboard->OnRise( Keyboard::EB_Apostrophe ) )
		{
			if( IsShiftHigh ) { m_MetersY += m_TilesY; Resized = true; }
			else { m_TilesY++; m_MetersY += MetersPerTileY; Resized = true; }
		}
		if( pKeyboard->OnRise( Keyboard::EB_Period ) )
		{
			if( IsShiftHigh ) { if( m_MetersZ > m_TilesZ ) { m_MetersZ -= m_TilesZ; Resized = true; } }
			else { if( m_TilesZ > 1 ) { m_TilesZ--; m_MetersZ -= MetersPerTileZ; Resized = true; } }
		}
		if( pKeyboard->OnRise( Keyboard::EB_Slash ) )
		{
			if( IsShiftHigh ) { m_MetersZ += m_TilesZ; Resized = true; }
			else { m_TilesZ++; m_MetersZ += MetersPerTileZ; Resized = true; }
		}
		if( Resized )
		{
			FixUpPortalsAfterResize( OldTilesX, OldTilesY, OldTilesZ );

			InitializeGridMeshes();
			InitializePortalMesh();
			InitializeHelpMesh();
			StoreUndoState();
		}
	}

	// X: jump to traced location
	// ROSATODO: Jump to traced location (minus normal, snapped to grid, a la spawning code)
	//if( pKeyboard->OnRise( Keyboard::EB_X ) )
	//{
	//	m_CameraLocation = kVoxelHalfExtents + GetCoordsForVoxel( FacingVoxel );
	//}

	// K: print spawner manifest
	if( pKeyboard->OnRise( Keyboard::EB_K ) )
	{
		Array<SimpleString> Spawners;
		FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
		{
			const SBrush& Brush = BrushIter.GetValue();
			if( Brush.m_Type != EBT_Spawner )
			{
				continue;
			}

			const SimpleString&	SpawnerDef		= m_SpawnerDefs[ Brush.m_DefIndex ].m_DefName;
			const SimpleString	SpawnerString	= SimpleString::PrintF( "%s %s", SpawnerDef.CStr(), Brush.m_Location.GetString().CStr() );
			Spawners.PushBack( SpawnerString );
		}
		Spawners.InsertionSort();

		PRINTF( "Spawner manifest\n" );
		PRINTF( "%d spawners\n", Spawners.Size() );
		FOR_EACH_ARRAY( SpawnerIter, Spawners, SimpleString )
		{
			const SimpleString& Spawner = SpawnerIter.GetValue();
			PRINTF( "\t%s\n", Spawner.CStr() );
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_G ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			TryStartTranslateAnchor();
		}
		else
		{
			TryStartTranslate();
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_R ) )
	{
		TryStartRotate();
	}

	// X: Scale brushes
	// Ctrl+X: reset scale
	if( IsInRoomMode() && pKeyboard->OnRise( Keyboard::EB_X ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
			{
				SBrush& Brush = m_Brushes[ BrushIter.GetValue() ];
				SetBrushScale( Brush, 1.0f );
			}
		}
		else
		{
			TryStartScale();
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_Delete ) )
	{
		DeleteSelectedBrushes();
		StoreUndoState();
	}

	// U: select all [visible]
	if( pKeyboard->OnRise( Keyboard::EB_U ) )
	{
		SelectAllBrushes();
	}

	// I: invert selection
	if( pKeyboard->OnRise( Keyboard::EB_I ) )
	{
		Array<uint> SelectedBrushes = m_SelectedBrushes;
		SelectAllBrushes();
		DeselectBrushes( SelectedBrushes );
	}

	// O: select none
	if( pKeyboard->OnRise( Keyboard::EB_O ) )
	{
		DeselectAllBrushes();
	}

	// HACKHACK: Aim select, hold Shift and LMB to constantly select while aiming with crosshair
	if( pKeyboard->IsHigh( Keyboard::EB_Mouse_Left ) && pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
	{
		// Trace scene to find a thing to select
		static const float skTraceDistance = 1000.0f;
		const Segment TraceSegment = Segment( m_CameraLocation, m_CameraLocation + m_CameraOrientation.ToVector() * skTraceDistance );

		CollisionInfo Info;
		if( TraceBrushes( TraceSegment, Info ) )
		{
			SBrush& HitBrush = m_Brushes[ Info.m_Out_UserFlags ];
			SelectBrush( HitBrush );
		}
	}
	// Normal selection
	else if( pKeyboard->OnRise( Keyboard::EB_Mouse_Left ) )
	{
		if( !pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			DeselectAllBrushes();
		}

		// Trace scene to find a thing to select
		int X, Y;
		pMouse->GetPosition( X, Y, m_Framework->GetWindow() );
		const Segment TraceSegment = Unproject( X, Y );

		CollisionInfo Info;
		if( TraceBrushes( TraceSegment, Info ) )
		{
			SBrush& HitBrush = m_Brushes[ Info.m_Out_UserFlags ];
			ToggleBrush( HitBrush );
		}
	}

	if( pKeyboard->OnRise( Keyboard::EB_Mouse_Middle ) )
	{
		StartBoxSelect();
	}

	// RMB: "Eyedropper" on brushes (geo and spawners)
	if( pKeyboard->OnRise( Keyboard::EB_Mouse_Right ) )
	{
		int X, Y;
		pMouse->GetPosition( X, Y, m_Framework->GetWindow() );
		const Segment TraceSegment = Unproject( X, Y );

		CollisionInfo Info;
		if( TraceBrushes( TraceSegment, Info ) )
		{
			const SBrush& HitBrush = m_Brushes[ Info.m_Out_UserFlags ];
			if( HitBrush.m_Type == EBT_Room )
			{
				m_RoomElements.m_CurrentDefIndex			= HitBrush.m_DefIndex;
				m_RoomElements.m_CurrentCategoryIndex		= m_RoomElements.m_CategoryMap[ HitBrush.m_DefIndex ];
			}
			else if( HitBrush.m_Type == EBT_Geo )
			{
				m_GeoElements.m_CurrentDefIndex				= HitBrush.m_DefIndex;
				m_GeoElements.m_CurrentCategoryIndex		= m_GeoElements.m_CategoryMap[ HitBrush.m_DefIndex ];

				uint MatDefIndex;
				if( FindMatDef( HitBrush.m_Mat, &MatDefIndex ) )
				{
					m_MatElements.m_CurrentDefIndex			= MatDefIndex;
					m_MatElements.m_CurrentCategoryIndex	= m_MatElements.m_CategoryMap[ MatDefIndex ];
				}
			}
			else if( HitBrush.m_Type == EBT_Spawner )
			{
				m_SpawnerElements.m_CurrentDefIndex			= HitBrush.m_DefIndex;
				m_SpawnerElements.m_CurrentCategoryIndex	= m_SpawnerElements.m_CategoryMap[ HitBrush.m_DefIndex ];
			}
		}
		else if( TraceBoundPlanes( TraceSegment, Info ) )
		{
			const Vector	TileLoc			= Info.m_Out_Intersection + Info.m_Out_Plane.m_Normal * 0.9f;
			const uint		MetersPerTileX	= m_MetersX / m_TilesX;
			const uint		MetersPerTileY	= m_MetersY / m_TilesY;
			const uint		MetersPerTileZ	= m_MetersZ / m_TilesZ;
			const uint		TileX			= static_cast<int>( Round( TileLoc.x ) / MetersPerTileX );
			const uint		TileY			= static_cast<int>( Round( TileLoc.y ) / MetersPerTileY );
			const uint		TileZ			= static_cast<int>( Round( TileLoc.z ) / MetersPerTileZ );
			if( TileX >= 0 && TileX < m_TilesX &&
				TileY >= 0 && TileY < m_TilesY &&
				TileZ >= 0 && TileZ < m_TilesZ )
			{
				const uint	TileIndex	= GetTileIndex( TileX, TileY, TileZ );
				const uint	PortalIndex	= GetPortalIndex( -Info.m_Out_Plane.m_Normal );
				m_PortalElements.m_CurrentDefIndex			= m_Portals[ TileIndex ].m_Portals[ PortalIndex ];
				m_PortalElements.m_CurrentCategoryIndex		= m_PortalElements.m_CategoryMap[ m_Portals[ TileIndex ].m_Portals[ PortalIndex ] ];
			}
		}
	}

	// ROSATODO: Create prefab file from selected elements
	if( pKeyboard->OnRise( Keyboard::EB_J ) && m_SelectedBrushes.Size() > 0 )
	{
		TrySavePrefab();
	}

	if( IsInRoomMode() )
	{
		// V: Add spawner brush
		// Ctrl+V: Replace selected brushes with new brush
		TickCreateBrushInput( Keyboard::EB_V, EBT_Spawner, m_SpawnerElements );

		// B: Add geo mesh brush
		// Ctrl+B: Replace selected brushes with new brush
		TickCreateBrushInput( Keyboard::EB_B, EBT_Geo, m_GeoElements );

		// P: Add prefab
		// Ctrl+P: Replace selected brushes with new prefab
		TickCreateBrushInput( Keyboard::EB_P, EBT_Prefab, m_PrefabElements );
	}
	else
	{
		// B: Add room brush (in world mode)
		// Ctrl+B: Replace selected room brushes with new brush
		TickCreateBrushInput( Keyboard::EB_B, EBT_Room, m_RoomElements );

		// P: Add prefab (no validation on room vs. world prefab at the moment)
		// Ctrl+P: Replace selected brushes with new prefab
		TickCreateBrushInput( Keyboard::EB_P, EBT_Prefab, m_PrefabElements );
	}

	// T: Tag portal
	if( pKeyboard->OnRise( Keyboard::EB_T ) )
	{
		int X, Y;
		pMouse->GetPosition( X, Y, m_Framework->GetWindow() );
		const Segment TraceSegment = Unproject( X, Y );

		CollisionInfo Info;
		if( TraceBrushes( TraceSegment, Info ) )
		{
			// Ignore, trace hit a brush
		}
		else if( TraceBoundPlanes( TraceSegment, Info ) )
		{
			const Vector	TileLoc			= Info.m_Out_Intersection + Info.m_Out_Plane.m_Normal * 0.9f;
			const uint		MetersPerTileX	= m_MetersX / m_TilesX;
			const uint		MetersPerTileY	= m_MetersY / m_TilesY;
			const uint		MetersPerTileZ	= m_MetersZ / m_TilesZ;
			const uint		TileX			= static_cast<int>( Round( TileLoc.x ) / MetersPerTileX );
			const uint		TileY			= static_cast<int>( Round( TileLoc.y ) / MetersPerTileY );
			const uint		TileZ			= static_cast<int>( Round( TileLoc.z ) / MetersPerTileZ );
			if( TileX >= 0 && TileX < m_TilesX &&
				TileY >= 0 && TileY < m_TilesY &&
				TileZ >= 0 && TileZ < m_TilesZ )
			{
				const uint	TileIndex	= GetTileIndex( TileX, TileY, TileZ );
				const uint	PortalIndex	= GetPortalIndex( -Info.m_Out_Plane.m_Normal );
				const uint	PortalDef	= pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) ? 0 : m_PortalElements.m_CurrentDefIndex;	// Hold Ctrl to untag portal (assumes first index is Portal_None!)
				m_Portals[ TileIndex ].m_Portals[ PortalIndex ] = PortalDef;
				InitializePortalMesh();
				StoreUndoState();
			}
		}
	}

	// C: Clone selection (a la Blender's Duplicate, but Shift+D is taken for camera movement)
	if( m_SelectedBrushes.Size() > 0 && pKeyboard->OnRise( Keyboard::EB_C ) )
	{
		Array<uint>	NewBrushes;
		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			SBrush&				Brush			= m_Brushes[ BrushIter.GetValue() ];
			// Make copies of these; CreateBrush will expand m_Brushes and potentially unseat references.
			const Vector		Location		= Brush.m_Location;
			const Angles		Orientation		= Brush.m_Orientation;
			const float			Scale			= Brush.m_Scale;
			const HashedString	Mat				= Brush.m_Mat;
			const Array<uint>	LinkedBrushes	= Brush.m_LinkedBrushes;
			SBrush&				NewBrush		= CreateBrush( Brush.m_Type, Brush.m_DefIndex, Location, Orientation, Scale, LinkedBrushes );

			uint MatDefIndex;
			if( FindMatDef( Mat, &MatDefIndex ) )
			{
				ApplyMatToBrush( MatDefIndex, NewBrush );
			}

			NewBrushes.PushBack( GetIndexOfBrush( NewBrush ) );
		}

		DeselectAllBrushes();
		SelectBrushes( NewBrushes );
		TryStartTranslate();
	}

	// N: Snap selected brushes to grid (with regard to anchor brush)
	// Ctrl+N: Reset orientation of selected brushes
	if( m_SelectedBrushes.Size() > 0 && pKeyboard->OnRise( Keyboard::EB_N ) )
	{
		const bool		ResetOrientation		= pKeyboard->IsHigh( Keyboard::EB_Virtual_Control );
		SBrush&			AnchorBrush				= m_Brushes[ m_SelectedBrushes[ 0 ] ];
		const Vector	OldAnchorLocation		= AnchorBrush.m_Location;
		const Vector	NewAnchorLocation		= ResetOrientation ? OldAnchorLocation : SnapToGrid( AnchorBrush.m_Location );
		const Vector	LocationDelta			= NewAnchorLocation - AnchorBrush.m_Location;
		const Angles	NewAnchorOrientation	= ResetOrientation ? Angles() : SnapToGrid( AnchorBrush.m_Orientation );
		const Matrix	RotationMatrix			= NewAnchorOrientation.ToMatrix() * AnchorBrush.m_Orientation.ToMatrix().GetInverse();

		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			SBrush&			Brush			= m_Brushes[ BrushIter.GetValue() ];

			const Angles	NewOrientation	= ( Brush.m_Orientation.ToMatrix() * RotationMatrix ).ToAngles();
			SetBrushOrientation( Brush, NewOrientation );

			const Vector	AnchorToBrush	= Brush.m_Location - OldAnchorLocation;
			const Vector	NewLocation		= NewAnchorLocation + ( AnchorToBrush * RotationMatrix );
			SetBrushLocation( Brush, NewLocation );
		}

		StoreUndoState();
	}

	// H: hide selected brushes
	// Shift+H: hide everything except selected brushes
	// Alt+H: unhide everything
	if( pKeyboard->OnRise( Keyboard::EB_H ) )
	{
		if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) )
		{
			UnhideAllBrushes();
			StoreUndoState();
		}
		else if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
		{
			Array<uint>	SelectedBrushes = m_SelectedBrushes;	// Make a copy because hiding also unselects
			HideAllBrushes();
			UnhideBrushes( SelectedBrushes );
			SelectBrushes( SelectedBrushes );
			StoreUndoState();
		}
		else
		{
			Array<uint>	SelectedBrushes = m_SelectedBrushes;	// Make a copy because hiding also unselects
			HideBrushes( SelectedBrushes );
			StoreUndoState();
		}
	}

	// M: Apply selected mat to selected geo brushes
	// Ctrl+M: Remove mat (reset to default)
	if( m_SelectedBrushes.Size() > 0 && pKeyboard->OnRise( Keyboard::EB_M ) )
	{
		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			SBrush& Brush = m_Brushes[ BrushIter.GetValue() ];
			if( Brush.m_Type != EBT_Geo )
			{
				continue;
			}

			if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
			{
				RemoveMatFromBrush( Brush );
			}
			else
			{
				ApplyMatToBrush( m_MatElements.m_CurrentDefIndex, Brush );
			}
		}

		StoreUndoState();
	}

	if( m_SelectedBrushes.Size() > 1 && pKeyboard->OnRise( Keyboard::EB_F ) )
	{
		const bool	Unlink		= pKeyboard->IsHigh( Keyboard::EB_Virtual_Shift );
		SBrush&		AnchorBrush	= m_Brushes[ m_SelectedBrushes[ 0 ] ];
		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			if( 0 == BrushIter.GetIndex() )
			{
				continue;
			}

			if( Unlink )
			{
				AnchorBrush.m_LinkedBrushes.RemoveItem( BrushIter.GetValue() );
			}
			else
			{
				AnchorBrush.m_LinkedBrushes.PushBackUnique( BrushIter.GetValue() );
			}
		}

		StoreUndoState();
	}
}

void RosaTools::TickCreateBrushInput( const uint Key, const EBrushType BrushType, const SElements& Elements )
{
	Keyboard* const	pKeyboard	= m_Framework->GetKeyboard();
	Mouse* const	pMouse		= m_Framework->GetMouse();

	if( pKeyboard->OnRise( Key ) )
	{
		if( m_SelectedBrushes.Size() > 0 && pKeyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			const SBrush&	AnchorBrush	= m_Brushes[ m_SelectedBrushes[ 0 ] ];

			uint MatDefIndex;
			const bool HasMat = FindMatDef( AnchorBrush.m_Mat, &MatDefIndex );

			const Vector	Location	= AnchorBrush.m_Location;
			const Angles	Orientation	= AnchorBrush.m_Orientation;
			const float		Scale		= AnchorBrush.m_Scale;
			DeleteSelectedBrushes();
			Array<uint>		NewBrushes;
			CreateBrushes( BrushType, Elements.m_CurrentDefIndex, Location, Orientation, Scale, NewBrushes );

			if( HasMat && BrushType != EBT_Prefab )
			{
				for( uint BrushIndex = 0; BrushIndex < NewBrushes.Size(); ++BrushIndex )
				{
					SBrush& NewBrush = GetBrush( NewBrushes[ BrushIndex ] );
					ApplyMatToBrush( MatDefIndex, NewBrush );
				}
			}

			SelectBrushes( NewBrushes );
			StoreUndoState();
		}
		else
		{
			// Trace scene to find where to put brush
			int X, Y;
			pMouse->GetPosition( X, Y, m_Framework->GetWindow() );
			const Segment TraceSegment = Unproject( X, Y );

			CollisionInfo Info;
			if( TraceBrushes( TraceSegment, Info ) || TraceBoundPlanes( TraceSegment, Info ) )
			{
				DeselectAllBrushes();
				const float		NormalPushOut	= ( BrushType == EBT_Spawner ) ? 0.5f : 0.0f;
				const Vector	Location		= Info.m_Out_Intersection + Info.m_Out_Plane.m_Normal * NormalPushOut;
				const Vector	SnapLocation	= SnapToGrid( Location );
				Array<uint>		NewBrushes;
				CreateBrushes( BrushType, Elements.m_CurrentDefIndex, SnapLocation, Angles(), 1.0f, NewBrushes );

				// HACKHACK: If a geo brush has default albedo, apply the currently selected mat; this is my new pattern for Zeta, using untextured blockout brushes
				if( BrushType == EBT_Geo )
				{
					ITexture* const pDefaultAlbedo = m_Framework->GetRenderer()->GetTextureManager()->GetTexture( DEFAULT_TEXTURE, TextureManager::ETL_Permanent );

					// There should only be one new brush, but iterate why not
					for( uint NewBrushIndex = 0; NewBrushIndex < NewBrushes.Size(); ++NewBrushIndex )
					{
						SBrush&					NewBrush		= GetBrush( NewBrushes[ NewBrushIndex ] );
						const SGeoDef&			GeoDef			= m_GeoDefs[ NewBrush.m_DefIndex ];
						bool					NeedsMat		= false;
						for( uint NewBrushMeshIndex = 0; NewBrushMeshIndex < NewBrush.m_Meshes.Size(); ++NewBrushMeshIndex )
						{
							const SimpleString&	MeshFilename	= GeoDef.m_MeshNames[ NewBrushMeshIndex ];
							const Mesh* const	pOriginalMesh	= DynamicMeshManager::GetInstance()->GetMesh( MeshFilename.CStr() );
							if( pDefaultAlbedo == pOriginalMesh->GetTexture( 0 ) )
							{
								NeedsMat = true;
								break;
							}
						}
						if( NeedsMat )
						{
							ApplyMatToBrush( m_MatElements.m_CurrentDefIndex, NewBrush );
						}
					}
				}

				SelectBrushes( NewBrushes );
				StoreUndoState();
			}
			else
			{
				WARN;
			}
		}
	}
}

void RosaTools::ApplyMatToBrush( const uint MatDefIndex, SBrush& Brush )
{
	ASSERT( Brush.m_Type == EBT_Geo );

	IRenderer* const		pRenderer		= m_Framework->GetRenderer();
	TextureManager* const	pTextureManager	= pRenderer->GetTextureManager();

	const SMatDef&			MatDef			= m_MatDefs[ MatDefIndex ];
	Brush.m_Mat								= MatDef.m_DefName;

	const char*				pOverlay		= ( MatDef.m_Overlay == "" ) ? DEFAULT_TEXTURE : MatDef.m_Overlay.CStr();

	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh& BrushMesh = BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->SetTexture( 0, pTextureManager->GetTexture( MatDef.m_Albedo.CStr() ) );
		BrushMesh.m_Mesh->SetTexture( 1, pTextureManager->GetTexture( pOverlay ) );
	}
}

void RosaTools::RemoveMatFromBrush( SBrush& Brush )
{
	ASSERT( Brush.m_Type == EBT_Geo );

	IRenderer* const		pRenderer		= m_Framework->GetRenderer();
	TextureManager* const	pTextureManager	= pRenderer->GetTextureManager();

	Brush.m_Mat								= HashedString::NullString;

	// Reset the texture on the brush meshes by looking up the original meshes
	const SGeoDef& GeoDef = m_GeoDefs[ Brush.m_DefIndex ];
	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh&			BrushMesh		= BrushMeshIter.GetValue();
		const SimpleString&	MeshFilename	= GeoDef.m_MeshNames[ BrushMeshIter.GetIndex() ];
		const Mesh* const	pOriginalMesh	= DynamicMeshManager::GetInstance()->GetMesh( MeshFilename.CStr() );
		BrushMesh.m_Mesh->SetTexture( 0, pOriginalMesh->GetTexture( 0 ) );
		BrushMesh.m_Mesh->SetTexture( 1, pTextureManager->GetTexture( DEFAULT_TEXTURE ) );
	}
}

bool RosaTools::TraceBoundPlanes( const Segment& TraceSegment, CollisionInfo& Info )
{
	CollisionInfo MinInfo;

	for( uint BoundPlaneIndex = 0; BoundPlaneIndex < m_BoundPlanes.Size(); ++BoundPlaneIndex )
	{
		const Plane& BoundPlane = m_BoundPlanes[ BoundPlaneIndex ];

		// Ignore bound planes traced from the back side
		if( TraceSegment.GetDirection().Dot( BoundPlane.m_Normal ) > 0.0f )
		{
			continue;
		}

		CollisionInfo CheckInfo;
		if( TraceSegment.Intersects( BoundPlane, &CheckInfo ) )
		{
			if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo = CheckInfo;
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaTools::TraceBrushes( const Segment& TraceSegment, CollisionInfo& Info )
{
	CollisionInfo MinInfo;

	if( IsInRoomMode() )
	{
		CollisionInfo GeoInfo;
		if( TraceGeo( TraceSegment, GeoInfo, false ) )
		{
			if( GeoInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo.CopyOutParametersFrom( GeoInfo );
			}
		}

		CollisionInfo SpawnerInfo;
		if( TraceSpawners( TraceSegment, SpawnerInfo ) )
		{
			if( SpawnerInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo.CopyOutParametersFrom( SpawnerInfo );
			}
		}
	}
	else
	{
		CollisionInfo RoomInfo;
		if( TraceRooms( TraceSegment, RoomInfo ) )
		{
			if( RoomInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo.CopyOutParametersFrom( RoomInfo );
			}
		}
	}


	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaTools::TraceGeo( const Segment& TraceSegment, CollisionInfo& Info, const bool IgnoreNonBlockers )
{
	CollisionInfo MinInfo;

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& Brush = BrushIter.GetValue();
		if( Brush.m_Hidden )
		{
			continue;
		}

		if( Brush.m_Type != EBT_Geo )
		{
			continue;
		}

		const SGeoDef& BrushDef = m_GeoDefs[ Brush.m_DefIndex ];
		FOR_EACH_ARRAY( HullIter, BrushDef.m_Hulls, SConvexHull )
		{
			const SConvexHull&	Hull			= HullIter.GetValue();

			if( IgnoreNonBlockers &&
				( 0 == ( Hull.m_CollisionFlags & EECF_BlocksEntities ) ||
				  0 == ( Hull.m_CollisionFlags & EECF_BlocksNav ) ) )
			{
				// This doesn't block entities, and we're ignoring non-blockers (for nav snap points)
				continue;
			}

			ConvexHull			TransformedHull	= Hull.m_Hull;
			TransformedHull.MoveBy( Brush.m_Location, Brush.m_Orientation, Brush.m_Scale );

			CollisionInfo CheckInfo;
			if( TransformedHull.Sweep( TraceSegment.m_Point1, Vector(), TraceSegment.m_Point2 - TraceSegment.m_Point1, &CheckInfo ) )
			{
				// Pass the index of the hit brush out in user flags
				CheckInfo.m_Out_UserFlags = BrushIter.GetIndex();

				if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
				{
					MinInfo = CheckInfo;
				}
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaTools::TraceSpawners( const Segment& TraceSegment, CollisionInfo& Info )
{
	CollisionInfo MinInfo;

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& Brush = BrushIter.GetValue();
		if( Brush.m_Hidden )
		{
			continue;
		}

		if( Brush.m_Type != EBT_Spawner )
		{
			continue;
		}

		// ROSATODO: When I have scaled spawner brushes, apply scale matrix here too.
		// Make a modified TraceSegment so we can effectively intersect with an oriented box.
		const SSpawnerDef&	SpawnerDef			= m_SpawnerDefs[ Brush.m_DefIndex ];
		const Matrix		RotationMatrix		= Brush.m_Orientation.ToMatrix();
		const Matrix		TranslationMatrix	= Matrix::CreateTranslation( Brush.m_Location + ( SpawnerDef.m_Offset * RotationMatrix ) );
		const Matrix		TransformMatrix		= ( RotationMatrix * TranslationMatrix ).GetInverse();
		const Vector		RelativeStart		= TraceSegment.m_Point1 * TransformMatrix;
		const Vector		RelativeEnd			= TraceSegment.m_Point2 * TransformMatrix;
		const Segment		RelativeSegment		= Segment( RelativeStart, RelativeEnd );

		CollisionInfo CheckInfo;
		if( RelativeSegment.Intersects( SpawnerDef.m_TraceExtents, &CheckInfo ) )
		{
			// Pass the index of the hit brush out in user flags
			CheckInfo.m_Out_UserFlags = BrushIter.GetIndex();

			if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo = CheckInfo;
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaTools::TraceRooms( const Segment& TraceSegment, CollisionInfo& Info )
{
	CollisionInfo MinInfo;

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& Brush = BrushIter.GetValue();
		if( Brush.m_Hidden )
		{
			continue;
		}

		if( Brush.m_Type != EBT_Room )
		{
			continue;
		}

		const SRoomDef&	RoomDef				= m_RoomDefs[ Brush.m_DefIndex ];
		//const AABB&		RoomAABB			= RoomDef.m_Extents;
		AABB			RoomAABB			= RoomDef.m_Extents;
		RoomAABB.m_Max.z					= RoomAABB.m_Min.z;	// HACKHACK: Only trace against ground plane
		const Matrix	TranslationMatrix	= Matrix::CreateTranslation( Brush.m_Location );
		const Matrix	RotationMatrix		= Brush.m_Orientation.ToMatrix();
		const Matrix	TransformMatrix		= ( RotationMatrix * TranslationMatrix ).GetInverse();
		const Vector	RelativeStart		= TraceSegment.m_Point1 * TransformMatrix;
		const Vector	RelativeEnd			= TraceSegment.m_Point2 * TransformMatrix;
		const Segment	RelativeSegment		= Segment( RelativeStart, RelativeEnd );

		CollisionInfo CheckInfo;
		if( RelativeSegment.Intersects( RoomAABB, &CheckInfo ) )
		{
			// Pass the index of the hit brush out in user flags
			CheckInfo.m_Out_UserFlags = BrushIter.GetIndex();

			if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo = CheckInfo;
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

void RosaTools::TryClear()
{
	const int Response = ConditionalMessageBox( "Clear world?" );
	if( Response == IDYES )
	{
		Clear();
	}
}

void RosaTools::Clear()
{
	ClearBrushes();

	ResetPortals();
	InitializePortalMesh();

	m_NavVerts.Clear();
	m_NavEdges.Clear();
	m_NavFaces.Clear();
	m_SelectedNavVerts.Clear();
	m_SelectedNavEdges.Clear();
	m_SelectedNavFaces.Clear();
	m_NavSnapPoints.Clear();

	if( HasUndoStates() )
	{
		StoreUndoState();
	}
}

void RosaTools::RenderElements( const struct SElements& Elements, const uint NumDefs )
{
	IRenderer* const	pRenderer				= m_Framework->GetRenderer();
	static const uint	kSelectedPaletteColor	= ARGB_TO_COLOR( 255, 128, 192, 255 );

	// Draw categories
	const uint NumCategoryButtons = Elements.m_CategoryButtons.Size();
	for( uint CategoryButtonIndex = 0; CategoryButtonIndex < NumCategoryButtons; ++CategoryButtonIndex )
	{
		const SElementCategoryButton& CategoryButton = Elements.m_CategoryButtons[ CategoryButtonIndex ];
		pRenderer->AddMesh( CategoryButton.m_Mesh );

		// Draw a box around the selected category
		if( Elements.m_CurrentCategoryIndex == CategoryButton.m_CategoryIndex )
		{
			const Vector BoxMin( CategoryButton.m_BoxMin.x, 0.0f, CategoryButton.m_BoxMin.y );
			const Vector BoxMax( CategoryButton.m_BoxMax.x, 0.0f, CategoryButton.m_BoxMax.y );
			pRenderer->DEBUGDrawBox2D( BoxMin, BoxMax, kSelectedPaletteColor );
		}
	}

	for( uint DefIndex = 0; DefIndex < NumDefs; ++DefIndex )
	{
		// Draw only elements in the selected category
		const uint CategoryIndex = Elements.m_CategoryMap[ DefIndex ];
		if( CategoryIndex == Elements.m_CurrentCategoryIndex )
		{
			const SElementButton& Button = Elements.m_Buttons[ DefIndex ];
			pRenderer->AddMesh( Button.m_Mesh );

			// Draw a box around the selected elements
			if( Elements.m_CurrentDefIndex == Button.m_DefIndex )
			{
				const Vector BoxMin( Button.m_BoxMin.x, 0.0f, Button.m_BoxMin.y );
				const Vector BoxMax( Button.m_BoxMax.x, 0.0f, Button.m_BoxMax.y );
				pRenderer->DEBUGDrawBox2D( BoxMin, BoxMax, kSelectedPaletteColor );
			}
		}
	}
}

uint RosaTools::GetSpawnerNameColor( const HashedString& SpawnerName )
{
	Map<HashedString, uint>::Iterator Iter = m_SpawnerNameColors.Search( SpawnerName );
	if( Iter.IsValid() )
	{
		return Iter.GetValue();
	}

	STATICHASH( EditorR );
	const uint EditorR = ConfigManager::GetInheritedInt( sEditorR, 255, SpawnerName );

	STATICHASH( EditorG );
	const uint EditorG = ConfigManager::GetInheritedInt( sEditorG, 255, SpawnerName );

	STATICHASH( EditorB );
	const uint EditorB = ConfigManager::GetInheritedInt( sEditorB, 192, SpawnerName );

	const uint Color = ARGB_TO_COLOR( 0, EditorR, EditorG, EditorB );
	m_SpawnerNameColors.Insert( SpawnerName, Color );
	return Color;
}

void RosaTools::TickRender()
{
	IRenderer* const pRenderer = m_Framework->GetRenderer();

	View* const		pMainView			= m_Framework->GetMainView();
	Display* const	pDisplay			= m_Framework->GetDisplay();
	const Matrix	VPMatrix			= pMainView->GetViewProjectionMatrix();
	const float		DisplayWidth		= static_cast<float>( pDisplay->m_Width );
	const float		DisplayHeight		= static_cast<float>( pDisplay->m_Height );

	static const uint	kClearColor		= ARGB_TO_COLOR( 255, 16, 24, 32 );
	static const uint	kBrushLinkColor	= ARGB_TO_COLOR( 255, 96, 244, 16 );

	if( IsInNormalMode() ||
		m_SubTool == EST_NavMesh )
	{
		pRenderer->AddMesh( m_HelpMesh );
	}

	if( IsInNormalMode() && m_SelectedBrushes.Size() > 0 )
	{
		pRenderer->DEBUGDrawSphere( m_TransformAnchor, 1.0f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );
		pRenderer->DEBUGDrawSphere( m_TransformAnchor, 0.75f, ARGB_TO_COLOR( 255, 0, 0, 0 ) );
		pRenderer->DEBUGDrawSphere( m_TransformAnchor, 0.5f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );
		pRenderer->DEBUGDrawCross( m_TransformAnchor, 1.0f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );
		pRenderer->DEBUGDrawCoords( m_TransformAnchor, Angles(), 1.0f, false );
	}

	if( m_SubTool == EST_NavMesh )
	{
		pRenderer->DEBUGDrawSphere( m_NavSnapPoint, 1.0f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );
		pRenderer->DEBUGDrawSphere( m_NavSnapPoint, 0.75f, ARGB_TO_COLOR( 255, 0, 0, 0 ) );
		pRenderer->DEBUGDrawSphere( m_NavSnapPoint, 0.5f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );
		pRenderer->DEBUGDrawCross( m_NavSnapPoint, 1.0f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );

		if( m_NavMesh )
		{
			pRenderer->AddMesh( m_NavMesh );
		}

		if( m_NavSnapMesh )
		{
			pRenderer->AddMesh( m_NavSnapMesh );
		}
	}

	const uint NumGridMeshes = m_GridMeshes.Size();
	for( uint GridMeshIndex = 0; GridMeshIndex < NumGridMeshes; ++GridMeshIndex )
	{
		Mesh* const pGridMesh = m_GridMeshes[ GridMeshIndex ];
		pRenderer->AddMesh( pGridMesh );
	}

	if( m_PortalMesh )
	{
		pRenderer->AddMesh( m_PortalMesh );
	}

	if( m_IsBoxSelecting )
	{
		Mouse* const pMouse = m_Framework->GetMouse();
		int MouseX, MouseY;
		pMouse->GetPosition( MouseX, MouseY, m_Framework->GetWindow() );

		int MouseXLo = Min( MouseX, m_TransformStartCursorX );
		int MouseXHi = Max( MouseX, m_TransformStartCursorX );
		int MouseYLo = Min( MouseY, m_TransformStartCursorY );
		int MouseYHi = Max( MouseY, m_TransformStartCursorY );

		const Vector		BoxMin				= Vector( static_cast<float>( MouseXLo ), 0.0f, static_cast<float>( MouseYLo ) );
		const Vector		BoxMax				= Vector( static_cast<float>( MouseXHi ), 0.0f, static_cast<float>( MouseYHi ) );
		static const uint	kBoxSelectOutline	= ARGB_TO_COLOR( 255, 255, 255, 255 );
		pRenderer->DEBUGDrawBox2D( BoxMin, BoxMax, kBoxSelectOutline );
	}

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& Brush = BrushIter.GetValue();
		if( Brush.m_Hidden )
		{
			continue;
		}

		FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
		{
			SBrushMesh& BrushMesh = BrushMeshIter.GetValue();
			pRenderer->AddMesh( BrushMesh.m_Mesh );
		}

		if( Brush.m_Selected )
		{
			FOR_EACH_ARRAY( LinkedBrushIter, Brush.m_LinkedBrushes, uint )
			{
				const uint		LinkedBrushIndex	= LinkedBrushIter.GetValue();
				const SBrush&	LinkedBrush			= m_Brushes[ LinkedBrushIndex ];
				pRenderer->DEBUGDrawLine( Brush.m_Location, LinkedBrush.m_Location, kBrushLinkColor, false );
			}
		}

		if( Brush.m_Type == EBT_Spawner )
		{
			// Print spawner names and comments
			const Vector4 ProjectedLocation = Vector4( Brush.m_Location ) * VPMatrix;
			if( ProjectedLocation.z >= 0.0f )
			{
				const float			Distance		= ( Brush.m_Location - m_CameraLocation ).Length();
				static const float	sFadeOutNear	= 20.0f;
				static const float	sFadeOutFar		= 40.0f;
				const float			Alpha			= Saturate( InvLerp( Distance, sFadeOutFar, sFadeOutNear ) );

				if( Alpha > 0.0f )
				{
					// Unless we're holding Alt (or this spawner is selected), do
					// a line check and hide any spawners that are occluded by geo
					// ROSANOTE: I could restore this later, but for modules (instead of whole levels), it's not useful.
					//if( !Brush.m_Selected && !IsAltHigh && TraceGeo( ... ) )
					//{
					//	continue;
					//}

					const Vector4		NormalizedLocation	= ProjectedLocation / ProjectedLocation.w;
					const Vector2		ScaledLocation		= Vector2( DisplayWidth * ( NormalizedLocation.x * 0.5f + 0.5f ), DisplayHeight * ( -NormalizedLocation.y * 0.5f + 0.5f ) );
					const SRect			Rect				= SRect( Floor( ScaledLocation.x ), Floor( ScaledLocation.y ), 0.0f, 0.0f );
					const SRect			ShadowRect			= SRect( Rect.m_Left + 1.0f, Rect.m_Top + 1.0f, 0.0f, 0.0f );
					Font* const			pFont				= pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG );
					const byte			ByteAlpha			= static_cast<byte>( 255.0f * Alpha );
					const byte			ByteShadowAlpha		= static_cast<byte>( 128.0f * Alpha );

					const SimpleString&	SpawnerDef			= m_SpawnerDefs[ Brush.m_DefIndex ].m_DefName;
					MAKEHASH( SpawnerDef );
					STATICHASH( Comment );
					const SimpleString&	SpawnerComment		= ConfigManager::GetInheritedString( sComment, "", sSpawnerDef );
					const SimpleString	SpawnerText			= SimpleString::PrintF( "%s\n%s", SpawnerDef.CStr(), SpawnerComment.CStr() );

					const uint			PrintColor			= GetSpawnerNameColor( SpawnerDef ) | ARGB_TO_COLOR( ByteAlpha, 0, 0, 0 );
					const uint			ShadowColor			= ARGB_TO_COLOR( ByteShadowAlpha, 8, 8, 32 );

					pRenderer->DEBUGPrint( SpawnerText.CStr(), pFont, ShadowRect, ShadowColor );
					pRenderer->DEBUGPrint( SpawnerText.CStr(), pFont, Rect, PrintColor );
				}
			}
		}
		else if( Brush.m_Type == EBT_Room )
		{
			// Print room names
			const Vector4 ProjectedLocation = Vector4( Brush.m_Location ) * VPMatrix;
			if( ProjectedLocation.z >= 0.0f )
			{
				const float			Distance		= ( Brush.m_Location - m_CameraLocation ).Length();
				static const float	sFadeOutNear	= 10.0f;
				static const float	sFadeOutFar		= 20.0f;
				const float			Alpha			= Saturate( InvLerp( Distance, sFadeOutFar, sFadeOutNear ) );

				if( Alpha > 0.0f || Brush.m_Selected )
				{
					const Vector4		NormalizedLocation	= ProjectedLocation / ProjectedLocation.w;
					const Vector2		ScaledLocation		= Vector2( DisplayWidth * ( NormalizedLocation.x * 0.5f + 0.5f ), DisplayHeight * ( -NormalizedLocation.y * 0.5f + 0.5f ) );
					const SRect			Rect				= SRect( Floor( ScaledLocation.x ), Floor( ScaledLocation.y ), 0.0f, 0.0f );
					const SRect			ShadowRect			= SRect( Rect.m_Left + 1.0f, Rect.m_Top + 1.0f, 0.0f, 0.0f );
					Font* const			pFont				= pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG );
					const byte			ByteAlpha			= static_cast<byte>( 255.0f * Alpha );
					const byte			ByteShadowAlpha		= static_cast<byte>( 128.0f * Alpha );

					const SimpleString&	RoomDef		= m_RoomDefs[ Brush.m_DefIndex ].m_DefName;
					const uint			PrintColor	= Brush.m_Selected ? ARGB_TO_COLOR( 255, 255, 255, 0 ) : ARGB_TO_COLOR( ByteAlpha, 192, 192, 192 );
					const uint			ShadowColor	= Brush.m_Selected ? ARGB_TO_COLOR( 128, 8, 8, 32 ) : ARGB_TO_COLOR( ByteShadowAlpha, 8, 8, 32 );
					pRenderer->DEBUGPrint( RoomDef.CStr(), pFont, ShadowRect, ShadowColor );
					pRenderer->DEBUGPrint( RoomDef.CStr(), pFont, Rect, PrintColor );
				}
			}
		}
		else if( Brush.m_Type == EBT_Geo )
		{
			// Print selected brush and material names
			if( Brush.m_Selected )
			{
				const Vector4 ProjectedLocation = Vector4( Brush.m_Location ) * VPMatrix;
				if( ProjectedLocation.z >= 0.0f )
				{
					const Vector4		NormalizedLocation	= ProjectedLocation / ProjectedLocation.w;
					const Vector2		ScaledLocation		= Vector2( DisplayWidth * ( NormalizedLocation.x * 0.5f + 0.5f ), DisplayHeight * ( -NormalizedLocation.y * 0.5f + 0.5f ) );
					const SRect			Rect				= SRect( Floor( ScaledLocation.x ), Floor( ScaledLocation.y ), 0.0f, 0.0f );
					const SRect			ShadowRect			= SRect( Rect.m_Left + 1.0f, Rect.m_Top + 1.0f, 0.0f, 0.0f );
					Font* const			pFont				= pRenderer->GetFontManager()->GetFont( DEFAULT_FONT_TAG );

					const SimpleString&	GeoDef				= m_GeoDefs[ Brush.m_DefIndex ].m_DefName;
					SimpleString		GeoText				= GeoDef;
					uint MatDefIndex;
					if( FindMatDef( Brush.m_Mat, &MatDefIndex ) )
					{
						GeoText								= SimpleString::PrintF( "%s\n%s", GeoText.CStr(), m_MatDefs[ MatDefIndex ].m_DefName.CStr() );
					}
					const uint			PrintColor			= ARGB_TO_COLOR( 255, 255, 255, 0 );
					const uint			ShadowColor			= ARGB_TO_COLOR( 128, 8, 8, 32 );
					pRenderer->DEBUGPrint( GeoText.CStr(), pFont, ShadowRect, ShadowColor );
					pRenderer->DEBUGPrint( GeoText.CStr(), pFont, Rect, PrintColor );
				}
			}
		}
	}

	if( m_SubTool == EST_Rooms )
	{
		RenderElements( m_RoomElements, m_RoomDefs.Size() );
	}
	else if( m_SubTool == EST_Spawners )
	{
		RenderElements( m_SpawnerElements, m_SpawnerDefs.Size() );
	}
	else if( m_SubTool == EST_Geo )
	{
		RenderElements( m_GeoElements, m_GeoDefs.Size() );
	}
	else if( m_SubTool == EST_Prefabs || m_SubTool ==  EST_WorldPrefabs )
	{
		RenderElements( m_PrefabElements, m_PrefabDefs.Size() );
	}
	else if( m_SubTool == EST_Portals )
	{
		RenderElements( m_PortalElements, m_PortalDefs.Size() );
	}
	else if( m_SubTool == EST_Mats )
	{
		RenderElements( m_MatElements, m_MatDefs.Size() );
	}
}

void RosaTools::TrySavePrefab()
{
	const SimpleString FileDesc	= "Rosa Prefab Files";
	const SimpleString FileExt	= "rosaprefab";

	SimpleString SaveFileName;
	if( FileUtil::GetSaveFile( m_Framework->GetWindow()->GetHWnd(), FileDesc, FileExt, SaveFileName ) )
	{
		SavePrefab( FileStream( SaveFileName.CStr(), FileStream::EFM_Write ) );
	}
}

#define VERSION_PREFAB_EMPTY	0
#define VERSION_PREFAB_BASE		1
#define VERSION_PREFAB_MATS		2
#define VERSION_PREFAB_CURRENT	2

void RosaTools::SavePrefab( const IDataStream& Stream )
{
	// Save selected elements relative to anchor transform
	DEVASSERT( m_SelectedBrushes.Size() > 0 );
	const SBrush&	AnchorBrush			= m_Brushes[ m_SelectedBrushes[0] ];
	const Vector	AnchorLocation		= AnchorBrush.m_Location;
	const Angles	AnchorOrientation	= AnchorBrush.m_Orientation;
	const Matrix	InvAnchorOrientationMatrix	= AnchorOrientation.ToMatrix().GetInverse();

	SPrefabDef PrefabDef;
	FOR_EACH_ARRAY( SelectedBrushIter, m_SelectedBrushes, uint )
	{
		const uint		SelectedBrushIndex	= SelectedBrushIter.GetValue();
		const SBrush&	SelectedBrush		= m_Brushes[ SelectedBrushIndex ];

		if( SelectedBrush.m_Type != EBT_Geo &&
			SelectedBrush.m_Type != EBT_Spawner &&
			SelectedBrush.m_Type != EBT_Room )
		{
			// Ignore other brush types
			continue;
		}

		SPrefabPart&	PrefabPart			= PrefabDef.m_Parts.PushBack();
		PrefabPart.m_Type					= SelectedBrush.m_Type;
		PrefabPart.m_DefIndex				= SelectedBrush.m_DefIndex;

		const Vector	PartTranslationWS	= SelectedBrush.m_Location - AnchorLocation;
		const Vector	PartTranslationOS	= PartTranslationWS * InvAnchorOrientationMatrix;
		PrefabPart.m_TranslationOffsetLo	= PartTranslationOS;

		const Angles	PartOrientation		= ( SelectedBrush.m_Orientation.ToMatrix() * InvAnchorOrientationMatrix ).ToAngles();
		PrefabPart.m_OrientationOffsetLo	= PartOrientation;

		// ROSATODO: Save part scale if necessary; ideally I can avoid it

		PrefabPart.m_Mat					= SelectedBrush.m_Mat;
	}

	Stream.WriteUInt32( VERSION_PREFAB_CURRENT );
	Stream.WriteUInt32( PrefabDef.m_Parts.Size() );
	FOR_EACH_ARRAY( PrefabPartIter, PrefabDef.m_Parts, SPrefabPart )
	{
		const SPrefabPart&	PrefabPart	= PrefabPartIter.GetValue();

		SimpleString		BrushDefName;
		if( PrefabPart.m_Type == EBT_Geo )
		{
			BrushDefName = m_GeoDefs[ PrefabPart.m_DefIndex ].m_DefName;
		}
		else if( PrefabPart.m_Type == EBT_Spawner )
		{
			BrushDefName = m_SpawnerDefs[ PrefabPart.m_DefIndex ].m_DefName;
		}
		else if( PrefabPart.m_Type == EBT_Room )
		{
			BrushDefName = m_RoomDefs[ PrefabPart.m_DefIndex ].m_DefName;
		}

		Stream.Write<EBrushType>( PrefabPart.m_Type );
		Stream.WriteString( BrushDefName );
		Stream.Write<Vector>( PrefabPart.m_TranslationOffsetLo );
		Stream.Write<Angles>( PrefabPart.m_OrientationOffsetLo );
		Stream.Write<HashedString>( PrefabPart.m_Mat );
	}
}

void RosaTools::LoadPrefab( const IDataStream& Stream, SPrefabDef& PrefabDef )
{
	const uint					Version			= Stream.ReadUInt32();

	if( Version < VERSION_PREFAB_BASE )
	{
		return;
	}

	const uint				NumParts		= Stream.ReadUInt32();
	FOR_EACH_INDEX( PartIndex, NumParts )
	{
		const EBrushType	BrushType		= Stream.Read<EBrushType>();
		const SimpleString	BrushDefName	= Stream.ReadString();
		uint				BrushDefIndex;

		if( ( BrushType == EBT_Geo && FindGeoDef( BrushDefName, &BrushDefIndex ) ) ||
			( BrushType == EBT_Spawner && FindSpawnerDef( BrushDefName, &BrushDefIndex ) ) ||
			( BrushType == EBT_Room && FindRoomDef( BrushDefName, & BrushDefIndex ) ) )
		{
			// Okay!
		}
		else
		{
			continue;
		}

		SPrefabPart&	PrefabPart	= PrefabDef.m_Parts.PushBack();
		PrefabPart.m_Type					= BrushType;
		PrefabPart.m_DefIndex				= BrushDefIndex;
		PrefabPart.m_TranslationOffsetLo	=
		PrefabPart.m_TranslationOffsetHi	= Stream.Read<Vector>();
		PrefabPart.m_OrientationOffsetLo	=
		PrefabPart.m_OrientationOffsetHi	= Stream.Read<Angles>();
		PrefabPart.m_Mat					= ( Version >= VERSION_PREFAB_MATS ) ? Stream.Read<HashedString>() : HashedString::NullString;
	}
}

void RosaTools::TryQuickSave()
{
	const SimpleString& CurrentMapName = IsInRoomMode() ? m_CurrentRoomName : m_CurrentWorldName;
	if( CurrentMapName == "" )
	{
		TrySave();
		return;
	}

	const SimpleString Message = SimpleString::PrintF( "Save %s?", CurrentMapName.CStr() );
	const int Response = ConditionalMessageBox( Message );
	if( Response == IDYES )
	{
		Save( CurrentMapName );
	}
}

void RosaTools::TrySave()
{
	const SimpleString FileDesc = IsInWorldMode() ? "Rosa World Files" : "Rosa Room Files";
	const SimpleString FileExt = IsInWorldMode() ? "rosaworld" : "rosaroom";

	SimpleString SaveFileName;
	if( FileUtil::GetSaveFile( m_Framework->GetWindow()->GetHWnd(), FileDesc, FileExt, SaveFileName ) )
	{
		Save( SaveFileName );
	}
}

void RosaTools::TryLoad()
{
	const SimpleString FileDesc = IsInWorldMode() ? "Rosa World Files" : "Rosa Room Files";
	const SimpleString FileExt = IsInWorldMode() ? "rosaworld" : "rosaroom";

	SimpleString LoadFileName;
	if( FileUtil::GetLoadFile( m_Framework->GetWindow()->GetHWnd(), FileDesc, FileExt, LoadFileName ) )
	{
		Load( LoadFileName );
	}
}

void RosaTools::Save( const SimpleString& Filename )
{
	SimpleString& CurrentMapName = IsInRoomMode() ? m_CurrentRoomName : m_CurrentWorldName;
	CurrentMapName = Filename;
	SaveStream( FileStream( Filename.CStr(), FileStream::EFM_Write ) );
}

void RosaTools::Load( const SimpleString& Filename )
{
	LoadStream( FileStream( Filename.CStr(), FileStream::EFM_Read ) );
	SimpleString& CurrentMapName = IsInRoomMode() ? m_CurrentRoomName : m_CurrentWorldName;
	CurrentMapName = Filename;
}

void RosaTools::SaveStream( const IDataStream& Stream )
{
	if( IsInWorldMode() )
	{
		SaveWorld( Stream );
	}
	else
	{
		SaveRoom( Stream );
	}
}

void RosaTools::LoadStream( const IDataStream& Stream )
{
	if( IsInWorldMode() )
	{
		LoadWorld( Stream );
	}
	else
	{
		LoadRoom( Stream );
	}
}

// NOTE: Unless there's a bake step or I ignore modules in world save/load, this needs to stay in sync with those functions
void RosaTools::SaveWorld( const IDataStream& Stream )
{
	RosaRoomEditor TempRoom;

	TempRoom.m_MapType					= EMT_World;
	TempRoom.m_TOOLS_CameraLocation		= m_CameraLocation;
	TempRoom.m_TOOLS_CameraOrientation	= m_CameraOrientation;

	TempRoom.m_TilesX	= m_TilesX;
	TempRoom.m_TilesY	= m_TilesY;
	TempRoom.m_TilesZ	= m_TilesZ;
	TempRoom.m_MetersX	= m_MetersX;
	TempRoom.m_MetersY	= m_MetersY;
	TempRoom.m_MetersZ	= m_MetersZ;

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		RosaRoomEditor::SBrush&	RoomBrush	= TempRoom.m_Brushes.PushBack();
		const SBrush&			Brush		= BrushIter.GetValue();

		ASSERT( Brush.m_Type == EBT_Room );
		RoomBrush.m_Type		= Brush.m_Type;
		RoomBrush.m_Selected	= Brush.m_Selected;
		RoomBrush.m_Hidden		= Brush.m_Hidden;
		RoomBrush.m_Location	= Brush.m_Location;
		RoomBrush.m_Orientation	= Brush.m_Orientation;

		// Set defname to the room module if needed
		const SRoomDef& RoomDef = m_RoomDefs[ Brush.m_DefIndex ];
		RoomBrush.m_DefName = RoomDef.m_DefName;
	}

	TempRoom.Save( Stream );

	if( !m_SavingUndoState )
	{
		m_SavedUndoStateIndex = m_UndoStateIndex;
	}
}

void RosaTools::LoadWorld( const IDataStream& Stream )
{
	RosaRoomEditor TempRoom;

	TempRoom.Load( Stream );
	ASSERT( TempRoom.m_MapType == EMT_World );

	Reinitialize();

	if( !m_LoadingUndoState )
	{
		m_CameraLocation	= TempRoom.m_TOOLS_CameraLocation;
		m_CameraOrientation	= TempRoom.m_TOOLS_CameraOrientation;
	}

	m_TilesX	= TempRoom.m_TilesX;
	m_TilesY	= TempRoom.m_TilesY;
	m_TilesZ	= TempRoom.m_TilesZ;
	m_MetersX	= TempRoom.m_MetersX;
	m_MetersY	= TempRoom.m_MetersY;
	m_MetersZ	= TempRoom.m_MetersZ;

	ClearBrushes();
	for( uint BrushIndex = 0; BrushIndex < TempRoom.m_Brushes.Size(); ++BrushIndex )
	{
		const RosaRoomEditor::SBrush& RoomBrush = TempRoom.m_Brushes[ BrushIndex ];

		ASSERT( RoomBrush.m_Type == EBT_Room );

		uint DefIndex = 0;
		if( !FindRoomDef( RoomBrush.m_DefName, &DefIndex ) )
		{
			// NOTE: To fix up deprecated rooms:
			// Deprecated	= true
			// RemappedDef	= "..."

			MAKEHASHFROM( OriginalRoomDef, RoomBrush.m_DefName );
			STATICHASH( RemappedDef );
			const SimpleString RemappedRoomDef = ConfigManager::GetString( sRemappedDef, "", sOriginalRoomDef );

			if( !FindRoomDef( RemappedRoomDef, &DefIndex ) )
			{
				PRINTF( "Unknown and unremapped room \"%s\" loaded; if you resave this map, the room will be lost.\n", RoomBrush.m_DefName.CStr() );
				WARNDESC( "Unknown and unremapped room loaded; if you resave this map, the room will be lost." );
				continue;
			}
		}

		SBrush& Brush	= CreateBrush( RoomBrush.m_Type, DefIndex, RoomBrush.m_Location, RoomBrush.m_Orientation, RoomBrush.m_Scale, RoomBrush.m_LinkedBrushes );
		SetBrushSelected( Brush, RoomBrush.m_Selected );
		Brush.m_Hidden	= RoomBrush.m_Hidden;
	}

	ResetPortals();

	InitializeGridMeshes();
	InitializePortalMesh();
	InitializeHelpMesh();

	if( !m_LoadingUndoState )
	{
		StoreUndoState();
	}
}

void RosaTools::SaveRoom( const IDataStream& Stream )
{
	RosaRoomEditor TempRoom;

	TempRoom.m_MapType					= EMT_Room;
	TempRoom.m_TOOLS_CameraLocation		= m_CameraLocation;
	TempRoom.m_TOOLS_CameraOrientation	= m_CameraOrientation;

	TempRoom.m_TilesX	= m_TilesX;
	TempRoom.m_TilesY	= m_TilesY;
	TempRoom.m_TilesZ	= m_TilesZ;
	TempRoom.m_MetersX	= m_MetersX;
	TempRoom.m_MetersY	= m_MetersY;
	TempRoom.m_MetersZ	= m_MetersZ;

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		RosaRoomEditor::SBrush&	RoomBrush	= TempRoom.m_Brushes.PushBack();
		const SBrush&			Brush		= BrushIter.GetValue();

		RoomBrush.m_Type			= Brush.m_Type;
		RoomBrush.m_Selected		= Brush.m_Selected;
		RoomBrush.m_Hidden			= Brush.m_Hidden;
		RoomBrush.m_Location		= Brush.m_Location;
		RoomBrush.m_Orientation		= Brush.m_Orientation;
		RoomBrush.m_Scale			= Brush.m_Scale;
		RoomBrush.m_Mat				= Brush.m_Mat;
		RoomBrush.m_LinkedBrushes	= Brush.m_LinkedBrushes;

		if( Brush.m_Type == EBT_Geo )
		{
			const SGeoDef& BrushDef = m_GeoDefs[ Brush.m_DefIndex ];
			RoomBrush.m_DefName = BrushDef.m_DefName;
		}
		else if( Brush.m_Type == EBT_Spawner )
		{
			const SSpawnerDef& SpawnerDef = m_SpawnerDefs[ Brush.m_DefIndex ];
			RoomBrush.m_DefName = SpawnerDef.m_DefName;
		}
	}

	FOR_EACH_ARRAY( PortalIter, m_Portals, SPortals )
	{
		RosaRoomEditor::SPortals&		RoomPortals	= TempRoom.m_Portals.PushBack();
		const SPortals&					Portals		= PortalIter.GetValue();
		for( uint Index = 0; Index < 6; ++Index )
		{
			RosaRoomEditor::SPortal&	RoomPortal		= RoomPortals.m_Portals[ Index ];
			const uint					PortalDefIndex	= Portals.m_Portals[ Index ];
			const SPortalDef&			PortalDef		= m_PortalDefs[ PortalDefIndex ];

			RoomPortal.m_DefName	= PortalDef.m_DefName;
		}
	}

	TempRoom.m_NavVerts	= m_NavVerts;
	TempRoom.m_NavEdges	= m_NavEdges;
	TempRoom.m_NavFaces	= m_NavFaces;

	// HACKHACK: Populate nav face heights at the last minute to keep them in sync with geo changes.
	// This would be a RoomBaker feature but ::TraceGeo exists so it's easier to do here.
	FOR_EACH_ARRAY( NavFaceIter, m_NavFaces, SNavFace )
	{
		SNavFace&		NavFace				= NavFaceIter.GetValue();
		CollisionInfo	Info;
		const Vector	NavFaceCentroid		= GetNavFaceCentroid( NavFace );
		const Vector	UpVector			= Vector( 0.0f, 0.0f, 1.0f );
		const float		TraceOffset			= 0.1f;												// Magic number, start trace above nav centroid in case it intersects ground
		const float		TraceLength			= static_cast<float>( m_MetersZ ) * 2.0f;			// m_MetersZ covers the whole room, x2 is just in case nav goes out of bounds for any reason
		const Vector	TraceStart			= NavFaceCentroid	+ UpVector * TraceOffset;
		const Vector	TraceEnd			= TraceStart		+ UpVector * TraceLength;
		const Segment	TraceSegment		= Segment( TraceStart, TraceEnd );
		const bool		IgnoreNonBlockers	= true;
		if( TraceGeo( TraceSegment, Info, IgnoreNonBlockers ) ) // I used to do || TraceBoundPlanes( TraceSegment, Info ) ) but then stairwells into a room above would be falsely clipped.
		{
			if( Info.m_Out_HitT > 0.0f )
			{
				NavFace.m_Height			= ( Info.m_Out_HitT * TraceLength ) + TraceOffset;	// Re-add the magic number so the height starts from centroid (obviously, this doesn't work great for slopes)
			}
			else
			{
				PRINTF( "Nav height trace started intersecting geo, couldn't get valid height information" );
				NavFace.m_Height			= 0.0f;
			}
		}
		else
		{
			NavFace.m_Height				= TraceLength + TraceOffset;
		}
	}

	TempRoom.Save( Stream );

	if( !m_SavingUndoState )
	{
		m_SavedUndoStateIndex = m_UndoStateIndex;
	}
}

void RosaTools::LoadRoom( const IDataStream& Stream )
{
	RosaRoomEditor TempRoom;

	TempRoom.Load( Stream );
	ASSERT( TempRoom.m_MapType == EMT_Room );

	Reinitialize();

	if( !m_LoadingUndoState )
	{
		m_CameraLocation	= TempRoom.m_TOOLS_CameraLocation;
		m_CameraOrientation	= TempRoom.m_TOOLS_CameraOrientation;
	}

	m_TilesX	= TempRoom.m_TilesX;
	m_TilesY	= TempRoom.m_TilesY;
	m_TilesZ	= TempRoom.m_TilesZ;
	m_MetersX	= TempRoom.m_MetersX;
	m_MetersY	= TempRoom.m_MetersY;
	m_MetersZ	= TempRoom.m_MetersZ;

	ClearBrushes();
	for( uint BrushIndex = 0; BrushIndex < TempRoom.m_Brushes.Size(); ++BrushIndex )
	{
		const RosaRoomEditor::SBrush& RoomBrush = TempRoom.m_Brushes[ BrushIndex ];

		uint DefIndex = 0;
		if( RoomBrush.m_Type == EBT_Geo )
		{
			if( !FindGeoDef( RoomBrush.m_DefName, &DefIndex ) )
			{
				// NOTE: To fix up deprecated geo:
				// Deprecated	= true
				// RemappedDef	= "..."
				// (See also Deprecated in rosaworldgen.cpp) [ROSATODO: that part]

				MAKEHASHFROM( OriginalGeoDef, RoomBrush.m_DefName );
				STATICHASH( RemappedDef );
				const SimpleString RemappedGeoDef = ConfigManager::GetString( sRemappedDef, "", sOriginalGeoDef );

				if( !FindGeoDef( RemappedGeoDef, &DefIndex ) )
				{
					PRINTF( "Unknown and unremapped geo \"%s\" loaded; if you resave this map, the geo will be lost.\n", RoomBrush.m_DefName.CStr() );
					WARNDESC( "Unknown and unremapped geo loaded; if you resave this map, the geo will be lost." );
					continue;
				}
			}
		}
		else if( RoomBrush.m_Type == EBT_Spawner )
		{
			if( !FindSpawnerDef( RoomBrush.m_DefName, &DefIndex ) )
			{
				// NOTE: To fix up deprecated spawners:
				// Deprecated	= true
				// RemappedDef	= "..."

				MAKEHASHFROM( OriginalSpawnerDef, RoomBrush.m_DefName );
				STATICHASH( RemappedDef );
				const SimpleString RemappedSpawnerDef = ConfigManager::GetString( sRemappedDef, "", sOriginalSpawnerDef );

				if( !FindSpawnerDef( RemappedSpawnerDef, &DefIndex ) )
				{
					PRINTF( "Unknown and unremapped spawner \"%s\" loaded; if you resave this map, the spawners will be lost.\n", RoomBrush.m_DefName.CStr() );
					WARNDESC( "Unknown and unremapped spawner loaded; if you resave this map, the spawners will be lost." );
					continue;
				}
			}
		}

		SBrush& Brush	= CreateBrush( RoomBrush.m_Type, DefIndex, RoomBrush.m_Location, RoomBrush.m_Orientation, RoomBrush.m_Scale, RoomBrush.m_LinkedBrushes );
		SetBrushSelected( Brush, RoomBrush.m_Selected );
		Brush.m_Hidden	= RoomBrush.m_Hidden;

		uint MatDefIndex;
		if( FindMatDef( RoomBrush.m_Mat, &MatDefIndex ) )
		{
			ApplyMatToBrush( MatDefIndex, Brush );
		}
	}

	ResetPortals();
	const uint NumPortals = m_TilesX * m_TilesY * m_TilesZ;
	ASSERT( NumPortals == TempRoom.m_Portals.Size() );
	for( uint PortalIndex = 0; PortalIndex < NumPortals; ++PortalIndex )
	{
		SPortals&						Portals		= m_Portals[ PortalIndex ];
		const RosaRoomEditor::SPortals& RoomPortals = TempRoom.m_Portals[ PortalIndex ];
		for( uint Index = 0; Index < 6; ++Index )
		{
			const RosaRoomEditor::SPortal& RoomPortal = RoomPortals.m_Portals[ Index ];
			uint PortalDefIndex = 0;
			if( !FindPortalDef( RoomPortal.m_DefName, &PortalDefIndex ) )
			{
				// NOTE: To fix up deprecated portals:
				// RemappedDef	= "..."

				MAKEHASHFROM( OriginalPortalDef, RoomPortal.m_DefName );
				STATICHASH( RemappedDef );
				const SimpleString RemappedPortalDef = ConfigManager::GetString( sRemappedDef, "", sOriginalPortalDef );

				if( !FindPortalDef( RemappedPortalDef, &PortalDefIndex ) )
				{
					PRINTF( "Unknown and unremapped portal \"%s\" loaded; if you resave this map, the portal will be lost.\n", RoomPortal.m_DefName.CStr() );
					WARNDESC( "Unknown and unremapped portal loaded; if you resave this map, the portal will be lost." );
					continue;
				}
			}

			Portals.m_Portals[ Index ] = PortalDefIndex;
		}
	}

	m_NavVerts	= TempRoom.m_NavVerts;
	m_NavEdges	= TempRoom.m_NavEdges;
	m_NavFaces	= TempRoom.m_NavFaces;
	m_SelectedNavVerts.Clear();
	m_SelectedNavEdges.Clear();
	m_SelectedNavFaces.Clear();
	FOR_EACH_ARRAY( VertIter, m_NavVerts, SNavVert ) { SetNavVertSelected( VertIter.GetIndex(), VertIter.GetValue().m_Selected ); }
	FOR_EACH_ARRAY( EdgeIter, m_NavEdges, SNavEdge ) { SetNavEdgeSelected( EdgeIter.GetIndex(), EdgeIter.GetValue().m_Selected ); }
	FOR_EACH_ARRAY( FaceIter, m_NavFaces, SNavFace ) { SetNavFaceSelected( FaceIter.GetIndex(), FaceIter.GetValue().m_Selected ); }
	m_NavSnapPoints.Clear();
	ReduceNavElements();
	CreateNavMesh();

	InitializeGridMeshes();
	InitializePortalMesh();
	InitializeHelpMesh();

	if( !m_LoadingUndoState )
	{
		StoreUndoState();
	}
}

void RosaTools::LoadWorldFromStoredModules()
{
	ASSERT( IsInWorldMode() );

	RosaWorld* const	pWorld					= RosaFramework::GetInstance()->GetWorld();
	RosaWorldGen* const	pWorldGen				= pWorld->GetWorldGen();
	const Array<RosaWorldGen::SModule>&	Modules	= pWorldGen->GetModules();

	const Vector	CameraLocation		= m_CameraLocation;
	const Angles	CameraOrientation	= m_CameraOrientation;

	Reinitialize();

	m_TilesX			= 1;
	m_TilesY			= 1;
	m_TilesZ			= 1;
	m_MetersX			= pWorldGen->m_TileSizeX;
	m_MetersY			= pWorldGen->m_TileSizeY;
	m_MetersZ			= pWorldGen->m_TileSizeZ;

	m_CameraLocation	= CameraLocation;
	m_CameraOrientation	= CameraOrientation;

	FOR_EACH_ARRAY( ModuleIter, Modules, RosaWorldGen::SModule )
	{
		const RosaWorldGen::SModule&	Module				= ModuleIter.GetValue();
		const SimpleString				RawRoomName			= SimpleString( "../Raw/" ) + Module.m_Filename.Replace( ".rrm", ".rosaroom" );	// HACKHACK

		uint DefIndex = 0;
		if( !FindRoomDef( RawRoomName, &DefIndex ) )
		{
			PRINTF( "Unknown room \"%s\" loaded; if you resave this map, the room will be lost.\n", RawRoomName.CStr() );
			WARNDESC( "Unknown room loaded; if you resave this map, the room will be lost." );
			continue;
		}

		// Module.m_Location is the "low loc," not the origin.
		// We need to restore the origin a la RosaWorldGen::GetOriginFromLowLocForWorld().

		// Peek at the room file for its dimensions
		RosaRoomEditor					TempRoom;
		TempRoom.Load( FileStream( RawRoomName.CStr(), FileStream::EFM_Read ) );
		ASSERT( TempRoom.m_MapType == EMT_Room );

		// Grow the bounds as rooms are added
		m_TilesX	= Max( m_TilesX, Module.m_Location.X + TempRoom.m_TilesX );
		m_TilesY	= Max( m_TilesY, Module.m_Location.Y + TempRoom.m_TilesY );
		m_TilesZ	= Max( m_TilesZ, Module.m_Location.Z + TempRoom.m_TilesZ );
		m_MetersX	= m_TilesX * pWorldGen->m_TileSizeX;
		m_MetersY	= m_TilesY * pWorldGen->m_TileSizeY;
		m_MetersZ	= m_TilesZ * pWorldGen->m_TileSizeZ;

		RosaWorldGen::SRoomLoc			OriginLoc;
		int OffsetX, OffsetY, OffsetZ;
		pWorldGen->GetTransformedOffset( TempRoom.m_TilesX, TempRoom.m_TilesY, TempRoom.m_TilesZ, Module.m_Transform, OffsetX, OffsetY, OffsetZ );
		OriginLoc.X	= Module.m_Location.X + Max( -OffsetX, 0 );
		OriginLoc.Y	= Module.m_Location.Y + Max( -OffsetY, 0 );
		OriginLoc.Z	= Module.m_Location.Z + Max( -OffsetZ, 0 );

		const Angles					Orientation			= pWorldGen->GetAnglesForTransform( Module.m_Transform );

		static const Vector				skHalfVector		= Vector( 0.5f, 0.5f, 0.5f );
		const Matrix					OrientationMat		= Orientation.ToMatrix();
		const Vector					Location			= ( skHalfVector * OrientationMat ) + Vector( static_cast<float>( OriginLoc.X ), static_cast<float>( OriginLoc.Y ), static_cast<float>( OriginLoc.Z ) );

		static const float				skScale				= 1.0f;
		static const Array<uint>		skLinkedBrushes;
		CreateBrush( EBT_Room, DefIndex, Location, Orientation, skScale, skLinkedBrushes );
	}

	DeselectAllBrushes();

	ResetPortals();

	InitializeGridMeshes();
	InitializePortalMesh();
	InitializeHelpMesh();

	if( !m_LoadingUndoState )
	{
		StoreUndoState();
	}
}

void RosaTools::ConvertWorldToRoom()
{
	ASSERT( IsInWorldMode() );
	SetSubTool( EST_RoomMode, false );

	// NOTE: This is largely copied from LoadRoom, unify them

	const uint		MetersPerTileX		= m_MetersX / m_TilesX;
	const uint		MetersPerTileY		= m_MetersY / m_TilesY;
	const uint		MetersPerTileZ		= m_MetersZ / m_TilesZ;
	const Vector	MetersPerTile		= Vector( static_cast<float>( MetersPerTileX ), static_cast<float>( MetersPerTileY ), static_cast<float>( MetersPerTileZ ) );

	const uint		TilesX				= m_TilesX;
	const uint		TilesY				= m_TilesY;
	const uint		TilesZ				= m_TilesZ;
	const uint		MetersX				= m_MetersX;
	const uint		MetersY				= m_MetersY;
	const uint		MetersZ				= m_MetersZ;

	const Vector	CameraLocation		= m_CameraLocation * MetersPerTile;
	const Angles	CameraOrientation	= m_CameraOrientation;

	// Make a local copy of world's room brushes, we're about to blow them all away
	// HACKHACK: Don't copy the whole array, it won't deep copy meshes and stuff.
	// Just grab what we need.
	Array<SBrush>	WorldRoomBrushes;
	WorldRoomBrushes.Reserve( m_Brushes.Size() );
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush&	Brush		= BrushIter.GetValue();
		ASSERT( Brush.m_Type == EBT_Room );

		SBrush&			NewBrush	= WorldRoomBrushes.PushBack();
		NewBrush.m_Type				= Brush.m_Type;
		NewBrush.m_Location			= Brush.m_Location;
		NewBrush.m_Orientation		= Brush.m_Orientation;
		NewBrush.m_DefIndex			= Brush.m_DefIndex;
	}

	Reinitialize();

	m_TilesX			= TilesX;
	m_TilesY			= TilesY;
	m_TilesZ			= TilesZ;
	m_MetersX			= MetersX;
	m_MetersY			= MetersY;
	m_MetersZ			= MetersZ;

	m_CameraLocation	= CameraLocation;
	m_CameraOrientation	= CameraOrientation;

	FOR_EACH_ARRAY( WorldRoomBrushIter, WorldRoomBrushes, SBrush )
	{
		const SBrush&		WorldRoomBrush					= WorldRoomBrushIter.GetValue();
		ASSERT( WorldRoomBrush.m_Type == EBT_Room );
		const SRoomDef&		RoomDef							= m_RoomDefs[ WorldRoomBrush.m_DefIndex ];

		static const Vector	skHalfVector					= Vector( 0.5f, 0.5f, 0.5f );
		const Matrix		WorldRoomBrushOrientationMat	= WorldRoomBrush.m_Orientation.ToMatrix();
		const Vector		WorldRoomBrushLocation			= ( WorldRoomBrush.m_Location - ( skHalfVector * WorldRoomBrushOrientationMat ) ) * MetersPerTile;
		AddLoadRoomWithTransform( FileStream( RoomDef.m_DefName.CStr(), FileStream::EFM_Read ), WorldRoomBrushLocation, WorldRoomBrush.m_Orientation );
	}

	DeselectAllBrushes();

	// NOTE: I'm not trying to fix up portals; it's easy enough to do those by hand,
	// and there's a good chance I'll be using this feature to adapt different tile sizes
	// so they wouldn't work anyway.
	ResetPortals();

	m_NavSnapPoints.Clear();
	ReduceNavElements();
	CreateNavMesh();

	DeselectAllNav();

	InitializeGridMeshes();
	InitializePortalMesh();
	InitializeHelpMesh();

	if( !m_LoadingUndoState )
	{
		StoreUndoState();
	}
}

void RosaTools::AddLoadRoomWithTransform( const IDataStream& Stream, const Vector& Location, const Angles& Orientation )
{
	RosaRoomEditor TempRoom;

	TempRoom.Load( Stream );
	ASSERT( TempRoom.m_MapType == EMT_Room );

	const Matrix	LocationMat		= Matrix::CreateTranslation( Location );
	const Matrix	OrientationMat	= Orientation.ToMatrix();
	const Matrix	TransformMat	= OrientationMat * LocationMat;
	const uint		NumBrushes		= m_Brushes.Size();

	// NOTE: This is largely copied from LoadRoom, unify them
	for( uint BrushIndex = 0; BrushIndex < TempRoom.m_Brushes.Size(); ++BrushIndex )
	{
		const RosaRoomEditor::SBrush& RoomBrush = TempRoom.m_Brushes[ BrushIndex ];

		uint DefIndex = 0;
		if( RoomBrush.m_Type == EBT_Geo )
		{
			if( !FindGeoDef( RoomBrush.m_DefName, &DefIndex ) )
			{
				// NOTE: To fix up deprecated geo:
				// Deprecated	= true
				// RemappedDef	= "..."
				// (See also Deprecated in rosaworldgen.cpp) [ROSATODO: that part]

				MAKEHASHFROM( OriginalGeoDef, RoomBrush.m_DefName );
				STATICHASH( RemappedDef );
				const SimpleString RemappedGeoDef = ConfigManager::GetString( sRemappedDef, "", sOriginalGeoDef );

				if( !FindGeoDef( RemappedGeoDef, &DefIndex ) )
				{
					PRINTF( "Unknown and unremapped geo \"%s\" loaded; if you resave this map, the geo will be lost.\n", RoomBrush.m_DefName.CStr() );
					WARNDESC( "Unknown and unremapped geo loaded; if you resave this map, the geo will be lost." );
					continue;
				}
			}
		}
		else if( RoomBrush.m_Type == EBT_Spawner )
		{
			if( !FindSpawnerDef( RoomBrush.m_DefName, &DefIndex ) )
			{
				// NOTE: To fix up deprecated spawners:
				// Deprecated	= true
				// RemappedDef	= "..."

				MAKEHASHFROM( OriginalSpawnerDef, RoomBrush.m_DefName );
				STATICHASH( RemappedDef );
				const SimpleString RemappedSpawnerDef = ConfigManager::GetString( sRemappedDef, "", sOriginalSpawnerDef );

				if( !FindSpawnerDef( RemappedSpawnerDef, &DefIndex ) )
				{
					PRINTF( "Unknown and unremapped spawner \"%s\" loaded; if you resave this map, the spawners will be lost.\n", RoomBrush.m_DefName.CStr() );
					WARNDESC( "Unknown and unremapped spawner loaded; if you resave this map, the spawners will be lost." );
					continue;
				}
			}
		}

		const Vector	BrushLocation		= RoomBrush.m_Location * TransformMat;
		const Angles	BrushOrientation	= ( RoomBrush.m_Orientation.ToMatrix() * OrientationMat ).ToAngles();
		SBrush& Brush	= CreateBrush( RoomBrush.m_Type, DefIndex, BrushLocation, BrushOrientation, RoomBrush.m_Scale, RoomBrush.m_LinkedBrushes );
		SetBrushSelected( Brush, RoomBrush.m_Selected );
		Brush.m_Hidden	= RoomBrush.m_Hidden;

		// Add an offset to the new brush's LinkedBrushes to fix up
		FOR_EACH_ARRAY( LinkedBrushIter, Brush.m_LinkedBrushes, uint )
		{
			uint&	LinkedBrush	= LinkedBrushIter.GetValue();
			LinkedBrush			+= NumBrushes;
		}

		uint MatDefIndex;
		if( FindMatDef( RoomBrush.m_Mat, &MatDefIndex ) )
		{
			ApplyMatToBrush( MatDefIndex, Brush );
		}
	}

	// NOTE: We don't need to do anything with portals here

	const uint	NumNavVerts	= m_NavVerts.Size();
	const uint	NumNavEdges	= m_NavEdges.Size();
	const uint	NumNavFaces	= m_NavFaces.Size();

	m_NavVerts.Append( TempRoom.m_NavVerts );
	m_NavEdges.Append( TempRoom.m_NavEdges );
	m_NavFaces.Append( TempRoom.m_NavFaces );

	// Apply transform to new nav verts, apply offsets to edges and faces
	FOR_EACH_INDEX_FROM( NavVertIndex, NumNavVerts, m_NavVerts.Size() )
	{
		SNavVert&	NavVert	= m_NavVerts[ NavVertIndex ];
		NavVert.m_Vert		= NavVert.m_Vert * TransformMat;
	}

	FOR_EACH_INDEX_FROM( NavEdgeIndex, NumNavEdges, m_NavEdges.Size() )
	{
		SNavEdge&	NavEdge	= m_NavEdges[ NavEdgeIndex ];
		NavEdge.m_VertA		+= NumNavVerts;
		NavEdge.m_VertB		+= NumNavVerts;
	}

	FOR_EACH_INDEX_FROM( NavFaceIndex, NumNavFaces, m_NavFaces.Size() )
	{
		SNavFace&	NavFace	= m_NavFaces[ NavFaceIndex ];
		NavFace.m_EdgeA		+= NumNavEdges;
		NavFace.m_EdgeB		+= NumNavEdges;
		NavFace.m_EdgeC		+= NumNavEdges;
	}
}

void RosaTools::StoreUndoState()
{
	if( m_LoadingUndoState )
	{
		return;
	}

	m_SavingUndoState = true;

	// Drop oldest state if we've reached our maximum
	// ROSAHACK: Hard-coded limit! Configurate?
	static const uint sMaxUndoStates = 100;
	if( m_UndoStateIndex + 1 == sMaxUndoStates )
	{
		m_UndoStates.PopFront();
	}
	else
	{
		m_UndoStateIndex++;
	}

	ASSERT( m_UndoStateIndex >= 0 );

	// Clear any states beyond this point, in case we're branching from somewhere in the undo history.
	// (Don't use Resize(), it won't properly destruct the states.)
	while( m_UndoStates.Size() > static_cast<uint>( m_UndoStateIndex ) )
	{
		m_UndoStates.PopBack();
	}

	DynamicMemoryStream UndoStateStream;
	SaveStream( UndoStateStream );
	m_UndoStates.PushBack( UndoStateStream.GetArray() );

	m_SavingUndoState = false;
}

void RosaTools::TryUndo()
{
	const int UndoIndex = m_UndoStateIndex - 1;
	if( UndoIndex < 0 )
	{
		return;
	}

	m_LoadingUndoState = true;

	// HACKHACK around initialization resetting this
	SimpleString& CurrentMapName = IsInRoomMode() ? m_CurrentRoomName : m_CurrentWorldName;
	const SimpleString MapName = CurrentMapName;

	const TUndoState& UndoState = m_UndoStates[ UndoIndex ];
	LoadStream( MemoryStream( UndoState.GetData(), UndoState.MemorySize() ) );
	m_UndoStateIndex--;

	// HACKHACK
	CurrentMapName = MapName;

	m_LoadingUndoState = false;
}

void RosaTools::TryRedo()
{
	const int RedoIndex = m_UndoStateIndex + 1;
	if( RedoIndex >= static_cast<int>( m_UndoStates.Size() ) )
	{
		return;
	}

	m_LoadingUndoState = true;

	const TUndoState& RedoState = m_UndoStates[ RedoIndex ];
	LoadStream( MemoryStream( RedoState.GetData(), RedoState.MemorySize() ) );
	m_UndoStateIndex++;

	m_LoadingUndoState = false;
}

void RosaTools::TryStartTranslate()
{
	if( m_SelectedBrushes.Size() < 1 )
	{
		return;
	}

	StartTranslate();
}

void RosaTools::StartTranslate()
{
	m_IsTranslating = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );

	FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
	{
		SBrush&	Brush			= m_Brushes[ BrushIter.GetValue() ];
		Brush.m_TranslateStart	= Brush.m_Location;
	}

	// Also move anchor now that it's a separate transform
	m_TranslateAnchorStart	= m_TransformAnchor;

	Project( m_TransformAnchor, m_TransformAnchorScreenX, m_TransformAnchorScreenY );
}

void RosaTools::EndTranslate( const bool Cancel )
{
	m_IsTranslating = false;

	if( Cancel )
	{
		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			SBrush& Brush = m_Brushes[ BrushIter.GetValue() ];
			SetBrushLocation( Brush, Brush.m_TranslateStart );
		}

		// Also move anchor now that it's a separate transform
		SetAnchorLocation( m_TranslateAnchorStart );
	}
	else
	{
		StoreUndoState();
	}
}

void RosaTools::TickTranslate( const bool Vertical )
{
	ASSERT( m_IsTranslating );

	Mouse* const pMouse = m_Framework->GetMouse();
	int CursorX, CursorY;
	pMouse->GetPosition( CursorX, CursorY, m_Framework->GetWindow() );

	const int OffsetX = CursorX - m_TransformStartCursorX;
	const int OffsetY = CursorY - m_TransformStartCursorY;

	const int		AnchorScreenX		= m_TransformAnchorScreenX + OffsetX;
	const int		AnchorScreenY		= m_TransformAnchorScreenY + OffsetY;
	const Segment	UnprojectedSegment	= Unproject( AnchorScreenX, AnchorScreenY );
	const Line		UnprojectedLine		= Line( UnprojectedSegment.m_Point1, UnprojectedSegment.GetDirection() );

	Vector UnprojectedAnchor;
	if( Vertical )
	{
		const Line ZLine	= Line( m_TranslateAnchorStart, Vector( 0.0f, 0.0f, 1.0f ) );
		UnprojectedAnchor	= ZLine.NearestPointTo( UnprojectedLine );
	}
	else
	{
		const Plane XYPlane	= Plane( Vector( 0.0f, 0.0f, 1.0f ), -m_TranslateAnchorStart.z );
		UnprojectedAnchor	= UnprojectedLine.GetIntersection( XYPlane );
	}

	const Vector AnchorOffset = SnapToGrid( UnprojectedAnchor - m_TranslateAnchorStart );

	FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
	{
		SBrush&			Brush		= m_Brushes[ BrushIter.GetValue() ];
		const Vector	NewLocation	= Brush.m_TranslateStart + AnchorOffset;
		SetBrushLocation( Brush, NewLocation );
	}

	// Also move anchor now that it's a separate transform
	SetAnchorLocation( m_TranslateAnchorStart + AnchorOffset );
}

void RosaTools::SetBrushLocation( SBrush& Brush, const Vector& Location )
{
	Brush.m_Location					= Location;
	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh&		BrushMesh		= BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->m_Location	= Brush.m_Location + ( ( BrushMesh.m_Offset * Brush.m_Scale ) * Brush.m_Orientation.ToMatrix() );
		BrushMesh.m_Mesh->RecomputeAABB();
	}
}

void RosaTools::TryStartTranslateAnchor()
{
	if( m_SelectedBrushes.Size() < 1 )
	{
		return;
	}

	StartTranslateAnchor();
}

void RosaTools::StartTranslateAnchor()
{
	m_IsTranslatingAnchor = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );

	m_TranslateAnchorStart	= m_TransformAnchor;

	Project( m_TransformAnchor, m_TransformAnchorScreenX, m_TransformAnchorScreenY );
}

void RosaTools::EndTranslateAnchor( const bool Cancel )
{
	m_IsTranslatingAnchor = false;

	if( Cancel )
	{
		SetAnchorLocation( m_TranslateAnchorStart );
	}
	else
	{
		StoreUndoState();
	}
}

void RosaTools::TickTranslateAnchor( const bool Vertical )
{
	ASSERT( m_IsTranslatingAnchor );

	Mouse* const pMouse = m_Framework->GetMouse();
	int CursorX, CursorY;
	pMouse->GetPosition( CursorX, CursorY, m_Framework->GetWindow() );

	const int OffsetX = CursorX - m_TransformStartCursorX;
	const int OffsetY = CursorY - m_TransformStartCursorY;

	const int		AnchorScreenX		= m_TransformAnchorScreenX + OffsetX;
	const int		AnchorScreenY		= m_TransformAnchorScreenY + OffsetY;
	const Segment	UnprojectedSegment	= Unproject( AnchorScreenX, AnchorScreenY );
	const Line		UnprojectedLine		= Line( UnprojectedSegment.m_Point1, UnprojectedSegment.GetDirection() );

	Vector UnprojectedAnchor;
	if( Vertical )
	{
		const Line ZLine	= Line( m_TranslateAnchorStart, Vector( 0.0f, 0.0f, 1.0f ) );
		UnprojectedAnchor	= ZLine.NearestPointTo( UnprojectedLine );
	}
	else
	{
		const Plane XYPlane	= Plane( Vector( 0.0f, 0.0f, 1.0f ), -m_TranslateAnchorStart.z );
		UnprojectedAnchor	= UnprojectedLine.GetIntersection( XYPlane );
	}

	const Vector AnchorOffset = SnapToGrid( UnprojectedAnchor - m_TranslateAnchorStart );

	SetAnchorLocation( m_TranslateAnchorStart + AnchorOffset );
}

void RosaTools::SetAnchorLocation( const Vector& Location )
{
	m_TransformAnchor = Location;
}

void RosaTools::TryStartRotate()
{
	if( m_SelectedBrushes.Size() < 1 )
	{
		return;
	}

	StartRotate();
}

void RosaTools::StartRotate()
{
	m_IsRotating = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );

	FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
	{
		SBrush&	Brush			= m_Brushes[ BrushIter.GetValue() ];
		Brush.m_TranslateStart	= Brush.m_Location;
		Brush.m_RotateStart		= Brush.m_Orientation;
	}

	Project( m_TransformAnchor, m_TransformAnchorScreenX, m_TransformAnchorScreenY );
}

void RosaTools::EndRotate( const bool Cancel )
{
	m_IsRotating = false;

	if( Cancel )
	{
		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			SBrush& Brush = m_Brushes[ BrushIter.GetValue() ];
			SetBrushLocation( Brush, Brush.m_TranslateStart );
			SetBrushOrientation( Brush, Brush.m_RotateStart );
		}
	}
	else
	{
		StoreUndoState();
	}
}

void RosaTools::TickRotate( const Vector& RotateAxis )
{
	ASSERT( m_IsRotating );

	Mouse* const pMouse = m_Framework->GetMouse();
	int CursorX, CursorY;
	pMouse->GetPosition( CursorX, CursorY, m_Framework->GetWindow() );

	const float		PlaneDistance		= RotateAxis.Dot( m_TransformAnchor );
	const Plane		RotatePlane			= Plane( RotateAxis, -PlaneDistance );

	const Segment	StartSegment		= Unproject( m_TransformStartCursorX, m_TransformStartCursorY );
	const Line		StartLine			= Line( StartSegment.m_Point1, StartSegment.GetDirection() );
	const Vector	TransformStart		= StartLine.GetIntersection( RotatePlane );

	const Segment	UnprojectedSegment	= Unproject( CursorX, CursorY );
	const Line		UnprojectedLine		= Line( UnprojectedSegment.m_Point1, UnprojectedSegment.GetDirection() );
	const Vector	UnprojectedCursor	= UnprojectedLine.GetIntersection( RotatePlane );
	const Vector	AnchorToStart		= TransformStart - m_TransformAnchor;
	const Vector	AnchorToCursor		= UnprojectedCursor - m_TransformAnchor;
	const float		CosTheta			= Clamp( AnchorToStart.GetNormalized().Dot( AnchorToCursor.GetNormalized() ), -1.0f, 1.0f );	// Somehow this was > 1, causing NaNs from ACos
	const float		Theta				= SnapToGrid( ACos( CosTheta ) * Sign( AnchorToStart.Cross( AnchorToCursor ).Dot( RotateAxis ) ), DEGREES_TO_RADIANS( m_RotateGridSize ) );
	const Matrix	RotationMatrix		= Matrix::CreateRotation( RotateAxis, Theta );

	FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
	{
		SBrush&			Brush			= m_Brushes[ BrushIter.GetValue() ];

		const Angles	NewOrientation	= ( Brush.m_RotateStart.ToMatrix() * RotationMatrix ).ToAngles();
		SetBrushOrientation( Brush, NewOrientation );

		const Vector	AnchorToBrush	= Brush.m_TranslateStart - m_TransformAnchor;
		const Vector	NewLocation		= m_TransformAnchor + ( AnchorToBrush * RotationMatrix );
		SetBrushLocation( Brush, NewLocation );
	}
}

void RosaTools::SetBrushOrientation( SBrush& Brush, const Angles& Orientation )
{
	Brush.m_Orientation					= Orientation;
	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh&		BrushMesh		= BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->m_Location	= Brush.m_Location + ( ( BrushMesh.m_Offset * Brush.m_Scale ) * Brush.m_Orientation.ToMatrix() );
		BrushMesh.m_Mesh->m_Rotation	= Orientation;
		BrushMesh.m_Mesh->RecomputeAABB();
	}
}

void RosaTools::TryStartScale()
{
	if( m_SelectedBrushes.Size() < 1 )
	{
		return;
	}

	StartScale();
}

void RosaTools::StartScale()
{
	m_IsScaling = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );

	FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
	{
		SBrush&	Brush			= m_Brushes[ BrushIter.GetValue() ];
		Brush.m_TranslateStart	= Brush.m_Location;
		Brush.m_ScaleStart		= Brush.m_Scale;
	}

	Project( m_TransformAnchor, m_TransformAnchorScreenX, m_TransformAnchorScreenY );
}

void RosaTools::EndScale( const bool Cancel )
{
	m_IsScaling = false;

	if( Cancel )
	{
		FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
		{
			SBrush& Brush = m_Brushes[ BrushIter.GetValue() ];
			SetBrushLocation( Brush, Brush.m_TranslateStart );
			SetBrushScale( Brush, Brush.m_ScaleStart );
		}
	}
	else
	{
		StoreUndoState();
	}
}

void RosaTools::TickScale()
{
	ASSERT( m_IsScaling );

	Mouse* const pMouse = m_Framework->GetMouse();
	int CursorX, CursorY;
	pMouse->GetPosition( CursorX, CursorY, m_Framework->GetWindow() );

	const Vector2	Anchor2D		= Vector2( static_cast<float>( m_TransformAnchorScreenX ), static_cast<float>( m_TransformAnchorScreenY ) );
	const Vector2	Start2D			= Vector2( static_cast<float>( m_TransformStartCursorX ), static_cast<float>( m_TransformStartCursorY ) );
	const Vector2	Current2D		= Vector2( static_cast<float>( CursorX ), static_cast<float>( CursorY ) );
	const float		StartLength		= ( Start2D - Anchor2D ).Length();
	const float		CurrentLength	= ( Current2D - Anchor2D ).Length();
	const float		Scale			= ( StartLength > EPSILON ) ? ( CurrentLength / StartLength ) : 1.0f;

	FOR_EACH_ARRAY( BrushIter, m_SelectedBrushes, uint )
	{
		SBrush&			Brush			= m_Brushes[ BrushIter.GetValue() ];

		const float		NewScale		= Brush.m_ScaleStart * Scale;
		SetBrushScale( Brush, NewScale );

		const Vector	AnchorToBrush	= Brush.m_TranslateStart - m_TransformAnchor;
		const Vector	NewLocation		= m_TransformAnchor + ( AnchorToBrush * Scale );
		SetBrushLocation( Brush, NewLocation );
	}
}

void RosaTools::SetBrushScale( SBrush& Brush, const float Scale )
{
	Brush.m_Scale						= Scale;
	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh&		BrushMesh		= BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->m_Location	= Brush.m_Location + ( ( BrushMesh.m_Offset * Brush.m_Scale ) * Brush.m_Orientation.ToMatrix() );
		BrushMesh.m_Mesh->m_Scale		= Vector( Scale, Scale, Scale );;
		BrushMesh.m_Mesh->RecomputeAABB();
	}
}

void RosaTools::StartBoxSelect()
{
	m_IsBoxSelecting = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );
}

void RosaTools::EndBoxSelect()
{
	m_IsBoxSelecting = false;

	Mouse* const pMouse = m_Framework->GetMouse();
	int MouseX, MouseY;
	pMouse->GetPosition( MouseX, MouseY, m_Framework->GetWindow() );

	int MouseXLo = Min( MouseX, m_TransformStartCursorX );
	int MouseXHi = Max( MouseX, m_TransformStartCursorX );
	int MouseYLo = Min( MouseY, m_TransformStartCursorY );
	int MouseYHi = Max( MouseY, m_TransformStartCursorY );

	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		SBrush&	Brush	= BrushIter.GetValue();
		if( Brush.m_Hidden )
		{
			continue;
		}

		if( ( IsInRoomMode() && Brush.m_Type != EBT_Geo && Brush.m_Type != EBT_Spawner ) ||
			( IsInWorldMode() && Brush.m_Type != EBT_Room ) )
		{
			continue;
		}

		int LocationScreenX, LocationScreenY;
		if( !Project( Brush.m_Location, LocationScreenX, LocationScreenY ) )
		{
			continue;
		}

		if( LocationScreenX < MouseXLo ||
			LocationScreenX > MouseXHi ||
			LocationScreenY < MouseYLo ||
			LocationScreenY > MouseYHi )
		{
			continue;
		}

		SelectBrush( Brush );
	}

	StoreUndoState();
}

Array<uint> RosaTools::GetSelectedNavVerts() const
{
	Array<uint> SelectedVerts = m_SelectedNavVerts;

	FOR_EACH_ARRAY( EdgeIter, m_SelectedNavEdges, uint )
	{
		const uint		EdgeIndex	= EdgeIter.GetValue();
		const SNavEdge&	Edge		= m_NavEdges[ EdgeIndex ];
		SelectedVerts.PushBackUnique( Edge.m_VertA );
		SelectedVerts.PushBackUnique( Edge.m_VertB );
	}

	FOR_EACH_ARRAY( FaceIter, m_SelectedNavFaces, uint )
	{
		const uint		FaceIndex	= FaceIter.GetValue();
		const SNavFace&	Face		= m_NavFaces[ FaceIndex ];
		const SNavEdge&	EdgeA		= m_NavEdges[ Face.m_EdgeA ];
		const SNavEdge&	EdgeB		= m_NavEdges[ Face.m_EdgeB ];
		const SNavEdge&	EdgeC		= m_NavEdges[ Face.m_EdgeC ];
		SelectedVerts.PushBackUnique( EdgeA.m_VertA );
		SelectedVerts.PushBackUnique( EdgeA.m_VertB );
		SelectedVerts.PushBackUnique( EdgeB.m_VertA );
		SelectedVerts.PushBackUnique( EdgeB.m_VertB );
		SelectedVerts.PushBackUnique( EdgeC.m_VertA );
		SelectedVerts.PushBackUnique( EdgeC.m_VertB );
	}

	return SelectedVerts;
}

void RosaTools::TryStartTranslateNav()
{
	if( m_SelectedNavVerts.Empty() &&
		m_SelectedNavEdges.Empty() &&
		m_SelectedNavFaces.Empty() )
	{
		return;
	}

	StartTranslateNav();
}

void RosaTools::StartTranslateNav()
{
	m_IsTranslating = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );

	Array<uint> SelectedNavVerts = GetSelectedNavVerts();
	FOR_EACH_ARRAY( VertIter, SelectedNavVerts, uint )
	{
		SNavVert&	Vert		= m_NavVerts[ VertIter.GetValue() ];
		Vert.m_TransformStart	= Vert.m_Vert;
	}

	// Also move anchor now that it's a separate transform
	m_TranslateAnchorStart	= m_TransformAnchor;

	Project( m_TransformAnchor, m_TransformAnchorScreenX, m_TransformAnchorScreenY );
}

void RosaTools::EndTranslateNav( const bool Cancel )
{
	m_IsTranslating = false;

	if( Cancel )
	{
		Array<uint> SelectedNavVerts = GetSelectedNavVerts();
		FOR_EACH_ARRAY( VertIter, SelectedNavVerts, uint )
		{
			SNavVert&	Vert	= m_NavVerts[ VertIter.GetValue() ];
			Vert.m_Vert			= Vert.m_TransformStart;
		}

		// Also move anchor now that it's a separate transform
		SetAnchorLocation( m_TranslateAnchorStart );
	}
	else
	{
		StoreUndoState();
	}
}

void RosaTools::TickTranslateNav( const bool Vertical )
{
	ASSERT( m_IsTranslating );

	Mouse* const pMouse = m_Framework->GetMouse();
	int CursorX, CursorY;
	pMouse->GetPosition( CursorX, CursorY, m_Framework->GetWindow() );

	const int OffsetX = CursorX - m_TransformStartCursorX;
	const int OffsetY = CursorY - m_TransformStartCursorY;

	const int		AnchorScreenX		= m_TransformAnchorScreenX + OffsetX;
	const int		AnchorScreenY		= m_TransformAnchorScreenY + OffsetY;
	const Segment	UnprojectedSegment	= Unproject( AnchorScreenX, AnchorScreenY );
	const Line		UnprojectedLine		= Line( UnprojectedSegment.m_Point1, UnprojectedSegment.GetDirection() );

	Vector UnprojectedAnchor;
	if( Vertical )
	{
		const Line ZLine	= Line( m_TranslateAnchorStart, Vector( 0.0f, 0.0f, 1.0f ) );
		UnprojectedAnchor	= ZLine.NearestPointTo( UnprojectedLine );
	}
	else
	{
		const Plane XYPlane	= Plane( Vector( 0.0f, 0.0f, 1.0f ), -m_TranslateAnchorStart.z );
		UnprojectedAnchor	= UnprojectedLine.GetIntersection( XYPlane );
	}

	const Vector AnchorOffset = SnapToGrid( UnprojectedAnchor - m_TranslateAnchorStart );

	Array<uint> SelectedNavVerts = GetSelectedNavVerts();
	FOR_EACH_ARRAY( VertIter, SelectedNavVerts, uint )
	{
		SNavVert&	Vert	= m_NavVerts[ VertIter.GetValue() ];
		Vert.m_Vert			= Vert.m_TransformStart + AnchorOffset;
	}

	// Also move anchor now that it's a separate transform
	SetAnchorLocation( m_TranslateAnchorStart + AnchorOffset );
}

void RosaTools::TryStartRotateNav()
{
	if( m_SelectedNavVerts.Empty() &&
		m_SelectedNavEdges.Empty() &&
		m_SelectedNavFaces.Empty() )
	{
		return;
	}

	StartRotateNav();
}

void RosaTools::StartRotateNav()
{
	m_IsRotating = true;

	Mouse* const pMouse = m_Framework->GetMouse();
	pMouse->GetPosition( m_TransformStartCursorX, m_TransformStartCursorY, m_Framework->GetWindow() );

	Array<uint> SelectedNavVerts = GetSelectedNavVerts();
	FOR_EACH_ARRAY( VertIter, SelectedNavVerts, uint )
	{
		SNavVert&	Vert		= m_NavVerts[ VertIter.GetValue() ];
		Vert.m_TransformStart	= Vert.m_Vert;
	}

	Project( m_TransformAnchor, m_TransformAnchorScreenX, m_TransformAnchorScreenY );
}

void RosaTools::EndRotateNav( const bool Cancel )
{
	m_IsRotating = false;

	if( Cancel )
	{
		Array<uint> SelectedNavVerts = GetSelectedNavVerts();
		FOR_EACH_ARRAY( VertIter, SelectedNavVerts, uint )
		{
			SNavVert&	Vert	= m_NavVerts[ VertIter.GetValue() ];
			Vert.m_Vert			= Vert.m_TransformStart;
		}
	}
	else
	{
		StoreUndoState();
	}
}

void RosaTools::TickRotateNav( const Vector& RotateAxis )
{
	ASSERT( m_IsRotating );

	Mouse* const pMouse = m_Framework->GetMouse();
	int CursorX, CursorY;
	pMouse->GetPosition( CursorX, CursorY, m_Framework->GetWindow() );

	const float		PlaneDistance		= RotateAxis.Dot( m_TransformAnchor );
	const Plane		RotatePlane			= Plane( RotateAxis, -PlaneDistance );

	const Segment	StartSegment		= Unproject( m_TransformStartCursorX, m_TransformStartCursorY );
	const Line		StartLine			= Line( StartSegment.m_Point1, StartSegment.GetDirection() );
	const Vector	TransformStart		= StartLine.GetIntersection( RotatePlane );

	const Segment	UnprojectedSegment	= Unproject( CursorX, CursorY );
	const Line		UnprojectedLine		= Line( UnprojectedSegment.m_Point1, UnprojectedSegment.GetDirection() );
	const Vector	UnprojectedCursor	= UnprojectedLine.GetIntersection( RotatePlane );
	const Vector	AnchorToStart		= TransformStart - m_TransformAnchor;
	const Vector	AnchorToCursor		= UnprojectedCursor - m_TransformAnchor;
	const float		CosTheta			= AnchorToStart.GetNormalized().Dot( AnchorToCursor.GetNormalized() );
	const float		Theta				= SnapToGrid( ACos( CosTheta ) * Sign( AnchorToStart.Cross( AnchorToCursor ).Dot( RotateAxis ) ), DEGREES_TO_RADIANS( m_RotateGridSize ) );
	const Matrix	RotationMatrix		= Matrix::CreateRotation( RotateAxis, Theta );

	Array<uint> SelectedNavVerts = GetSelectedNavVerts();
	FOR_EACH_ARRAY( VertIter, SelectedNavVerts, uint )
	{
		SNavVert&		Vert			= m_NavVerts[ VertIter.GetValue() ];
		const Vector	AnchorToVert	= Vert.m_TransformStart - m_TransformAnchor;
		Vert.m_Vert						= m_TransformAnchor + ( AnchorToVert * RotationMatrix );
	}
}

bool RosaTools::Project( const Vector& WorldPos, int& ScreenX, int& ScreenY ) const
{
	const Matrix	VP				= m_Framework->GetMainView()->GetViewProjectionMatrix();
	const Vector4	UnprojectedPos	= WorldPos;
	const Vector4	ProjectedPos	= UnprojectedPos * VP;
	const Vector	ScreenPos		= ProjectedPos / ProjectedPos.w;

	ScreenX	= static_cast<int>( ( ScreenPos.x + 1.0f ) * 0.5f * m_Framework->GetDisplay()->m_Width );
	ScreenY	= static_cast<int>( ( ScreenPos.y - 1.0f ) * -0.5f * m_Framework->GetDisplay()->m_Height );

	return ProjectedPos.z >= 0.0f;
}

Segment RosaTools::Unproject( const int ScreenX, const int ScreenY ) const
{
	const float ProjectedX = ( static_cast<float>( ScreenX ) / m_Framework->GetDisplay()->m_Width ) * 2.0f - 1.0f;
	const float ProjectedY = ( static_cast<float>( ScreenY ) / m_Framework->GetDisplay()->m_Height ) * -2.0f + 1.0f;

	// Multiply by inverse VP matrix to get trace line
	const Matrix	InvVP			= ( m_Framework->GetMainView()->GetViewProjectionMatrix() ).GetInverse();
	const Vector4	ProjectedFar	= Vector4( ProjectedX, ProjectedY, 1.0f, 1.0f );
	const Vector4	UnprojectedFar	= ProjectedFar * InvVP;
	const Vector	TraceEnd		= UnprojectedFar / UnprojectedFar.w;

	return Segment( m_CameraLocation, TraceEnd );
}

float RosaTools::GetScreenDistSq( const int ScreenXA, const int ScreenYA, const int ScreenXB, const int ScreenYB ) const
{
	const float DeltaX = static_cast<float>( ScreenXB - ScreenXA );
	const float DeltaY = static_cast<float>( ScreenYB - ScreenYA );
	return Square( DeltaX ) + Square( DeltaY );
}

void RosaTools::DeleteSelectedBrushes()
{
	// Make a sorted copy so we can safely delete without messing with the indices
	Array<uint> SelectedBrushes = m_SelectedBrushes;
	SelectedBrushes.QuickSort();
	FOR_EACH_ARRAY_REVERSE( BrushIter, SelectedBrushes, uint )
	{
		DeleteBrush( BrushIter.GetValue() );
	}

	m_SelectedBrushes.Clear();
}

void RosaTools::DeleteBrush( const uint BrushIndex )
{
	{
		SBrush& Brush = m_Brushes[ BrushIndex ];

		FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
		{
			SBrushMesh& BrushMesh = BrushMeshIter.GetValue();
			SafeDelete( BrushMesh.m_Mesh );
		}
		// ROSANOTE: Also delete hull meshes if I render those
	}

	m_Brushes.Remove( BrushIndex );

	// Unlink deleted brushes, shift indices down
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		SBrush& Brush = BrushIter.GetValue();
		FOR_EACH_ARRAY_REVERSE( LinkedBrushIter, Brush.m_LinkedBrushes, uint )
		{
			uint& LinkedBrushIndex = LinkedBrushIter.GetValue();
			if( LinkedBrushIndex == BrushIndex )
			{
				Brush.m_LinkedBrushes.Remove( LinkedBrushIter );
			}
			else if( LinkedBrushIndex > BrushIndex )
			{
				LinkedBrushIndex--;
			}
		}
	}
}

uint RosaTools::GetIndexOfBrush( const SBrush& Brush )
{
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		const SBrush& OtherBrush = BrushIter.GetValue();
		if( &Brush == &OtherBrush )
		{
			return BrushIter.GetIndex();
		}
	}

	// Something went very wrong if we can't find the brush in m_Brushes.
	WARN;
	return 0;
}

void RosaTools::SelectBrush( SBrush& Brush )
{
	DEVASSERT( !Brush.m_Hidden );

	Brush.m_Selected				= true;

	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh& BrushMesh = BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->m_ConstantColor	= Vector4( 1.0f, 1.5f, 1.5f, 1.0f );
	}

	m_SelectedBrushes.PushBackUnique( GetIndexOfBrush( Brush ) );

	if( m_SelectedBrushes.Size() == 1 )
	{
		m_TransformAnchor = Brush.m_Location;
	}
}

void RosaTools::SelectBrushes( const Array<uint>& Brushes )
{
	FOR_EACH_ARRAY( BrushIter, Brushes, uint )
	{
		SelectBrush( m_Brushes[ BrushIter.GetValue() ] );
	}
}

void RosaTools::SelectAllBrushes()
{
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		SBrush& Brush = BrushIter.GetValue();

		if( Brush.m_Hidden )
		{
			continue;
		}

		SelectBrush( Brush );
	}
}

void RosaTools::DeselectBrush( SBrush& Brush )
{
	Brush.m_Selected				= false;

	FOR_EACH_ARRAY( BrushMeshIter, Brush.m_Meshes, SBrushMesh )
	{
		SBrushMesh& BrushMesh = BrushMeshIter.GetValue();
		BrushMesh.m_Mesh->m_ConstantColor	= Vector4( 1.0f, 1.0f, 1.0f, 1.0f );
	}

	m_SelectedBrushes.RemoveItem( GetIndexOfBrush( Brush ) );
}

void RosaTools::DeselectBrushes( const Array<uint>& Brushes )
{
	FOR_EACH_ARRAY( BrushIter, Brushes, uint )
	{
		DeselectBrush( m_Brushes[ BrushIter.GetValue() ] );
	}
}

void RosaTools::DeselectAllBrushes()
{
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		DeselectBrush( BrushIter.GetValue() );
	}

	DEVASSERT( m_SelectedBrushes.Empty() );
	m_SelectedBrushes.Clear();
}

void RosaTools::ToggleBrush( SBrush& Brush )
{
	if( Brush.m_Selected )
	{
		DeselectBrush( Brush );
	}
	else
	{
		SelectBrush( Brush );
	}
}

void RosaTools::SetBrushSelected( SBrush& Brush, const bool Selected )
{
	Selected ? SelectBrush( Brush ) : DeselectBrush( Brush );
}

void RosaTools::HideBrush( SBrush& Brush )
{
	Brush.m_Hidden = true;
	DeselectBrush( Brush );
}

void RosaTools::UnhideBrush( SBrush& Brush )
{
	Brush.m_Hidden = false;
}

void RosaTools::HideBrushes( const Array<uint>& Brushes )
{
	FOR_EACH_ARRAY( BrushIter, Brushes, uint )
	{
		HideBrush( m_Brushes[ BrushIter.GetValue() ] );
	}
}

void RosaTools::UnhideBrushes( const Array<uint>& Brushes )
{
	FOR_EACH_ARRAY( BrushIter, Brushes, uint )
	{
		UnhideBrush( m_Brushes[ BrushIter.GetValue() ] );
	}
}

void RosaTools::HideAllBrushes()
{
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		HideBrush( BrushIter.GetValue() );
	}
}

void RosaTools::UnhideAllBrushes()
{
	FOR_EACH_ARRAY( BrushIter, m_Brushes, SBrush )
	{
		UnhideBrush( BrushIter.GetValue() );
	}
}

float RosaTools::SnapToGrid( const float Value, const float GridSize ) const
{
	if( !m_GridActive )
	{
		return Value;
	}

	DEVASSERT( GridSize > EPSILON );
	return Round( Value / GridSize ) * GridSize;
}

Vector RosaTools::SnapToGrid( const Vector& Location ) const
{
	Vector SnapVector;

	SnapVector.x = SnapToGrid( Location.x, m_GridSize );
	SnapVector.y = SnapToGrid( Location.y, m_GridSize );
	SnapVector.z = SnapToGrid( Location.z, m_GridSize );

	return SnapVector;
}

Angles RosaTools::SnapToGrid( const Angles& Orientation ) const
{
	Angles SnapAngles;

	const float GridSizeRadians	= DEGREES_TO_RADIANS( m_RotateGridSize );
	SnapAngles.Pitch	= SnapToGrid( Orientation.Pitch, GridSizeRadians );
	SnapAngles.Roll		= SnapToGrid( Orientation.Roll, GridSizeRadians );
	SnapAngles.Yaw		= SnapToGrid( Orientation.Yaw, GridSizeRadians );

	return SnapAngles;
}

#endif // BUILD_ROSA_TOOLS
