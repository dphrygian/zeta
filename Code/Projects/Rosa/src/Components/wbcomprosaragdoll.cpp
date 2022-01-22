#include "core.h"
#include "wbcomprosaragdoll.h"
#include "wbevent.h"
#include "wbeventmanager.h"
#include "hashedstring.h"
#include "configmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosavisible.h"
#include "wbcomprosamesh.h"
#include "wbcomprosacollision.h"
#include "Components/wbcomprodinknowledge.h"
#include "matrix.h"
#include "mathcore.h"
#include "bonearray.h"
#include "rosamesh.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "idatastream.h"
#include "ray.h"
#include "collisioninfo.h"
#include "reversehash.h"
#include "fontmanager.h"

WBCompRosaRagdoll::WBCompRosaRagdoll()
:	m_Masses()
,	m_Active( false )
,	m_Asleep( false )
,	m_BlendInterp()
,	m_SpringK( 0.0f )
,	m_DamperC( 0.0f )
,	m_DefaultMassRadius( 0.0f )
,	m_MassRadii()
,	m_MinRagdollCollisionEventSpeed( 0.0f )
{
}

WBCompRosaRagdoll::~WBCompRosaRagdoll()
{
}

/*virtual*/ void WBCompRosaRagdoll::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( SpringK );
	m_SpringK = ConfigManager::GetInheritedFloat( sSpringK, 0.0f, sDefinitionName );

	STATICHASH( DamperC );
	m_DamperC = ConfigManager::GetInheritedFloat( sDamperC, 0.0f, sDefinitionName );

	STATICHASH( DefaultMassRadius );
	m_DefaultMassRadius = ConfigManager::GetInheritedFloat( sDefaultMassRadius, 0.0f, sDefinitionName );

	STATICHASH( NumMassRadii );
	const uint NumMassRadii = ConfigManager::GetInheritedInt( sNumMassRadii, 0, sDefinitionName );
	FOR_EACH_INDEX( MassRadiusIndex, NumMassRadii )
	{
		const HashedString	Mass	= ConfigManager::GetInheritedSequenceHash(	"MassRadius%dMass",		MassRadiusIndex, HashedString::NullString,	sDefinitionName );
		const float			Radius	= ConfigManager::GetInheritedSequenceFloat(	"MassRadius%dRadius",	MassRadiusIndex, 0.0f,						sDefinitionName );
		m_MassRadii[ Mass ] = Radius;
	}

	STATICHASH( RosaRagdoll );
	STATICHASH( MinRagdollCollisionEventSpeed );
	const float DefaultMinRagdollCollisionEventSpeed = ConfigManager::GetFloat( sMinRagdollCollisionEventSpeed, 0.0f, sRosaRagdoll );
	m_MinRagdollCollisionEventSpeed = ConfigManager::GetInheritedFloat( sMinRagdollCollisionEventSpeed, DefaultMinRagdollCollisionEventSpeed, sDefinitionName );
}

/*virtual*/ void WBCompRosaRagdoll::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( StartRagdoll );
	STATIC_HASHED_STRING( StopRagdoll );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		WBCompRosaMesh* const pMesh = WB_GETCOMP( GetEntity(), RosaMesh );
		ASSERT( pMesh );
		ASSERT( pMesh->GetMesh() );
		pMesh->GetMesh()->AddBoneModifier( this );

		// TEMPHACK
		//StartRagdoll( 4.0f );
	}
	else if( EventName == sStartRagdoll )
	{
		STATIC_HASHED_STRING( BlendTime );
		const float BlendTime = Event.GetFloat( sBlendTime );

		StartRagdoll( BlendTime );
	}
	else if( EventName == sStopRagdoll )
	{
		STATIC_HASHED_STRING( BlendTime );
		const float BlendTime = Event.GetFloat( sBlendTime );

		StopRagdoll( BlendTime );
	}
}

void WBCompRosaRagdoll::StartRagdoll( const float BlendTime )
{
	m_BlendInterp.Reset( Interpolator<float>::EIT_Linear, m_BlendInterp.GetValue(), 1.0f, BlendTime );
}

void WBCompRosaRagdoll::StopRagdoll( const float BlendTime )
{
	m_BlendInterp.Reset( Interpolator<float>::EIT_Linear, m_BlendInterp.GetValue(), 0.0f, BlendTime );
}

void WBCompRosaRagdoll::SleepRagdoll()
{
	m_Asleep = true;
}

// DLP 28 Nov 2021: Not currently used; we could wake it if the body gets a collision or something.
void WBCompRosaRagdoll::WakeRagdoll()
{
	m_Asleep = false;
}

// Hacked up from WBCompRosaCollision::Collide
void WBCompRosaRagdoll::Collide( SRagdollMass& Mass, const Vector& StartLocation, Vector& InOutMovement, SRagdollCollisionInfo* const pOutMaxRagdollCollisionInfo )
{
	static const Vector	sUpVector		= Vector( 0.0f, 0.0f, 1.0f );
	static const float	sMoveLimit		= 1.0e-5f;	// At this threshold, we just don't move at all (this is larger than for normal collisions! and also should really be relative to delta time)
	static const int	sIterationLimit	= 2;		// At this many collisions, bail out with no movement (this is smaller than for normal collisions)

	if( InOutMovement.LengthSquared() <= sMoveLimit )
	{
		InOutMovement.Zero();
		return;
	}

	RosaWorld* const	pWorld			= GetWorld();

	WBCompRosaCollision* const	pCollision			= WB_GETCOMP( GetEntity(), RosaCollision );
	const float					FrictionCoefficient	= pCollision->GetFrictionCoefficient();
	const float					Elasticity			= pCollision->GetElasticity();

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= EECF_RagdollCollision;

	uint NumIterations			= 0;

	const bool					SentCollidedEventLastTick	= Mass.m_SentCollidedEventLastTick;
	Mass.m_SentCollidedEventLastTick						= false;

	for(;;)
	{
		NumIterations++;

		const Segment SweepSegment = Segment( StartLocation, StartLocation + InOutMovement );
		if( !pWorld->Sweep( SweepSegment, Mass.m_HalfExtents, Info ) )
		{
			break;
		}

		// Complete collision, probably started the move inside geometry.
		if( Info.m_Out_Plane.m_Normal.LengthSquared() == 0.0f )
		{
			InOutMovement.Zero();
			Mass.m_WSVelocity.Zero();

			// Eject mass gradually, if possible
			Vector FoundLocation = StartLocation;
			if( pWorld->FindSpot( FoundLocation, Mass.m_HalfExtents, Info ) )
			{
				const Vector	EjectionDirection	= ( FoundLocation - StartLocation ).GetNormalized();
				InOutMovement						= EjectionDirection * 0.005f;	// Only move by 5mm each tick, that's the most it can pop
				//InOutMovement = FoundLocation - StartLocation;
			}

			break;
		}

		const Vector OldInOutMovement = InOutMovement;
		InOutMovement = Info.m_Out_Plane.ProjectVector( InOutMovement );

		const float	FloorDot		= Info.m_Out_Plane.m_Normal.Dot( sUpVector );
		const bool	LandedOnFloor	= ( FloorDot >= 0.5f );	// 60 degree slope or shallower is walkable
		const float	Friction		= LandedOnFloor ? FrictionCoefficient : 1.0f;

		// Something went wrong
		if( OldInOutMovement == InOutMovement )
		{
			WARN;
			InOutMovement.Zero();
			break;
		}

		const Vector OldVelocity					= Mass.m_WSVelocity;
		const Vector VelocityInCollisionPlane		= Info.m_Out_Plane.ProjectVector( OldVelocity );
		const Vector OrthogonalVelocityComponent	= VelocityInCollisionPlane * Friction;

		const Vector VelocityInCollisionNormal		= OldVelocity.ProjectionOnto( Info.m_Out_Plane.m_Normal );
		const Vector ParallelVelocityComponent		= VelocityInCollisionNormal * -Elasticity;

		const Vector NewVelocity					= OrthogonalVelocityComponent + ParallelVelocityComponent;
		Mass.m_WSVelocity							= NewVelocity;

		if( NULL != pOutMaxRagdollCollisionInfo )
		{
			// Only update this if we're returning collision info; else we're matching an animation and it doesn't count for collisions
			Mass.m_SentCollidedEventLastTick				= true;

			// Only send collision events if there's been at least one tick since this mass's previous collision.
			// Trying to minimize collisions especially when masses end up clumped up in or near geo.
			if( !SentCollidedEventLastTick )
			{
				const float	RagdollCollisionSpeed			= VelocityInCollisionNormal.Length();
				if( RagdollCollisionSpeed > pOutMaxRagdollCollisionInfo->m_Speed )
				{
					pOutMaxRagdollCollisionInfo->m_Speed	= RagdollCollisionSpeed;
					pOutMaxRagdollCollisionInfo->m_Velocity	= VelocityInCollisionNormal;
					pOutMaxRagdollCollisionInfo->m_Normal	= Info.m_Out_Plane.m_Normal;
				}
			}
		}

		if( NumIterations >= sIterationLimit )
		{
			InOutMovement.Zero();
			Mass.m_WSVelocity.Zero();
			break;
		}

		if( InOutMovement.LengthSquared() <= sMoveLimit )
		{
			InOutMovement.Zero();
			break;
		}
	}

	//if( InOutMovement != Vector() && pWorld->CheckClearance( StartLocation + InOutMovement, Mass.m_HalfExtents, Info ) )
	//{
	//	// Expensive, but I need a final check to forbid any movement that would get things stuck in geo.
	//	// I'm seeing cases where AnyCollision is false and InOutMovement is nonzero, but the sweep didn't
	//	// detect a collision where this *does*. Probably a problem with ConvexHull::Sweep.
	//	BREAKPOINT;
	//	InOutMovement.Zero();
	//	Mass.m_WSVelocity.Zero();
	//}
}

/*virtual*/ void WBCompRosaRagdoll::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	m_BlendInterp.Tick( DeltaTime );
	if( m_BlendInterp.GetValue() > 0.0f )
	{
		m_Active = true;
	}
	else
	{
		if( m_Active )
		{
			WB_MAKE_EVENT( OnRagdollStopped, GetEntity() );
			WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnRagdollStopped, GetEntity() );
		}

		m_Active = false;
		m_Masses.Clear();
	}

	if( !m_Active || m_Asleep || m_Masses.Empty() )
	{
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	WBCompRosaMesh* const		pMesh			= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMesh );

	const Vector				MeshLocation	= pTransform->GetLocation() + pMesh->GetMeshOffset();
	const Angles				MeshOrientation	= pTransform->GetOrientation();
	const float					MeshScale		= pTransform->GetScale();
	const Vector				MeshScaleVector	= Vector( MeshScale, MeshScale, MeshScale );
	const Matrix				MeshMatrix		= Matrix::CreateScale( MeshScaleVector ) * MeshOrientation.ToMatrix() * Matrix::CreateTranslation( MeshLocation );
	const Matrix				InvMeshMatrix	= MeshMatrix.GetInverse();

	const Vector				Gravity			= Vector( 0.0f, 0.0f, pTransform->GetGravity() );

	AABB						WSMassBounds;
	AABB						OSMassBounds;

	bool						AnyMovement		= false;

	SRagdollCollisionInfo		MaxRagdollCollisionInfo;

	FOR_EACH_ARRAY( MassIter, m_Masses, SRagdollMass )
	{
		SRagdollMass&	Mass				= MassIter.GetValue();

		const uint		MassIndex			= MassIter.GetIndex();
		const bool		UpdateMassToBone	= ( MassIndex == 0 ) || m_BlendInterp.GetValue() < 1.0f;	// Always move the root mass, and keep the masses in sync with animation while blending

		if( UpdateMassToBone )
		{
			// Move the mass to where the bone should be, but sweep to make sure
			// we don't cause masses to interpenetrate walls and stuff.
			const Vector	BoneWSLocation	= Mass.m_BoneOSLocation * MeshMatrix;
			Vector			LocationOffset	= BoneWSLocation - Mass.m_WSLocation;

			Collide( Mass, Mass.m_WSLocation, LocationOffset, NULL );
			Mass.m_WSLocation				+= LocationOffset;
			Mass.m_OSLocation				= Mass.m_WSLocation * InvMeshMatrix;
			DEBUGASSERT( Mass.m_OSLocation == Mass.m_OSLocation );	// Hunt for NaNs
			AnyMovement						= AnyMovement || !LocationOffset.IsZero();
		}

		// Don't drive the root mass with physics, it should be locked to entity/animation
		if( 0 == MassIndex )
		{
			Mass.m_WSLocation	= Mass.m_OSLocation * MeshMatrix;	// Keep root updated from latest transform
			WSMassBounds		= AABB::CreateFromCenterAndExtents( Mass.m_WSLocation, Mass.m_HalfExtents );
			OSMassBounds		= AABB::CreateFromCenterAndExtents( Mass.m_OSLocation, Mass.m_HalfExtents );
			continue;
		}

		Vector			Acceleration;
		Acceleration			+= Gravity;

		FOR_EACH_ARRAY( SpringIter, Mass.m_Springs, SRagdollSpring )
		{
			const SRagdollSpring&	Spring				= SpringIter.GetValue();
			const bool				HasParent			= ( Spring.m_ParentMass >= 0 );
			const Vector			ParentLocation		= HasParent ? m_Masses[ Spring.m_ParentMass ].m_WSLocation : Vector( MeshLocation.x, MeshLocation.y, Mass.m_WSLocation.z );	// HACKHACK: Constrain XY to mesh location, at any Z
			//const Vector			ParentVelocity		= HasParent ? m_Masses[ Spring.m_ParentMass ].m_WSVelocity : pTransform->GetVelocity();
			const Vector			SpringOffset		= Mass.m_WSLocation - ParentLocation;
			const Vector			SpringNormal		= SpringOffset.GetNormalized();
			if( SpringNormal.IsZero() )
			{
				// Prevent NaNs
				continue;
			}

			const float				SpringLength		= SpringOffset.Length();
			const float				SpringDelta			= SpringLength - Spring.m_WSSpringLength;
			const Vector			SpringForce			= -m_SpringK * ( SpringDelta * SpringNormal );

			// I don't think damping the relative velocity is correct, damp *all* velocity
			const Vector			DamperForce			= -m_DamperC * Mass.m_WSVelocity;
			//const Vector			RelativeVelocity	= Mass.m_WSVelocity - ParentVelocity;
			//const Vector			SpringVelocity		= RelativeVelocity.ProjectionOnto( SpringNormal );
			//const Vector			DamperForce			= -DamperC * SpringVelocity;

			const Vector			TotalForce			= SpringForce + DamperForce;
			DEBUGASSERT( TotalForce == TotalForce );	// Hunt for NaNs
			Acceleration								+= TotalForce;
		}

		Mass.m_WSVelocity				+= Acceleration * DeltaTime;	// Integrating velocity before position is Euler-Cromer, stabler than Euler
		Vector			LocationOffset	= Mass.m_WSVelocity * DeltaTime;

		Collide( Mass, Mass.m_WSLocation, LocationOffset, &MaxRagdollCollisionInfo );
		Mass.m_WSLocation				+= LocationOffset;
		Mass.m_OSLocation				= Mass.m_WSLocation * InvMeshMatrix;
		DEBUGASSERT( Mass.m_OSLocation == Mass.m_OSLocation );	// Hunt for NaNs
		AnyMovement						= AnyMovement || !LocationOffset.IsZero();

		// DLP 8 Jun 2021: Use actual bone locations here instead of mass locations,
		// to prevent bounds (for frobbables and meshes) being oversized if the masses
		// get hung up on geo and detached from mesh.
		WSMassBounds.ExpandTo( AABB::CreateFromCenterAndExtents( Mass.m_BoneOSLocation * MeshMatrix,	Mass.m_HalfExtents ) );
		OSMassBounds.ExpandTo( AABB::CreateFromCenterAndExtents( Mass.m_BoneOSLocation,					Mass.m_HalfExtents ) );
	}

	if( MaxRagdollCollisionInfo.m_Speed > m_MinRagdollCollisionEventSpeed )
	{
		WB_MAKE_EVENT( OnRagdollCollided, GetEntity() );
		WB_SET_AUTO( OnRagdollCollided, Float, CollisionSpeed, MaxRagdollCollisionInfo.m_Speed );
		WB_SET_AUTO( OnRagdollCollided, Vector, CollisionVelocity, MaxRagdollCollisionInfo.m_Velocity );
		WB_SET_AUTO( OnRagdollCollided, Vector, CollisionNormal, MaxRagdollCollisionInfo.m_Normal );
		WB_DISPATCH_EVENT( GetEventManager(), OnRagdollCollided, GetEntity() );
	}

	WB_MAKE_EVENT( OnRagdollTicked, GetEntity() );
	WB_SET_AUTO( OnRagdollTicked, Vector, WSMassBoundsMin, WSMassBounds.m_Min );
	WB_SET_AUTO( OnRagdollTicked, Vector, WSMassBoundsMax, WSMassBounds.m_Max );
	WB_SET_AUTO( OnRagdollTicked, Vector, OSMassBoundsMin, OSMassBounds.m_Min );
	WB_SET_AUTO( OnRagdollTicked, Vector, OSMassBoundsMax, OSMassBounds.m_Max );
	WB_SET_AUTO( OnRagdollTicked, Float, BlendAlpha, m_BlendInterp.GetValue() );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnRagdollTicked, GetEntity() );

	if( !AnyMovement )
	{
		SleepRagdoll();
	}
}

/*virtual*/ void WBCompRosaRagdoll::ModifyBone( const SBoneInfo& BoneInfo, const uint BoneIndex, const Matrix& ParentBoneMatrix, SBone& InOutBone )
{
	if( !m_Active )
	{
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	WBCompRosaMesh* const		pMesh			= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMesh );

	const Vector				MeshLocation	= pTransform->GetLocation() + pMesh->GetMeshOffset();
	const Angles				MeshOrientation	= pTransform->GetOrientation();
	const float					MeshScale		= pTransform->GetScale();
	const Vector				MeshScaleVector	= Vector( MeshScale, MeshScale, MeshScale );
	const Matrix				MeshMatrix		= Matrix::CreateScale( MeshScaleVector ) * MeshOrientation.ToMatrix() * Matrix::CreateTranslation( MeshLocation );

	bool						InitMass		= false;

	if( BoneIndex >= m_Masses.Size() )
	{
		// Lazily fill mass array
		SRagdollMass&	Mass		= m_Masses.PushBack();
		InitMass					= true;

		const Map<HashedString, float>::Iterator MassRadiusIter = m_MassRadii.Search( BoneInfo.m_Name );
		const float		MassRadius	= MassRadiusIter.IsValid() ? MassRadiusIter.GetValue() : m_DefaultMassRadius;
		Mass.m_HalfExtents			= Vector( MassRadius, MassRadius, MassRadius );

		if( BoneInfo.m_ParentIndex >= 0 )
		{
			// Add a spring to the parent
			SRagdollSpring&	SpringA		= Mass.m_Springs.PushBack();
			SpringA.m_WSSpringLength	= ( BoneInfo.m_BoneEnd - BoneInfo.m_BoneStart ).Length() * MeshScale;
			SpringA.m_ParentMass		= BoneInfo.m_ParentIndex;

			const SBoneInfo&	ParentBoneInfo	= pMesh->GetBones()->GetBoneInfo( BoneInfo.m_ParentIndex );
			if( ParentBoneInfo.m_ParentIndex >= 0 )
			{
				// Add another spring to the parent's parent, if any; this should help prevent bodies folding on themselves
				SRagdollSpring&		SpringB			= Mass.m_Springs.PushBack();
				SpringB.m_WSSpringLength			= ( BoneInfo.m_BoneEnd - ParentBoneInfo.m_BoneStart ).Length() * MeshScale;
				SpringB.m_ParentMass				= ParentBoneInfo.m_ParentIndex;
			}

			const uint NumBones = pMesh->GetBones()->GetNumBones();
			for( uint OtherBoneIndex = 0; OtherBoneIndex < NumBones; ++OtherBoneIndex )
			{
				if( BoneIndex == OtherBoneIndex )
				{
					continue;
				}

				const SBoneInfo&	OtherBoneInfo	= pMesh->GetBones()->GetBoneInfo( OtherBoneIndex );
				if( OtherBoneInfo.m_ParentIndex != BoneInfo.m_ParentIndex )
				{
					continue;
				}

				// Add spring to the siblings, if any; this should help keep shoulders/hips/etc. at a reasonable distance
				SRagdollSpring&		SpringC			= Mass.m_Springs.PushBack();
				SpringC.m_WSSpringLength			= ( BoneInfo.m_BoneEnd - OtherBoneInfo.m_BoneEnd ).Length() * MeshScale;
				SpringC.m_ParentMass				= OtherBoneIndex;
			}
		}
		else
		{
			// Give the root a zero-extent spring to the mesh location
			// (see Tick for how this is handled; the Z is ignored)
			Mass.m_Springs.PushBack();
		}
	}
	else // Don't update bones when populating; the OS location hasn't been set yet, and we can wait a tick since this blends in anyway
	{
		Matrix			BoneMatrix		= InOutBone.m_Quat.ToMatrix();
		BoneMatrix.SetTranslationElements( InOutBone.m_Vector );
		const Matrix	InvPoseMatrix	= ( BoneMatrix * ParentBoneMatrix ).GetInverse();

		// This took me five hours to solve so I'll try to document it clearly.
		// There used to be a lot of subtractions to get the translation and rotation
		// deltas from bone start to bone end and from bone start to mass. Those
		// have gone away by actually putting everything into bone space, so that
		// bone start is the origin and the default orientation is no rotation.
		// The last piece of the puzzle was just getting the matrix multiplies
		// right... the mass needs to be put into bone space by canceling the
		// pose matrix (bone * parent bone), and the bone end is put into the
		// same space by multiplying by the inverse bind pose. One final gotcha
		// is that my quaternion product operator seems backward from what I
		// always expect. Anyway, it works now and bones have stopped rolling.
		//const Vector	BSBoneEnd		= BoneInfo.m_BoneEnd * BoneInfo.m_InvBindPose;			// Location of bone end, relative to this bone in current orientation. (This should be only Y+)
		const Vector	BSMass			= m_Masses[ BoneIndex ].m_OSLocation * InvPoseMatrix;	// Location of mass, relative to this bone in current orientation.
		const Angles	NewBoneAngles	= BSMass.ToAngles();									// Angular offset from forward vector (to bone end) to the mass.
		const Quat		BoneAdjustment	= NewBoneAngles.ToQuaternion();							// That, represented as a quaternion.
		const Quat		NewBoneQuat		= BoneAdjustment * InOutBone.m_Quat;					// That, applied to the bone (because my quaternion products are backwards or something, again).

		InOutBone.m_Quat				= InOutBone.m_Quat.SLERP( m_BlendInterp.GetValue(), NewBoneQuat );
	}

	// Update mass location from (updated) bones. (Actually happens in Tick now, as it should.)
	Matrix			BoneMatrix	= InOutBone.m_Quat.ToMatrix();
	BoneMatrix.SetTranslationElements( InOutBone.m_Vector );

	const Matrix	PoseMatrix	= ( BoneInfo.m_InvBindPose * BoneMatrix * ParentBoneMatrix );
	const Vector	OSMass		= BoneInfo.m_BoneEnd * PoseMatrix;

	SRagdollMass&	Mass		= m_Masses[ BoneIndex ];
	Mass.m_BoneOSLocation		= OSMass;

	if( InitMass )
	{
		// This is normally done in the tick while blending in;
		// just ensure it's initialized if we don't blend at all.
		// Note that this can put the mass inside geo!
		Mass.m_OSLocation	= OSMass;
		Mass.m_WSLocation	= OSMass * MeshMatrix;
	}
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaRagdoll::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	WBCompRosaMesh* const		pMesh			= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMesh );

	GetFramework()->GetRenderer()->DEBUGPrint(
		SimpleString::PrintF("%sActive: %d\nAsleep: %d", DebugRenderLineFeed().CStr(), m_Active ? 1 : 0, m_Asleep ? 1 : 0),
		pTransform->GetLocation(),
		GetFramework()->GetMainView(),
		GetFramework()->GetDisplay(),
		DEFAULT_FONT_TAG,
		ARGB_TO_COLOR( 255, 255, 255, 255 ),
		ARGB_TO_COLOR( 0, 0, 0, 0 ) );

	FOR_EACH_ARRAY( MassIter, m_Masses, SRagdollMass )
	{
		const SRagdollMass&	Mass		= MassIter.GetValue();

		GetFramework()->GetRenderer()->DEBUGDrawBox( Mass.m_WSLocation - Mass.m_HalfExtents, Mass.m_WSLocation + Mass.m_HalfExtents, ARGB_TO_COLOR( 255, 128, 255, 0 ) );

		Vector				BoneTranslation;
		Angles				BoneOrientation;
		pMesh->GetBoneTransform( MassIter.GetIndex(), BoneTranslation, BoneOrientation );
		GetFramework()->GetRenderer()->DEBUGDrawCoords( BoneTranslation, BoneOrientation, 0.1f, false );

		SimpleString		MassName	= "";
		const HashedString	MassHash	= pMesh->GetBoneName( MassIter.GetIndex() );	// This assumes masses and bones are 1:1, I may change that later?
		if( ReverseHash::IsRegistered( MassHash ) )
		{
			MassName					+= SimpleString::PrintF( "%s ", ReverseHash::ReversedHash( MassHash ).CStr() );
		}
		MassName					+= SimpleString::PrintF( "(%d)", MassIter.GetIndex() );
		GetFramework()->GetRenderer()->DEBUGPrint( MassName, Mass.m_WSLocation, GetFramework()->GetMainView(), GetFramework()->GetDisplay(), DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 255, 255 ), ARGB_TO_COLOR( 0, 0, 0, 0 ) );
	}
}
#endif

#define VERSION_EMPTY						0
#define VERSION_BASICS						1
#define VERSION_RAGDOLLMASS_WSLOCATION		2
#define VERSION_RAGDOLLMASS_HALFEXTENTS		3
#define VERSION_RAGDOLLMASS_BONEOSLOCATION	4
#define VERSION_ASLEEP						5
#define VERSION_ASLEEP_DEPR					6
#define VERSION_CURRENT						6

uint WBCompRosaRagdoll::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_Masses.Size()
	FOR_EACH_ARRAY( MassIter, m_Masses, SRagdollMass )
	{
		const SRagdollMass& Mass = MassIter.GetValue();
		Size += sizeof( Vector );	// m_OSLocation
		Size += sizeof( Vector );	// m_WSLocation
		Size += sizeof( Vector );	// m_WSVelocity
		Size += sizeof( float );	// m_HalfExtents (as radius)
		Size += 4;					// m_Springs.Size();
		Size += sizeof( SRagdollSpring ) * Mass.m_Springs.Size();
	}

	Size += 1;									// m_Active

	Size += sizeof( Interpolator<float> );		// m_BlendInterp

	return Size;
}

void WBCompRosaRagdoll::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_Masses.Size() );
	FOR_EACH_ARRAY( MassIter, m_Masses, SRagdollMass )
	{
		const SRagdollMass& Mass = MassIter.GetValue();
		Stream.Write<Vector>( Mass.m_OSLocation );
		Stream.Write<Vector>( Mass.m_WSLocation );
		Stream.Write<Vector>( Mass.m_BoneOSLocation );
		Stream.Write<Vector>( Mass.m_WSVelocity );
		Stream.WriteFloat( Mass.m_HalfExtents.x );
		Stream.WriteArray( Mass.m_Springs );
	}

	Stream.WriteBool( m_Active );

	Stream.Write<Interpolator<float> >( m_BlendInterp );
}

void WBCompRosaRagdoll::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_BASICS )
	{
		const uint NumMasses = Stream.ReadUInt32();
		m_Masses.Reserve( NumMasses );
		m_Masses.Clear();
		FOR_EACH_INDEX( MassIndex, NumMasses )
		{
			SRagdollMass& Mass	= m_Masses.PushBack();
			Stream.Read<Vector>( Mass.m_OSLocation );
			if( Version >= VERSION_RAGDOLLMASS_WSLOCATION )
			{
				Stream.Read<Vector>( Mass.m_WSLocation );
			}
			if( Version >= VERSION_RAGDOLLMASS_BONEOSLOCATION )
			{
				Stream.Read<Vector>( Mass.m_BoneOSLocation );
			}
			Stream.Read<Vector>( Mass.m_WSVelocity );
			if( Version >= VERSION_RAGDOLLMASS_HALFEXTENTS )
			{
				// Not serialized, but we need to fix these up
				const float MassRadius	= Stream.ReadFloat();
				Mass.m_HalfExtents		= Vector( MassRadius, MassRadius, MassRadius );
			}
			Stream.ReadArray( Mass.m_Springs );
		}

		m_Active = Stream.ReadBool();
		if( Version >= VERSION_ASLEEP && Version < VERSION_ASLEEP_DEPR )
		{
			Stream.ReadBool();
		}

		m_BlendInterp = Stream.Read<Interpolator<float> >();
	}
}
