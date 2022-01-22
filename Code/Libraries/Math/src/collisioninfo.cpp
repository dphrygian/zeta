#include "core.h"
#include "collisioninfo.h"

CollisionInfo::CollisionInfo()
:	m_In_CollideWorld( false )
,	m_In_CollideEntities( false )
,	m_In_StopAtAnyCollision( false )
,	m_In_CollidingEntity( NULL )
,	m_In_UserFlags( 0 )
,	m_Out_Collision( false )
,	m_Out_Plane()
,	m_Out_Intersection()
,	m_Out_HitT( 1.0f )	// 30 Mar 2013: Changed from 0 to 1 so a trace that doesn't collide has a max T. Shouldn't cause problems, but just in case.
,	m_Out_HitEntity( NULL )
,	m_Out_HitName( HashedString::NullString )
,	m_Out_UserFlags( 0 )
{
}

void CollisionInfo::ResetInParameters()
{
	m_In_CollideWorld		= false;
	m_In_CollideEntities	= false;
	m_In_StopAtAnyCollision	= false;
	m_In_CollidingEntity	= NULL;
	m_In_UserFlags			= 0;
}

void CollisionInfo::ResetOutParameters()
{
	m_Out_Collision		= false;
	m_Out_Plane			= Plane();
	m_Out_Intersection	= Vector();
	m_Out_HitT			= 1.0f;
	m_Out_HitEntity		= NULL;
	m_Out_HitName		= HashedString::NullString;
	m_Out_UserFlags		= 0;
}

void CollisionInfo::CopyInParametersFrom( const CollisionInfo& OtherInfo )
{
	m_In_CollideWorld		= OtherInfo.m_In_CollideWorld;
	m_In_CollideEntities	= OtherInfo.m_In_CollideEntities;
	m_In_StopAtAnyCollision	= OtherInfo.m_In_StopAtAnyCollision;
	m_In_CollidingEntity	= OtherInfo.m_In_CollidingEntity;
	m_In_UserFlags			= OtherInfo.m_In_UserFlags;
}

void CollisionInfo::CopyOutParametersFrom( const CollisionInfo& OtherInfo )
{
	m_Out_Collision		= OtherInfo.m_Out_Collision;
	m_Out_Plane			= OtherInfo.m_Out_Plane;
	m_Out_Intersection	= OtherInfo.m_Out_Intersection;
	m_Out_HitT			= OtherInfo.m_Out_HitT;
	m_Out_HitEntity		= OtherInfo.m_Out_HitEntity;
	m_Out_HitName		= OtherInfo.m_Out_HitName;
	m_Out_UserFlags		= OtherInfo.m_Out_UserFlags;
}
