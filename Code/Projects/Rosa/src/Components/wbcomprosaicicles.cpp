#include "core.h"
#include "wbcomprosaicicles.h"
#include "configmanager.h"
#include "wbcomprosaanchor.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "rosaworld.h"
#include "wbeventmanager.h"
#include "collisioninfo.h"
#include "rosagame.h"
#include "Components/wbcompstatmod.h"

WBCompRosaIcicles::WBCompRosaIcicles()
:	m_CheckDistance( 0.0f )
{
}

WBCompRosaIcicles::~WBCompRosaIcicles()
{
}

/*virtual*/ void WBCompRosaIcicles::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( CheckDistance );
	m_CheckDistance = ConfigManager::GetInheritedFloat( sCheckDistance, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaIcicles::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnTouched );

	const HashedString EventName = Event.GetEventName();
	// TODO
	Unused( EventName );
}

/*virtual*/ void WBCompRosaIcicles::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	WBEntity* const				pEntity		= GetEntity();
	ASSERT( pEntity );

	WBCompRosaAnchor* const		pAnchor		= WB_GETCOMP( pEntity, RosaAnchor );
	ASSERT( pAnchor );

	if( !pAnchor->IsAnchored() )
	{
		return;
	}

	WBEntity* const				pPlayer		= RosaGame::GetPlayer();

	if( !pPlayer )
	{
		return;
	}

	WBCompStatMod* const		pPlayerStatMod		= WB_GETCOMP( pPlayer, StatMod );
	ASSERT( pPlayerStatMod );

	WB_MODIFY_FLOAT( IgnoreIcicles, 0.0f, pPlayerStatMod );
	const bool IgnoreIcicles = ( WB_MODDED( IgnoreIcicles ) != 0.0f );

	if( IgnoreIcicles )
	{
		return;
	}

	RosaWorld* const		pWorld				= GetWorld();
	ASSERT( pWorld );

	WBCompRosaTransform* const	pTransform			= pEntity->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pTransform );

	WBCompRosaCollision* const	pCollision			= WB_GETCOMP( pEntity, RosaCollision );
	ASSERT( pCollision );

	WBCompRosaTransform* const	pPlayerTransform	= pPlayer->GetTransformComponent<WBCompRosaTransform>();
	ASSERT( pPlayerTransform );

	WBCompRosaCollision* const	pPlayerCollision	= WB_GETCOMP( pPlayer, RosaCollision );
	ASSERT( pPlayerCollision );

	const Vector	CheckOffset	= Vector( 0.0f, 0.0f, -m_CheckDistance );
	const Vector	TraceStart	= pTransform->GetLocation();
	Vector			TraceEnd	= TraceStart + CheckOffset;

	// Early out if player doesn't intersect the trace bounds.
	Vector			EntityExtents	= pCollision->GetExtents();
	EntityExtents.z					= 0.0f;
	const AABB		TraceBox		= AABB( TraceEnd - EntityExtents, TraceStart + EntityExtents );
	const AABB		PlayerBox		= pPlayerCollision->GetBounds();
	if( !TraceBox.Intersects( PlayerBox ) )
	{
		return;
	}

	// Move trace up to the top of player so we don't check occlusion beyond player.
	const float	PlayerTopZ	= PlayerBox.m_Max.z;
	TraceEnd.z				= PlayerTopZ;

	// Do an occlusion test to make sure there's nothing blocking the trace.
	CollisionInfo Info;
	Info.m_In_CollideWorld			= true;
	Info.m_In_CollideEntities		= true;
	Info.m_In_CollidingEntity		= pEntity;
	Info.m_In_UserFlags				= EECF_Occlusion;
	Info.m_In_StopAtAnyCollision	= true;

	const bool Occluded				= pWorld->LineCheck( TraceStart, TraceEnd, Info );
	if( Occluded )
	{
		return;
	}

	// All checks passed. Unanchor to initiate falling sequence
	WB_MAKE_EVENT( Unanchor, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), Unanchor, pEntity );
}
