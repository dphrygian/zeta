#ifndef ROSATOOLS_H
#define ROSATOOLS_H

#include "rosagame.h"	// For BUILD_ROSA_TOOLS

#if BUILD_ROSA_TOOLS

#include "rosaworld.h"
#include "rosaworldgen.h"
#include "array.h"
#include "vector.h"
#include "angles.h"
#include "plane.h"

class RosaFramework;
class RosaWorld;
class RosaRoomEditor;
class Mesh;
class IDataStream;
class Segment;
struct SReadMeshBuffers;
class MeshFactory;
class Triangle;

typedef Map<Vector, uint> TGeoSizeIndexMap;

class RosaTools
{
public:
	RosaTools();
	~RosaTools();

	void	InitializeFromDefinition( const SimpleString& DefinitionName );
	void	Reinitialize();
	void	ShutDown();

	bool	IsInToolMode() const;
	void	ToggleToolMode();

	void	Tick( const float DeltaTime );
	void	TickInput( const float DeltaTime );
	void	TickRender();

	Vector	GetCameraLocation() const { return m_CameraLocation; }
	Angles	GetCameraOrientation() const { return m_CameraOrientation; }
	void	SetCameraTransform( const Vector& Location, const Angles& Orientation );

private:
	friend class RosaRoomEditor;
	friend class RoomBaker;

	struct SElementCategoryButton
	{
		SElementCategoryButton()
		:	m_Mesh( NULL )
		,	m_CategoryIndex( 0 )
		,	m_BoxMin()
		,	m_BoxMax()
		{
		}

		Mesh*			m_Mesh;
		uint			m_CategoryIndex;
		Vector2			m_BoxMin;
		Vector2			m_BoxMax;
	};

	struct SElementButton
	{
		SElementButton()
		:	m_Mesh( NULL )
		,	m_DefIndex( 0 )
		,	m_BoxMin()
		,	m_BoxMax()
		{
		}

		Mesh*			m_Mesh;
		uint			m_DefIndex;
		Vector2			m_BoxMin;
		Vector2			m_BoxMax;
	};

	struct SElements
	{
		SElements()
		:	m_Categories( NULL )
		,	m_CategoryButtons( 0 )
		,	m_Buttons()
		,	m_CategoryMap()
		,	m_CurrentCategoryIndex( 0 )
		,	m_CurrentDefIndex( 0 )
		{
		}

		Array<SimpleString>				m_Categories;
		Array<SElementCategoryButton>	m_CategoryButtons;
		Array<SElementButton>			m_Buttons;
		Map<uint, uint>					m_CategoryMap;			// Map from the defindex to the first containing category
		uint							m_CurrentCategoryIndex;
		uint							m_CurrentDefIndex;
	};

	struct SBrushMesh
	{
		SBrushMesh()
		:	m_Mesh( NULL )
		,	m_Offset()
		{
		}

		Mesh*	m_Mesh;
		Vector	m_Offset;
	};

	struct SBrush
	{
		SBrush()
		:	m_Type( EBT_None )
		,	m_Selected( false )
		,	m_Hidden( false )
		,	m_Meshes()
		,	m_TranslateStart()
		,	m_RotateStart()
		,	m_ScaleStart( 0.0f )
		,	m_DefIndex()
		,	m_Location()
		,	m_Orientation()
		,	m_Scale( 1.0f )
		,	m_Mat()
		,	m_LinkedBrushes()
		{
		}

		// For editor only
		EBrushType			m_Type;
		bool				m_Selected;
		bool				m_Hidden;
		Array<SBrushMesh>	m_Meshes;
		// ROSATODO: Also render hull meshes if it seems useful
		Vector				m_TranslateStart;
		Angles				m_RotateStart;
		float				m_ScaleStart;
		uint				m_DefIndex;

		// For exporting
		Vector				m_Location;
		Angles				m_Orientation;
		float				m_Scale;
		// ROSANOTE: Currently, I'm not supporting scale because I don't
		// expect it would have much utility outside grayboxing.
		HashedString		m_Mat;	// For overriding brush's default texture/surface properties
		Array<uint>			m_LinkedBrushes;	// Replacing LoopMetadata from Neon
	};

	struct SPortalDef
	{
		SPortalDef()
		:	m_DefName()
		,	m_Portal()
		{
		}

		SimpleString	m_DefName;
		SPortal			m_Portal;
	};

	struct SPrefabPart
	{
		SPrefabPart()
		:	m_Type( EBT_None )
		,	m_DefIndex( 0 )
		,	m_TranslationOffsetLo()
		,	m_TranslationOffsetHi()
		,	m_OrientationOffsetLo()
		,	m_OrientationOffsetHi()
		,	m_Mat()
		{
		}

		EBrushType		m_Type;
		uint			m_DefIndex;
		Vector			m_TranslationOffsetLo;
		Vector			m_TranslationOffsetHi;
		Angles			m_OrientationOffsetLo;
		Angles			m_OrientationOffsetHi;
		HashedString	m_Mat;
	};

	struct SPrefabDef
	{
		SPrefabDef()
		:	m_DefName()
		,	m_Parts()
		{
		};

		SimpleString		m_DefName;
		Array<SPrefabPart>	m_Parts;
	};

	struct SNavVert
	{
		SNavVert()
		:	m_Vert()
		,	m_TransformStart()
		,	m_Selected( false )
		{
		}

		SNavVert( const Vector Vert )
		:	m_Vert( Vert )
		,	m_TransformStart()
		,	m_Selected( false )
		{
		}

		Vector	m_Vert;
		Vector	m_TransformStart;
		bool	m_Selected;
	};

	struct SNavEdge
	{
		SNavEdge()
		:	m_VertA( 0 )
		,	m_VertB( 0 )
		,	m_Selected( false )
		{
		}

		SNavEdge( const uint VertA, const uint VertB )
		:	m_VertA( VertA )
		,	m_VertB( VertB )
		,	m_Selected( false )
		{
		}

		uint	m_VertA;
		uint	m_VertB;
		bool	m_Selected;
	};

	struct SNavFace
	{
		SNavFace()
		:	m_EdgeA( 0 )
		,	m_EdgeB( 0 )
		,	m_EdgeC( 0 )
		,	m_Selected( false )
		,	m_Height( 0.0f )
		,	m_Props( ENP_None )
		{
		}

		SNavFace( const uint EdgeA, const uint EdgeB, const uint EdgeC )
		:	m_EdgeA( EdgeA )
		,	m_EdgeB( EdgeB )
		,	m_EdgeC( EdgeC )
		,	m_Selected( false )
		,	m_Height( 0.0f )
		,	m_Props( ENP_None )
		{
		}

		uint	m_EdgeA;
		uint	m_EdgeB;
		uint	m_EdgeC;
		bool	m_Selected;

		// HACKHACK: Store height here because ::TraceGeo exists so it's easier to do in tools than in RoomBaker.
		// This is populated when saving, so it should stay in sync with geo changes.
		float	m_Height;

		uint	m_Props;
	};

	enum ENavType
	{
		ENT_Vert,
		ENT_Edge,
		ENT_Face,
	};

	enum ESubTool
	{
		EST_RoomMode,
		EST_WorldMode,
		EST_Spawners,
		EST_Geo,
		EST_Prefabs,
		EST_WorldPrefabs, // HACKHACK, I'm not actually making separate elements for these, but I need to distinguish which normal mode to return to
		EST_Portals,
		EST_NavMesh,
		EST_Mats,
		EST_Rooms,
	};

	void	SetSubTool( const ESubTool SubTool, const bool ReinitGridMeshes );
	void	SetMeshSubTool_NoCache();

	bool	IsInRoomMode() const { return !IsInWorldMode(); }
	bool	IsInWorldMode() const { return m_SubTool == EST_WorldMode || m_SubTool == EST_Rooms || m_SubTool == EST_WorldPrefabs; }
	bool	IsInNormalMode() const { return m_SubTool == EST_RoomMode || m_SubTool == EST_WorldMode; }

	void	InitializeGridMeshes();
	void	CreateGridMesh( const Array<Vector>& Corners, const uint Color, Array<Vector>& OutPositions, Array<uint>& OutColors, Array<index_t>& OutIndices );
	void	DeleteGridMeshes();

	void	InitializePortalMesh();
	void	CreatePortalMesh( const Array<Vector>& Corners, const uint Color, Array<Vector>& OutPositions, Array<uint>& OutColors, Array<index_t>& OutIndices );
	uint	GetTileIndex( const uint TileX, const uint TileY, const uint TileZ ) const;
	uint	GetPortalIndex( const Vector& Facing ) const;
	uint	GetPortalColor( const SPortalDef& Portal ) const;

	void	InitializeHelpMesh();

	void	InitializeElementButtonMeshes( SElements& Elements, const Array<SimpleString>& DefNames );
	void	DeleteElementButtonMeshes( SElements& Elements );

	void	RenderElements( const SElements& Elements, const uint NumDefs );

	uint	GetSpawnerNameColor( const HashedString& SpawnerName );

	bool	TraceBoundPlanes( const Segment& TraceSegment, CollisionInfo& Info );
	bool	TraceGeo( const Segment& TraceSegment, CollisionInfo& Info, const bool IgnoreNonBlockers );
	bool	TraceSpawners( const Segment& TraceSegment, CollisionInfo& Info );
	bool	TraceBrushes( const Segment& TraceSegment, CollisionInfo& Info );
	bool	TraceRooms( const Segment& TraceSegment, CollisionInfo& Info );

	void	AppendCategories( const SimpleString& LevelName );
	void	AppendRoomCategory( const SimpleString& Category );
	void	AppendSpawnerCategory( const SimpleString& Category );
	void	AppendGeoCategory( const SimpleString& Category );
	void	AppendPrefabCategory( const SimpleString& Category );
	void	AppendPortalCategory( const SimpleString& Category );
	void	AppendMatCategory( const SimpleString& Category );

	// Return defindex
	uint	CreateRoomDef( const SimpleString& RoomDefName );
	uint	FindOrCreateRoomDef( const SimpleString& DefName );
	uint	CreateSpawnerDef( const SimpleString& SpawnerDefName );
	uint	FindOrCreateSpawnerDef( const SimpleString& DefName );
	uint	CreateGeoDef( const SimpleString& GeoDefName );
	uint	FindOrCreateGeoDef( const SimpleString& DefName );
	uint	CreatePrefabDef( const SimpleString& PrefabDefName );

	bool	ParseGeoDefSize( const SimpleString& GeoDefName, HashedString& OutTag, Vector& OutSize ) const;
	void	BuildGeoSizeLinks();
	uint	FindGeoSizeLink( const TGeoSizeIndexMap& TagGeo, const Vector& GeoDefSize, const uint& GeoDefIndex, const uint Axis, const bool Sign ) const;

	void	TickNormalInput( const float DeltaTime );
	void	TickCreateBrushInput( const uint Key, const EBrushType BrushType, const SElements& Elements );
	void	TickCameraInput( const float DeltaTime );
	void	TickGridInput();
	void	TickUndoInput();
	void	TickSaveLoadInput();
	void	TickNavMeshInput( const float DeltaTime );
	void	TickElementsInput( SElements& Elements );

	bool	HasUnsavedChanges() const { return HasUndoStates() && m_SavedUndoStateIndex != m_UndoStateIndex; }
	bool	HasUndoStates() const { return m_UndoStates.Size() > 1; }
	int		ConditionalMessageBox( const SimpleString& Message ) const;

	void	TryClear();
	void	Clear();

	void	TryStartTranslate();
	void	StartTranslate();
	void	EndTranslate( const bool Cancel );
	void	TickTranslate( const bool Vertical );
	void	SetBrushLocation( SBrush& Brush, const Vector& Location );

	void	TryStartTranslateAnchor();
	void	StartTranslateAnchor();
	void	EndTranslateAnchor( const bool Cancel );
	void	TickTranslateAnchor( const bool Vertical );
	void	SetAnchorLocation( const Vector& Location );

	void	TryStartRotate();
	void	StartRotate();
	void	EndRotate( const bool Cancel );
	void	TickRotate( const Vector& RotateAxis );
	void	SetBrushOrientation( SBrush& Brush, const Angles& Orientation );

	void	TryStartScale();
	void	StartScale();
	void	EndScale( const bool Cancel );
	void	TickScale();
	void	SetBrushScale( SBrush& Brush, const float Scale );

	void	StartBoxSelect();
	void	EndBoxSelect();

	void	TryStartTranslateNav();
	void	StartTranslateNav();
	void	EndTranslateNav( const bool Cancel );
	void	TickTranslateNav( const bool Vertical );

	void	TryStartRotateNav();
	void	StartRotateNav();
	void	EndRotateNav( const bool Cancel );
	void	TickRotateNav( const Vector& RotateAxis );

	bool	Project( const Vector& WorldPos, int& ScreenX, int& ScreenY ) const;	// Returns true if point is in front of view
	Segment	Unproject( const int ScreenX, const int ScreenY ) const;
	float	GetScreenDistSq( const int ScreenXA, const int ScreenYA, const int ScreenXB, const int ScreenYB ) const;

	uint			GetIndexOfBrush( const SBrush& Brush );
	SBrush&			GetBrush( const uint BrushIndex )		{ return m_Brushes[ BrushIndex ]; }
	const SBrush&	GetBrush( const uint BrushIndex ) const	{ return m_Brushes[ BrushIndex ]; }

	void		DeleteSelectedBrushes();
	void		DeleteBrush( const uint BrushIndex );
	void		SelectBrush( SBrush& Brush );
	void		SelectBrushes( const Array<uint>& Brushes );
	void		SelectAllBrushes();
	void		DeselectBrush( SBrush& Brush );
	void		DeselectBrushes( const Array<uint>& Brushes );
	void		DeselectAllBrushes();
	void		ToggleBrush( SBrush& Brush );
	void		SetBrushSelected( SBrush& Brush, const bool Selected );

	void		HideBrush( SBrush& Brush );
	void		UnhideBrush( SBrush& Brush );
	void		HideBrushes( const Array<uint>& Brushes );
	void		UnhideBrushes( const Array<uint>& Brushes );
	void		HideAllBrushes();
	void		UnhideAllBrushes();

	float		SnapToGrid( const float Value, const float GridSize ) const;
	Vector		SnapToGrid( const Vector& Location ) const;
	Angles		SnapToGrid( const Angles& Orientation ) const;

	void	ClearBrushes();
	SBrush&	CreateBrush( const EBrushType Type, const uint DefIndex, const Vector& Location, const Angles& Orientation, const float Scale, const Array<uint>& LinkedBrushes );
	void	CreateBrushes( const EBrushType Type, const uint DefIndex, const Vector& Location, const Angles& Orientation, const float Scale, Array<uint>& OutBrushes );

	void	AppendHullMesh( const Vector& Base, const Vector& Dims, Array<Vector>& InOutPositions, Array<Vector2>& InOutUVs, Array<Vector>& InOutNormals, Array<index_t>& InOutIndices ) const;
	Mesh*	CreateHullsMesh( const Array<Vector>& Positions, const Array<Vector2>& UVs, const Array<Vector>& Normals, const Array<index_t>& Indices ) const;

	bool	FindRoomDef( const SimpleString& SearchDef, uint* const pOutIndex = NULL ) const;
	bool	FindSpawnerDef( const SimpleString& SearchDef, uint* const pOutIndex = NULL ) const;
	bool	FindGeoDef( const SimpleString& SearchDef, uint* const pOutIndex = NULL ) const;
	bool	FindPrefabDef( const SimpleString& SearchDef, uint* const pOutIndex = NULL ) const;
	bool	FindPortalDef( const SimpleString& SearchDef, uint* const pOutIndex = NULL ) const;
	bool	FindMatDef( const HashedString& SearchDef, uint* const pOutIndex = NULL ) const;

	void	ResetPortals();
	void	FixUpPortalsAfterResize( const uint OldTilesX, const uint OldTilesY, const uint OldTilesZ );

	void	ApplyMatToBrush( const uint MatDefIndex, SBrush& Brush );
	void	RemoveMatFromBrush( SBrush& Brush );

	bool	DoesBrushProvideNavMeshSnapPoints( const SBrush& Brush ) const;
	AABB	GetBoundsForBrush( const SBrush& Brush ) const;
	void	CacheNavMeshSnapPoints();
	void	FindNavSnapPoint( const bool ShouldSnapToGrid );
	void	GatherHullPlanes( const SBrush& Brush, Array<Plane>& Planes );
	void	CreateNavMesh();
	void	CreateNavSnapMesh();
	void	AddNavVertToMesh( const SNavVert& Vert, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const;
	void	AddNavEdgeToMesh( const SNavEdge& Edge, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const;
	void	AddNavFaceToMesh( const SNavFace& Face, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const;
	void	AddNavSnapPointToMesh( const Vector& SnapPoint, Array<Vector>& Positions, Array<uint>& Colors, Array<index_t>& Indices ) const;
	void	SelectAllNav();
	void	DeselectAllNav();
	void	SetNavVertSelected( const uint VertIndex, const bool Selected );
	void	SelectNavVert( const uint VertIndex );
	void	DeselectNavVert( const uint VertIndex );
	void	DeselectNavVerts( const Array<uint>& Verts );
	void	ToggleNavVert( const uint VertIndex );
	void	SetNavEdgeSelected( const uint EdgeIndex, const bool Selected );
	void	SelectNavEdge( const uint EdgeIndex );
	void	DeselectNavEdge( const uint EdgeIndex );
	void	DeselectNavEdges( const Array<uint>& Edges );
	void	ToggleNavEdge( const uint EdgeIndex );
	void	SetNavFaceSelected( const uint FaceIndex, const bool Selected );
	void	SelectNavFace( const uint FaceIndex );
	void	DeselectNavFace( const uint FaceIndex );
	void	DeselectNavFaces( const Array<uint>& Faces );
	void	ToggleNavFace( const uint FaceIndex );
	uint	GetNearestEdge( const SNavFace& Face, const SNavVert& Vert ) const;	// Get index of nearest edge on Face to Vert
	bool	GetNearestNavElement( const int ScreenX, const int ScreenY, uint& OutNavIndex, uint& OutNavType ) const;
	Vector	GetNavEdgeCentroid( const SNavEdge& Edge ) const;
	Segment	GetNavEdgeSegment( const SNavEdge& Edge ) const;
	Vector	GetNavFaceCentroid( const SNavFace& Face ) const;
	void	ConnectVertToVert( const uint VertAIndex, const uint VertBIndex );
	void	ConnectVerts( const uint VertAIndex, const uint VertBIndex, const uint VertCIndex );
	void	ConnectVertToEdge( const uint VertIndex, const uint EdgeIndex );
	void	ConnectEdges( const uint EdgeAIndex, const uint EdgeBIndex, const uint EdgeCIndex );
	void	ConnectEdges( const uint EdgeAIndex, const uint EdgeBIndex );
	bool	GetUnsharedVerts( const SNavEdge& EdgeA, const SNavEdge& EdgeB, uint& OutVertA, uint& OutVertB );
	void	DeleteSelectedNavElements();
	void	DeleteNavVert( const uint VertIndex );
	void	DeleteNavEdge( const uint EdgeIndex );
	void	DeleteNavFace( const uint FaceIndex );
	bool	EdgeHasVert( const SNavEdge& Edge, const uint VertIndex ) const;
	bool	FaceHasEdge( const SNavFace& Face, const uint EdgeIndex ) const;
	void	ReduceNavElements(); // Remove duplicates and fix up references
	Array<uint>	GetSelectedNavVerts() const;	// Taking into account any selected edges and faces

	// Reused by RoomBaker
	static SConvexHull	CreateHull( const SimpleString& HullName, const HashedString& Surface, MeshFactory* const pMeshFactory );									// Create and preprocess a convex hull
	static SConvexHull	CreateHullWithTris( const SimpleString& HullName, const HashedString& Surface, MeshFactory* const pMeshFactory, Array<Triangle>& OutTris );	// Same but retain the raw triangles
	static SConvexHull	CreateHullFromAABB( const AABB& HullBound, const HashedString& Surface );
	static void			ReadMeshCallback( void* pVoid, const SReadMeshBuffers& Buffers );
	static void			ReadMeshCallback_Tris( void* pVoid, const SReadMeshBuffers& Buffers );

	void	StoreUndoState();	// Call this after any undo-able state change
	void	TryUndo();
	void	TryRedo();

	void	TryQuickSave();
	void	TrySave();
	void	TryLoad();

	void	Save( const SimpleString& Filename );
	void	Load( const SimpleString& Filename );
	void	SaveStream( const IDataStream& Stream );
	void	LoadStream( const IDataStream& Stream );
	void	SaveWorld( const IDataStream& Stream );
	void	LoadWorld( const IDataStream& Stream );
	void	SaveRoom( const IDataStream& Stream );
	void	LoadRoom( const IDataStream& Stream );

	void	ConvertWorldToRoom();
	void	AddLoadRoomWithTransform( const IDataStream& Stream, const Vector& Location, const Angles& Orientation );

	void	LoadWorldFromStoredModules();

	void	TrySavePrefab();
	void	SavePrefab( const IDataStream& Stream );
	void	LoadPrefab( const IDataStream& Stream, SPrefabDef& PrefabDef );

	RosaFramework*		m_Framework;

	bool				m_ToolMode;

	ESubTool			m_SubTool;

	uint				m_TilesX;
	uint				m_TilesY;
	uint				m_TilesZ;
	uint				m_MetersX;
	uint				m_MetersY;
	uint				m_MetersZ;

	Array<Mesh*>		m_GridMeshes;
	Mesh*				m_PortalMesh;

	struct SRoomDef
	{
		SRoomDef()
		:	m_DefName()
		,	m_Extents()
		{
		};

		SimpleString	m_DefName;	// Actually the room filename!
		AABB			m_Extents;
	};

	struct SSpawnerDef
	{
		SSpawnerDef()
		:	m_DefName()
		,	m_MeshName()
		,	m_Offset()
		,	m_MeshOffset()
		,	m_TraceExtents()
		,	m_AddNavMeshSnapPoints( false )
		{
		};

		SimpleString	m_DefName;
		SimpleString	m_MeshName;
		Vector			m_Offset;		// Used for both mesh and trace
		Vector			m_MeshOffset;
		AABB			m_TraceExtents;
		bool			m_AddNavMeshSnapPoints;
	};

	struct SGeoDef
	{
		SGeoDef()
		:	m_DefName()
		,	m_MeshNames()
		,	m_Hulls()
		,	m_Bounds()
		,	m_LinkXNeg( 0xffffffff )
		,	m_LinkXPos( 0xffffffff )
		,	m_LinkYNeg( 0xffffffff )
		,	m_LinkYPos( 0xffffffff )
		,	m_LinkZNeg( 0xffffffff )
		,	m_LinkZPos( 0xffffffff )
		{
		};

		SimpleString		m_DefName;
		Array<SimpleString>	m_MeshNames;
		Array<SConvexHull>	m_Hulls;
		AABB				m_Bounds;	// Bounds of all hulls together

		// Links to nearest geodefs of same size, 0xffffffff if invalid
		uint				m_LinkXNeg;
		uint				m_LinkXPos;
		uint				m_LinkYNeg;
		uint				m_LinkYPos;
		uint				m_LinkZNeg;
		uint				m_LinkZPos;
	};

	struct SPortals
	{
		SPortals()
		{
			memset( m_Portals, 0, sizeof( m_Portals ) );
		}

		// Ordered X+ X- Y+ Y- Z+ Z-
		uint	m_Portals[6];
	};

	struct SMatDef
	{
		SMatDef()
		:	m_DefName()
		,	m_Albedo()
		,	m_Overlay()
		{
		}

		SimpleString	m_DefName;
		SimpleString	m_Albedo;
		SimpleString	m_Overlay;
		// ROSANOTE: Other mat properties (normal map, spec map, surface, etc.) don't
		// matter here because we don't display them in the editor and they are loaded
		// from config at runtime, not saved into the room file.
	};

	SElements			m_RoomElements;
	SElements			m_SpawnerElements;
	SElements			m_GeoElements;
	SElements			m_PrefabElements;
	SElements			m_PortalElements;
	SElements			m_MatElements;

	Array<SRoomDef>		m_RoomDefs;
	Array<SSpawnerDef>	m_SpawnerDefs;
	Array<SGeoDef>		m_GeoDefs;
	Array<SPrefabDef>	m_PrefabDefs;
	Array<SPortalDef>	m_PortalDefs;
	Array<SMatDef>		m_MatDefs;

	// Map from geo tag to a map of dimensions to corresponding geodef index
	Map<HashedString, TGeoSizeIndexMap> m_GeoDefSizeMap;

	Mesh*				m_HelpMesh;

	Vector				m_CameraLocation;
	Angles				m_CameraOrientation;

	float				m_CameraSpeed;
	Vector				m_CameraVelocity;
	Angles				m_CameraRotationalVelocity;

	Array<Plane>		m_BoundPlanes;

	SimpleString		m_CurrentRoomName;
	SimpleString		m_CurrentWorldName;

	typedef Array<byte> TUndoState;			// Undo states are just in-memory room modules
	Array<TUndoState>	m_UndoStates;
	int					m_UndoStateIndex;	// Index of the current state; Undo loads i-1, Redo loads i+1
	int					m_SavedUndoStateIndex;
	bool				m_LoadingUndoState;
	bool				m_SavingUndoState;

	Array<SBrush>		m_Brushes;
	Array<uint>			m_SelectedBrushes;

	Array<SPortals>		m_Portals;

	bool				m_GridActive;
	float				m_GridSize;
	float				m_RotateGridSize;

	int					m_TransformStartCursorX;	// Used for translation, rotation, scale, box select, etc.
	int					m_TransformStartCursorY;	// Used for translation, rotation, scale, box select, etc.
	int					m_TransformAnchorScreenX;	// Used for translation, etc.
	int					m_TransformAnchorScreenY;	// Used for translation, etc.
	Vector				m_TransformAnchor;			// Used for translation, etc.
	Vector				m_TranslateAnchorStart;		// Ditto, equivalent to SBrush::m_TranslateStart but also used as anchor while anchor is being transformed (in Translate and TranslateAnchor)

	bool				m_IsTranslating;
	bool				m_IsTranslatingAnchor;
	bool				m_IsRotating;
	bool				m_IsScaling;
	bool				m_IsBoxSelecting;

	Array<Vector>		m_NavSnapPoints;
	Vector				m_NavSnapPoint;

	// No need to know about connectivity here, I'll determine that in RoomBaker
	Array<SNavVert>	m_NavVerts;
	Array<SNavEdge>	m_NavEdges;
	Array<SNavFace>	m_NavFaces;
	Array<uint>		m_SelectedNavVerts;	// Indexes into m_NavVerts
	Array<uint>		m_SelectedNavEdges;	// Indexes into m_NavEdges
	Array<uint>		m_SelectedNavFaces;	// Indexes into m_NavFaces
	Mesh*			m_NavMesh;
	Mesh*			m_NavSnapMesh;

	Map<HashedString, uint>	m_SpawnerNameColors;	// For highlighting spawners in editor
};

#endif // BUILD_ROSA_TOOLS

#endif // ROSATOOLS_H
