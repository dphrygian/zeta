#include "core.h"
#include "roombaker.h"
#include "rosaroom.h"
#include "rosaroom-editor.h"
#include "rosatools.h"
#include "filestream.h"
#include "configmanager.h"
#include "meshfactory.h"
#include "frameworkutil.h"
#include "fileutil.h"
#include "mathcore.h"
#include "triangle.h"
#include "matrix.h"

bool VertCompare( const Vector& VertA, const Vector& VertB )
{
	return VertA.Equals( VertB, EPSILON );
}

Array<uint> Jarvis_2DConvexHull( const Array<Vector>& Verts )
{
	ASSERT( Verts.Size() >= 3 );

	uint	LeftmostIndex	= 0;
	FOR_EACH_ARRAY( VertIter, Verts, Vector )
	{
		const Vector& Vert		= VertIter.GetValue();
		const Vector& Leftmost	= Verts[ LeftmostIndex ];
		if( Vert.x < Leftmost.x ||
			( Vert.x == Leftmost.x && Vert.y < Leftmost.y ) )	// Handle collinear leftmost verts
		{
			LeftmostIndex	= VertIter.GetIndex();
		}
	}

	Array<uint>	OutHullPointIndices;
	uint		HullPointIndex	= LeftmostIndex;
	uint		EndpointIndex	= 0;

	do
	{
		EndpointIndex				= 0;
		const Vector&	HullPoint	= Verts[ HullPointIndex ];
		ASSERT( !OutHullPointIndices.Find( HullPointIndex ) );
		OutHullPointIndices.PushBack( HullPointIndex );

		FOR_EACH_INDEX_FROM( NextIndex, 1, Verts.Size() )
		{
			const Vector&	NextVert		= Verts[ NextIndex ];
			const Vector	HullToEnd		= Verts[ EndpointIndex ] - HullPoint;
			const Vector	HullToNext		= NextVert - HullPoint;
			const Vector	HullToEndN		= HullToEnd.GetNormalized();
			const Vector	HullToNextN		= HullToNext.GetNormalized();
			const Vector	HullToEndRight	= HullToEndN.Cross( Vector::Up ); // No need to normalize, we're only using this for sign of dot
			const float		NextRightDot	= HullToEndRight.Dot( HullToNextN );
			const bool		NextIsLeft		= NextRightDot < 0.0f;
			const bool		NextIsCollinear	= NextRightDot == 0.0f;	// ROSATODO: Should this be an epsilon test?
			const bool		NextIsFurther	= HullToNext.LengthSquared() > HullToEnd.LengthSquared();
			if( EndpointIndex == HullPointIndex || NextIsLeft || ( NextIsCollinear && NextIsFurther ) )
			{
				EndpointIndex = NextIndex;
			}
		}

		HullPointIndex = EndpointIndex;
	}
	while ( EndpointIndex != LeftmostIndex );

	return OutHullPointIndices;
}

void TryAddEdgePlanes( SConvexHull& Hull, const Array<Triangle>& TransformedHullTris, const Matrix& SwizzleMatrix )
{
	const Matrix	InvSwizzleMatrix	= SwizzleMatrix.GetInverse();
	// Flatten the triangle edges onto XY plane
	Plane			AxialPlane			= Plane( Vector( 0.0f, 0.0f, 1.0f ), 0.0f );
	Array<Vector>	FlattenedVerts;
	FOR_EACH_ARRAY( TriIter, TransformedHullTris, Triangle )
	{
		Triangle&	Tri	= TriIter.GetValue();
		FlattenedVerts.PushBackUniqueCompare( AxialPlane.ProjectPoint( SwizzleMatrix * Tri.m_Vec1 ), VertCompare );
		FlattenedVerts.PushBackUniqueCompare( AxialPlane.ProjectPoint( SwizzleMatrix * Tri.m_Vec2 ), VertCompare );
		FlattenedVerts.PushBackUniqueCompare( AxialPlane.ProjectPoint( SwizzleMatrix * Tri.m_Vec3 ), VertCompare );
	}

	// ROSATODO: Remove collinear verts (sets of 3 or more verts lying on a line)?

	// Get the 2D convex hull using Jarvis gift wrapping algorithm,
	// then add planes for the 2D hull edges
	Array<uint>		HullVertIndices			= Jarvis_2DConvexHull( FlattenedVerts );
	const uint		NumHullVerts			= HullVertIndices.Size();
	FOR_EACH_INDEX( HullVertIndexA, NumHullVerts )
	{
		const uint		HullVertIndexB	= ( HullVertIndexA + 1 ) % NumHullVerts;
		const Vector	HullVertA		= FlattenedVerts[ HullVertIndices[ HullVertIndexA ] ];
		const Vector	HullVertB		= FlattenedVerts[ HullVertIndices[ HullVertIndexB ] ];
		const Vector	HullNormal		= ( HullVertA - HullVertB ).GetNormalized().Cross( Vector::Up ).GetNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
		const Plane		HullPlane		= Plane( InvSwizzleMatrix * HullNormal, InvSwizzleMatrix * HullVertA );
		Hull.m_Hull.TryAddPlane( HullPlane, EPSILON );
	}
}

RoomBaker::RoomBaker()
:	m_pMeshFactory( NULL )
{
}

RoomBaker::~RoomBaker()
{
}

void RoomBaker::BakeRoom( const RosaRoomEditor& PreBakeRoom, const SimpleString& OutFilename ) const
{
	// Now switch to the Baked directory so we can load content.
	FileUtil::ChangeWorkingDirectory( "../Baked" );

	// Load everything from the game's usual config files.
	FrameworkUtil::MinimalLoadConfigFiles( "Config/default.ccf" );

	RosaRoom PostBakeRoom;

	Array<Array<Triangle> >	HullTrisArray;

	// Copy over the easy stuff first.
	PostBakeRoom.m_TilesX	= PreBakeRoom.m_TilesX;
	PostBakeRoom.m_TilesY	= PreBakeRoom.m_TilesY;
	PostBakeRoom.m_TilesZ	= PreBakeRoom.m_TilesZ;
	PostBakeRoom.m_MetersX	= PreBakeRoom.m_MetersX;
	PostBakeRoom.m_MetersY	= PreBakeRoom.m_MetersY;
	PostBakeRoom.m_MetersZ	= PreBakeRoom.m_MetersZ;

	// Bounds should contain at least the part inside module size
	const float	fSizeX				= static_cast<float>( PreBakeRoom.m_MetersX );
	const float	fSizeY				= static_cast<float>( PreBakeRoom.m_MetersY );
	const float	fSizeZ				= static_cast<float>( PreBakeRoom.m_MetersZ );
	PostBakeRoom.m_RenderBound		= AABB( Vector(), Vector( fSizeX, fSizeY, fSizeZ ) );
	PostBakeRoom.m_CollisionBound	= PostBakeRoom.m_RenderBound;

	// Cache for portal stuff
	const float	fMetersPerTileX		= fSizeX / static_cast<float>( PreBakeRoom.m_TilesX );
	const float	fMetersPerTileY		= fSizeY / static_cast<float>( PreBakeRoom.m_TilesY );
	const float	fMetersPerTileZ		= fSizeZ / static_cast<float>( PreBakeRoom.m_TilesZ );

	// Init the portals from the editor room; only the name hash is saved,
	// but the other properties are used to make hulls just below.
	FOR_EACH_ARRAY( PortalsIter, PreBakeRoom.m_Portals, RosaRoomEditor::SPortals )
	{
		const RosaRoomEditor::SPortals&	PreBakePortals	= PortalsIter.GetValue();
		SPortals&						PostBakePortals	= PostBakeRoom.m_Portals.PushBack();
		for( uint Index = 0; Index < 6; ++Index )
		{
			const RosaRoomEditor::SPortal&	PreBakePortal	= PreBakePortals.m_Portals[ Index ];
			SPortal&						PostBakePortal	= PostBakePortals.m_Portals[ Index ];

			PostBakePortal.m_DefNameHash = PreBakePortal.m_DefName;
			PostBakePortal.InitializeFromDefinition( PostBakePortal.m_DefNameHash );
		}
	}

	// Create hull brushes for any untagged portal surface
	// (ROSATODO: coalesce these like in RosaWorldGen::CreatePortalsForSector)
	FOR_EACH_ARRAY( PortalsIter, PostBakeRoom.m_Portals, SPortals )
	{
		const uint	PortalLoc		= PortalsIter.GetIndex();
		const uint	PortalX			= PortalLoc % PostBakeRoom.m_TilesX;
		const uint	PortalY			= ( PortalLoc / PostBakeRoom.m_TilesX ) % PostBakeRoom.m_TilesY;
		const uint	PortalZ			= ( ( PortalLoc / PostBakeRoom.m_TilesX ) / PostBakeRoom.m_TilesY ) % PostBakeRoom.m_TilesZ;
		SPortals&	PostBakePortals	= PortalsIter.GetValue();

		for( uint Index = 0; Index < 6; ++Index )
		{
			SPortal&	PostBakePortal	= PostBakePortals.m_Portals[ Index ];

			if( PostBakePortal.m_FrontTag )
			{
				// Skip actual portals
				continue;
			}

			if( ( Index == EPI_East		&& PortalX != PostBakeRoom.m_TilesX - 1 ) ||
				( Index == EPI_West		&& PortalX != 0 ) ||
				( Index == EPI_North	&& PortalY != PostBakeRoom.m_TilesY - 1 ) ||
				( Index == EPI_South	&& PortalY != 0 ) ||
				( Index == EPI_Up		&& PortalZ != PostBakeRoom.m_TilesZ - 1 ) ||
				( Index == EPI_Down		&& PortalZ != 0 ) )
			{
				// Skip non-boundary portals, they're meaningless
				continue;
			}

			// Start with the full volume of the portal region, then flatten to the appropriate side
			const Vector		PortalBoundLo	= Vector( PortalX * fMetersPerTileX, PortalY * fMetersPerTileY, PortalZ * fMetersPerTileZ );
			const Vector		PortalBoundHi	= Vector( ( PortalX + 1 ) * fMetersPerTileX, ( PortalY + 1 ) * fMetersPerTileY, ( PortalZ + 1 ) * fMetersPerTileZ );
			AABB				PortalBound		= AABB( PortalBoundLo, PortalBoundHi );
			if( Index == EPI_East )		{ PortalBound.m_Min.x = PortalBound.m_Max.x; }
			if( Index == EPI_West )		{ PortalBound.m_Max.x = PortalBound.m_Min.x; }
			if( Index == EPI_North )	{ PortalBound.m_Min.y = PortalBound.m_Max.y; }
			if( Index == EPI_South )	{ PortalBound.m_Max.y = PortalBound.m_Min.y; }
			if( Index == EPI_Up )		{ PortalBound.m_Min.z = PortalBound.m_Max.z; }
			if( Index == EPI_Down )		{ PortalBound.m_Max.z = PortalBound.m_Min.z; }

			RosaRoom::SBrush&	HullBrush	= PostBakeRoom.m_Brushes.PushBack();
			HullBrush.m_Type				= EBT_Geo;
			SConvexHull&		ConvexHull	= HullBrush.m_Hulls.PushBack( RosaTools::CreateHullFromAABB( PortalBound, HashedString::NullString ) );
			ConvexHull.m_CollisionFlags		= EECF_BlocksWorld | EECF_BlocksEntities | EECF_BlocksBlockers | EECF_BlocksRagdolls;	// These don't block traces! (Or occlusion/audio/etc.)

			// Push a dummy triangles array to keep tris/hulls arrays in sync
			Array<Triangle>&	HullTris	= HullTrisArray.PushBack();
			Unused( HullTris );
		}
	}

	const uint NumPortalBrushes = PostBakeRoom.m_Brushes.Size();

	// Reload brush defs and recompute collision planes from meshes.
	FOR_EACH_ARRAY( BrushIter, PreBakeRoom.m_Brushes, RosaRoomEditor::SBrush )
	{
		const RosaRoomEditor::SBrush&	PreBakeBrush	= BrushIter.GetValue();
		RosaRoom::SBrush&				PostBakeBrush	= PostBakeRoom.m_Brushes.PushBack();

		PostBakeBrush.m_DefName			= PreBakeBrush.m_DefName;
		PostBakeBrush.m_Type			= PreBakeBrush.m_Type;
		PostBakeBrush.m_Location		= PreBakeBrush.m_Location;
		PostBakeBrush.m_Orientation		= PreBakeBrush.m_Orientation;
		PostBakeBrush.m_Scale			= PreBakeBrush.m_Scale;
		PostBakeBrush.m_Mat				= PreBakeBrush.m_Mat;

		// HACKHACK: Since we inject brushes for portals, we need to adjust link indices
		FOR_EACH_ARRAY( LinkedBrushIter, PreBakeBrush.m_LinkedBrushes, uint )
		{
			PostBakeBrush.m_LinkedBrushes.PushBack( NumPortalBrushes + LinkedBrushIter.GetValue() );
		}

		const bool						HasMat			= PostBakeBrush.m_Mat != HashedString::NullString;

		// ROSATODO: This is all essentially identical to creating brush defs in RosaTools::AppendGeoCategory,
		// except that it writes to a RosaRoom::SBrush instead of a RosaTools::SBrushDef. Unify somehow.
		if( PreBakeBrush.m_Type == EBT_Geo )
		{
			// HACKHACK: If the geodef is a .cbr file, load the config file now
			if( PreBakeBrush.m_DefName.EndsWith( ".cbr" ) )
			{
				ConfigManager::Load( FileStream( PreBakeBrush.m_DefName.CStr(), FileStream::EFM_Read ) );
			}

			MAKEHASHFROM( BrushDefName, PreBakeBrush.m_DefName );

			STATICHASH( Invisible );
			const bool Invisible	= HasMat ? ConfigManager::GetInheritedBool( sInvisible, false, PostBakeBrush.m_Mat ) : false;

			if( Invisible )
			{
				// Don't export mesh for brushes with an invisible material
			}
			else
			{
				STATICHASH( Material );
				const char* const	pMatMaterial	= HasMat ?	ConfigManager::GetInheritedString( sMaterial, NULL, PostBakeBrush.m_Mat ) : NULL;

				STATICHASH( CastsShadows );
				const bool			MatCastsShadows	= HasMat ?	ConfigManager::GetInheritedBool( sCastsShadows, true, PostBakeBrush.m_Mat ) : true;

				STATICHASH( Mesh );
				const SimpleString	MeshName		= ConfigManager::GetInheritedString( sMesh, "", sBrushDefName );

				if( MeshName != "" )
				{
					RosaRoom::SMesh& Mesh	= PostBakeBrush.m_Meshes.PushBack();
					Mesh.m_MeshName			= MeshName;

					STATICHASH( Material );
					Mesh.m_MaterialName		= pMatMaterial ? pMatMaterial :	ConfigManager::GetInheritedString( sMaterial, "", sBrushDefName );

					STATICHASH( CastsShadows );
					Mesh.m_CastsShadows		= MatCastsShadows ?				ConfigManager::GetInheritedBool( sCastsShadows, true, sBrushDefName ) : false;
				}

				STATICHASH( NumMeshes );
				const uint NumMeshes = ConfigManager::GetInheritedInt( sNumMeshes, 0, sBrushDefName );
				for( uint MeshIndex = 0; MeshIndex < NumMeshes; ++MeshIndex )
				{
					RosaRoom::SMesh& Mesh	= PostBakeBrush.m_Meshes.PushBack();
					Mesh.m_MeshName			=								ConfigManager::GetInheritedSequenceString(	"Mesh%d",				MeshIndex, "",		sBrushDefName );
					Mesh.m_MaterialName		= pMatMaterial ? pMatMaterial :	ConfigManager::GetInheritedSequenceString(	"Mesh%dMaterial",		MeshIndex, "",		sBrushDefName );
					Mesh.m_CastsShadows		= MatCastsShadows ?				ConfigManager::GetInheritedSequenceBool(	"Mesh%dCastsShadows",	MeshIndex, true,	sBrushDefName ) : false;
				}
			}

			// 24 Nov 2019: Let material completely disable collision on a brush, ignoring any hulls
			STATICHASH( Collision );
			const bool	MatCollision	= HasMat ? ConfigManager::GetInheritedBool( sCollision, true, PostBakeBrush.m_Mat ) : true;

			if( MatCollision )
			{
				STATICHASH( Surface );
				const HashedString	MatSurface			= HasMat ? ConfigManager::GetInheritedHash( sSurface, HashedString::NullString, PostBakeBrush.m_Mat ) : HashedString::NullString;

				STATICHASH( BlocksOcclusion );
				const bool			MatBlocksOcclusion	= HasMat ? ConfigManager::GetInheritedBool( sBlocksOcclusion, true, PostBakeBrush.m_Mat ) : true;

				STATICHASH( Hull );
				const SimpleString	HullName			= ConfigManager::GetInheritedString( sHull, "", sBrushDefName );

				if( HullName != "" )
				{
					STATICHASH( Surface );
					const HashedString	Surface			= MatSurface ? MatSurface :	ConfigManager::GetInheritedHash( sSurface, HashedString::NullString, sBrushDefName );

					STATICHASH( BlocksEntities );
					const bool			BlocksEntities	=							ConfigManager::GetInheritedBool( sBlocksEntities, true, sBrushDefName );

					STATICHASH( BlocksTrace );
					const bool			BlocksTrace		=							ConfigManager::GetInheritedBool( sBlocksTrace, true, sBrushDefName );

					STATICHASH( BlocksOcclusion );
					const bool			BlocksOcclusion	= MatBlocksOcclusion ?		ConfigManager::GetInheritedBool( sBlocksOcclusion, true, sBrushDefName ) : false;

					STATICHASH( BlocksAudio );
					const bool			BlocksAudio		=							ConfigManager::GetInheritedBool( sBlocksAudio, true, sBrushDefName );

					Array<Triangle>&	HullTris		= HullTrisArray.PushBack();
					SConvexHull& ConvexHull				= PostBakeBrush.m_Hulls.PushBack( RosaTools::CreateHullWithTris( HullName, Surface, m_pMeshFactory, HullTris ) );

					if( BlocksEntities )	{ ConvexHull.m_CollisionFlags |= EECF_BlocksWorld | EECF_BlocksEntities | EECF_BlocksBlockers | EECF_BlocksRagdolls; }
					if( BlocksTrace )		{ ConvexHull.m_CollisionFlags |= EECF_BlocksTrace;		}
					if( BlocksOcclusion )	{ ConvexHull.m_CollisionFlags |= EECF_BlocksOcclusion;	}
					if( BlocksAudio )		{ ConvexHull.m_CollisionFlags |= EECF_BlocksAudio;	}
				}

				STATICHASH( NumHulls );
				const uint NumHulls = ConfigManager::GetInheritedInt( sNumHulls, 0, sBrushDefName );
				for( uint HullIndex = 0; HullIndex < NumHulls; ++HullIndex )
				{
					const SimpleString	IndexedHullName	=							ConfigManager::GetInheritedSequenceString(	"Hull%d",					HullIndex, "",							sBrushDefName );
					const HashedString	Surface			= MatSurface ? MatSurface :	ConfigManager::GetInheritedSequenceHash(	"Hull%dSurface",			HullIndex, HashedString::NullString,	sBrushDefName );
					const bool			BlocksEntities	=							ConfigManager::GetInheritedSequenceBool(	"Hull%dBlocksEntities",		HullIndex, true,						sBrushDefName );
					const bool			BlocksTrace		=							ConfigManager::GetInheritedSequenceBool(	"Hull%dBlocksTrace",		HullIndex, true,						sBrushDefName );
					const bool			BlocksOcclusion	= MatBlocksOcclusion ?		ConfigManager::GetInheritedSequenceBool(	"Hull%dBlocksOcclusion",	HullIndex, true,						sBrushDefName ) : false;
					const bool			BlocksAudio		=							ConfigManager::GetInheritedSequenceBool(	"Hull%dBlocksAudio",		HullIndex, true,						sBrushDefName );

					Array<Triangle>&	HullTris		= HullTrisArray.PushBack();
					SConvexHull&		ConvexHull		= PostBakeBrush.m_Hulls.PushBack( RosaTools::CreateHullWithTris( IndexedHullName, Surface, m_pMeshFactory, HullTris ) );

					if( BlocksEntities )	{ ConvexHull.m_CollisionFlags |= EECF_BlocksWorld | EECF_BlocksEntities | EECF_BlocksBlockers | EECF_BlocksRagdolls;	}
					if( BlocksTrace )		{ ConvexHull.m_CollisionFlags |= EECF_BlocksTrace;		}
					if( BlocksOcclusion )	{ ConvexHull.m_CollisionFlags |= EECF_BlocksOcclusion;	}
					if( BlocksAudio )		{ ConvexHull.m_CollisionFlags |= EECF_BlocksAudio;		}
				}
			}

			STATICHASH( NumAmbientLights );
			const uint NumAmbientLights = ConfigManager::GetInheritedInt( sNumAmbientLights, 0, sBrushDefName );
			FOR_EACH_INDEX( AmbientLightIndex, NumAmbientLights )
			{
				RosaRoom::SAmbientLight&	AmbientLight	= PostBakeBrush.m_AmbientLights.PushBack();

				AmbientLight.m_Mesh		= ConfigManager::GetInheritedSequenceString( "AmbientLight%d",			AmbientLightIndex, "", sBrushDefName );
				AmbientLight.m_Cubemap	= ConfigManager::GetInheritedSequenceString( "AmbientLight%dCubemap",	AmbientLightIndex, "", sBrushDefName );
				AmbientLight.m_Hull		= RosaTools::CreateHull( AmbientLight.m_Mesh, HashedString::NullString, m_pMeshFactory );

				// Apply the local transforms so we can treat these identically to collision hulls
				AmbientLight.m_Hull.m_Bounds = AmbientLight.m_Hull.m_Bounds.GetTransformedBound( PostBakeBrush.m_Location, PostBakeBrush.m_Orientation, Vector( PostBakeBrush.m_Scale, PostBakeBrush.m_Scale, PostBakeBrush.m_Scale ) );
				AmbientLight.m_Hull.m_Hull.MoveBy( PostBakeBrush.m_Location, PostBakeBrush.m_Orientation, PostBakeBrush.m_Scale );
				// (Unlike collision hulls, I don't need to add AABB expansion planes
				// because I don't need to sweep against ambient light shapes.)
			}

			STATICHASH( NumFogMeshes );
			const uint NumFogMeshes = ConfigManager::GetInheritedInt( sNumFogMeshes, 0, sBrushDefName );
			FOR_EACH_INDEX( FogMeshIndex, NumFogMeshes )
			{
				RosaRoom::SFogMesh&	FogMesh	= PostBakeBrush.m_FogMeshes.PushBack();

				FogMesh.m_Mesh			= ConfigManager::GetInheritedSequenceString(	"FogMesh%d",			FogMeshIndex, "", sBrushDefName );
				FogMesh.m_FogMeshDef	= ConfigManager::GetInheritedSequenceHash(		"FogMesh%dFogMeshDef",	FogMeshIndex, "", sBrushDefName );
			}
		}
		else if( PreBakeBrush.m_Type == EBT_Spawner )
		{
			// Do nothing
		}
	}

	// Postprocess brushes
	uint HullTrisIndex = 0;
	FOR_EACH_ARRAY( BrushIter, PostBakeRoom.m_Brushes, RosaRoom::SBrush )
	{
		RosaRoom::SBrush&	PostBakeBrush	= BrushIter.GetValue();

		// Apply local transforms to convex hulls now, so I can add the AABB expansion planes.
		// Then in RosaWorldGen, we'll only need to apply the room transform, not add any planes.
		FOR_EACH_ARRAY( HullIter, PostBakeBrush.m_Hulls, SConvexHull )
		{
			SConvexHull& Hull = HullIter.GetValue();

			// Position/orient the bounds, and expand room collision bound to contain anything beyond the module size
			Hull.m_Bounds = Hull.m_Bounds.GetTransformedBound( PostBakeBrush.m_Location, PostBakeBrush.m_Orientation, Vector( PostBakeBrush.m_Scale, PostBakeBrush.m_Scale, PostBakeBrush.m_Scale ) );
			PostBakeRoom.m_CollisionBound.ExpandTo( Hull.m_Bounds );

			// Position/orient the hull
			Hull.m_Hull.MoveBy( PostBakeBrush.m_Location, PostBakeBrush.m_Orientation, PostBakeBrush.m_Scale );

			// Add AABB expansion planes
			Hull.m_Hull.TryAddPlane( Plane( Vector( -1.0f, 0.0f, 0.0f ), Hull.m_Bounds.m_Min.x ), EPSILON );
			Hull.m_Hull.TryAddPlane( Plane( Vector( 1.0f, 0.0f, 0.0f ), -Hull.m_Bounds.m_Max.x ), EPSILON );
			Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, -1.0f, 0.0f ), Hull.m_Bounds.m_Min.y ), EPSILON );
			Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, 1.0f, 0.0f ), -Hull.m_Bounds.m_Max.y ), EPSILON );
			Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, 0.0f, -1.0f ), Hull.m_Bounds.m_Min.z ), EPSILON );
			Hull.m_Hull.TryAddPlane( Plane( Vector( 0.0f, 0.0f, 1.0f ), -Hull.m_Bounds.m_Max.z ), EPSILON );

			// Also add AABB expansion planes for relevant edges
			const Array<Triangle>&	HullTris	= HullTrisArray[ HullTrisIndex++ ];
			if( HullTris.Empty() )
			{
				continue;
			}

			// Transform triangles into brush space
			Array<Triangle>	TransformedHullTris	= HullTris;	// Make a copy to modify
			const Matrix	ScaleMatrix			= Matrix::CreateScale( Vector( PostBakeBrush.m_Scale, PostBakeBrush.m_Scale, PostBakeBrush.m_Scale ) );
			const Matrix	RotationMatrix		= PostBakeBrush.m_Orientation.ToMatrix();
			const Matrix	TranslationMatrix	= Matrix::CreateTranslation( PostBakeBrush.m_Location );
			const Matrix	Transform			= ScaleMatrix * RotationMatrix * TranslationMatrix;
			FOR_EACH_ARRAY( TriIter, TransformedHullTris, Triangle )
			{
				Triangle&	Tri	= TriIter.GetValue();
				Tri.m_Vec1 = Transform * Tri.m_Vec1;
				Tri.m_Vec2 = Transform * Tri.m_Vec2;
				Tri.m_Vec3 = Transform * Tri.m_Vec3;
			}

			const Vector	X					= Vector( 1.0f, 0.0f, 0.0f );
			const Vector	Y					= Vector( 0.0f, 1.0f, 0.0f );
			const Vector	Z					= Vector( 0.0f, 0.0f, 1.0f );
			TryAddEdgePlanes( Hull, TransformedHullTris, Matrix() );							// Add XY planes
			TryAddEdgePlanes( Hull, TransformedHullTris, Matrix::CreateCoordinate( X, Z, Y ) );	// Add XZ planes
			TryAddEdgePlanes( Hull, TransformedHullTris, Matrix::CreateCoordinate( Z, X, Y ) );	// Add YZ planes
		}
	}

	// Convert editor nav data in game nav data
	PostBakeRoom.m_NavNodes.Reserve( PreBakeRoom.m_NavFaces.Size() );
	PostBakeRoom.m_NavEdges.Reserve( PreBakeRoom.m_NavFaces.Size() * 3 );
	FOR_EACH_ARRAY( FaceIter, PreBakeRoom.m_NavFaces, RosaTools::SNavFace )
	{
		const RosaTools::SNavFace&	PreNavFace	= FaceIter.GetValue();
		const RosaTools::SNavEdge&	PreNavEdgeA	= PreBakeRoom.m_NavEdges[ PreNavFace.m_EdgeA ];
		const Vector&				NavVertAA	= PreBakeRoom.m_NavVerts[ PreNavEdgeA.m_VertA ].m_Vert;
		const Vector&				NavVertAB	= PreBakeRoom.m_NavVerts[ PreNavEdgeA.m_VertB ].m_Vert;
		const RosaTools::SNavEdge&	PreNavEdgeB	= PreBakeRoom.m_NavEdges[ PreNavFace.m_EdgeB ];
		const Vector&				NavVertBA	= PreBakeRoom.m_NavVerts[ PreNavEdgeB.m_VertA ].m_Vert;
		const Vector&				NavVertBB	= PreBakeRoom.m_NavVerts[ PreNavEdgeB.m_VertB ].m_Vert;
		const RosaTools::SNavEdge&	PreNavEdgeC	= PreBakeRoom.m_NavEdges[ PreNavFace.m_EdgeC ];
		const Vector&				NavVertCA	= PreBakeRoom.m_NavVerts[ PreNavEdgeC.m_VertA ].m_Vert;
		const Vector&				NavVertCB	= PreBakeRoom.m_NavVerts[ PreNavEdgeC.m_VertB ].m_Vert;

		const uint					EdgeIndexA	= PostBakeRoom.m_NavEdges.Size();
		const uint					EdgeIndexB	= EdgeIndexA + 1;
		const uint					EdgeIndexC	= EdgeIndexA + 2;

		SNavNode& NavNode	= PostBakeRoom.m_NavNodes.PushBack();
		SNavEdge& NavEdgeA	= PostBakeRoom.m_NavEdges.PushBack();
		SNavEdge& NavEdgeB	= PostBakeRoom.m_NavEdges.PushBack();
		SNavEdge& NavEdgeC	= PostBakeRoom.m_NavEdges.PushBack();

		NavNode.m_EdgeA		= EdgeIndexA;
		NavNode.m_EdgeB		= EdgeIndexB;
		NavNode.m_EdgeC		= EdgeIndexC;

		NavEdgeA.m_VertA	= NavVertAA;
		NavEdgeA.m_VertB	= NavVertAB;
		NavEdgeB.m_VertA	= NavVertBA;
		NavEdgeB.m_VertB	= NavVertBB;
		NavEdgeC.m_VertA	= NavVertCA;
		NavEdgeC.m_VertB	= NavVertCB;

		NavEdgeA.m_Width	= ( NavVertAB - NavVertAA ).Length2D();
		NavEdgeB.m_Width	= ( NavVertBB - NavVertBA ).Length2D();
		NavEdgeC.m_Width	= ( NavVertCB - NavVertCA ).Length2D();
		ASSERT( NavEdgeA.m_Width > 0.0f );
		ASSERT( NavEdgeB.m_Width > 0.0f );
		ASSERT( NavEdgeC.m_Width > 0.0f );

		// For each edge, find the face with the same edge (if any) and link the backnode
		FOR_EACH_ARRAY( BackfaceIter, PreBakeRoom.m_NavFaces, RosaTools::SNavFace )
		{
			if( BackfaceIter.GetIndex() == FaceIter.GetIndex() )
			{
				continue;
			}

			const RosaTools::SNavFace&	PreBackFace	= BackfaceIter.GetValue();

			if( PreNavFace.m_EdgeA == PreBackFace.m_EdgeA ||
				PreNavFace.m_EdgeA == PreBackFace.m_EdgeB ||
				PreNavFace.m_EdgeA == PreBackFace.m_EdgeC )
			{
				if( NavEdgeA.m_BackNode != NAV_NULL )
				{
					PRINTF( "Edge %s-%s already has a back node!\n", NavEdgeA.m_VertA.GetString().CStr(), NavEdgeA.m_VertB.GetString().CStr() );
					WARNDESC( "Nav edge already has a back node, see log for details" );
				}

				NavEdgeA.m_BackNode = BackfaceIter.GetIndex();
			}

			if( PreNavFace.m_EdgeB == PreBackFace.m_EdgeA ||
				PreNavFace.m_EdgeB == PreBackFace.m_EdgeB ||
				PreNavFace.m_EdgeB == PreBackFace.m_EdgeC )
			{
				if( NavEdgeB.m_BackNode != NAV_NULL )
				{
					PRINTF( "Edge %s-%s already has a back node!\n", NavEdgeB.m_VertA.GetString().CStr(), NavEdgeB.m_VertB.GetString().CStr() );
					WARNDESC( "Nav edge already has a back node, see log for details" );
				}

				NavEdgeB.m_BackNode = BackfaceIter.GetIndex();
			}

			if( PreNavFace.m_EdgeC == PreBackFace.m_EdgeA ||
				PreNavFace.m_EdgeC == PreBackFace.m_EdgeB ||
				PreNavFace.m_EdgeC == PreBackFace.m_EdgeC )
			{
				if( NavEdgeC.m_BackNode != NAV_NULL )
				{
					PRINTF( "Edge %s-%s already has a back node!\n", NavEdgeC.m_VertA.GetString().CStr(), NavEdgeC.m_VertB.GetString().CStr() );
					WARNDESC( "Nav edge already has a back node, see log for details" );
				}

				NavEdgeC.m_BackNode = BackfaceIter.GetIndex();
			}
		}

		// Get the three unique verts for the triangle
		Array<Vector> NavVerts;
		NavVerts.PushBackUnique( NavVertAA );
		NavVerts.PushBackUnique( NavVertAB );
		NavVerts.PushBackUnique( NavVertBA );
		NavVerts.PushBackUnique( NavVertBB );
		NavVerts.PushBackUnique( NavVertCA );
		NavVerts.PushBackUnique( NavVertCB );
		ASSERT( NavVerts.Size() == 3 );

		NavNode.m_Tri = Triangle( NavVerts[0], NavVerts[1], NavVerts[2] );
		const Vector UpVector = Vector( 0.0f, 0.0f, 1.0f );
		if( NavNode.m_Tri.GetNormal().Dot( UpVector ) < 0.0f )
		{
			// Fix winding order
			NavNode.m_Tri = Triangle( NavVerts[0], NavVerts[2], NavVerts[1] );
		}

		NavNode.m_Centroid	= NavNode.m_Tri.GetCentroid();

		NavNode.m_Bounds	= NavNode.m_Tri.GetAABB();

		// HACKHACK: Since the room stores nav height, set any older format rooms to have a generous fixed height
		NavNode.m_Height	= ( PreNavFace.m_Height > 0.0f ) ? PreNavFace.m_Height : 10.0f;

		// DLP 24 May 2020: These can be baked in now!
		NavNode.m_Props		= PreNavFace.m_Props;
	}

	// Switch back to the Tools directory as soon as possible.
	FileUtil::ChangeWorkingDirectory( "../Tools" );

	PostBakeRoom.Save( FileStream( OutFilename.CStr(), FileStream::EFM_Write ) );
}

RosaWorldGen::ERoomXform GetRoomTransform( const Angles& Orientation )
{
	const float Yaw = Mod( TWOPI + Orientation.Yaw, TWOPI );

	if( Equal( Yaw, DEGREES_TO_RADIANS( 0.0f ) ) )		{ return RosaWorldGen::ERoomXform::ERT_None; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 90.0f ) ) )		{ return RosaWorldGen::ERoomXform::ERT_Rot90; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 180.0f ) ) )	{ return RosaWorldGen::ERoomXform::ERT_Rot180; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 270.0f ) ) )	{ return RosaWorldGen::ERoomXform::ERT_Rot270; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 360.0f ) ) )	{ return RosaWorldGen::ERoomXform::ERT_None; }

	WARN;
	return RosaWorldGen::ERoomXform::ERT_None;
}

void RoomBaker::BakeWorld( const RosaRoomEditor& PreBakeRoom, const IDataStream& Stream ) const
{
	Array<SWorldGenBakedRoom> BakedRooms;

	FOR_EACH_ARRAY( BrushIter, PreBakeRoom.m_Brushes, RosaRoomEditor::SBrush )
	{
		const RosaRoomEditor::SBrush&	PreBakeBrush	= BrushIter.GetValue();
		if( PreBakeBrush.m_Type == EBT_Room )
		{
			SWorldGenBakedRoom& BakedRoom = BakedRooms.PushBack();
			BakedRoom.m_RoomFilename = PreBakeBrush.m_DefName.Replace( "../Raw/", "" ).Replace( ".rosaroom", ".rrm" );	// Fix up raw -> baked asset references
			// NOTE: Brushes are saved with origins in the middle of a tile, so no need to add +1/2 to round safely
			// NOTE: Using Floor so that negative locations behave correctly (-0.5f is saved as -1, not 0)
			BakedRoom.m_RoomLoc = RosaWorldGen::SRoomLoc(
				static_cast<int>( Floor( PreBakeBrush.m_Location.x ) ),
				static_cast<int>( Floor( PreBakeBrush.m_Location.y ) ),
				static_cast<int>( Floor( PreBakeBrush.m_Location.z ) ) );
			BakedRoom.m_RoomTransform = GetRoomTransform( PreBakeBrush.m_Orientation );
		}
	}

	// DLP 2 Jan 2020: Adding meters/tile so we can get rid of the TileSizeX/Y/Z config
	Stream.WriteUInt32( PreBakeRoom.m_MetersX / PreBakeRoom.m_TilesX );
	Stream.WriteUInt32( PreBakeRoom.m_MetersY / PreBakeRoom.m_TilesY );
	Stream.WriteUInt32( PreBakeRoom.m_MetersZ / PreBakeRoom.m_TilesZ );

	Stream.WriteUInt32( PreBakeRoom.m_TilesX );
	Stream.WriteUInt32( PreBakeRoom.m_TilesY );
	Stream.WriteUInt32( PreBakeRoom.m_TilesZ );

	Stream.WriteUInt32( BakedRooms.Size() );
	FOR_EACH_ARRAY( BakedRoomIter, BakedRooms, SWorldGenBakedRoom )
	{
		const SWorldGenBakedRoom& BakedRoom = BakedRoomIter.GetValue();
		Stream.WriteString( BakedRoom.m_RoomFilename );
		Stream.Write<RosaWorldGen::SRoomLoc>( BakedRoom.m_RoomLoc );
		Stream.WriteUInt32( BakedRoom.m_RoomTransform );
	}
}

int RoomBaker::Bake( const SimpleString& InFilename, const SimpleString& OutFilename )
{
	// Load this just in case I decide to put something in there
	ConfigManager::Load( FileStream( "tools.cfg", FileStream::EFM_Read ) );

	// Create a factory using a null renderer, since we only need the ReadMeshCallback here.
	m_pMeshFactory = new MeshFactory( NULL );

	RosaRoomEditor PreBakeRoom;
	PreBakeRoom.Load( FileStream( InFilename.CStr(), FileStream::EFM_Read ) );

	if( PreBakeRoom.m_MapType == EMT_Room )
	{
		BakeRoom( PreBakeRoom, OutFilename );
	}
	else
	{
		BakeWorld( PreBakeRoom, FileStream( OutFilename.CStr(), FileStream::EFM_Write ) );
	}

	SafeDelete( m_pMeshFactory );

	return 0;
}
