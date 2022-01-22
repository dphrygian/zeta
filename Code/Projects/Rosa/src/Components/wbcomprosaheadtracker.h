#ifndef WBCOMPROSAHEADTRACKER_H
#define WBCOMPROSAHEADTRACKER_H

#include "wbrosacomponent.h"
#include "ibonemodifier.h"
#include "wbentityref.h"
#include "vector.h"
#include "quat.h"
#include "hashedstring.h"
#include "angles.h"
#include "irodinresourceuser.h"

class BoneArray;
class Matrix;

class WBCompRosaHeadTracker : public WBRosaComponent, public IBoneModifier, public IRodinResourceUser
{
public:
	WBCompRosaHeadTracker();
	virtual ~WBCompRosaHeadTracker();

	DEFINE_WBCOMP( RosaHeadTracker, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	// IBoneModifier interface
	virtual void	ModifyBone( const SBoneInfo& BoneInfo, const uint BoneIndex, const Matrix& ParentBoneMatrix, SBone& InOutBone );

	// IRodinResourceUser
	virtual bool	OnResourceStolen( const HashedString& Resource );
	virtual void	OnResourceReturned( const HashedString& Resource );

	Vector			GetEyesLocation() const;
	Vector			GetLookDirection() const;
	Angles			GetLookAngles() const;	// Returns OS angles (i.e., angular offset from transform orientation)

	void			SetMaxRotation( const float MaxRotation )	{ m_MaxRotationRadians	= MaxRotation; }
	void			SetLookVelocity( const float Velocity )		{ m_LookVelocity		= Velocity; }

	bool			HasHeadResource() const	{ return m_HasHeadResource; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			LookAtEntity( WBEntity* const pLookAtTarget );
	void			LookAtLocation( const Vector& LookAtTarget );
	void			LookAtAngles( const Angles& LookAtTarget );
	void			StopLooking( const bool LockOrientation );

	enum ETrackMode
	{
		ETM_None,
		ETM_Location,
		ETM_Entity,
		ETM_Angles,
	};

	ETrackMode		m_TrackMode;
	HashedString	m_HeadBoneName;				// Config
	HashedString	m_EyesBoneName;				// Config
	Vector			m_EyesOffset;				// Config; object space location of the eyes *relative to transform*; only used if we don't have an EyesBoneName
	float			m_MaxRotationRadians;		// Config/serialized
	float			m_LookVelocity;				// Config/serialized, radians per second
	float			m_FixedPitch;				// Config/serialized: for security cameras in Neon
	bool			m_UseFixedPitch;			// Config/serialized
	WBEntityRef		m_LookAtTargetEntity;
	Vector			m_LookAtTargetLocation;
	Angles			m_LookAtTargetAngles;
	Quat			m_LastHeadBoneOrientation;	// Copy so we can constrain to the actual animated head orientation instead of entity orientation (this is object space, not bone space, and pre-headtracking)
	Quat			m_LookRotationOS_Sim;		// Copy that we use for simming; in fixed pitch mode, this has no pitch in it
	Quat			m_LookRotationOS_Final;		// Copy that we use for queries; in fixed pitch mode, this has the fixed pitch applied
	bool			m_LockOrientation;			// Serialized

	HashedString	m_HeadResource;				// Config
	bool			m_HasHeadResource;			// Transient
};

#endif // WBCOMPROSAHEADTRACKER_H
