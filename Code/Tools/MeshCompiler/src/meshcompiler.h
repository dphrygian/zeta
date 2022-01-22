#ifndef MESHCOMPILER_H
#define MESHCOMPILER_H

#include "vector2.h"
#include "vector4.h"
#include "vector.h"
#include "matrix.h"
#include "quat.h"
#include "meshfactory.h"
#include "hashedstring.h"
#include "3d.h"
#include "array.h"
#include "aabb.h"
#include "triangle.h"
#include "animation.h"
#include "simplestring.h"

class Segment;
class Animation;
class IDataStream;

class TiXmlElement;

struct SNamedBone
{
	// Mirror of SBoneInfo
	HashedString	m_Name;
	c_int32			m_ParentIndex;		// Index of parent bone, -1 if root/unparented
	float			m_Length;
	Matrix			m_InvBindPose;
	Vector			m_BoneStart;		// local space position in bind pose
	Vector			m_BoneEnd;			// local space position in bind pose
	Quat			m_Orientation;		// local space orientation in bind pose

	// Additional properties
	Array<Quat>		m_FrameQuats;		// For rotation
	Array<Vector>	m_FrameTransVecs;	// For translation
};

class MeshCompiler
{
public:
	MeshCompiler() {}
	~MeshCompiler();
	int Compile( const char* const InFilename, const char* const OutFilename, const bool LongIndices );
	int TouchDependencies( const char* const InFilename, const char* const OutFilename );

private:
	c_uint32	GetIndexForTempVertex();
	bool		FindExistingVertex( c_uint32& OutIndex ) const;
	void		CalculateTangents();
	c_uint8		GetIndexForBone( const HashedString& Name );
	void		NormalizeWeights();
	void		Write( const IDataStream& Stream );
	void		CompileArmature( const TiXmlElement* const pArm, const bool BaseArmature );
	void		CompileAnimations( const TiXmlElement* const pArm, const uint FrameOffset );
	void		FixUpAnimLengths();
	void		CompileFace( TiXmlElement* Face );
	void		CompileMaterial( TiXmlElement* Material );
	void		CalculateAABB();
	AABB		CalculateAABBForBasePose();
	AABB		CalculateAABBForFrame( const uint Frame );
	Vector		ApplyBonesToPosition( const uint Frame, const uint VertexIndex ) const;
	Matrix		GetBoneMatrix( const c_int32 BoneIndex, const uint Frame, const bool InvBindPose ) const;

	struct SBoneWeights
	{
		float m_Data[ MAX_BONES ];
	};

	SCompiledMeshHeader	m_Header;
	Array<Vector>		m_Positions;
	Array<Vector4>		m_Colors;
	Array<Vector2>		m_UVs;
	Array<Vector>		m_Normals;
	Array<Vector>		m_NormalsB;
	Array<Vector4>		m_Tangents;
	Array<c_uint32>		m_Indices;
	Array<SBoneData>	m_BoneIndices;	// Into m_Bones array
	Array<SBoneData>	m_ByteBoneWeights;
	Array<SBoneWeights>	m_FloatBoneWeights;
	Array<SNamedBone>	m_Bones;
	Array<SAnimData>	m_AnimData;
	Array<SimpleString>	m_Materials;
	Array<Triangle>		m_RawTris;

	Vector				m_TempPos;
	Vector2				m_TempUV;
	Vector				m_TempNorm;
	Vector				m_TempNormB;
	Vector4				m_TempColor;
	SBoneData			m_TempBoneIndex;
	SBoneWeights		m_TempBoneWeight;

	AABB				m_AABB;

	SimpleString		m_StrippedFilename;	// Stripped of leading folders and extension(s)

	// Cached config
	float				m_AORadius;
	float				m_AOPushOut;
	bool				m_BakeAOForDynamicMeshes;
	bool				m_BakeAOForAnimatedMeshes;
	bool				m_TraceTriangleBacks;
};

#endif // MESHCOMPILER_H
