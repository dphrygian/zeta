#ifndef BONEARRAY_H
#define BONEARRAY_H

// All this animation stuff was poorly named, but BoneArray
// contains all the static animation data for a particular
// rig (or armature, in Blender's terms). Non-static data
// (i.e., current state) is in AnimationState.

#include "vector.h"
#include "quat.h"
#include "matrix.h"
#include "array.h"

struct SAnimData;
class Animation;

#define INVALID_INDEX	-1
#define FRAME_TPOSE		0

struct SBoneInfo
{
	HashedString	m_Name;
	c_int32			m_ParentIndex;		// -1 if root
	float			m_Length;
	Matrix			m_InvBindPose;
	Matrix			m_BindPose;			// keeping this around helps with modifiers
	Quat			m_InvBindPoseQuat;	// keeping this around helps with modifiers
	Quat			m_BindPoseQuat;		// keeping this around helps with modifiers
	Vector			m_BoneStart;		// local space position in bind pose
	Vector			m_BoneEnd;			// local space position in bind pose
	Quat			m_Orientation;		// local space orientation in bind pose
};

// Transform relative to parent bone space, applied to inverse bind pose to produce animated pose
struct SBone
{
	Quat	m_Quat;
	Vector	m_Vector;
};

// 1D array, laid out like a 2D array, all bones by all frames (frame-major)
class BoneArray
{
public:
	BoneArray();
	~BoneArray();

	int				AddReference();
	int				Release();

	void			Init( const SBoneInfo* const pBoneInfos, const SBone* const pBones, const char* const MeshFilename, const SAnimData* const pAnimData, const int NumFrames, const int NumBones, const int NumAnims );
	int				GetBoneIndex( const HashedString& Name ) const;	// Returns the bone index, or INVALID_INDEX if not found

	// Frame 0 should be bine pose/T-pose, and frame numbers should match Blender
	const SBone&	GetBone( const int Index, const int Frame = FRAME_TPOSE ) const
	{
		DEVASSERT( Index >= 0 && Index < m_NumBones );
		DEVASSERT( Frame >= 0 && Frame < m_NumFrames );
		DEVASSERT( m_Bones.GetData() );

		return m_Bones[ ( Frame * m_NumBones ) + Index ];
	}

	inline void		GetInterpolatedBone( const int Index, const int Frame, const int NextFrame, const float Delta, SBone& OutInterpolatedBone ) const
	{
		DEVASSERT( Index >= 0 && Index < m_NumBones );
		DEVASSERT( Frame >= 0 && Frame < m_NumFrames );
		DEVASSERT( NextFrame >= 0 && NextFrame < m_NumFrames );
		DEVASSERT( Delta >= 0.f && Delta <= 1.f );
		DEVASSERT( m_Bones.GetData() );

		const SBone& Bone1 = m_Bones[ ( m_NumBones * Frame ) + Index ];
		const SBone& Bone2 = m_Bones[ ( m_NumBones * NextFrame ) + Index ];

		GetInterpolatedBone( Bone1, Bone2, Delta, OutInterpolatedBone );
	}

	static inline void	GetInterpolatedBone( const SBone& Bone1, const SBone& Bone2, const float Delta, SBone& OutInterpolatedBone )
	{
		OutInterpolatedBone.m_Quat		= Bone1.m_Quat.SLERP( Delta, Bone2.m_Quat );
		OutInterpolatedBone.m_Vector	= Bone1.m_Vector.LERP( Delta, Bone2.m_Vector );
	}

	static inline void	GetAdditiveBone( const SBone& PoseBone, const SBone& RestPoseBone, const SBone& AdditiveBone, SBone& OutBone )
	{
		const Quat		AdditiveRot	= AdditiveBone.m_Quat * RestPoseBone.m_Quat.GetInverse();
		const Vector	AdditivePos	= AdditiveBone.m_Vector - RestPoseBone.m_Vector;

		OutBone.m_Quat				= AdditiveRot * PoseBone.m_Quat; // Quaternion product order continues to confuse me
		OutBone.m_Vector			= AdditivePos + PoseBone.m_Vector;
	}

	int					GetNumFrames() const;
	int					GetNumBones() const;
	int					GetNumAnimations() const;

	const SBoneInfo&	GetBoneInfo( const uint Index ) const { return m_BoneInfos[ Index ]; }

	Animation*			GetAnimation( int Index ) const;
	Animation*			GetAnimation( const HashedString& Name ) const;
	int					GetAnimationIndex( const Animation* pAnimation ) const;	// Returns INVALID_INDEX if not found

private:
	int					m_RefCount;

	int					m_NumFrames;
	int					m_NumBones;
	int					m_NumAnimations;

	Array<SBoneInfo>	m_BoneInfos;	// size = NumBones
	Array<SBone>		m_Bones;		// size = NumBones * NumFrames
	Array<Animation>	m_Animations;	// size = NumAnimations
};

#endif // BONEARRAY_H
