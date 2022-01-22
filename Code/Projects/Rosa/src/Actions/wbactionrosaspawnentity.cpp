#include "core.h"
#include "wbactionrosaspawnentity.h"
#include "configmanager.h"
#include "angles.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacamera.h"
#include "Components/wbcomprosaheadtracker.h"
#include "Components/wbcompowner.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcomprosalinkedentities.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "rosaframework.h"
#include "rosaworld.h"
#include "collisioninfo.h"
#include "rosaworldgen.h"

WBActionRosaSpawnEntity::WBActionRosaSpawnEntity()
:	m_EntityDef()
,	m_EntityDefPE()
,	m_LocationPE()
,	m_OrientationPE()
,	m_UseHeadTracker( false )
,	m_YawOnly( false )
,	m_SpawnImpulseZ( 0.0f )
,	m_SpawnImpulse( 0.0f )
,	m_SpawnImpulsePE()
,	m_SpawnOffsetZ( 0.0f )
{
}

WBActionRosaSpawnEntity::~WBActionRosaSpawnEntity()
{
}

/*virtual*/ void WBActionRosaSpawnEntity::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityDef = ConfigManager::GetString( sEntity, "", sDefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityDefPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityDefPE.InitializeFromDefinition( EntityDefPE );

	STATICHASH( LocationPE );
	const SimpleString LocationPE = ConfigManager::GetString( sLocationPE, "", sDefinitionName );
	m_LocationPE.InitializeFromDefinition( LocationPE );

	STATICHASH( OrientationPE );
	const SimpleString OrientationPE = ConfigManager::GetString( sOrientationPE, "", sDefinitionName );
	m_OrientationPE.InitializeFromDefinition( OrientationPE );

	STATICHASH( UseHeadTracker );
	m_UseHeadTracker = ConfigManager::GetBool( sUseHeadTracker, false, sDefinitionName );

	STATICHASH( YawOnly );
	m_YawOnly = ConfigManager::GetBool( sYawOnly, false, sDefinitionName );

	STATICHASH( SpawnImpulseZ );
	m_SpawnImpulseZ = ConfigManager::GetFloat( sSpawnImpulseZ, 0.0f, sDefinitionName );

	STATICHASH( SpawnImpulse );
	m_SpawnImpulse = ConfigManager::GetFloat( sSpawnImpulse, 0.0f, sDefinitionName );

	STATICHASH( SpawnImpulsePE );
	const SimpleString SpawnImpulsePE = ConfigManager::GetString( sSpawnImpulsePE, "", sDefinitionName );
	m_SpawnImpulsePE.InitializeFromDefinition( SpawnImpulsePE );

	STATICHASH( SpawnOffsetZ );
	m_SpawnOffsetZ = ConfigManager::GetFloat( sSpawnOffsetZ, 0.0f, sDefinitionName );
}

// Borrowed from WBCompRosaItem spawn drop code. Maybe unify?
/*virtual*/ void WBActionRosaSpawnEntity::Execute()
{
	WBAction::Execute();

	WBEntity* const					pEntity				= GetEntity();

	WBParamEvaluator::SPEContext	PEContext;
	PEContext.m_Entity = pEntity;
	m_EntityDefPE.Evaluate( PEContext );
	const SimpleString				EntityDef			= ( m_EntityDefPE.GetType() == WBParamEvaluator::EPT_String ) ? m_EntityDefPE.GetString() : m_EntityDef;

	WBEntity* const					pSpawnedEntity		= WBWorld::GetInstance()->CreateEntity( EntityDef );

	// Let subclasses handle the entity too
	PostSpawn( pSpawnedEntity );
}

/*virtual*/ void WBActionRosaSpawnEntity::PostSpawn( WBEntity* const pSpawnedEntity )
{
	if( NULL == pSpawnedEntity )
	{
		return;
	}

	WBEntity* const					pOwnerEntity		= GetTopmostOwner();

	WBCompRosaTransform* const		pSpawnedTransform	= pSpawnedEntity->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pSpawnedTransform );

	WBCompOwner* const				pSpawnedOwner		= pSpawnedEntity->GetOwnerComponent<WBCompOwner>();

	WBCompRosaLinkedEntities* const	pLinkedEntities		= WB_GETCOMP( pOwnerEntity, RosaLinkedEntities );

	if( pSpawnedOwner )
	{
		pSpawnedOwner->SetOwner( pOwnerEntity );
	}

	Vector SpawnLocation;
	Vector SpawnImpulse;
	Angles SpawnOrientation;
	GetSpawnTransform( pSpawnedEntity, SpawnLocation, SpawnImpulse, SpawnOrientation );

	pSpawnedTransform->SetInitialTransform( SpawnLocation, SpawnOrientation );
	pSpawnedTransform->ApplyImpulse( SpawnImpulse );

	if( pLinkedEntities && pLinkedEntities->GetLinkedEntities().Size() )
	{
		WB_MAKE_EVENT( SetLinkedEntities, NULL );
		WB_SET_AUTO( SetLinkedEntities, Pointer, LinkedEntities, const_cast<Array<WBEntityRef>*>( &pLinkedEntities->GetLinkedEntities() ) );
		WB_DISPATCH_EVENT( GetEventManager(), SetLinkedEntities, pSpawnedEntity );
	}

	// Notify instigator that we spawned this thing
	WB_MAKE_EVENT( OnSpawnedEntityAction, GetEntity() );
	WB_SET_AUTO( OnSpawnedEntityAction, Entity, SpawnedEntity, pSpawnedEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnSpawnedEntityAction, GetEntity() );
}

// Borrowed (with simplifications) from WBCompRosaItem spawn drop code. Maybe unify?
void WBActionRosaSpawnEntity::GetSpawnTransform( WBEntity* const pSpawnedEntity, Vector& OutLocation, Vector& OutImpulse, Angles& OutOrientation )
{
	WBEntity* const					pActionEntity	= GetEntity();
	DEVASSERT( pActionEntity );

	WBEntity* const					pOwnerEntity	= GetTopmostOwner();
	DEVASSERT( pOwnerEntity );

	WBCompRosaTransform* const		pTransform		= pOwnerEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCamera* const			pCamera			= WB_GETCOMP( pOwnerEntity, RosaCamera );

	WBCompRosaHeadTracker* const	pHeadTracker	= WB_GETCOMP( pOwnerEntity, RosaHeadTracker );
	const bool						UseHeadTracker	= m_UseHeadTracker && pHeadTracker;

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pActionEntity;
	m_LocationPE.Evaluate( PEContext );
	m_OrientationPE.Evaluate( PEContext );

	// Get location
	{
		if( m_LocationPE.HasRoot() )
		{
			OutLocation	= m_LocationPE.GetVector();
		}
		else if( UseHeadTracker )
		{
			OutLocation	= pHeadTracker->GetEyesLocation();
		}
		else
		{
			OutLocation	= pTransform->GetLocation();
			if( pCamera )
			{
				pCamera->ModifyTranslation( WBCompRosaCamera::EVM_All, OutLocation );
			}
		}

		OutLocation.z			+= m_SpawnOffsetZ;

		WBCompRosaCollision* const pCollision = WB_GETCOMP( pSpawnedEntity, RosaCollision );
		if( pCollision )
		{
			CollisionInfo Info;
			Info.m_In_CollideWorld		= true;
			Info.m_In_CollideEntities	= true;
			Info.m_In_CollidingEntity	= pOwnerEntity;	// Using the owner, not the spawned entity (which should be at origin at the moment)
			Info.m_In_UserFlags			= EECF_EntityCollision;

			RosaWorld* const pWorld = RosaFramework::GetInstance()->GetWorld();
			pWorld->FindSpot( OutLocation, pCollision->GetExtents(), Info );
		}
	}

	// Get orientation
	{
		if( m_OrientationPE.HasRoot() )
		{
			OutOrientation	= m_OrientationPE.GetAngles();
		}
		else if( UseHeadTracker )
		{
			OutOrientation	= pHeadTracker->GetLookDirection().ToAngles();
		}
		else
		{
			OutOrientation	= pTransform->GetOrientation();
			if( pCamera )
			{
				pCamera->ModifyOrientation( WBCompRosaCamera::EVM_All, OutOrientation );
			}
		}

		if( m_YawOnly )
		{
			OutOrientation.Pitch	= 0.0f;
			OutOrientation.Roll		= 0.0f;
		}
	}

	// Get impulse
	{
		if( UseHeadTracker )
		{
			OutImpulse			= pHeadTracker->GetLookDirection();
		}
		else
		{
			OutImpulse			= OutOrientation.ToVector();
		}

		OutImpulse.z			+= m_SpawnImpulseZ;
		OutImpulse.FastNormalize();

		m_SpawnImpulsePE.Evaluate( PEContext );
		const float SpawnImpulseSize = ( m_SpawnImpulsePE.GetType() == WBParamEvaluator::EPT_Float ) ? m_SpawnImpulsePE.GetFloat() : m_SpawnImpulse;
		OutImpulse *= SpawnImpulseSize;
	}
}
