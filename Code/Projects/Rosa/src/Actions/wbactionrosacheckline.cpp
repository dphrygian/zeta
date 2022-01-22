#include "core.h"
#include "wbactionrosacheckline.h"
#include "configmanager.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacamera.h"
#include "Components/wbcomprosaheadtracker.h"
#include "Components/wbcomprosamesh.h"
#include "Components/wbcompowner.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "rosaframework.h"
#include "rosaworld.h"
#include "segment.h"
#include "ray.h"
#include "collisioninfo.h"
#include "mathfunc.h"

// Magic number to return a "safe" hit location pushed out by this much along the normal.
// This saves doing the multiply and add all over the place in Loom script.
// This doesn't seem like something I'd need to configure per line trace.
#define SAFE_HIT_LOCATION_OFFSET 0.125f

WBActionRosaCheckLine::WBActionRosaCheckLine()
:	m_EntityPE()
,	m_LineStartPE()
,	m_LineDirectionPE()
,	m_LineLength( 0.0f )
,	m_LineLengthPE()
,	m_CheckTag()
,	m_CollideEntities( false )
,	m_BlastNumTracesPE()
,	m_BlastAnglePE()
{
}

WBActionRosaCheckLine::~WBActionRosaCheckLine()
{
}

/*virtual*/ void WBActionRosaCheckLine::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( LineStartPE );
	const SimpleString LineStartPE = ConfigManager::GetString( sLineStartPE, "", sDefinitionName );
	m_LineStartPE.InitializeFromDefinition( LineStartPE );

	STATICHASH( LineDirectionPE );
	const SimpleString LineDirectionPE = ConfigManager::GetString( sLineDirectionPE, "", sDefinitionName );
	m_LineDirectionPE.InitializeFromDefinition( LineDirectionPE );

	STATICHASH( LineLength );
	m_LineLength = ConfigManager::GetFloat( sLineLength, 0.0f, sDefinitionName );

	STATICHASH( LineLengthPE );
	const SimpleString LineLengthPE = ConfigManager::GetString( sLineLengthPE, "", sDefinitionName );
	m_LineLengthPE.InitializeFromDefinition( LineLengthPE );

	STATICHASH( CheckTag );
	m_CheckTag = ConfigManager::GetHash( sCheckTag, HashedString::NullString, sDefinitionName );

	STATICHASH( CollideEntities );
	m_CollideEntities = ConfigManager::GetBool( sCollideEntities, true, sDefinitionName );

	STATICHASH( BlastNumTracesPE );
	const SimpleString BlastNumTracesPE = ConfigManager::GetString( sBlastNumTracesPE, "", sDefinitionName );
	m_BlastNumTracesPE.InitializeFromDefinition( BlastNumTracesPE );

	STATICHASH( BlastAnglePE );
	const SimpleString BlastAnglePE = ConfigManager::GetString( sBlastAnglePE, "", sDefinitionName );
	m_BlastAnglePE.InitializeFromDefinition( BlastAnglePE );
}

// Copied from WBActionRosaSpawnEntity::GetSpawnTransform. For reals, unify this stuff somewhere.
void WBActionRosaCheckLine::GetLineTransform( WBEntity* const pEntity, Vector& OutLocation, Angles& OutOrientation )
{
	DEVASSERT( pEntity );
	WBCompRosaTransform* const		pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	WBCompRosaCamera* const			pCamera			= WB_GETCOMP( pEntity, RosaCamera );
	WBCompRosaHeadTracker* const	pHeadTracker	= WB_GETCOMP( pEntity, RosaHeadTracker );

	// Evaluate line PEs in the context of the EntityPE, not (necessarily) action owner
	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;

	m_LineStartPE.Evaluate( PEContext );
	m_LineDirectionPE.Evaluate( PEContext );
	m_LineLengthPE.Evaluate( PEContext );

	// Get location
	{
		if( m_LineStartPE.IsEvaluated() )
		{
			OutLocation			= m_LineStartPE.GetVector();
		}
		else if( pHeadTracker )
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
		if( m_LineDirectionPE.IsEvaluated() )
		{
			OutOrientation		= m_LineDirectionPE.GetAngles();
		}
		else if( pHeadTracker )
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

void WBActionRosaCheckLine::ExecuteSingleTrace()
{
	WBEntity* const			pActionEntity	= GetEntity();
	WBEntity* const			pOwnerEntity	= GetTopmostOwner();
	RosaWorld* const		pWorld			= RosaFramework::GetInstance()->GetWorld();
	WBEventManager* const	pEventManager	= GetEventManager();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pActionEntity;
	m_EntityPE.Evaluate( PEContext );
	WBEntity* const pSourceEntity = m_EntityPE.IsEvaluated() ? m_EntityPE.GetEntity() : pOwnerEntity;

	Vector					LineStart;
	Angles					LineOrientation;
	GetLineTransform( pSourceEntity, LineStart, LineOrientation );
	const Vector			LineDirection	= LineOrientation.ToVector();
	const float				LineLength		= m_LineLengthPE.IsEvaluated() ? m_LineLengthPE.GetFloat() : m_LineLength;

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= m_CollideEntities;
	Info.m_In_CollidingEntity	= pOwnerEntity;
	Info.m_In_UserFlags			= EECF_Trace | EECF_CollideBones;

	if( LineLength > 0.0f )
	{
		const Vector	LineEnd			= LineStart + LineDirection * LineLength;
		const Segment	TraceSegment	= Segment( LineStart, LineEnd );
		pWorld->Trace( TraceSegment, Info );
	}
	else
	{
		const Ray		TraceRay		= Ray( LineStart, LineDirection );
		pWorld->Trace( TraceRay, Info );
	}

	if( Info.m_Out_Collision )
	{
		const Vector	HitLocation		= Info.m_Out_Intersection;
		const Vector	HitNormal		= Info.m_Out_Plane.m_Normal;
		const Vector	SafeHitLocation	= HitLocation + ( HitNormal * SAFE_HIT_LOCATION_OFFSET );

		// Notify this entity that the line check hit that entity.
		if( Info.m_Out_HitEntity )
		{
			WBEntity* const			pHitEntity	= static_cast<WBEntity*>( Info.m_Out_HitEntity );
			WBCompRosaMesh* const	pMesh		= WB_GETCOMP( pHitEntity, RosaMesh );

			// If there's a valid m_HitName in Info, use that instead of doing GetNearestBone
			const bool				HasHitName	= ( Info.m_Out_HitName != HashedString::NullString );
			const HashedString		HitBone		= HasHitName ? Info.m_Out_HitName : ( pMesh ? pMesh->GetNearestBoneName( HitLocation ) : HashedString::NullString );

			WB_MAKE_EVENT( OnLineCheck, pActionEntity );
			WB_SET_AUTO( OnLineCheck, Hash,		CheckTag,			m_CheckTag );
			WB_SET_AUTO( OnLineCheck, Entity,	Checked,			pHitEntity );
			WB_SET_AUTO( OnLineCheck, Vector,	LineStart,			LineStart );
			WB_SET_AUTO( OnLineCheck, Vector,	LineDirection,		LineDirection );
			WB_SET_AUTO( OnLineCheck, Vector,	HitLocation,		HitLocation );
			WB_SET_AUTO( OnLineCheck, Vector,	SafeHitLocation,	SafeHitLocation );
			WB_SET_AUTO( OnLineCheck, Vector,	HitNormal,			HitNormal );
			WB_SET_AUTO( OnLineCheck, Hash,		HitBone,			HitBone );
			WB_SET_AUTO( OnLineCheck, Float,	HitT,				Info.m_Out_HitT );
			WB_DISPATCH_EVENT( pEventManager, OnLineCheck, pActionEntity );
		}
		else
		{
			WB_MAKE_EVENT( OnLineCheckHitGeo, pActionEntity );
			WB_SET_AUTO( OnLineCheckHitGeo, Hash,	CheckTag,			m_CheckTag );
			WB_SET_AUTO( OnLineCheckHitGeo, Vector,	LineStart,			LineStart );
			WB_SET_AUTO( OnLineCheckHitGeo, Vector,	LineDirection,		LineDirection );
			WB_SET_AUTO( OnLineCheckHitGeo, Vector,	HitLocation,		HitLocation );
			WB_SET_AUTO( OnLineCheckHitGeo, Vector,	SafeHitLocation,	SafeHitLocation );
			WB_SET_AUTO( OnLineCheckHitGeo, Vector,	HitNormal,			HitNormal );
			WB_SET_AUTO( OnLineCheckHitGeo, Float,	HitT,				Info.m_Out_HitT );
			WB_SET_AUTO( OnLineCheckHitGeo, Hash,	HitSurface,			Info.m_Out_UserFlags );
			WB_DISPATCH_EVENT( pEventManager, OnLineCheckHitGeo, pActionEntity );
		}
	}
	else
	{
		WB_MAKE_EVENT( OnLineCheckMissed, pActionEntity );
		WB_SET_AUTO( OnLineCheckMissed, Hash,	CheckTag,		m_CheckTag );
		WB_SET_AUTO( OnLineCheckMissed, Vector,	LineStart,		LineStart );
		WB_SET_AUTO( OnLineCheckMissed, Vector,	LineDirection,	LineDirection );
		WB_DISPATCH_EVENT( pEventManager, OnLineCheckMissed, pActionEntity );
	}
}

void WBActionRosaCheckLine::ExecuteBlastTraces()
{
	WBEntity* const			pActionEntity	= GetEntity();
	WBEntity* const			pOwnerEntity	= GetTopmostOwner();
	RosaWorld* const		pWorld			= RosaFramework::GetInstance()->GetWorld();
	WBEventManager* const	pEventManager	= GetEventManager();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pActionEntity;
	m_EntityPE.Evaluate( PEContext );
	WBEntity* const pSourceEntity = m_EntityPE.IsEvaluated() ? m_EntityPE.GetEntity() : pOwnerEntity;

	// Evaluate blast PEs in the context of the action owner, unlike line PEs which may be another entity (e.g. player owner of a weapon)
	m_BlastNumTracesPE.Evaluate( PEContext );
	m_BlastAnglePE.Evaluate( PEContext );

	Vector					LineStart;
	Angles					LineOrientation;
	GetLineTransform( pSourceEntity, LineStart, LineOrientation );
	const Vector			LineDirection	= LineOrientation.ToVector();
	const float				LineLength		= m_LineLengthPE.IsEvaluated() ? m_LineLengthPE.GetFloat() : m_LineLength;
	const uint				BlastNumTraces	= m_BlastNumTracesPE.GetInt();
	const float				BlastAngle		= DEGREES_TO_RADIANS( m_BlastAnglePE.GetFloat() );

	Array<CollisionInfo>			BlastInfos;
	Array<Vector>					BlastDirections;
	Array<uint>						BlastMisses;		// Index into Infos/BlastDirections
	Array<uint>						BlastGeoHits;		// Same
	Map<WBEntity*, Array<uint> >	BlastEntityHits;	// Map entity to all their hitting infos

	DEVASSERT( BlastNumTraces > 0 );
	FOR_EACH_INDEX( BlastIndex, BlastNumTraces )
	{
		// ROSATODO: This part! Do proper sampling.
		// Even distribution of points in a unit circle is theta = rand(0, 2pi), r = sqrt(rand(0,1));
		// I may or may not be able to use that given how I'm still getting line direction above.
		const Vector	BlastRight			= LineDirection.Cross( Vector::Up );
		const Matrix	RotationA			= Matrix::CreateRotation( BlastRight, Math::Random( -BlastAngle, BlastAngle ) );
		const Matrix	RotationB			= Matrix::CreateRotation( LineDirection, Math::Random( 0.0f, TWOPI ) );
		const Vector	RotatedDirection	= ( LineDirection * RotationA ) * RotationB;
		const Vector	BlastDirection		= ( BlastIndex == 0 ) ? LineDirection : RotatedDirection;	// First shot always goes dead center
		BlastDirections.PushBack( BlastDirection );

		CollisionInfo& Info			= BlastInfos.PushBack();
		Info.m_In_CollideWorld		= true;
		Info.m_In_CollideEntities	= m_CollideEntities;
		Info.m_In_CollidingEntity	= pOwnerEntity;
		Info.m_In_UserFlags			= EECF_Trace | EECF_CollideBones;

		if( LineLength > 0.0f )
		{
			const Vector	LineEnd			= LineStart + BlastDirection * LineLength;
			const Segment	TraceSegment	= Segment( LineStart, LineEnd );
			pWorld->Trace( TraceSegment, Info );
		}
		else
		{
			const Ray		TraceRay		= Ray( LineStart, BlastDirection );
			pWorld->Trace( TraceRay, Info );
		}

		if( Info.m_Out_Collision )
		{
			if( Info.m_Out_HitEntity )
			{
				WBEntity* const	pHitEntity	= static_cast<WBEntity*>( Info.m_Out_HitEntity );
				BlastEntityHits[ pHitEntity ].PushBack( BlastIndex );
			}
			else
			{
				BlastGeoHits.PushBack( BlastIndex );
			}
		}
		else
		{
			BlastMisses.PushBack( BlastIndex );
		}
	}

	FOR_EACH_ARRAY( BlastMissIter, BlastMisses, uint )
	{
		const uint				BlastIndex		= BlastMissIter.GetValue();
		const Vector&			BlastDirection	= BlastDirections[ BlastIndex ];
		//const CollisionInfo&	Info			= BlastInfos[ BlastIndex ];

		WB_MAKE_EVENT( OnLineCheckBlastMissed, pActionEntity );
		WB_SET_AUTO( OnLineCheckBlastMissed, Hash,		CheckTag,		m_CheckTag );
		WB_SET_AUTO( OnLineCheckBlastMissed, Vector,	LineStart,		LineStart );
		WB_SET_AUTO( OnLineCheckBlastMissed, Vector,	LineDirection,	BlastDirection );
		WB_DISPATCH_EVENT( pEventManager, OnLineCheckBlastMissed, pActionEntity );
	}

	FOR_EACH_ARRAY( BlastGeoHitIter, BlastGeoHits, uint )
	{
		const uint				BlastIndex		= BlastGeoHitIter.GetValue();
		const Vector&			BlastDirection	= BlastDirections[ BlastIndex ];
		const CollisionInfo&	Info			= BlastInfos[ BlastIndex ];

		const Vector			HitLocation		= Info.m_Out_Intersection;
		const Vector			HitNormal		= Info.m_Out_Plane.m_Normal;
		const Vector			SafeHitLocation	= HitLocation + ( HitNormal * SAFE_HIT_LOCATION_OFFSET );

		WB_MAKE_EVENT( OnLineCheckBlastHitGeo, pActionEntity );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Hash,		CheckTag,			m_CheckTag );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Vector,	LineStart,			LineStart );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Vector,	LineDirection,		BlastDirection );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Vector,	HitLocation,		HitLocation );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Vector,	SafeHitLocation,	SafeHitLocation );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Vector,	HitNormal,			HitNormal );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Float,		HitT,				Info.m_Out_HitT );
		WB_SET_AUTO( OnLineCheckBlastHitGeo, Hash,		HitSurface,			Info.m_Out_UserFlags );
		WB_DISPATCH_EVENT( pEventManager, OnLineCheckBlastHitGeo, pActionEntity );
	}

	FOR_EACH_MAP( BlastEntityHitsIter, BlastEntityHits, WBEntity*, Array<uint> )
	{
		const Array<uint>&	BlastHits	= BlastEntityHitsIter.GetValue();

		// First iterate over the infos separately
		FOR_EACH_ARRAY( BlastHitIter, BlastHits, uint )
		{
			const uint				BlastIndex		= BlastHitIter.GetValue();
			const Vector&			BlastDirection	= BlastDirections[ BlastIndex ];
			const CollisionInfo&	Info			= BlastInfos[ BlastIndex ];

			const Vector			HitLocation		= Info.m_Out_Intersection;
			const Vector			HitNormal		= Info.m_Out_Plane.m_Normal;
			const Vector			SafeHitLocation	= HitLocation + ( HitNormal * SAFE_HIT_LOCATION_OFFSET );

			WBEntity* const			pHitEntity		= static_cast<WBEntity*>( Info.m_Out_HitEntity );
			WBCompRosaMesh* const	pMesh			= WB_GETCOMP( pHitEntity, RosaMesh );
			DEVASSERT( pHitEntity == BlastEntityHitsIter.GetKey() );

			// If there's a valid m_HitName in Info, use that instead of doing GetNearestBone
			const bool				HasHitName	= ( Info.m_Out_HitName != HashedString::NullString );
			const HashedString		HitBone		= HasHitName ? Info.m_Out_HitName : ( pMesh ? pMesh->GetNearestBoneName( HitLocation ) : HashedString::NullString );

			WB_MAKE_EVENT( OnLineCheckBlast, pActionEntity );
			WB_SET_AUTO( OnLineCheckBlast, Hash,	CheckTag,			m_CheckTag );
			WB_SET_AUTO( OnLineCheckBlast, Entity,	Checked,			pHitEntity );
			WB_SET_AUTO( OnLineCheckBlast, Vector,	LineStart,			LineStart );
			WB_SET_AUTO( OnLineCheckBlast, Vector,	LineDirection,		BlastDirection );
			WB_SET_AUTO( OnLineCheckBlast, Vector,	HitLocation,		HitLocation );
			WB_SET_AUTO( OnLineCheckBlast, Vector,	SafeHitLocation,	SafeHitLocation );
			WB_SET_AUTO( OnLineCheckBlast, Vector,	HitNormal,			HitNormal );
			WB_SET_AUTO( OnLineCheckBlast, Hash,	HitBone,			HitBone );
			WB_SET_AUTO( OnLineCheckBlast, Float,	HitT,				Info.m_Out_HitT );
			WB_DISPATCH_EVENT( pEventManager, OnLineCheckBlast, pActionEntity );
		}

		// Also send one summed event for this entity, with a fractional part to indicate amount of damage to do.
		// Use the first hit for location/normal/etc.
		// But just ignore HitBone for this, there's no clear answer to that problem.
		{
			const float				BlastPct		= static_cast<float>( BlastHits.Size() ) / static_cast<float>( BlastNumTraces );
			const uint				FirstBlastIndex	= BlastHits[0];
			const Vector&			BlastDirection	= BlastDirections[ FirstBlastIndex ];
			const CollisionInfo&	Info			= BlastInfos[ FirstBlastIndex ];

			const Vector			HitLocation		= Info.m_Out_Intersection;
			const Vector			HitNormal		= Info.m_Out_Plane.m_Normal;
			const Vector			SafeHitLocation	= HitLocation + ( HitNormal * SAFE_HIT_LOCATION_OFFSET );

			WBEntity* const			pHitEntity		= static_cast<WBEntity*>( Info.m_Out_HitEntity );
			DEVASSERT( pHitEntity == BlastEntityHitsIter.GetKey() );

			WB_MAKE_EVENT( OnLineCheckBlastSum, pActionEntity );
			WB_SET_AUTO( OnLineCheckBlastSum, Hash,		CheckTag,			m_CheckTag );
			WB_SET_AUTO( OnLineCheckBlastSum, Entity,	Checked,			pHitEntity );
			WB_SET_AUTO( OnLineCheckBlastSum, Vector,	LineStart,			LineStart );
			WB_SET_AUTO( OnLineCheckBlastSum, Vector,	LineDirection,		BlastDirection );
			WB_SET_AUTO( OnLineCheckBlastSum, Vector,	HitLocation,		HitLocation );
			WB_SET_AUTO( OnLineCheckBlastSum, Vector,	SafeHitLocation,	SafeHitLocation );
			WB_SET_AUTO( OnLineCheckBlastSum, Vector,	HitNormal,			HitNormal );
			WB_SET_AUTO( OnLineCheckBlastSum, Float,	HitT,				Info.m_Out_HitT );
			WB_SET_AUTO( OnLineCheckBlastSum, Float,	BlastPercent,		BlastPct );
			WB_DISPATCH_EVENT( pEventManager, OnLineCheckBlastSum, pActionEntity );
		}
	}
}

/*virtual*/ void WBActionRosaCheckLine::Execute()
{
	WBAction::Execute();

	if( m_BlastNumTracesPE.IsInitialized() )
	{
		ExecuteBlastTraces();
	}
	else
	{
		ExecuteSingleTrace();
	}
}
