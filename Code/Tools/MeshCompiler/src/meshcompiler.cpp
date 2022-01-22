#include "core.h"
#include "meshcompiler.h"
#include "fileutil.h"
#include "filestream.h"
#include "configmanager.h"
#include "segment.h"
#include "idatastream.h"
#include "filestream.h"
#include "mathcore.h"
#include "reversehash.h"

#include "TinyXML/tinyxml.h"

#include <stdio.h>
#include <memory.h>

#define MESH_COMPILER_VERBOSE 0

// TODO: I make a lot of implicit assumptions here about the
// order of attributes instead of using their names. Fix it.
// That's the whole point of using XML.

void GetXMLMatrix( const TiXmlAttribute* pAttr, Matrix& m )
{
	for( int i = 0; i < 4; ++i )
	{
		for( int j = 0; j < 4; ++j )
		{
			m.m[i][j] = pAttr->FloatValue();
			pAttr = pAttr->Next();
		}
	}
}

void CleanStrncpy( char* Dest, const char* Src, int Num )
{
	Num = Num - 1;
	for( int i = 0; i < Num; ++i )
	{
		if( *Src )
		{
			Dest[i] = *Src;
			++Src;
		}
		else
		{
			Dest[i] = '\0';
		}
	}
	Dest[ Num ] = '\0';
}

void ReplaceExtension( char* String, const char* Ext1, const char* Ext2 )
{
	char* SubString = strstr( String, Ext1 );
	strcpy_s( SubString, 5, Ext2 );
}

void Swizzle( Quat& q )
{
	float t = q.y;
	q.y = q.z;	// This should maybe be negated
	q.z = t;
}

void Swizzle( Vector& v )
{
	float t = v.y;
	v.y = v.z;	// This should maybe be negated too?
	v.z = t;
}

MeshCompiler::~MeshCompiler()
{
}

void MeshCompiler::CompileArmature( const TiXmlElement* const pArm, const bool BaseArmature )
{
	ASSERT( pArm );

	m_Header.m_HasSkeleton = true;

	const TiXmlAttribute* pArmAttr = pArm->FirstAttribute();	// "frames"
	ASSERT( pArmAttr );

	const uint FrameOffset = BaseArmature ? 0 : ( m_Header.m_NumFrames - 1 );	// -1 to account for Blender anims being 1-based

	if( BaseArmature )
	{
		m_Header.m_NumFrames = pArmAttr->IntValue() + 1;	// +1 for T-pose at frame 0
	}
	else
	{
		m_Header.m_NumFrames += pArmAttr->IntValue();
	}

	for( const TiXmlElement* pBone = pArm->FirstChildElement( "bonedef" ); pBone; pBone = pBone->NextSiblingElement( "bonedef" ) )
	{
		const TiXmlAttribute*	pBoneAttr	= pBone->FirstAttribute();
		const HashedString		BoneName	= pBoneAttr->Value();

		if( BaseArmature )
		{
			SNamedBone sbone;
			sbone.m_Name					= BoneName;

			pBoneAttr						= pBoneAttr->Next();
			const HashedString	ParentName	= pBoneAttr->Value();
			if( ParentName )
			{
				// Bones should be exported so children are ordered after parents; we should be able to find this
				sbone.m_ParentIndex = GetIndexForBone( ParentName );
			}
			else
			{
				sbone.m_ParentIndex = -1;
			}

			pBoneAttr						= pBoneAttr->Next();
			sbone.m_Length					= pBoneAttr->FloatValue();

			const TiXmlElement* const	pInvBindPoseElement	= pBone->FirstChildElement( "invbind" );
			const TiXmlAttribute*		pInvBindPoseAttr	= pInvBindPoseElement->FirstAttribute();
			GetXMLMatrix( pInvBindPoseAttr, sbone.m_InvBindPose );

			const Matrix	BindPose	= sbone.m_InvBindPose.GetInverse();
			sbone.m_Orientation			= BindPose.ToQuaternion();
			sbone.m_BoneStart			= BindPose.GetTranslationElements();
			sbone.m_BoneEnd				= sbone.m_BoneStart + sbone.m_Length * sbone.m_Orientation.ToAngles().ToVector();

			// Push bind pose (T-pose) at frame 0 (this isn't as simple as a Quat()/Vector() anymore!)
			const Matrix	ParentInvBindPose	= ( sbone.m_ParentIndex >= 0 ) ? m_Bones[ sbone.m_ParentIndex ].m_InvBindPose : Matrix();
			const Matrix	RelativeBindPose	= BindPose * ParentInvBindPose;
			sbone.m_FrameQuats.PushBack(		RelativeBindPose.ToQuaternion() );
			sbone.m_FrameTransVecs.PushBack(	RelativeBindPose.GetTranslationElements() );

			m_Bones.PushBack( sbone );
			++m_Header.m_NumBones;
		}

		for( const TiXmlElement* pFrame = pBone->FirstChildElement( "frame" ); pFrame; pFrame = pFrame->NextSiblingElement( "frame" ) )
		{
			c_uint8 BoneIndex = GetIndexForBone( BoneName );
			SNamedBone& sbone = m_Bones[ BoneIndex ];

			Matrix FrameMatrix;
			const TiXmlAttribute* pFrameAttr = pFrame->FirstAttribute();

			// In case I ever need separate frame values, and because it makes the intermediate file easier to debug
			const int FrameNum = pFrameAttr->IntValue();
			Unused( FrameNum );

			Quat& Rotation = sbone.m_FrameQuats.PushBack();
			const TiXmlElement* const pRotElement = pFrame->FirstChildElement( "rot" );
			if( pRotElement )
			{
				const TiXmlAttribute* pRotAttr = pRotElement->FirstAttribute();
				Rotation.w = pRotAttr->FloatValue();
				pRotAttr = pRotAttr->Next();
				Rotation.x = pRotAttr->FloatValue();
				pRotAttr = pRotAttr->Next();
				Rotation.y = pRotAttr->FloatValue();
				pRotAttr = pRotAttr->Next();
				Rotation.z = pRotAttr->FloatValue();
			}

			Vector& Location = sbone.m_FrameTransVecs.PushBack();
			const TiXmlElement* const pLocElement = pFrame->FirstChildElement( "loc" );
			if( pLocElement )
			{
				const TiXmlAttribute* pLocAttr = pLocElement->FirstAttribute();
				Location.x = pLocAttr->FloatValue();
				pLocAttr = pLocAttr->Next();
				Location.y = pLocAttr->FloatValue();
				pLocAttr = pLocAttr->Next();
				Location.z = pLocAttr->FloatValue();
			}
		}
	}

	CompileAnimations( pArm, FrameOffset );
}

void MeshCompiler::CompileAnimations( const TiXmlElement* const pArm, const uint FrameOffset )
{
	for( const TiXmlElement* pAnim = pArm->FirstChildElement( "anim" ); pAnim; pAnim = pAnim->NextSiblingElement( "anim" ) )
	{
		SAnimData AnimData;

		// Get starting frame and name (patch up lengths later)
		const TiXmlAttribute* pAnimAttr	= pAnim->FirstAttribute();
		AnimData.m_StartFrame			= static_cast<c_uint16>( FrameOffset + pAnimAttr->IntValue() );

		pAnimAttr						= pAnimAttr->Next();
		const char* const pName			= pAnimAttr->Value();
		CleanStrncpy( AnimData.m_Name, pName, ANIM_NAME_LENGTH );
		AnimData.m_HashedName			= AnimData.m_Name;

		m_AnimData.PushBack( AnimData );
		++m_Header.m_NumAnims;
	}
}

void MeshCompiler::FixUpAnimLengths()
{
	for( int AnimIndex = 0; AnimIndex < ( m_Header.m_NumAnims - 1 ); ++AnimIndex )
	{
		SAnimData& ThisAnim	= m_AnimData[ AnimIndex ];
		SAnimData& NextAnim	= m_AnimData[ AnimIndex + 1 ];
		ThisAnim.m_Length	= NextAnim.m_StartFrame - ThisAnim.m_StartFrame;
	}

	// Patch up the last animation's length separately
	if( m_Header.m_NumAnims )
	{
		SAnimData& LastAnim	= m_AnimData[ m_Header.m_NumAnims - 1 ];
		LastAnim.m_Length	= static_cast<c_uint16>( m_Header.m_NumFrames - LastAnim.m_StartFrame );
	}
}

void MeshCompiler::CompileFace( TiXmlElement* Face )
{
	uint NumVertsInFace = 0;
	uint TempIndices[4];
	Vector TempPositions[4];

	for( TiXmlElement* Vert = Face->FirstChildElement( "vert" ); Vert; Vert = Vert->NextSiblingElement( "vert" ) )
	{
		// Push back cached indices to handle 4-sided faces
		if( NumVertsInFace == 3 )
		{
			m_Indices.PushBack( TempIndices[0] );
			m_Indices.PushBack( TempIndices[2] );
		}

		m_TempPos			= Vector();
		m_TempUV			= Vector2();
		m_TempNorm			= Vector();
		m_TempNormB			= Vector();
		m_TempColor			= Vector4( 1.0f, 1.0f, 1.0f, 1.0f );
		m_TempBoneIndex		= SBoneData();
		m_TempBoneWeight	= SBoneWeights();

		TiXmlElement* Pos = Vert->FirstChildElement( "pos" );
		if( Pos )
		{
			TiXmlAttribute* PosAttr = Pos->FirstAttribute();
			m_TempPos.x = (float)PosAttr->DoubleValue();
			PosAttr = PosAttr->Next();
			m_TempPos.y = (float)PosAttr->DoubleValue();
			PosAttr = PosAttr->Next();
			m_TempPos.z = (float)PosAttr->DoubleValue();
		}

		TempPositions[ NumVertsInFace ] = m_TempPos;

		TiXmlElement* UVEl = Vert->FirstChildElement( "uv" );
		if( UVEl )
		{
			m_Header.m_HasUVs = true;
			TiXmlAttribute* UVAttr = UVEl->FirstAttribute();
			m_TempUV.x = (float)UVAttr->DoubleValue();
			UVAttr = UVAttr->Next();
			m_TempUV.y = (float)UVAttr->DoubleValue();

			// Blender uses OpenGL-style (bottom-to-top) texture coordinates;
			// For now, at least, always convert to Direct3D-style.
			m_TempUV.y = 1.0f - m_TempUV.y;
		}

		TiXmlElement* Norm = Vert->FirstChildElement( "norm" );
		if( Norm )
		{
			m_Header.m_HasNormals = true;
			TiXmlAttribute* NormAttr = Norm->FirstAttribute();
			m_TempNorm.x = (float)NormAttr->DoubleValue();
			NormAttr = NormAttr->Next();
			m_TempNorm.y = (float)NormAttr->DoubleValue();
			NormAttr = NormAttr->Next();
			m_TempNorm.z = (float)NormAttr->DoubleValue();
		}

		TiXmlElement* NormB = Vert->FirstChildElement( "normb" );
		if( NormB )
		{
			m_Header.m_HasNormalsB = true;
			TiXmlAttribute* NormBAttr = NormB->FirstAttribute();
			m_TempNormB.x = (float)NormBAttr->DoubleValue();
			NormBAttr = NormBAttr->Next();
			m_TempNormB.y = (float)NormBAttr->DoubleValue();
			NormBAttr = NormBAttr->Next();
			m_TempNormB.z = (float)NormBAttr->DoubleValue();
		}

		TiXmlElement* const pColor = Vert->FirstChildElement( "col" );
		if( pColor )
		{
			m_Header.m_HasColors		= true;
			TiXmlAttribute* pColorAttr	= pColor->FirstAttribute();
			m_TempColor.r				= static_cast<float>( pColorAttr->IntValue() ) / 255.0f;
			pColorAttr					= pColorAttr->Next();
			m_TempColor.g				= static_cast<float>( pColorAttr->IntValue() ) / 255.0f;
			pColorAttr					= pColorAttr->Next();
			m_TempColor.b				= static_cast<float>( pColorAttr->IntValue() ) / 255.0f;
		}

		// Ignore bone elements unless we have an armature; if we don't, they're just named vertex groups
		if( m_Header.m_HasSkeleton )
		{
			// Default to just using the root bone, in case vert isn't in a vertex group
			m_TempBoneWeight.m_Data[0]	= 1.0f;

			int bIdx = 0;
			for( TiXmlElement* Bone = Vert->FirstChildElement( "bone" ); Bone; Bone = Bone->NextSiblingElement( "bone" ) )
			{
				TiXmlAttribute* BoneAttr = Bone->FirstAttribute();
				m_TempBoneIndex.m_Data[ bIdx ] = GetIndexForBone( HashedString( BoneAttr->Value() ) );
				BoneAttr = BoneAttr->Next();
				m_TempBoneWeight.m_Data[ bIdx ] = static_cast<float>( BoneAttr->DoubleValue() );
				++bIdx;
			}
		}

		c_uint32 Index = GetIndexForTempVertex();
		if( Index == 65536 && !m_Header.m_LongIndices )
		{
			WARNDESC( "Exceeded 65536 indices. Use -l to compile with long indices." );
		}
		m_Indices.PushBack( Index );

		// Cache indices to handle 4-side faces
		TempIndices[ NumVertsInFace++ ] = Index;
	}

	if( NumVertsInFace == 4 )
	{
		m_RawTris.PushBack( Triangle( TempPositions[0], TempPositions[1], TempPositions[2] ) );
		m_RawTris.PushBack( Triangle( TempPositions[0], TempPositions[2], TempPositions[3] ) );
	}
	else
	{
		m_RawTris.PushBack( Triangle( TempPositions[0], TempPositions[1], TempPositions[2] ) );
	}
}

void MeshCompiler::CompileMaterial( TiXmlElement* Material )
{
	TiXmlAttribute* MatAttr = Material->FirstAttribute();

	SimpleString Filename = FileUtil::StripLeadingUpFolders( MatAttr->Value() );
	Filename.Replace( '\\', '/' );
	if( Filename.Contains( "_DXT" ) )
	{
		Filename = Filename.Replace( ".tga", ".dds" );
	}

	m_Materials.PushBack( Filename );
	++m_Header.m_NumMaterials;
}

int MeshCompiler::TouchDependencies( const char* const InFilename, const char* const OutFilename )
{
	// HACKHACK: Assume this is in Intermediate/Anims.
	// StrippedFilename has no leading ../Intermediate/Anims and no extension (e.g., "Actors/meleeattack").
	const SimpleString StrippedFilename =
		FileUtil::Normalize(
			FileUtil::StripExtensions(
				FileUtil::StripLeadingFolder(
					FileUtil::StripLeadingFolder(
						FileUtil::StripLeadingUpFolders( InFilename ),
						"Intermediate" ),
					"Anims" )
				).CStr()
			);

	ConfigManager::Load( FileStream( "tools.cfg", FileStream::EFM_Read ) );

	// We need to get the filenames back out of the keys, so load this system before loading the anim config file
	ReverseHash::Initialize();

	STATICHASH( AnimationConfigFile );
	const SimpleString AnimationConfigFile = ConfigManager::GetArchetypeString( sAnimationConfigFile, ConfigManager::EmptyContext, "", StrippedFilename );
	ConfigManager::Load( FileStream( AnimationConfigFile.CStr(), FileStream::EFM_Read ) );

	// Reverse lookup AnimationMap and AnimSets
	STATICHASH( AnimationMap );
	Array<HashedString> AnimationMapKeys;
	ConfigManager::GetVariableNames( AnimationMapKeys, sAnimationMap );

	FOR_EACH_ARRAY( AnimationMapIter, AnimationMapKeys, HashedString )
	{
		const HashedString&	AnimationMapKey			= AnimationMapIter.GetValue();
		const SimpleString	MeshFilename			= ReverseHash::ReversedHash( AnimationMapKey );	// This is the anim map key, which is relative to Baked folder (e.g. "Meshes/body-base.cms")
		const HashedString	AnimationMapContext		= ConfigManager::GetHash( AnimationMapKey, HashedString::NullString, sAnimationMap );

		bool				IsMeshDependentOnThis	= false;
		STATICHASH( NumAnimSets );
		const uint NumAnimsSets = ConfigManager::GetInt( sNumAnimSets, 0, AnimationMapContext );
		FOR_EACH_INDEX( AnimSetIndex, NumAnimsSets )
		{
			const SimpleString AnimSetFilename = ConfigManager::GetSequenceString( "AnimSet%d", AnimSetIndex, "", AnimationMapContext );

			// HACKHACK: Assume this is also in Intermediate/Anims
			const SimpleString StrippedAnimSetFilename =
				FileUtil::Normalize(
					FileUtil::StripExtensions(
						FileUtil::StripLeadingFolder(
							FileUtil::StripLeadingFolder(
								FileUtil::StripLeadingUpFolders( AnimSetFilename.CStr() ),
								"Intermediate" ),
							"Anims" )
						).CStr()
					);

			if( StrippedAnimSetFilename != StrippedFilename )
			{
				continue;
			}

			// This (StrippedFilename) is one of the anim sets that MeshFilename is dependent on.
			IsMeshDependentOnThis = true;
			break;
		}

		if( IsMeshDependentOnThis )
		{
			// Touch the file so it will get baked after this. I mean, I'm making assumptions
			// here about the order of stuff in bake.py, and this will create a file if it
			// doesn't exist, but whatever, it should work fine.
			const SimpleString	IntermediateMeshFilename	=
				FileUtil::Normalize(
					( SimpleString( "../Intermediate/" ) +
						FileUtil::StripLeadingUpFolders( MeshFilename.CStr() )
						).Replace( ".cms", ".mesh" ).CStr()
				);

			// ShellExecute doesn't work for touch, apparently
			system( SimpleString::PrintF( "touch %s", IntermediateMeshFilename.CStr() ).CStr() );
		}
	}

	ReverseHash::ShutDown();

	// Also touch the out file for a timestamp so we don't redo THIS step
	system( SimpleString::PrintF( "touch %s", OutFilename ).CStr() );

	return 0;
}

int MeshCompiler::Compile( const char* const InFilename, const char* const OutFilename, const bool LongIndices )
{
	ReverseHash::Initialize();

	// HACKHACK: Assume this is in Intermediate/Meshes
	m_StrippedFilename =
		FileUtil::Normalize(
			FileUtil::StripExtensions(
				FileUtil::StripLeadingFolder(
					FileUtil::StripLeadingFolder(
						FileUtil::StripLeadingUpFolders( InFilename ),
						"Intermediate" ),
					"Meshes" )
				).CStr()
			);

	TiXmlDocument XMLDoc;
	XMLDoc.LoadFile( InFilename );
	TiXmlElement* RootElement = XMLDoc.FirstChildElement();	// "mesh"

	// Sanity check
	if( _stricmp( RootElement->Value(), "mesh" ) )
	{
		WARNDESC( "Input file is not a valid XML mesh file." );
		return -1;
	}

	ConfigManager::Load( FileStream( "tools.cfg", FileStream::EFM_Read ) );

	MAKEHASH( m_StrippedFilename );

	STATICHASH( BakeAOForDynamicMeshes );
	m_BakeAOForDynamicMeshes = ConfigManager::GetArchetypeBool( sBakeAOForDynamicMeshes, ConfigManager::EmptyContext, false, sm_StrippedFilename );

	STATICHASH( BakeAOForAnimatedMeshes );
	m_BakeAOForAnimatedMeshes = ConfigManager::GetArchetypeBool( sBakeAOForAnimatedMeshes, ConfigManager::EmptyContext, false, sm_StrippedFilename );

	STATICHASH( TraceTriangleBacks );
	m_TraceTriangleBacks = ConfigManager::GetArchetypeBool( sTraceTriangleBacks, ConfigManager::EmptyContext, false, sm_StrippedFilename );

	STATICHASH( DynamicAORadius );
	m_AORadius = ConfigManager::GetArchetypeFloat( sDynamicAORadius, ConfigManager::EmptyContext, 0.1f, sm_StrippedFilename );

	STATICHASH( DynamicAOPushOut );
	m_AOPushOut = ConfigManager::GetArchetypeFloat( sDynamicAOPushOut, ConfigManager::EmptyContext, 0.01f, sm_StrippedFilename );

	STATICHASH( AnimationConfigFile );
	const SimpleString AnimationConfigFile = ConfigManager::GetArchetypeString( sAnimationConfigFile, ConfigManager::EmptyContext, "", sm_StrippedFilename );
	ConfigManager::Load( FileStream( AnimationConfigFile.CStr(), FileStream::EFM_Read ) );

	STATICHASH( ResourceFilenamePattern );
	const SimpleString ResourceFilenamePattern = ConfigManager::GetArchetypeString( sResourceFilenamePattern, ConfigManager::EmptyContext, "", sm_StrippedFilename );
	const SimpleString ResourceFilename = SimpleString::PrintF( ResourceFilenamePattern.CStr(), m_StrippedFilename.CStr() );

	m_Header.m_LongIndices = LongIndices;

	// Get armature first, which will make it easier to handle bone references in verts
	TiXmlElement* pArm = RootElement->FirstChildElement( "armature" );
	if( pArm )
	{
		CompileArmature( pArm, true );

		// Append additional anim sets
		STATICHASH( AnimationMap );
		MAKEHASH( ResourceFilename );
		const SimpleString AnimationMapName = ConfigManager::GetString( sResourceFilename, "", sAnimationMap );

		STATICHASH( NumAnimSets );
		MAKEHASH( AnimationMapName );
		const uint NumAnimsSets = ConfigManager::GetInt( sNumAnimSets, 0, sAnimationMapName );
		for( uint AnimSetIndex = 0; AnimSetIndex < NumAnimsSets; ++AnimSetIndex )
		{
			const SimpleString AnimSet = ConfigManager::GetSequenceString( "AnimSet%d", AnimSetIndex, "", sAnimationMapName );
#if MESH_COMPILER_VERBOSE
			PRINTF( "Adding anim set %s.\n", AnimSet.CStr() );
#endif

			TiXmlDocument				AnimSetXMLDoc;
			AnimSetXMLDoc.LoadFile( AnimSet.CStr() );

			const TiXmlElement* const	pRootElement		= AnimSetXMLDoc.FirstChildElement( "mesh" );
			ASSERT( pRootElement );

			const TiXmlElement* const	pArmatureElement	= pRootElement->FirstChildElement( "armature" );
			ASSERT( pArmatureElement );

			CompileArmature( pArmatureElement, false );
		}

		FixUpAnimLengths();
	}

	// ROSATODO: Load and compile animations from additional anim sets (load anim config file first!)

	int NumFaces = 0;
	for( TiXmlElement* Face = RootElement->FirstChildElement( "face" ); Face; Face = Face->NextSiblingElement( "face" ) )
	{
		CompileFace( Face );
		NumFaces++;
	}

	for( TiXmlElement* Mat = RootElement->FirstChildElement( "tex" ); Mat; Mat = Mat->NextSiblingElement( "tex" ) )
	{
		CompileMaterial( Mat );
	}

	m_Header.m_NumVertices = m_Positions.Size();
	m_Header.m_NumIndices = m_Indices.Size();

	NormalizeWeights();

	CalculateAABB();

	if( m_Header.m_HasUVs && m_Header.m_HasNormals )
	{
		m_Header.m_HasTangents = true;
	}
#if MESH_COMPILER_VERBOSE
	PRINTF( "Calculating tangents...\n" );
#endif
	CalculateTangents();

#if MESH_COMPILER_VERBOSE
	PRINTF( "Compile successful!\n" );
	PRINTF( "Imported %d faces.\n", NumFaces );
#endif

	Write( FileStream( OutFilename, FileStream::EFM_Write ) );

#if MESH_COMPILER_VERBOSE
	PRINTF( "Exported %d vertices.\n", m_Header.m_NumVertices );
	PRINTF( "Exported %d indices (%d triangles).\n", m_Header.m_NumIndices, m_Header.m_NumIndices / 3 );
	if( m_Header.m_HasSkeleton )
	{
		PRINTF( "Exported %d bones.\n", m_Header.m_NumBones );
		PRINTF( "Exported %d frames.\n", m_Header.m_NumFrames );
		PRINTF( "Exported %d animations.\n", m_Header.m_NumAnims );
	}
#endif

	ReverseHash::ShutDown();

	return 0;
}

bool MeshCompiler::FindExistingVertex( c_uint32& OutIndex ) const
{
	const c_uint32 NumVerts = static_cast<c_uint32>( m_Positions.Size() );

	for( c_uint32 VertexIndex = 0; VertexIndex < NumVerts; ++VertexIndex )
	{
		if( !m_TempPos.Equals(		m_Positions[	VertexIndex ], EPSILON )	||
			!m_TempUV.Equals(		m_UVs[			VertexIndex ], EPSILON )	||
			!m_TempNorm.Equals(		m_Normals[		VertexIndex ], EPSILON )	||
			!m_TempNormB.Equals(	m_NormalsB[		VertexIndex ], EPSILON )	||
			!m_TempColor.Equals(	m_Colors[		VertexIndex ], EPSILON ) )
		{
			continue;
		}

		bool UnmatchedBone = false;
		for( int BoneIndex = 0; !UnmatchedBone && BoneIndex < MAX_BONES; ++BoneIndex )
		{
			if( m_TempBoneIndex.m_Data[		BoneIndex ] != m_BoneIndices[		VertexIndex ].m_Data[ BoneIndex ] ||
				m_TempBoneWeight.m_Data[	BoneIndex ] != m_FloatBoneWeights[	VertexIndex ].m_Data[ BoneIndex ] )
			{
				UnmatchedBone = true;
			}
		}
		if( UnmatchedBone )
		{
			continue;
		}

		// We found a match, return its index
		OutIndex = VertexIndex;
		return true;
	}

	return false;
}

// Finds the vertex and returns its index, adding it to the vectors if necessary (SIDE EFFECT!)
// Don't bother confirming tangent vector; it's a product of pos and norm
c_uint32 MeshCompiler::GetIndexForTempVertex()
{
	c_uint32 ExistingVertexIndex;
	if( FindExistingVertex( ExistingVertexIndex ) )
	{
		return ExistingVertexIndex;
	}

	// Create a new vertex and return its index
	const c_uint32 NewIndex = static_cast<c_uint32>( m_Positions.Size() );
	m_Positions.PushBack( m_TempPos );
	m_Colors.PushBack( m_TempColor );
	m_UVs.PushBack( m_TempUV );
	m_Normals.PushBack( m_TempNorm );
	m_NormalsB.PushBack( m_TempNormB );
	m_Tangents.PushBack( Vector4() );	// The tangent will actually be computed later, if UVs and normals are used
	m_BoneIndices.PushBack( m_TempBoneIndex );
	m_FloatBoneWeights.PushBack( m_TempBoneWeight );
	m_ByteBoneWeights.PushBack( SBoneData() );

	return NewIndex;
}

void MeshCompiler::CalculateAABB()
{
	m_AABB = CalculateAABBForBasePose();

	if( m_Header.m_HasSkeleton )
	{
		for( uint Frame = 0; Frame < m_Header.m_NumFrames; ++Frame )
		{
			m_AABB.ExpandTo( CalculateAABBForFrame( Frame ) );
		}
	}
}

AABB MeshCompiler::CalculateAABBForBasePose()
{
	ASSERT( m_Positions.Size() > 0 );

	AABB RetVal = AABB( m_Positions[0] );

	for( uint VertexIndex = 1; VertexIndex < m_Positions.Size(); ++VertexIndex )
	{
		RetVal.ExpandTo( m_Positions[ VertexIndex ] );
	}

	return RetVal;
}

AABB MeshCompiler::CalculateAABBForFrame( const uint Frame )
{
	ASSERT( m_Positions.Size() > 0 );

	AABB RetVal = AABB( ApplyBonesToPosition( Frame, 0 ) );

	for( uint VertexIndex = 1; VertexIndex < m_Positions.Size(); ++VertexIndex )
	{
		RetVal.ExpandTo( ApplyBonesToPosition( Frame, VertexIndex ) );
	}

	return RetVal;
}

Matrix MeshCompiler::GetBoneMatrix( const c_int32 BoneIndex, const uint Frame, const bool InvBindPose ) const
{
	const SNamedBone&	NamedBone			= m_Bones[ BoneIndex ];
	const Vector&		BoneTranslation		= NamedBone.m_FrameTransVecs[ Frame ];
	const Quat&			BoneOrientation		= NamedBone.m_FrameQuats[ Frame ];

	Matrix				BoneMatrix			= BoneOrientation.ToMatrix();
	BoneMatrix.SetTranslationElements( BoneTranslation );

	if( NamedBone.m_ParentIndex >= 0 )
	{
		const Matrix& ParentBoneMatrix	= GetBoneMatrix( NamedBone.m_ParentIndex, Frame, false );	// Use the transform *without* the inverse bind pose for children
		BoneMatrix						= BoneMatrix * ParentBoneMatrix;
	}

	if( InvBindPose )
	{
		BoneMatrix = NamedBone.m_InvBindPose * BoneMatrix;
	}

	return BoneMatrix;
}

Vector MeshCompiler::ApplyBonesToPosition( const uint Frame, const uint VertexIndex ) const
{
	const Vector&		Position			= m_Positions[ VertexIndex ];
	const SBoneData&	Indices				= m_BoneIndices[ VertexIndex ];
	const SBoneWeights&	Weights				= m_FloatBoneWeights[ VertexIndex ];

	Vector				RetVal;

	for( uint Bone = 0; Bone < MAX_BONES; ++Bone )
	{
		const c_uint8		BoneIndex			= Indices.m_Data[ Bone ];
		const float			BoneWeight			= Weights.m_Data[ Bone ];

		Matrix				BoneMatrix			= GetBoneMatrix( BoneIndex, Frame, true );
		const Vector		TransformedPoint	= Position * BoneMatrix;

		RetVal									+= TransformedPoint * BoneWeight;
	}

	return RetVal;
}

// Based on the example at http://www.terathon.com/code/tangent.php
void MeshCompiler::CalculateTangents()
{
	// Notes:
	// Bitangent = (Normal x Tangent) * Bitangent orientation
	// Tangent is the tangent-space vector along the +U axis (right)
	// Bitangent is the tangent-space vector along the +V axis (up)

	// Use Gram-Schmidt to orthonormalize the vector

	// Write the handedness (direction of bitangent) in w

	Vector* tan1 = new Vector[ m_Header.m_NumVertices * 2];
	Vector* tan2 = tan1 + m_Header.m_NumVertices;
	memset( tan1, 0, m_Header.m_NumVertices * 2 * sizeof( Vector ) );

	for( c_uint32 i = 0; i < m_Header.m_NumIndices; i += 3 )
	{
		const uint		i1		= m_Indices[ i ];
		const uint		i2		= m_Indices[ i + 1 ];
		const uint		i3		= m_Indices[ i + 2 ];

		const Vector&	v1		= m_Positions[ i1 ];
		const Vector&	v2		= m_Positions[ i2 ];
		const Vector&	v3		= m_Positions[ i3 ];

		const Vector2&	w1		= m_UVs[ i1 ];
		const Vector2&	w2		= m_UVs[ i2 ];
		const Vector2&	w3		= m_UVs[ i3 ];

		const float		x1		= v2.x - v1.x;
		const float		x2		= v3.x - v1.x;
		const float		y1		= v2.y - v1.y;
		const float		y2		= v3.y - v1.y;
		const float		z1		= v2.z - v1.z;
		const float		z2		= v3.z - v1.z;

		const float		s1		= w2.x - w1.x;
		const float		s2		= w3.x - w1.x;
		const float		t1		= w2.y - w1.y;
		const float		t2		= w3.y - w1.y;

		const float		r		= 1.0f / ( s1 * t2 - s2 * t1 );
		const Vector	sdir	= r * Vector( ( t2 * x1 - t1 * x2 ), ( t2 * y1 - t1 * y2 ), ( t2 * z1 - t1 * z2 ) );
		const Vector	tdir	= r * Vector( ( s1 * x2 - s2 * x1 ), ( s1 * y2 - s2 * y1 ), ( s1 * z2 - s2 * z1 ) );

		tan1[ i1 ] += sdir;
		tan1[ i2 ] += sdir;
		tan1[ i3 ] += sdir;

		tan2[ i1 ] += tdir;
		tan2[ i2 ] += tdir;
		tan2[ i3 ] += tdir;
	}

	FOR_EACH_INDEX( VertIndex, m_Header.m_NumVertices )
	{
		// ZETA: Bent normals are now in NormalsB, so always use main normals
		const Vector& Normal	= m_Normals[ VertIndex ];
		//ASSERT( Normal.LengthSquared() != 0.0f );

		const Vector& Tangent	= tan1[ VertIndex ];
		//ASSERT( Tangent.LengthSquared() != 0.0f );

		const Vector& TangentB	= tan2[ VertIndex ];
		//ASSERT( TangentB.LengthSquared() != 0.0f );

		// Gram-Schmidt orthogonalize
		m_Tangents[ VertIndex ]	= ( Tangent - ( Normal * Normal.Dot( Tangent ) ) ).GetNormalized();

		// Calculate handedness
		const Vector	Bitangent		= Normal.Cross( Tangent );
		const float		BitangentDot	= Bitangent.Dot( TangentB );
		//ASSERT( BitangentDot != 0.0f );	// Bitangent shouldn't be perpendicular to the second tangent, that would be a problem
		m_Tangents[ VertIndex ].w		= NonZeroSign( BitangentDot );
	}

	SafeDeleteArray( tan1 );
}

c_uint8 MeshCompiler::GetIndexForBone( const HashedString& Name )
{
	for( c_uint8 i = 0; i < m_Header.m_NumBones; ++i )
	{
		if( Name == m_Bones[i].m_Name )
		{
			return i;
		}
	}

	PRINTF( "Name %s refers to non-existent bone.", ReverseHash::ReversedHash( Name ).CStr() );
	WARNDESC( "Name refers to non-existent bone." );
	return 0;
}

void MeshCompiler::NormalizeWeights()
{
	// Make weights sum to 1.0 (or 255, actually)
	for( c_uint32 VertexIndex = 0; VertexIndex < m_Header.m_NumVertices; ++VertexIndex )
	{
		SBoneWeights&	FloatBoneWeights	= m_FloatBoneWeights[ VertexIndex ];
		SBoneData&		ByteBoneWeights		= m_ByteBoneWeights[ VertexIndex ];

		float WeightSum = 0.0f;
		for( uint BoneIndex = 0; BoneIndex < MAX_BONES; ++BoneIndex )
		{
			WeightSum += FloatBoneWeights.m_Data[ BoneIndex ];
		}

		for( uint BoneIndex = 0; BoneIndex < MAX_BONES; ++BoneIndex )
		{
			float&		FloatBoneWeight	= FloatBoneWeights.m_Data[ BoneIndex ];
			c_uint8&	ByteBoneWeight	= ByteBoneWeights.m_Data[ BoneIndex ];

			FloatBoneWeight	/= WeightSum;
			ByteBoneWeight	= static_cast<c_uint8>( 255.0f * FloatBoneWeight );
		}
	}
}

void MeshCompiler::Write( const IDataStream& Stream )
{
#if MESH_COMPILER_VERBOSE
	PRINTF( "Writing mesh...\n" );
#endif

	// Write header
	Stream.Write<SCompiledMeshHeader>( m_Header );

	// Write positions
	Stream.Write( sizeof( Vector ) * m_Header.m_NumVertices, m_Positions.GetData() );

	// Write colors
	if( m_Header.m_HasColors )
	{
		Stream.Write( sizeof( Vector4 ) * m_Header.m_NumVertices, m_Colors.GetData() );
	}

	// Write UVs
	if( m_Header.m_HasUVs )
	{
		Stream.Write( sizeof( Vector2 ) * m_Header.m_NumVertices, m_UVs.GetData() );
	}

	// Write normals
	if( m_Header.m_HasNormals )
	{
		Stream.Write( sizeof( Vector ) * m_Header.m_NumVertices, m_Normals.GetData() );
	}

	// Write secondary normals
	if( m_Header.m_HasNormalsB )
	{
		Stream.Write( sizeof( Vector ) * m_Header.m_NumVertices, m_NormalsB.GetData() );
	}

	// Write tangents
	if( m_Header.m_HasTangents )
	{
		Stream.Write( sizeof( Vector4 ) * m_Header.m_NumVertices, m_Tangents.GetData() );
	}

	// Normalize and write weights
	if( m_Header.m_HasSkeleton )
	{
		Stream.Write( sizeof( SBoneData ) * m_Header.m_NumVertices, m_BoneIndices.GetData() );
		Stream.Write( sizeof( SBoneData ) * m_Header.m_NumVertices, m_ByteBoneWeights.GetData() );
	}

	// Write indices
	if( m_Header.m_LongIndices )
	{
		Stream.Write( sizeof( c_uint32 ) * m_Header.m_NumIndices, m_Indices.GetData() );
	}
	else
	{
		for( uint i = 0; i < m_Header.m_NumIndices; ++i )
		{
			Stream.WriteUInt16( static_cast<c_uint16>( m_Indices[i] ) );
		}
	}

	if( m_Header.m_HasSkeleton )
	{
		ASSERT( m_Header.m_NumBones );
		for( uint i = 0; i < m_Header.m_NumBones; ++i )
		{
			Stream.WriteHashedString( m_Bones[i].m_Name );
			Stream.WriteInt32( m_Bones[i].m_ParentIndex );
			Stream.WriteFloat( m_Bones[i].m_Length );
			Stream.Write<Matrix>( m_Bones[i].m_InvBindPose );
			Stream.Write<Matrix>( m_Bones[i].m_InvBindPose.GetInverse() );
			Stream.Write<Quat>( m_Bones[i].m_InvBindPose.ToQuaternion() );
			Stream.Write<Quat>( m_Bones[i].m_InvBindPose.GetInverse().ToQuaternion() );
			Stream.Write<Vector>( m_Bones[i].m_BoneStart );
			Stream.Write<Vector>( m_Bones[i].m_BoneEnd );
			Stream.Write<Quat>( m_Bones[i].m_Orientation );
		}

		for( uint i = 0; i < m_Header.m_NumFrames; ++i )
		{
			for( uint j = 0; j < m_Header.m_NumBones; ++j )
			{
				Stream.Write<Quat>( m_Bones[j].m_FrameQuats[i] );
				Stream.Write<Vector>( m_Bones[j].m_FrameTransVecs[i] );
			}
		}

		if( m_Header.m_NumAnims )
		{
			Stream.Write( sizeof( SAnimData ) * m_Header.m_NumAnims, m_AnimData.GetData() );
		}
	}

	// Materials are a bit different than other data, in that they
	// need to be interpreted (for material type and to load the
	// specified file) on the engine instead of simply loaded.
	FOR_EACH_ARRAY( MaterialIter, m_Materials, SimpleString )
	{
		Stream.WriteString( MaterialIter.GetValue() );
	}

	Stream.Write<AABB>( m_AABB );

#if MESH_COMPILER_VERBOSE
	PRINTF( "%d bytes written.\n", Stream.GetPos() );
#endif
}
