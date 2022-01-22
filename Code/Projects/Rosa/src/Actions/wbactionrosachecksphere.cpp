#include "core.h"
#include "wbactionrosachecksphere.h"
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

WBActionRosaCheckSphere::WBActionRosaCheckSphere()
:	m_EntityPE()
,	m_RadiusSq( 0.0f )
,	m_RadiusRcp( 0.0f )
,	m_TraceCenter( false )
,	m_CheckTag()
{
}

WBActionRosaCheckSphere::~WBActionRosaCheckSphere()
{
}

/*virtual*/ void WBActionRosaCheckSphere::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( Radius );
	const float Radius = ConfigManager::GetFloat( sRadius, 0.0f, sDefinitionName );
	m_RadiusSq	= Square( Radius );
	m_RadiusRcp	= 1.0f / Radius;

	STATICHASH( TraceCenter );
	m_TraceCenter = ConfigManager::GetBool( sTraceCenter, false, sDefinitionName );

	STATICHASH( CheckTag );
	m_CheckTag = ConfigManager::GetHash( sCheckTag, HashedString::NullString, sDefinitionName );
}

// Borrowed (with simplifications) from WBActionRosaSpawnEntity::GetSpawnTransform. Maybe unify?
void WBActionRosaCheckSphere::GetSphereTransform( WBEntity* const pEntity, Vector& OutLocation ) const
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
}

/*virtual*/ void WBActionRosaCheckSphere::Execute()
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
	WBEventManager* const	pEventManager	= GetEventManager();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pActionEntity;
	m_EntityPE.Evaluate( PEContext );
	WBEntity* const pSourceEntity = ( m_EntityPE.GetType() == WBParamEvaluator::EPT_Entity ) ? m_EntityPE.GetEntity() : pOwnerEntity;

	Vector					SphereSource;
	GetSphereTransform( pSourceEntity, SphereSource );

	const uint NumCollidables = pCollidables->Size();
	for( uint CollidableIndex = 0; CollidableIndex < NumCollidables; ++CollidableIndex )
	{
		WBCompRosaCollision* const	pCollision				= ( *pCollidables )[ CollidableIndex ];
		WBEntity* const				pCollidableEntity		= pCollision->GetEntity();
		if( pCollidableEntity == pOwnerEntity )
		{
			continue;
		}

		// Check distance to entity's nearest point.
		const AABB					CollidableBounds		= pCollision->GetBounds();
		const Vector				CollidableLocation		= m_TraceCenter ? CollidableBounds.GetCenter() : CollidableBounds.GetClosestPoint( SphereSource );
		const Vector				CollidableOffset		= ( CollidableLocation - SphereSource );
		const float					DistanceSq				= CollidableOffset.LengthSquared();
		if( DistanceSq > m_RadiusSq )
		{
			continue;
		}

		const Vector				CollidableDirection		= CollidableOffset.GetFastNormalized();

		// Check collision to entity's nearest point.
		const Segment				TraceSegment			= Segment( SphereSource, CollidableLocation );
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
		const float				HitT	= CollidableOffset.Length() * m_RadiusRcp;	// This is *not* Info.m_Out_HitT!

		// All checks passed, notify this entity that the sphere check hit that entity.
		WB_MAKE_EVENT( OnSphereCheck, pActionEntity );
		WB_SET_AUTO( OnSphereCheck, Entity,	Checked,		pCollidableEntity );
		WB_SET_AUTO( OnSphereCheck, Hash,	CheckTag,		m_CheckTag );
		WB_SET_AUTO( OnSphereCheck, Vector, HitLocation,	CollidableLocation );
		WB_SET_AUTO( OnSphereCheck, Vector, LineDirection,	CollidableDirection );
		WB_SET_AUTO( OnSphereCheck, Hash,	HitBone,		HitBone );
		WB_SET_AUTO( OnSphereCheck, Float,	HitT,			HitT );
		WB_DISPATCH_EVENT( pEventManager, OnSphereCheck, pActionEntity );
	}

	// Send an event that we're done iterating, for mark-and-sweep patterns
	WB_MAKE_EVENT( OnSphereCheckFinished, pActionEntity );
	WB_SET_AUTO( OnSphereCheckFinished, Hash, CheckTag, m_CheckTag );
	WB_DISPATCH_EVENT( pEventManager, OnSphereCheckFinished, pActionEntity );
}
