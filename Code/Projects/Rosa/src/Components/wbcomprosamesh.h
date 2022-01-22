#ifndef WBCOMPROSAMESH_H
#define WBCOMPROSAMESH_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "angles.h"
#include "aabb.h"
#include "simplestring.h"
#include "array.h"
#include "bonearray.h"
#include "matrix.h"
#include "vector4.h"
#include "interpolator.h"
#include "rosamesh.h"
#include "segment.h"
#include "set.h"

class WBCompRosaMesh : public WBRosaComponent
{
public:
	WBCompRosaMesh();
	virtual ~WBCompRosaMesh();

	DEFINE_WBCOMP( RosaMesh, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Should tick after transform

	virtual void	HandleEvent( const WBEvent& Event );

	virtual bool	IsRenderable() { return true; }
	virtual void	Render();

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	RosaMesh*		GetMesh() const { return m_Mesh; }
	void			SetMeshOffset( const Vector& Offset ) { m_Offset = Offset; }
	const Vector	GetMeshOffset() const { return m_Offset + m_TransientOffset; }
	void			SetMeshScale( const Vector& Scale );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	static void		NotifyAnimationFinished( void* pVoid, class Mesh* pMesh, class Animation* pAnimation, bool Interrupted );
	void			OnAnimationFinished( class Mesh* pMesh, class Animation* pAnimation, bool Interrupted );

	bool			IsHidden() const { return m_Hidden; }

	// For pseudo root motion hack from Couriers
	float			GetAnimationVelocityScalar() const;
	void			GetAnimationVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity );

	int				GetBoneIndex( const HashedString& BoneName ) const;
	Vector			GetBoneLocation( const int BoneIndex ) const;
	void			GetBoneTransform( const int BoneIndex, Vector& OutTranslation, Angles& OutOrientation ) const;
	Segment			GetBoneSegment( const int BoneIndex ) const;
	int				GetNearestBone( const Vector& Location ) const;
	HashedString	GetNearestBoneName( const Vector& Location ) const;
	HashedString	GetBoneName( const int BoneIndex ) const;

	const BoneArray*	GetBones() const	{ return m_Mesh->GetBones(); }
	BoneArray*			GetBones()			{ return m_Mesh->GetBones(); }

	const Vector4&	GetHighlight() const { return m_CurrentHighlight.GetValue(); }
	Vector4			GetExposureRelativeHighlight() const;

	// HACKHACK to make sure shadow casting meshes outside visible sectors
	static const Set<Mesh*>& GetAllShadowLightMeshes();

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	struct SAttachedMesh
	{
		SAttachedMesh()
		:	m_Mesh( NULL )
		,	m_Tag()
		,	m_Hidden( false )
		,	m_AcceptsDecals( false )
		,	m_CastsShadows( false )
		,	m_CastsSelfShadows( false )
		,	m_OwnBounds( false )
		,	m_IsSkinned( false )
		,	m_Bone()
		,	m_BoneIndex( INVALID_INDEX )
		,	m_AbsoluteOffset( false )
		,	m_TranslationOffset()
		,	m_OrientationOffset()
		,	m_OrientationOffsetMatrix()
		,	m_AlbedoMapOverride()
		,	m_NormalMapOverride()
		,	m_SpecMapOverride()
		,	m_MaterialOverride()
		,	m_IsLight( false )
		,	m_IsShadowLight( false )
		,	m_IsFogLight( false )
		,	m_HasAddedIgnoredMeshes( false )
		,	m_IsAntiLight( false )
		,	m_IsFogMesh( false )
		,	m_LightCubeMask( 0 )
		{
		}

		RosaMesh*			m_Mesh;
		HashedString		m_Tag;
		bool				m_Hidden;					// Config/serialized
		bool				m_AcceptsDecals;			// Config
		bool				m_CastsShadows;				// Config
		bool				m_CastsSelfShadows;			// Config
		bool				m_OwnBounds;				// Config, uses own bounds instead of base mesh bounds (e.g. for large attachments like lights)
		bool				m_IsSkinned;				// Config, currently only used during init
		HashedString		m_Bone;						// Config/serialized
		int					m_BoneIndex;				// Transient, from m_Bone
		bool				m_AbsoluteOffset;			// Config; if true, offset is not scaled by base mesh scale
		Vector				m_TranslationOffset;		// Config/serialized
		Angles				m_OrientationOffset;		// Config/serialized
		Matrix				m_OrientationOffsetMatrix;	// Transient, from m_OrientationOffset; cached because it rarely changes (for now) and costs 6 trig functions to create
		SimpleString		m_AlbedoMapOverride;		// Config
		SimpleString		m_NormalMapOverride;		// Config
		SimpleString		m_SpecMapOverride;			// Config
		SimpleString		m_MaterialOverride;			// Config; may come from costume
		bool				m_IsLight;					// Config; convenience value to set up light multipass materials
		bool				m_IsShadowLight;			// Config; implies m_IsLight
		bool				m_IsFogLight;				// Config; DOES not imply m_IsLight ("emissive fog" might be a more accurate name)
		bool				m_HasAddedIgnoredMeshes;	// Transient
		bool				m_IsAntiLight;				// Config; convenience value to set up anti-light multipass materials
		bool				m_IsFogMesh;				// Config
		uint				m_LightCubeMask;			// Config
	};

	void			InitializeFromCostume();

	void			AddAttachmentSetsFromDefinition( const SimpleString& DefinitionName );
	void			AddAttachmentSetFromDefinition( const SimpleString& AttachmentSetDefinitionName, const bool FromSerialization );

	void			SetSendUpdatedEvent();

	void			SetMesh( const SimpleString& Mesh );
	void			SetAlbedoMap( const SimpleString& AlbedoMap );
	void			SetNormalMap( const SimpleString& NormalMap );
	void			SetSpecMap( const SimpleString& SpecMap );

	void			SetAttachmentBone( SAttachedMesh& AttachedMesh, const HashedString& Bone );
	void			SetAttachmentOrientationOffset( SAttachedMesh& AttachedMesh, const Angles& OrientationOffset );
	void			SetAttachmentMesh( SAttachedMesh& AttachedMesh, const SimpleString& AttachedMeshFilename );
	void			RemoveAttachments();

	void			UpdateMesh( const float DeltaTime );
	bool			UpdateMeshTransform();
	void			UpdateAttachedMeshes();
	void			RenderMesh( RosaMesh* const pMesh ) const;

	bool			IsWithinCullDistance() const;
	bool			IntersectsAnyVisibleSector() const;
	bool			IsMeshVisibleFromAnyVisibleSector( const RosaMesh* const pMesh ) const;

	void			SetMeshAttachmentsHidden( const HashedString& Tag, const bool Hidden );
	void			SetMeshAttachmentTransform( const HashedString& Tag, const HashedString& Bone, const Vector& TranslationOffset, const Angles& OrientationOffset );

	void			PlayAnimation( const HashedString& AnimationName, const bool Loop, const bool IgnoreIfAlreadyPlaying, const float PlayRate, const float BlendTime, const bool Layered, const bool Additive ) const;
	void			SetAnimationBlend( const HashedString& AnimationNameA, const HashedString& AnimationNameB, const float BlendAlpha ) const;
	void			CopyAnimationsFrom( WBEntity* const pSourceEntity ) const;

	float			GetLightImportanceScalar( const Mesh* const pMesh ) const;
	float			GetAntiLightImportanceScalar( const Mesh* const pMesh ) const;

	RosaMesh*			m_Mesh;
	AABB				m_OriginalMeshAABB;	// Transient, stored in case we change the mesh's "original" and need to restore it
	bool				m_Hidden;			// Config/serialized; the entire mesh and all attachments are hidden
	bool				m_BaseHidden;		// Config/serialized; only the base is hidden, but attachments will render
	bool				m_AcceptsDecals;	// Config
	bool				m_CastsShadows;		// Config
	bool				m_CastsSelfShadows;	// Config
	Vector				m_Offset;			// Config/serialized
	Vector				m_TransientOffset;	// Transient (HACKHACK for AI step-up interp)

	bool				m_UseOverrideBound;
	AABB				m_OverrideBound;

	float				m_DefaultAnimBlendTime;	// Config, used if BlendTime is 0 (HACKHACK: to force a blend time of 0, use a negative value!)

	// Optimization, cache components which are dependent on the updated event
	Array<WBComponent*>	m_TransformDependentComponents;		// Transient
	Array<WBComponent*>	m_AnimationDependentComponents;		// Transient

	// Optimization, avoid recalculating matrices for AABB unless needed
	bool				m_ForceUpdateTransform;
	Vector				m_OldTransform_Location;
	Angles				m_OldTransform_Rotation;
	Vector				m_OldTransform_Scale;

	SimpleString		m_MeshName;					// Serialized
	SimpleString		m_AlbedoMapName;			// Serialized
	SimpleString		m_NormalMapName;			// Serialized
	SimpleString		m_SpecMapName;				// Serialized
	SimpleString		m_AlbedoMapOverride;		// Config
	SimpleString		m_NormalMapOverride;		// Config
	SimpleString		m_SpecMapOverride;			// Config
	SimpleString		m_MaterialOverride;			// Config; may come from costume
	bool				m_AlwaysDraw;				// Config; ignore visibility culling
	bool				m_DrawForeground;			// Config
	bool				m_IsLight;					// Config; convenience value to set up light multipass materials
	bool				m_IsShadowLight;			// Config; implies m_IsLight
	bool				m_IsFogLight;				// Config; DOES not imply m_IsLight ("emissive fog" might be a more accurate name)
	bool				m_HasAddedIgnoredMeshes;	// Transient
	bool				m_IsAntiLight;				// Config; convenience value to set up anti-light multipass materials
	bool				m_IsDecal;					// Config
	bool				m_IsFogMesh;				// Config
	uint				m_LightCubeMask;			// Config

	// Keeping this around so we can apply it whenever mesh changes
	float				m_AnimationBonesTickRate;		// Config
	float				m_AnimationTransformTickRate;	// Config, allowed to be different from bones tick rate intentionally. NOTE: This causes apparent location to diverge from actual transform!
	float				m_NextAnimationTransformTime;	// Transient

	Interpolator<Vector4>	m_CurrentHighlight;
	float					m_HighlightInterpTime;

	// ROSATODO: Get rid of this if I can find a better solution (e.g., using sector intersection to determine visibility)
	float					m_CullDistanceSq;			// Config; If non-zero, mesh is only drawn within this distance from camera

	Array<SAttachedMesh>	m_AttachedMeshes;
	Array<SimpleString>		m_AttachmentSets;	// Serialized (this is how I restore random attachments that aren't wardrobes)
};

#endif // WBCOMPROSAMESH_H
