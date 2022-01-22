#include "core.h"
#include "wbcomprosaheadtracker.h"
#include "wbevent.h"
#include "hashedstring.h"
#include "configmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosavisible.h"
#include "wbcomprosamesh.h"
#include "Components/wbcomprodinknowledge.h"
#include "Components/wbcomprodinresourcemap.h"
#include "matrix.h"
#include "mathcore.h"
#include "bonearray.h"
#include "rosamesh.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "idatastream.h"

// NOTE: Like RosaAIMotion, this *never* releases its head resource. That's ok.

WBCompRosaHeadTracker::WBCompRosaHeadTracker()
:	m_TrackMode( ETM_None )
,	m_HeadBoneName()
,	m_EyesBoneName()
,	m_EyesOffset()
,	m_MaxRotationRadians( 0.0f )
,	m_LookVelocity( 0.0f )
,	m_FixedPitch( 0.0f )
,	m_UseFixedPitch( false )
,	m_LookAtTargetEntity()
,	m_LookAtTargetLocation()
,	m_LookAtTargetAngles()
,	m_LastHeadBoneOrientation()
,	m_LookRotationOS_Sim()
,	m_LookRotationOS_Final()
,	m_LockOrientation( false )
,	m_HeadResource()
,	m_HasHeadResource( false )
{
}

WBCompRosaHeadTracker::~WBCompRosaHeadTracker()
{
}

/*virtual*/ void WBCompRosaHeadTracker::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( HeadBoneName );
	m_HeadBoneName = ConfigManager::GetInheritedHash( sHeadBoneName, HashedString::NullString, sDefinitionName );

	STATICHASH( EyesBoneName );
	m_EyesBoneName = ConfigManager::GetInheritedHash( sEyesBoneName, HashedString::NullString, sDefinitionName );

	STATICHASH( EyesOffsetX );
	m_EyesOffset.x = ConfigManager::GetInheritedFloat( sEyesOffsetX, 0.0f, sDefinitionName );

	STATICHASH( EyesOffsetY );
	m_EyesOffset.y = ConfigManager::GetInheritedFloat( sEyesOffsetY, 0.0f, sDefinitionName );

	STATICHASH( EyesOffsetZ );
	m_EyesOffset.z = ConfigManager::GetInheritedFloat( sEyesOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( MaxRotation );
	m_MaxRotationRadians = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sMaxRotation, 0.0f, sDefinitionName ) );

	STATICHASH( LookVelocity );
	m_LookVelocity = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sLookVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( UseFixedPitch );
	m_UseFixedPitch = ConfigManager::GetInheritedBool( sUseFixedPitch, false, sDefinitionName );

	STATICHASH( FixedPitch );
	m_FixedPitch = ConfigManager::GetInheritedFloat( sFixedPitch, 0.0f, sDefinitionName );

	STATICHASH( HeadResource );
	m_HeadResource = ConfigManager::GetInheritedHash( sHeadResource, HashedString::NullString, sDefinitionName );

	if( HashedString::NullString == m_HeadResource )
	{
		// We don't have a resource map, so treat it like we always have the resource
		m_HasHeadResource = true;
	}
	else
	{
		WBCompRodinResourceMap* const pResourceMap = WB_GETCOMP( GetEntity(), RodinResourceMap );
		if( pResourceMap && pResourceMap->ClaimResource( this, m_HeadResource, false ) )
		{
			m_HasHeadResource = true;
		}
		else
		{
			WARNDESC( "Could not acquire head resource for RosaHeadTracker!" );
		}
	}
}

/*virtual*/ void WBCompRosaHeadTracker::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( LookAt );
	STATIC_HASHED_STRING( StopLooking );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( SetMaxRotation );
	STATIC_HASHED_STRING( SetLookVelocity );
	STATIC_HASHED_STRING( SetFixedPitch );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		WBCompRosaMesh* const pMesh = WB_GETCOMP( GetEntity(), RosaMesh );
		ASSERT( pMesh );
		ASSERT( pMesh->GetMesh() );
		pMesh->GetMesh()->AddBoneModifier( this );
	}
	else if( EventName == sLookAt )
	{
		STATIC_HASHED_STRING( LookAtLocation );
		const Vector LookTargetLocation = Event.GetVector( sLookAtLocation );

		STATIC_HASHED_STRING( LookAtAngles );
		const Angles LookTargetAngles = Event.GetAngles( sLookAtAngles );

		STATIC_HASHED_STRING( LookAtEntity );
		WBEntity* const pLookTargetEntity = Event.GetEntity( sLookAtEntity );

		if( pLookTargetEntity )
		{
			LookAtEntity( pLookTargetEntity );
		}
		else if( !LookTargetLocation.IsZero() )
		{
			LookAtLocation( LookTargetLocation );
		}
		else
		{
			LookAtAngles( LookTargetAngles );
		}
	}
	else if( EventName == sStopLooking )
	{
		STATIC_HASHED_STRING( LockOrientation );
		const bool LockOrientation = Event.GetBool( sLockOrientation );

		StopLooking( LockOrientation );
	}
	else if( EventName == sSetMaxRotation )
	{
		STATIC_HASHED_STRING( MaxRotation );
		const float MaxRotation = Event.GetFloat( sMaxRotation );

		SetMaxRotation( MaxRotation );
	}
	else if( EventName == sSetLookVelocity )
	{
		STATIC_HASHED_STRING( LookVelocity );
		const float LookVelocity = Event.GetFloat( sLookVelocity );

		SetLookVelocity( LookVelocity );
	}
	else if( EventName == sSetFixedPitch )
	{
		STATIC_HASHED_STRING( UseFixedPitch );
		const bool UseFixedPitch = Event.GetBool( sUseFixedPitch );

		STATIC_HASHED_STRING( FixedPitch );
		const float FixedPitch = Event.GetFloat( sFixedPitch );

		m_UseFixedPitch		= UseFixedPitch;
		m_FixedPitch		= FixedPitch;
	}
}

/*virtual*/ void WBCompRosaHeadTracker::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );
	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Matrix OrientationMatrix	= pTransform->GetOrientation().ToMatrix();
	const Vector EyesLocation		= GetEyesLocation();

	Vector	TargetLocation;
	Vector	DirectionToTarget;
	bool	IsLooking			= true;

	if( m_TrackMode == ETM_None )
	{
		if( m_LockOrientation )
		{
			// Keep looking in the direction we were last looking
			const Vector	ForwardDirection	= Vector( 0.0f, 1.0f, 0.0f );
			const Vector	TurnedDirectionOS	= ForwardDirection * m_LookRotationOS_Sim.ToMatrix();
			const Vector	TurnedDirectionWS	= TurnedDirectionOS * OrientationMatrix;
			DirectionToTarget = TurnedDirectionWS;
		}
		else
		{
			// Look straight ahead
			IsLooking = false;
		}
	}
	else if( m_TrackMode == ETM_Entity )
	{
		WBEntity* const pLookAtTarget = m_LookAtTargetEntity.Get();
		if( pLookAtTarget )
		{
			WBCompRosaTransform* const pLookAtTransform = pLookAtTarget->GetTransformComponent<WBCompRosaTransform>();
			ASSERT( pLookAtTransform );

			WBCompRosaVisible* const	pVisible = WB_GETCOMP( pLookAtTarget, RosaVisible );

			// Simulational honesty!
			WBCompRodinKnowledge* const pKnowledge = WB_GETCOMP( pEntity, RodinKnowledge );
			if( pKnowledge->IsCurrentlyVisible( pLookAtTarget ) )
			{
				static const bool IsAlreadyVisible = true;
				TargetLocation		= pVisible ? pVisible->GetVisibleLocation( IsAlreadyVisible ) : pLookAtTransform->GetLocation();
				DirectionToTarget	= TargetLocation - EyesLocation;
			}
			else
			{
				const bool HasLastKnownLocation = pKnowledge->GetLastKnownLocationFor( pLookAtTarget, TargetLocation );
				if( HasLastKnownLocation )
				{
					// We're good, we've got a last known location
					DirectionToTarget	= TargetLocation - EyesLocation;

					// ROSAHACK: Only yaw toward this location; it looks bad when AIs look down at floor
					DirectionToTarget.z = 0.0f;
				}
				else
				{
					// We don't know where our target is
					StopLooking( false );
				}
			}
		}
		else
		{
			// Our look-at target has been destroyed
			StopLooking( false );
		}
	}
	else if( m_TrackMode == ETM_Location )
	{
		TargetLocation		= m_LookAtTargetLocation;
		DirectionToTarget	= TargetLocation - EyesLocation;
	}
	else if( m_TrackMode == ETM_Angles )
	{
		DirectionToTarget	= m_LookAtTargetAngles.ToVector();
	}

	if( m_UseFixedPitch )
	{
		DirectionToTarget.z = 0.0f;
	}

	const Matrix	ReverseOrientation	= ( m_LastHeadBoneOrientation.ToMatrix() * pTransform->GetOrientation().ToMatrix() ).GetInverse();
	const Matrix	RotationToTarget	= DirectionToTarget.ToAngles().ToMatrix();
	const Matrix	RotationOS			= RotationToTarget * ReverseOrientation;
	const Quat		ToTarget			= IsLooking ? RotationOS.ToQuaternion() : Quat();
	const Quat		ConstrainedToTarget	= ( ToTarget.GetAngle() <= m_MaxRotationRadians ) ? ToTarget : Quat::CreateRotation( ToTarget.GetAxis(), m_MaxRotationRadians );
	const Quat		InvLookRotation		= m_LookRotationOS_Sim.GetInverse();
	const Quat		RelativeRotation	= ( InvLookRotation * ConstrainedToTarget ).GetFastNormalized();	// Normalize because even slight values of w>1 will screw up GetAngle()
	const float		RotationAngle		= RelativeRotation.GetAngle();
	DEVASSERT( FIsANumber( RotationAngle ) );
	const Vector	RotationAxis		= RelativeRotation.GetAxis();
	const float		AxisLength			= RotationAxis.Length();
	const Vector	NormalizedAxis		= ( AxisLength < EPSILON ) ? Vector() : ( RotationAxis / AxisLength );
	const float		NormalizedAngle		= RotationAngle * AxisLength;
	const float		TurnRadians			= Min( m_LookVelocity * DeltaTime, NormalizedAngle );
	const Quat		TurnRotation		= Quat::CreateRotation( NormalizedAxis, TurnRadians );

	m_LookRotationOS_Sim *= TurnRotation;

	if( m_UseFixedPitch )
	{
		const Vector	ForwardDirection	= Vector( 0.0f, 1.0f, 0.0f );
		const Vector	TurnedDirection		= ForwardDirection * m_LookRotationOS_Sim.ToMatrix();
		const Angles	TurnedAngles		= TurnedDirection.ToAngles();
		const Angles	FixedAngles			= Angles( m_FixedPitch, 0.0f, TurnedAngles.Yaw );
		m_LookRotationOS_Final				= FixedAngles.ToQuaternion();
	}
	else
	{
		m_LookRotationOS_Final				= m_LookRotationOS_Sim;
	}
}

/*virtual*/ void WBCompRosaHeadTracker::ModifyBone( const SBoneInfo& BoneInfo, const uint BoneIndex, const Matrix& ParentBoneMatrix, SBone& InOutBone )
{
	Unused( BoneIndex );

	if( !m_HasHeadResource )
	{
		return;
	}

	// Ignore everything but head bone
	if( BoneInfo.m_Name == m_HeadBoneName )
	{
		m_LastHeadBoneOrientation	= ( BoneInfo.m_InvBindPose * InOutBone.m_Quat.ToMatrix() * ParentBoneMatrix ).ToQuaternion();			// Get the animated orientation in object space by undoing the bind pose
		InOutBone.m_Quat			= ( BoneInfo.m_BindPoseQuat * m_LookRotationOS_Final * BoneInfo.m_InvBindPoseQuat ) * InOutBone.m_Quat;	// Apply the look rotation relative to the bind pose orientation, then apply to animated bone
	}
}

void WBCompRosaHeadTracker::LookAtEntity( WBEntity* const pLookAtTarget )
{
	m_TrackMode				= ETM_Entity;
	m_LookAtTargetEntity	= pLookAtTarget;
}

void WBCompRosaHeadTracker::LookAtLocation( const Vector& LookAtTarget )
{
	m_TrackMode				= ETM_Location;
	m_LookAtTargetLocation	= LookAtTarget;
	m_LookAtTargetEntity	= static_cast<WBEntity*>( NULL );
}

void WBCompRosaHeadTracker::LookAtAngles( const Angles& LookAtTarget )
{
	m_TrackMode				= ETM_Angles;
	m_LookAtTargetAngles	= LookAtTarget;
	m_LookAtTargetEntity	= static_cast<WBEntity*>( NULL );
}

void WBCompRosaHeadTracker::StopLooking( const bool LockOrientation )
{
	m_TrackMode			= ETM_None;
	m_LockOrientation	= LockOrientation;
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaHeadTracker::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	WBCompRosaTransform* const	pTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaMesh* const		pMesh			= WB_GETCOMP( GetEntity(), RosaMesh );
	DEVASSERT( pMesh );

	const BoneArray* const		pBones			= pMesh->GetBones();
	DEVASSERT( pBones );

	const int					HeadBoneIndex	= pBones->GetBoneIndex( m_HeadBoneName );
	DEVASSERT( HeadBoneIndex != INVALID_INDEX );

	const SBoneInfo&			HeadBone		= pBones->GetBoneInfo( HeadBoneIndex );
	const Vector				HeadOffset		= HeadBone.m_BoneStart;

	const Vector				HeadLocation	= pTransform->GetLocation() + pMesh->GetMeshOffset() + ( HeadOffset * pTransform->GetOrientation().ToMatrix() );
	const Vector				EyesLocation	= GetEyesLocation();

	GetFramework()->GetRenderer()->DEBUGDrawCross( EyesLocation, 0.25f, ARGB_TO_COLOR( 255, 255,  32,  32 ) );
	GetFramework()->GetRenderer()->DEBUGDrawCross( HeadLocation, 0.25f, ARGB_TO_COLOR( 255,  32, 255,  32 ) );

	const Vector LookDirectionOS = Vector( 0.0f, 1.0f, 0.0f ) * m_LookRotationOS_Final.ToMatrix();
	const Vector LookDirectionWS = LookDirectionOS * pTransform->GetOrientation().ToMatrix();
	GetFramework()->GetRenderer()->DEBUGDrawLine( EyesLocation - LookDirectionWS * 1.0f, EyesLocation + LookDirectionWS * 5.0f, ARGB_TO_COLOR( 255, 255,  32,  32 ) );
	GetFramework()->GetRenderer()->DEBUGDrawLine( HeadLocation - LookDirectionWS * 1.0f, HeadLocation + LookDirectionWS * 5.0f, ARGB_TO_COLOR( 255,  32, 255,  32 ) );
}
#endif

Vector WBCompRosaHeadTracker::GetEyesLocation() const
{
	WBCompRosaMesh* const			pMesh			= WB_GETCOMP( GetEntity(), RosaMesh );
	DEVASSERT( pMesh );

	const BoneArray* const			pBones			= pMesh->GetBones();
	DEVASSERT( pBones );

	const int						EyesBoneIndex	= pBones->GetBoneIndex( m_EyesBoneName );
	if( EyesBoneIndex == INVALID_INDEX )
	{
		WBCompRosaTransform* const	pTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		// TODO: Incorporate scale in here (this is legacy stuff, doesn't really matter)
		return pTransform->GetLocation() + ( m_EyesOffset * pTransform->GetOrientation().ToMatrix() );
	}
	else
	{
		return pMesh->GetBoneLocation( EyesBoneIndex );
	}
}

Vector WBCompRosaHeadTracker::GetLookDirection() const
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector LookDirectionOS = Vector( 0.0f, 1.0f, 0.0f ) * m_LookRotationOS_Final.ToMatrix();
	const Vector LookDirectionWS = LookDirectionOS * pTransform->GetOrientation().ToMatrix();

	return LookDirectionWS;
}

Angles WBCompRosaHeadTracker::GetLookAngles() const
{
	return m_LookRotationOS_Final.ToAngles();
}

/*virtual*/ bool WBCompRosaHeadTracker::OnResourceStolen( const HashedString& Resource )
{
	Unused( Resource );
	DEVASSERT( Resource == m_HeadResource );

	m_HasHeadResource = false;

	return true;
}

/*virtual*/ void WBCompRosaHeadTracker::OnResourceReturned( const HashedString& Resource )
{
	Unused( Resource );
	DEVASSERT( Resource == m_HeadResource );

	m_HasHeadResource = true;
}

#define VERSION_EMPTY			0
#define VERSION_MAXROTATION		1
#define VERSION_LOOKROTATIONOS	2
#define VERSION_LOOKVELOCITY	3
#define VERSION_FIXEDPITCH		4
#define VERSION_LOCKORIENTATION	5
#define VERSION_CURRENT			5

uint WBCompRosaHeadTracker::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;				// Version

	Size += 4;				// m_MaxRotationRadians

	Size += sizeof( Quat );	// m_LookRotationOS

	Size += 4;				// m_LookVelocity

	Size += 4;				// m_FixedPitch
	Size += 1;				// m_UseFixedPitch

	Size += 1;				// m_LockOrientation

	return Size;
}

void WBCompRosaHeadTracker::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteFloat( m_MaxRotationRadians );

	Stream.Write( sizeof( Quat ), &m_LookRotationOS_Sim );

	Stream.WriteFloat( m_LookVelocity );

	Stream.WriteFloat( m_FixedPitch );
	Stream.WriteBool( m_UseFixedPitch );

	Stream.WriteBool( m_LockOrientation );
}

void WBCompRosaHeadTracker::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_MAXROTATION )
	{
		m_MaxRotationRadians = Stream.ReadFloat();
	}

	if( Version >= VERSION_LOOKROTATIONOS )
	{
		Stream.Read( sizeof( Quat ), &m_LookRotationOS_Sim );
	}

	if( Version >= VERSION_LOOKVELOCITY )
	{
		m_LookVelocity = Stream.ReadFloat();
	}

	if( Version >= VERSION_FIXEDPITCH )
	{
		m_FixedPitch		= Stream.ReadFloat();
		m_UseFixedPitch		= Stream.ReadBool();
	}

	if( Version >= VERSION_LOCKORIENTATION )
	{
		m_LockOrientation = Stream.ReadBool();
	}
}
