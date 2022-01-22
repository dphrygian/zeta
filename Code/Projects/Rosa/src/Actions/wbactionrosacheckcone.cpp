#include "core.h"
#include "wbactionrosacheckcone.h"
#include "configmanager.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcomprosacamera.h"
#include "Components/wbcomprosaheadtracker.h"
#include "Components/wbcomprosamesh.h"
#include "Components/wbcompowner.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "mathcore.h"
#include "wbcomponentarrays.h"
#include "rosaframework.h"
#include "rosaworld.h"
#include "segment.h"
#include "collisioninfo.h"

WBActionRosaCheckCone::WBActionRosaCheckCone()
:	m_EntityPE()
,	m_ConeCosTheta( 0.0f )
,	m_ConeLengthSq( 0.0f )
,	m_TraceCenter( false )
,	m_CheckTag()
{
}

WBActionRosaCheckCone::~WBActionRosaCheckCone()
{
}

/*virtual*/ void WBActionRosaCheckCone::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( ConeAngle );
	m_ConeCosTheta = Cos( DEGREES_TO_RADIANS( ConfigManager::GetFloat( sConeAngle, 0.0f, sDefinitionName ) ) );

	STATICHASH( ConeLength );
	m_ConeLengthSq = Square( ConfigManager::GetFloat( sConeLength, 0.0f, sDefinitionName ) );

	STATICHASH( TraceCenter );
	m_TraceCenter = ConfigManager::GetBool( sTraceCenter, false, sDefinitionName );

	STATICHASH( CheckTag );
	m_CheckTag = ConfigManager::GetHash( sCheckTag, HashedString::NullString, sDefinitionName );
}

// Borrowed (with simplifications) from WBActionRosaSpawnEntity::GetSpawnTransform. Maybe unify?
void WBActionRosaCheckCone::GetConeTransform( WBEntity* const pEntity, Vector& OutLocation, Angles& OutOrientation ) const
{
	DEVASSERT( pEntity );
	WBCompRosaTransform* const		pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	WBCompRosaCamera* const			pCamera			= WB_GETCOMP( pEntity, RosaCamera );
	WBCompRosaHeadTracker* const	pHeadTracker	= WB_GETCOMP( pEntity, RosaHeadTracker );

	// Get location
	{
		if( pHeadTracker )
		{
			OutLocation			= pHeadTracker->GetEyesLocation();
		}
		else
		{
			OutLocation			= pTransform->GetLocation();
			if( pCamera )
			{
				pCamera->ModifyTranslation( WBCompRosaCamera::EVM_All, OutLocation );
			}
		}
	}

	// Get orientation
	{
		if( pHeadTracker )
		{
			OutOrientation		= pHeadTracker->GetLookDirection().ToAngles();
		}
		else
		{
			OutOrientation		= pTransform->GetOrientation();
			if( pCamera )
			{
				pCamera->ModifyOrientation( WBCompRosaCamera::EVM_All, OutOrientation );
			}
		}
	}
}

/*virtual*/ void WBActionRosaCheckCone::Execute()
{
	WBAction::Execute();

	const Array<WBCompRosaCollision*>* const pCollidables = WBComponentArrays::GetComponents<WBCompRosaCollision>();
	if( !pCollidables )
	{
		return;
	}

	WBEntity* const			pActionEntity	= GetEntity();
	WBEntity* const			pOwnerEntity	= GetTopmostOwner();
	RosaWorld* const		pWorld			= RosaFramework::GetInstance()->GetWorld();
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pActionEntity;
	m_EntityPE.Evaluate( PEContext );
	WBEntity* const pSourceEntity = ( m_EntityPE.GetType() == WBParamEvaluator::EPT_Entity ) ? m_EntityPE.GetEntity() : pOwnerEntity;

	Vector					ConeSource;
	Angles					ConeOrientation;
	GetConeTransform( pSourceEntity, ConeSource, ConeOrientation );
	const Vector			ConeDirection	= ConeOrientation.ToVector();

	const uint NumCollidables = pCollidables->Size();
	for( uint CollidableIndex = 0; CollidableIndex < NumCollidables; ++CollidableIndex )
	{
		WBCompRosaCollision* const	pCollision				= ( *pCollidables )[ CollidableIndex ];
		WBEntity* const				pCollidableEntity		= pCollision->GetEntity();
		if( pCollidableEntity == pOwnerEntity )
		{
			continue;
		}

		// Cone check should only hit things that a line check would hit (trace blockers).
		// But since I don't actually do a trace (except to find occlusion), early out if
		// this collidable doesn't block traces.
		if( !pCollision->MatchesAllCollisionFlags( EECF_BlocksTrace ) )
		{
			continue;
		}

		// Check distance to entity's nearest point.
		const AABB					CollidableBounds		= pCollision->GetBounds();
		const Vector				CollidableLocation		= m_TraceCenter ? CollidableBounds.GetCenter() : CollidableBounds.GetClosestPoint( ConeSource );
		const Vector				CollidableOffset		= ( CollidableLocation - ConeSource );
		const float					DistanceSq				= CollidableOffset.LengthSquared();
		if( DistanceSq > m_ConeLengthSq )
		{
			continue;
		}

		// Check angle to entity's nearest point. (Skip check if distance to nearest point is zero.)
		// NOTE: This check isn't really sufficient; if ConeDirection is tangential to the surface,
		// the nearest point will be perpendicular to ConeDirection even though some of the surface
		// may fall inside the cone. For now, I'm working around this in data by only using half-
		// sphere 90-degree "cones". (Or, picture standing next to a very large object; the nearest
		// point is not necessarily inside the cone even though part of the object may be inside.)
		const Vector				CollidableDirection		= CollidableOffset.GetFastNormalized();
		const float					CosTheta				= CollidableDirection.Dot( ConeDirection );
		if( DistanceSq > 0.0f && CosTheta < m_ConeCosTheta )
		{
			continue;
		}

		// Check collision to entity's nearest point.
		const Segment				TraceSegment			= Segment( ConeSource, CollidableLocation );
		CollisionInfo Info;
		Info.m_In_CollideWorld		= true;
		Info.m_In_CollideEntities	= true;
		Info.m_In_CollidingEntity	= pOwnerEntity;
		Info.m_In_UserFlags			= EECF_Trace;
		if( pWorld->Trace( TraceSegment, Info ) && Info.m_Out_HitEntity != pCollidableEntity )
		{
			continue;
		}

		WBCompRosaMesh* const	pMesh	= WB_GETCOMP( pCollidableEntity, RosaMesh );
		const HashedString		HitBone	= pMesh ? pMesh->GetNearestBoneName( CollidableLocation ) : HashedString::NullString;

		// All checks passed, notify this entity that the cone check hit that entity.
		WB_MAKE_EVENT( OnConeCheck, pActionEntity );
		WB_SET_AUTO( OnConeCheck, Entity,	Checked,		pCollidableEntity );
		WB_SET_AUTO( OnConeCheck, Hash,		CheckTag,		m_CheckTag );
		WB_SET_AUTO( OnConeCheck, Vector,	HitLocation,	CollidableLocation );
		WB_SET_AUTO( OnConeCheck, Vector,	HitNormal,		-CollidableDirection );	// HACKHACK since we don't have an actual normal
		WB_SET_AUTO( OnConeCheck, Vector,	LineDirection,	CollidableDirection );
		WB_SET_AUTO( OnConeCheck, Hash,		HitBone,		HitBone );
		WB_DISPATCH_EVENT( pEventManager, OnConeCheck, pActionEntity );
	}

	// Send an event that we're done iterating, for mark-and-sweep patterns
	WB_MAKE_EVENT( OnConeCheckFinished, pActionEntity );
	WB_SET_AUTO( OnConeCheckFinished, Hash, CheckTag, m_CheckTag );
	WB_DISPATCH_EVENT( pEventManager, OnConeCheckFinished, pActionEntity );
}
