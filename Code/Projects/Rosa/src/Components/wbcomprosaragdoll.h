#ifndef WBCOMPROSARAGDOLL_H
#define WBCOMPROSARAGDOLL_H

#include "wbrosacomponent.h"
#include "ibonemodifier.h"
#include "wbentityref.h"
#include "vector.h"
#include "quat.h"
#include "hashedstring.h"
#include "angles.h"
#include "interpolator.h"

class BoneArray;
class Matrix;

class WBCompRosaRagdoll : public WBRosaComponent, public IBoneModifier
{
public:
	WBCompRosaRagdoll();
	virtual ~WBCompRosaRagdoll();

	DEFINE_WBCOMP( RosaRagdoll, WBRosaComponent );

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

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	struct SRagdollSpring
	{
		SRagdollSpring()
		:	m_ParentMass( -1 )
		,	m_WSSpringLength( 0.0f )
		{
		}

		c_int32	m_ParentMass;
		float	m_WSSpringLength;	// World space instead of object space, because it may have mesh scale applied
	};

	struct SRagdollMass
	{
		SRagdollMass()
		:	m_OSLocation()
		,	m_WSLocation()
		,	m_BoneOSLocation()
		,	m_WSVelocity()
		,	m_Springs()
		,	m_SentCollidedEventLastTick( false )
		{
		}

		Vector					m_OSLocation;		// Serialized
		Vector					m_WSLocation;		// Serialized
		Vector					m_BoneOSLocation;	// Serialized
		Vector					m_WSVelocity;		// Serialized
		Vector					m_HalfExtents;		// Serialized (as radius)
		Array<SRagdollSpring>	m_Springs;			// Serialized
		bool					m_SentCollidedEventLastTick;	// Transient, bit of a hack to avoid spamming events
	};

	struct SRagdollCollisionInfo
	{
		SRagdollCollisionInfo()
		:	m_Speed( 0.0f )
		,	m_Velocity()
		,	m_Normal()
		{
		}

		float		m_Speed;
		Vector		m_Velocity;
		Vector		m_Normal;
	};

	void	StartRagdoll( const float BlendTime );
	void	StopRagdoll( const float BlendTime );

	void	SleepRagdoll();
	void	WakeRagdoll();

	void	Collide( SRagdollMass& Mass, const Vector& StartLocation, Vector& InOutMovement, SRagdollCollisionInfo* const pOutMaxRagdollCollisionInfo );

	Array<SRagdollMass>	m_Masses;		// Serialized
	bool				m_Active;		// Serialized
	bool				m_Asleep;		// Transient; this used to be serialized but we need to do one tick to set things like frob and mesh bounds, then we can go back to sleep
	Interpolator<float>	m_BlendInterp;	// Serialized

	float						m_SpringK;				// Config
	float						m_DamperC;				// Config
	float						m_DefaultMassRadius;	// Config
	Map<HashedString, float>	m_MassRadii;			// Config, map bone name to mass radius (half extent)

	float	m_MinRagdollCollisionEventSpeed;	// Config; smallest speed that generates an OnRagdollCollided event
};

#endif // WBCOMPROSARAGDOLL_H
