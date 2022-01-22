#include "core.h"
#include "ellipsoid.h"
#include "collisioninfo.h"
#include "aabb.h"
#include "triangle.h"
#include "sphere.h"
#include "ray.h"
#include "segment.h"

Ellipsoid::Ellipsoid()
:	m_Center( Vector( 0.0f, 0.0f, 0.0f ) ),
	m_Extents( Vector( 0.0f, 0.0f, 0.0f ) ) {}

Ellipsoid::Ellipsoid( const Vector& Center, const Vector& Extents )
:	m_Center( Center ),
	m_Extents( Extents ) {}

AABB Ellipsoid::GetAABB() const
{
	return AABB( m_Center - m_Extents, m_Center + m_Extents );
}

bool Ellipsoid::Intersects( const Ray& r, CollisionInfo* const pInfo ) const
{
	if( m_Extents.AnyZeros() )
	{
		return false;
	}

	Vector InvExtents = 1.0f / m_Extents;
	Vector ScaledDirection = r.m_Direction * InvExtents;
	Ray ScaledRay( r.m_Point * InvExtents, ScaledDirection.GetNormalized() );
	if( Sphere( m_Center * InvExtents, 1.0f ).Intersects( ScaledRay, pInfo ) )
	{
		if( pInfo )
		{
			pInfo->m_Out_HitT			/= ScaledDirection.Length();
			pInfo->m_Out_Intersection	*= m_Extents;
			pInfo->m_Out_Plane			= Plane( ( pInfo->m_Out_Plane.m_Normal * InvExtents ).GetNormalized(), pInfo->m_Out_Intersection );
		}
		return true;
	}
	return false;
}

bool Ellipsoid::Intersects( const Segment& s, CollisionInfo* const pInfo ) const
{
	Vector InvExtents = 1.0f / m_Extents;
	Segment ScaledSegment( s.m_Point1 * InvExtents, s.m_Point2 * InvExtents );
	if( Sphere( m_Center * InvExtents, 1.0f ).Intersects( ScaledSegment, pInfo ) )
	{
		if( pInfo )
		{
			pInfo->m_Out_Intersection	*= m_Extents;
			pInfo->m_Out_Plane			= Plane( ( pInfo->m_Out_Plane.m_Normal * InvExtents ).GetNormalized(), pInfo->m_Out_Intersection );
		}
		return true;
	}
	return false;
}

bool Ellipsoid::Intersects( const Ellipsoid& e ) const
{
	// Reduce this ellipsoid to a point, and expand the other one to the sum,
	// scale so that the summed ellipsoid is a sphere, and test intersection
	Ellipsoid Sum( e.m_Center, m_Extents + e.m_Extents );
	Vector InvExtents = 1.0f / Sum.m_Extents;
	return Sphere( Sum.m_Center * InvExtents, 1.0f ).Intersects( m_Center * InvExtents );
}

bool Ellipsoid::Intersects( const Triangle& t, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	Vector InvExtents = 1.0f / m_Extents;
	Triangle ScaledTriangle( t.m_Vec1 * InvExtents, t.m_Vec2 * InvExtents, t.m_Vec3 * InvExtents );
	if( Sphere( m_Center * InvExtents, 1.0f ).Intersects( ScaledTriangle, pInfo ) )
	{
		if( pInfo )
		{
			pInfo->m_Out_Intersection	*= m_Extents;
			pInfo->m_Out_Plane			= Plane( ( pInfo->m_Out_Plane.m_Normal * InvExtents ).GetNormalized(), pInfo->m_Out_Intersection );
		}
		return true;
	}
	return false;
}

bool Ellipsoid::Intersects( const AABB& a ) const
{
	// Transform everything into "ellipsoid space" (which just amounts
	// to a scale by the ellipsoid extents).
	const Vector InvExtents = 1.0f / m_Extents;
	const AABB ESpaceAABB( a.m_Min * InvExtents, a.m_Max * InvExtents );
	return Sphere( m_Center * InvExtents, 1.0f ).Intersects( ESpaceAABB );
}

bool Ellipsoid::SweepAgainst( const Ellipsoid& e, const Vector& v, CollisionInfo* const pInfo ) const
{
	Ellipsoid Sum( e.m_Center, m_Extents + e.m_Extents );
	Vector InvExtents = 1.0f / Sum.m_Extents;
	Vector ScaledCenter = m_Center * InvExtents;
	Segment Velocity( ScaledCenter, ScaledCenter + v * InvExtents );
	if( Sphere( Sum.m_Center * InvExtents, 1.0f ).Intersects( Velocity, pInfo ) )
	{
		if( pInfo )
		{
			pInfo->m_Out_Intersection	*= Sum.m_Extents;	// This isn't really correct; it returns the center of this ellipsoid at time of intersection
			pInfo->m_Out_Plane			= Plane( ( pInfo->m_Out_Plane.m_Normal * InvExtents ).GetNormalized(), pInfo->m_Out_Intersection );
		}
		return true;
	}
	return false;
}

bool Ellipsoid::SweepAgainst( const AABB& a, const Vector& v ) const
{
	// Transform everything into "ellipsoid space" (which just amounts
	// to a scale by the ellipsoid extents).
	const Vector InvExtents = 1.0f / m_Extents;
	const AABB ESpaceAABB( a.m_Min * InvExtents, a.m_Max * InvExtents );
	return Sphere( m_Center * InvExtents, 1.0f ).SweepAgainst( ESpaceAABB, v * InvExtents );
}
