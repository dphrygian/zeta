#ifndef MESH_H
#define MESH_H

#include "material.h"
#include "vector.h"
#include "angles.h"
#include "matrix.h"
#include "3d.h"
#include "aabb.h"
#include "simplestring.h"
#include "vector4.h"
#include "animationstate.h"
#include "array.h"
#include "map.h"
#include "set.h"

class BoneArray;
class IVertexBuffer;
class IIndexBuffer;
class IVertexDeclaration;
class ITexture;
class IBoneModifier;

// This is a very struct-y class, it's all
// public--and I hate classes that just have
// Get*/Set* functions on EVERYTHING, so I
// guess it's okay for now.

class Mesh
{
public:
	// Serves as default constructor
	Mesh(	IVertexBuffer* pVertexBuffer = NULL,
			IVertexDeclaration* pVertexDeclaration = NULL,
			IIndexBuffer* pIndexBuffer = NULL,
			BoneArray* pBones = NULL );
	virtual ~Mesh();

	void			Initialize( IVertexBuffer* pVertexBuffer, IVertexDeclaration* pVertexDeclaration, IIndexBuffer* pIndexBuffer, BoneArray* pBones );

	void			SetVertexDeclaration( IVertexDeclaration* const pVertexDeclaration );

	Matrix			GetConcatenatedTransforms();

	ITexture*		GetTexture( unsigned int Stage ) const;
	void			SetTexture( unsigned int Stage, ITexture* const Texture );

	bool			SupportsTexture( const uint Stage ) const;
	bool			SupportsAlphaBlend() const;

	IShaderProgram*	GetShaderProgram() const;

	const Material&	GetMaterial() const { return m_Material; }
	void			SetMaterialDefinition( const SimpleString& DefinitionName, IRenderer* const pRenderer );

	bool			HasMultiPassMaterials() const { return m_MultiPassMaterials.Size() > 0; }
	const Material&	GetMultiPassMaterial( const uint Index ) const { return m_MultiPassMaterials[ Index ]; }
	Material&		GetMultiPassMaterial( const uint Index ) { return m_MultiPassMaterials[ Index ]; }
	void			ClearMultiPassMaterials() { m_MultiPassMaterials.Clear(); }
	void			AddMultiPassMaterialDefinition( const SimpleString& DefinitionName, IRenderer* const pRenderer );
	void			SetAllowMaterialOverrides( const bool AllowMaterialOverrides ) { m_AllowMaterialOverrides = AllowMaterialOverrides; }
	bool			GetAllowMaterialOverrides() const { return m_AllowMaterialOverrides; }

	uint			GetMaterialFlags() const;
	void			SetMaterialFlags( unsigned int Flags, unsigned int Mask = MAT_ALL );
	void			SetMaterialFlag( unsigned int Flag, bool Set );

	const HashedString&	GetBucket() const { return m_Material.GetBucket(); }
	void				SetBucket( const HashedString& PrescribedBucket ) { m_Material.SetBucket( PrescribedBucket ); }

	bool			IsAnimationOwner() const { return IsAnimated() && m_AnimationState->GetOwnerMesh() == this; }
	bool			IsAnimated() const { DEBUGASSERT( !m_Bones || m_AnimationState ); return ( m_Bones != NULL ); }
	void			Tick( const float DeltaTime );
	void			CopyAnimationsFrom( Mesh* const pMesh );
	void			SuppressAnimEvents( const bool Suppress );
	void			PlayAnimation( const HashedString& AnimationName, AnimationState::SPlayAnimationParams& PlayParams );
	void			SetAnimationBlend( const HashedString& AnimationNameA, const HashedString& AnimationNameB, const float BlendAlpha );
	void			SetAnimation( int AnimationIndex, AnimationState::SPlayAnimationParams& PlayParams );
	Animation*		GetAnimation( const SimpleString& Name ) const;
	void			AddAnimationListener( const SAnimationListener& AnimationListener );
	void			RemoveAnimationListener( const SAnimationListener& AnimationListener );
	void			UpdateBones();	// This only applies updates if the matrices have been marked dirty, so it can be called as often as needed
	bool			AreBonesUpdated() const { DEBUGASSERT( IsAnimated() ); return m_AnimationState->AreBonesUpdated(); }
	void			AddBoneModifier( IBoneModifier* pBoneModifier );
	void			GetAnimationVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity );
	int				GetBoneIndex( const HashedString& BoneName ) const;

	const BoneArray*		GetBones() const			{ return m_Bones; }
	BoneArray*				GetBones()					{ return m_Bones; }

	const AnimationState*	GetAnimationState() const	{ return m_AnimationState; }
	AnimationState*			GetAnimationState()			{ return m_AnimationState; }

	const Matrix*			GetBoneMatrices() const		{ DEBUGASSERT( IsAnimated() ); return m_AnimationState->GetBoneMatrices(); }

	// Accessors used to serialize animation state
	int				GetAnimationIndex() const;
	float			GetAnimationTime() const;
	void			SetAnimationTime( const float AnimationTime );
	float			GetAnimationPlayRate() const;
	void			SetAnimationPlayRate( const float AnimationPlayRate );
	void			SetAnimationBonesTickRate( const float AnimationBonesTickRate );
	AnimationState::EAnimationEndBehavior GetAnimationEndBehavior() const;

	// Get the location to use when sorting alpha list
	virtual Vector	GetSortLocation();

	uint			GetNumVertices() const;
	void			SetNumVertices( const uint NumVertices );
	uint			GetNumIndices() const;
	void			SetNumIndices( const uint NumIndices );

	const AABB&		GetAABB() const { return m_AABB; }
	void			SetAABB( const AABB& Bounds );
	void			RecomputeAABB();

	const Vector4&	GetShaderConstant( const HashedString& Name ) const;
	void			SetShaderConstant( const HashedString& Name, const Vector4& Value );

// ROSATODO: Make private
public:
	IVertexBuffer*		m_VertexBuffer;
	IVertexDeclaration*	m_VertexDeclaration;
	IIndexBuffer*		m_IndexBuffer;
	BoneArray*			m_Bones;			// Static animation data (i.e., the rig or armature)

	Material		m_Material;					// NOTE: Always used for sorting into buckets, even when we have multiple passes
	Array<Material>	m_MultiPassMaterials;		// Only used for multiple passes so we don't pay the array pointer dereference in the common case
	bool			m_AllowMaterialOverrides;	// Usually true; can be set false in special cases where we already have the overridden material (merged static shadow geo meshes, for example)

	Vector			m_Location;
	Vector			m_Scale;
	Angles			m_Rotation;

	// ROSANOTE: Crude optimization for things that don't move; avoid redundant matrix multiplies in GetConcatenatedTransforms.
	Vector			m_CACHED_Location;
	Vector			m_CACHED_Scale;
	Angles			m_CACHED_Rotation;
	Matrix			m_PreconcatenatedTransform;

	AABB			m_AABB;
	AABB			m_OriginalAABB;	// Stored so we can recompute the bounds for a moving mesh from this. Only required when RecomputeAABB is used.

	AnimationState*	m_AnimationState;

	Vector4			m_ConstantColor;	// Mainly used for UI multiply color
	Map<HashedString, Vector4> m_ShaderConstants;	// Instead of continuing to add hacky extensions like m_ConstantColorB, let meshes store arbitrary things for SDPs to query

	Set<Mesh*>		m_IgnoredMeshes;	// HACKHACK for shadow casting lights

#if BUILD_DEV
	bool			m_DEV_IsDebugMesh;	// For automatically deleting debug lines and boxes
#endif

#if BUILD_DEBUG
	SimpleString	m_DEBUG_Name;
#endif
};

#endif // MESH_H
