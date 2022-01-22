#include "core.h"
#include "wbcomprosasignreader.h"
#include "wbeventmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosasign.h"
#include "wbcomprosacamera.h"
#include "wbcomponentarrays.h"
#include "configmanager.h"
#include "segment.h"
#include "ray.h"
#include "aabb.h"
#include "collisioninfo.h"
#include "rosaworld.h"
#include "rosaframework.h"
#include "inputsystem.h"

WBCompRosaSignReader::WBCompRosaSignReader()
:	m_SignTarget()
{
}

WBCompRosaSignReader::~WBCompRosaSignReader()
{
}

/*virtual*/ void WBCompRosaSignReader::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	WBEntity* const pOldTarget = m_SignTarget.Get();
	WBEntity* const pNewTarget = FindTargetSign();

	if( pOldTarget && pOldTarget != pNewTarget )
	{
		OnUnsetSignTarget( pOldTarget );
	}

	if( pNewTarget && pNewTarget != pOldTarget )
	{
		OnSetSignTarget( pNewTarget );
	}

	m_SignTarget = pNewTarget;
}

void WBCompRosaSignReader::OnSetSignTarget( WBEntity* const pSignTarget )
{
	WBCompRosaSign* const pSign = WB_GETCOMP( pSignTarget, RosaSign );
	ASSERT( pSign );
	pSign->SetIsSignTarget( true );
}

void WBCompRosaSignReader::OnUnsetSignTarget( WBEntity* const pSignTarget )
{
	WBCompRosaSign* const pSign = WB_GETCOMP( pSignTarget, RosaSign );
	ASSERT( pSign );
	pSign->SetIsSignTarget( false );
}

WBEntity* WBCompRosaSignReader::FindTargetSign() const
{
	const Array<WBCompRosaSign*>* const pSignsArrays = WBComponentArrays::GetComponents<WBCompRosaSign>();
	if( !pSignsArrays )
	{
		return NULL;
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
	const Ray		SignTraceRay	= Ray( StartLocation, Direction );

	// First, trace against world and collidables as a baseline for our trace distance.
	CollisionInfo CollidableInfo;
	CollidableInfo.m_In_CollideWorld	= true;
	CollidableInfo.m_In_CollideEntities	= true;
	CollidableInfo.m_In_UserFlags		= EECF_EntityCollision;

	RosaWorld* const	pWorld	= GetWorld();
	pWorld->Trace( SignTraceRay, CollidableInfo );
	const float CollidedDistance	= CollidableInfo.m_Out_HitT * pWorld->GetRayTraceLength();

	const Array<WBCompRosaSign*>& Signs = *pSignsArrays;

	WBEntity*	pBestEntity	= NULL;
	float		NearestT	= 0.0f;

	const uint NumSigns = Signs.Size();
	for( uint SignIndex = 0; SignIndex < NumSigns; ++SignIndex )
	{
		WBCompRosaSign* const pSign = Signs[ SignIndex ];
		DEBUGASSERT( pSign );

		if( !pSign->IsReadable() )
		{
			continue;
		}

		WBEntity* const pSignEntity = pSign->GetEntity();
		DEBUGASSERT( pSignEntity );

		if( pSignEntity->IsDestroyed() )
		{
			continue;
		}

		const AABB		SignBox				= pSign->GetBound();
		const float		ReadDistance		= pSign->GetReadDistance();
		const Vector	EndLocation			= StartLocation + Direction * ReadDistance;
		const Segment	SignTraceSegment	= Segment( StartLocation, EndLocation );

		CollisionInfo Info;
		if( !SignTraceSegment.Intersects( SignBox, &Info ) )
		{
			continue;
		}

		// Check if trace was blocked by some collision
		const float		SignDistance		= Info.m_Out_HitT * ReadDistance;
		if( SignDistance > CollidedDistance )
		{
			continue;
		}

		if( pBestEntity == NULL ||
			Info.m_Out_HitT < NearestT )
		{
			pBestEntity	= pSignEntity;
			NearestT	= Info.m_Out_HitT;
		}
	}

	return pBestEntity;
}
