#include "core.h"
#include "wbcomprosafrobber.h"
#include "wbeventmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosafrobbable.h"
#include "wbcomprosacamera.h"
#include "wbcomponentarrays.h"
#include "configmanager.h"
#include "segment.h"
#include "aabb.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "rosaframework.h"
#include "inputsystem.h"
#include "idatastream.h"

WBCompRosaFrobber::WBCompRosaFrobber()
:	m_FrobDistance( 0.0f )
,	m_FrobTarget()
,	m_DisableRefCount( 0 )
,	m_ForcedTarget()
,	m_AimDistance( 0.0f )
,	m_AimTarget()
,	m_AimTargetBone()
{
}

WBCompRosaFrobber::~WBCompRosaFrobber()
{
}

/*virtual*/ void WBCompRosaFrobber::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( FrobDistance );
	m_FrobDistance = ConfigManager::GetInheritedFloat( sFrobDistance, 0.0f, sDefinitionName );

	STATICHASH( AimDistance );
	m_AimDistance = ConfigManager::GetInheritedFloat( sAimDistance, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaFrobber::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	// Handle frob target
	{
		WBEntity* const pOldTarget = m_FrobTarget.Get();
		WBEntity* const pNewTarget = FindTargetFrobbable();

		if( pOldTarget && pOldTarget != pNewTarget )
		{
			OnUnsetFrobTarget( pOldTarget );
		}

		if( pNewTarget && pNewTarget != pOldTarget )
		{
			OnSetFrobTarget( pNewTarget );
		}

		m_FrobTarget = pNewTarget;
	}

	// Handle aim target
	{
		WBEntity* const	pOldTarget = m_AimTarget.Get();
		WBEntity*		pNewTarget;
		HashedString	NewTargetBone;
		FindAimTarget( pNewTarget, NewTargetBone );

		if( pOldTarget && pOldTarget != pNewTarget )
		{
			OnUnsetAimTarget( pOldTarget );
		}

		if( pNewTarget && pNewTarget != pOldTarget )
		{
			OnSetAimTarget( pNewTarget );
		}

		m_AimTarget		= pNewTarget;
		m_AimTargetBone	= NewTargetBone;
	}
}

void WBCompRosaFrobber::OnSetFrobTarget( WBEntity* const pFrobTarget )
{
	WBCompRosaFrobbable* const pFrobbable = WB_GETCOMP( pFrobTarget, RosaFrobbable );
	ASSERT( pFrobbable );
	pFrobbable->SetIsFrobTarget( true, GetEntity() );
}

void WBCompRosaFrobber::OnUnsetFrobTarget( WBEntity* const pFrobTarget )
{
	WBCompRosaFrobbable* const pFrobbable = WB_GETCOMP( pFrobTarget, RosaFrobbable );
	ASSERT( pFrobbable );
	pFrobbable->SetIsFrobTarget( false, GetEntity() );
}

void WBCompRosaFrobber::OnSetAimTarget( WBEntity* const pAimTarget )
{
	WBCompRosaFrobbable* const pFrobbable = WB_GETCOMP( pAimTarget, RosaFrobbable );
	ASSERT( pFrobbable );
	pFrobbable->SetIsAimTarget( true );
}

void WBCompRosaFrobber::OnUnsetAimTarget( WBEntity* const pAimTarget )
{
	WBCompRosaFrobbable* const pFrobbable = WB_GETCOMP( pAimTarget, RosaFrobbable );
	ASSERT( pFrobbable );
	pFrobbable->SetIsAimTarget( false );
}

/*virtual*/ void WBCompRosaFrobber::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnFrob );
	STATIC_HASHED_STRING( EnableFrob );
	STATIC_HASHED_STRING( DisableFrob );
	STATIC_HASHED_STRING( SetForcedTarget );
	STATIC_HASHED_STRING( ClearForcedTarget );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnFrob )
	{
		STATIC_HASHED_STRING( Input );
		const uint Input = Event.GetInt( sInput );

		TryFrob( Input );
	}
	else if( EventName == sEnableFrob )
	{
		DEVASSERT( m_DisableRefCount > 0 );
		if( m_DisableRefCount > 0 )
		{
			--m_DisableRefCount;
		}
	}
	else if( EventName == sDisableFrob )
	{
		m_DisableRefCount++;
	}
	else if( EventName == sSetForcedTarget )
	{
		STATIC_HASHED_STRING( ForcedTarget );
		m_ForcedTarget = Event.GetEntity( sForcedTarget );
	}
	else if( EventName == sClearForcedTarget )
	{
#if BUILD_DEV
		// Validate that we're clearing the actual forced target
		STATIC_HASHED_STRING( ForcedTarget );
		WBEntity* const pForcedTarget = Event.GetEntity( sForcedTarget );
		DEVASSERT( pForcedTarget == m_ForcedTarget.Get() );
#endif

		m_ForcedTarget = static_cast<WBEntity*>( NULL );
	}
}

/*virtual*/ void WBCompRosaFrobber::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Entity,	FrobTarget,		m_FrobTarget );
	WB_SET_CONTEXT( Event, Entity,	AimTarget,		m_AimTarget );
	WB_SET_CONTEXT( Event, Hash,	AimTargetBone,	m_AimTargetBone );
}

void WBCompRosaFrobber::TryFrob( const uint Input )
{
	WBEntity* const pTargetFrobbable = m_FrobTarget.Get();

#if BUILD_DEBUG
	if( pTargetFrobbable != NULL )
	{
		WBEntity* const pFoundTargetFrobbable = FindTargetFrobbable();
		ASSERT( pTargetFrobbable == pFoundTargetFrobbable );
	}
#endif

	if( pTargetFrobbable )
	{
		WB_MAKE_EVENT( MarshalFrob, pTargetFrobbable );
		WB_SET_AUTO( MarshalFrob, Entity, Frobber, GetEntity() );
		WB_SET_AUTO( MarshalFrob, Int, Input, Input );
		WB_DISPATCH_EVENT( GetEventManager(), MarshalFrob, pTargetFrobbable );

		// Untarget the frob target so we'll refresh the frob overlay on the next tick.
		OnUnsetFrobTarget( pTargetFrobbable );
		m_FrobTarget = static_cast<WBEntity*>( NULL );
	}
	else if( INPUT_TEST( Input, INPUT_ONRISE ) )
	{
		// HACKHACK: Convert an unfocused frob input to a reload, for more ideal controller mapping
		WB_MAKE_EVENT( TryReload, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), TryReload, GetEntity() );
	}
}

WBEntity* WBCompRosaFrobber::FindTargetFrobbable() const
{
	if( m_DisableRefCount > 0 )
	{
		return NULL;
	}

	InputSystem* const pInputSystem = GetFramework()->GetInputSystem();
	STATIC_HASHED_STRING( Frob );
	if( pInputSystem->IsSuppressed( sFrob ) )
	{
		return NULL;
	}

	if( m_ForcedTarget )
	{
		return m_ForcedTarget.Get();
	}

	const Array<WBCompRosaFrobbable*>* const	pFrobbablesArrays	= WBComponentArrays::GetComponents<WBCompRosaFrobbable>();
	if( !pFrobbablesArrays )
	{
		return NULL;
	}

	const Array<WBCompRosaFrobbable*>&			Frobbables			= *pFrobbablesArrays;

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCamera* const		pCamera		= WB_GETCOMP( pEntity, RosaCamera );

	Vector			StartLocation		= pTransform->GetLocation();
	if( pCamera )
	{
		pCamera->ModifyTranslation( WBCompRosaCamera::EVM_All, StartLocation );
	}
	const Vector	Direction			= pTransform->GetOrientation().ToVector();
	const Vector	EndLocation			= StartLocation + Direction * m_FrobDistance;
	const Segment	FrobTraceSegment	= Segment( StartLocation, EndLocation );

	// First, trace against world and collidables as a baseline for our trace distance.
	CollisionInfo CollidableInfo;
	CollidableInfo.m_In_CollideWorld	= true;
	CollidableInfo.m_In_CollideEntities	= true;
	CollidableInfo.m_In_UserFlags		= EECF_EntityCollision;

	GetWorld()->Trace( FrobTraceSegment, CollidableInfo );
	const float CollidableT = CollidableInfo.m_Out_HitT;

	WBEntity*	pBestEntity	= NULL;
	float		NearestT	= 0.0f;
	int			MinPriority	= 0;

	const uint NumFrobbables = Frobbables.Size();
	for( uint FrobbableIndex = 0; FrobbableIndex < NumFrobbables; ++FrobbableIndex )
	{
		WBCompRosaFrobbable* const pFrobbable = Frobbables[ FrobbableIndex ];
		DEBUGASSERT( pFrobbable );

		if( !pFrobbable->IsFrobbable() )
		{
			continue;
		}

		// We can skip anything that would not take precedence
		const int FrobPriority = pFrobbable->GetFrobPriority();
		if( pBestEntity && FrobPriority > MinPriority )
		{
			continue;
		}

		WBEntity* const pFrobbableEntity = pFrobbable->GetEntity();
		DEBUGASSERT( pFrobbableEntity );

		if( pFrobbableEntity->IsDestroyed() )
		{
			continue;
		}

		WBCompRosaTransform* const pFrobbableTransform = pFrobbableEntity->GetTransformComponent<WBCompRosaTransform>();
		DEBUGASSERT( pFrobbableTransform );

		const AABB		FrobbableBox			= pFrobbable->GetBound();
		const Vector	FrobbableCenter			= FrobbableBox.GetCenter();
		const Vector	FrobbableOrientation	= pFrobbableTransform->GetOrientation().ToVector();
		const Vector	TargetToFrobber			= ( StartLocation - FrobbableCenter ).GetFastNormalized();
		const float		CosAngleToFrobber		= TargetToFrobber.Dot( FrobbableOrientation );
		const float		CosFrobAngleLow			= pFrobbable->GetCosFrobAngleLow();
		const float		CosFrobAngleHigh		= pFrobbable->GetCosFrobAngleHigh();

		if( CosAngleToFrobber > CosFrobAngleLow ||
			CosAngleToFrobber < CosFrobAngleHigh )
		{
			continue;
		}

		CollisionInfo Info;
		if( !FrobTraceSegment.Intersects( FrobbableBox, &Info ) )
		{
			continue;
		}

		// Check if trace was blocked by some collision
		if( Info.m_Out_HitT > CollidableT )
		{
			continue;
		}

		if( pBestEntity == NULL ||
			FrobPriority < MinPriority ||
			Info.m_Out_HitT < NearestT )
		{
			pBestEntity	= pFrobbableEntity;
			NearestT	= Info.m_Out_HitT;
			MinPriority	= FrobPriority;
		}
	}

	return pBestEntity;
}

void WBCompRosaFrobber::GetAimTarget( WBEntity*& pOutAimTarget, HashedString& OutAimTargetBone )
{
	pOutAimTarget		= m_AimTarget.Get();
	OutAimTargetBone	= m_AimTargetBone;
}

void WBCompRosaFrobber::FindAimTarget( WBEntity*& pOutAimTarget, HashedString& OutAimTargetBone ) const
{
	pOutAimTarget		= NULL;
	OutAimTargetBone	= HashedString::NullString;

	if( m_DisableRefCount > 0 )
	{
		return;
	}

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCamera* const		pCamera		= WB_GETCOMP( pEntity, RosaCamera );


	Vector			StartLocation	= pTransform->GetLocation();
	if( pCamera )
	{
		pCamera->ModifyTranslation( WBCompRosaCamera::EVM_All, StartLocation );
	}
	const Vector	Direction		= pTransform->GetOrientation().ToVector();
	const Vector	EndLocation		= StartLocation + Direction * m_AimDistance;
	const Segment	AimTraceSegment	= Segment( StartLocation, EndLocation );

	CollisionInfo CollidableInfo;
	CollidableInfo.m_In_CollideWorld	= true;
	CollidableInfo.m_In_CollideEntities	= true;
	CollidableInfo.m_In_CollidingEntity	= pEntity;
	CollidableInfo.m_In_UserFlags		= EECF_Trace | EECF_CollideBones;	// Do a trace here so we only hit things a line check would hit

	if( !GetWorld()->Trace( AimTraceSegment, CollidableInfo ) )
	{
		return;
	}

	WBEntity* const pHitEntity = static_cast<WBEntity*>( CollidableInfo.m_Out_HitEntity );
	if( !pHitEntity )
	{
		return;
	}

	WBCompRosaFrobbable* const pFrobbable = WB_GETCOMP( pHitEntity, RosaFrobbable );
	if( !pFrobbable )
	{
		return;
	}

	if( !pFrobbable->CanBeAimedAt() )
	{
		return;
	}

	// All checks passed, we've found an aim target
	pOutAimTarget		= pHitEntity;
	OutAimTargetBone	= CollidableInfo.m_Out_HitName;
}

#define VERSION_EMPTY			0
#define VERSION_FORCEDTARGET	2
#define VERSION_CURRENT			2

uint WBCompRosaFrobber::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version

	Size += sizeof( WBEntityRef );	// m_ForcedTarget

	return Size;
}

void WBCompRosaFrobber::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.Write( sizeof( WBEntityRef ), &m_ForcedTarget );
}

void WBCompRosaFrobber::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_FORCEDTARGET )
	{
		Stream.Read( sizeof( WBEntityRef ), &m_ForcedTarget );
	}
}
