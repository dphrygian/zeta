#include "core.h"
#include "segment.h"
#include "triangle.h"
#include "aabb.h"
#include "collisioninfo.h"
#include "mathcore.h"
#include "sphere.h"
#include "cylinder.h"
#include "ellipsoid.h"

Segment::Segment()
:	m_Point1()
,	m_Point2()
{
}

Segment::Segment( const Vector& Point1, const Vector& Point2 )
:	m_Point1( Point1 )
,	m_Point2( Point2 )
{
}

// From Real-Time Collision Detection, pp. 149-151
// See below for the version that returns the nearest point on both segments.
Vector Segment::NearestPointTo( const Segment& Other, float* pTValue /*= NULL*/ ) const
{
	const Vector	d1	= m_Point2			- m_Point1;
	const Vector	d2	= Other.m_Point2	- Other.m_Point1;
	const Vector	r	= m_Point1			- Other.m_Point1;

	const float		a	= d1.LengthSquared();
	const float		e	= d2.LengthSquared();
	const float		f	= d2.Dot( r );

	float			s	= 0.0f;

	// NOTE: Both segments degenerate test goes here, clipped
	// for my implementation because it's redundant.

	if( a >= SMALLER_EPSILON )	// Make sure first segment isn't degenerate
	{
		const float c = d1.Dot( r );
		if( e < SMALLER_EPSILON )
		{
			// Second segment is degenerate
			s = Saturate( -c / a );
		}
		else
		{
			// General non-degenerate case
			const float b = d1.Dot( d2 );
			const float Denom = ( a * e ) - ( b * b );

			// If segments are not parallel, compute closest point on this line to other
			// line and clamp to this segment. Else pick arbitrary place on line for now,
			// which we'll do by leaving s at 0.
			if( Denom != 0.0f )
			{
				s = Saturate( ( ( b * f ) - ( c * e ) ) / Denom );
			}

			// Now compute point on other line closest to this segment.
			const float tnom = ( b * s ) + f;
			if( tnom < 0.0f )
			{
				s = Saturate( -c / a );
			}
			else if( tnom > e )
			{
				s = Saturate( ( b - c ) / a );
			}
			// Else the s we computed above is still valid
		}
	}

	// Return the t-value if user needs it
	if( pTValue )
	{
		*pTValue = s;
	}

	return m_Point1 + ( d1 * s );
}

void Segment::NearestPointTo( const Segment& Other, Vector& OutPointOnThis, Vector& OutPointOnOther, float* pThisTValue /*= NULL*/, float* pOtherTValue /*= NULL*/ ) const
{
	const Vector	d1	= m_Point2			- m_Point1;
	const Vector	d2	= Other.m_Point2	- Other.m_Point1;
	const Vector	r	= m_Point1			- Other.m_Point1;

	const float		a	= d1.LengthSquared();
	const float		e	= d2.LengthSquared();
	const float		f	= d2.Dot( r );

	float			s	= 0.0f;
	float			t	= 0.0f;

	if( a < SMALLER_EPSILON && e < SMALLER_EPSILON )
	{
		// Both segments are degenerate (points)
		// Do nothing
	}
	else if( a < SMALLER_EPSILON )
	{
		// First segment is degenerate
		t = Saturate( f / e );
	}
	else
	{
		const float c = d1.Dot( r );
		if( e < SMALLER_EPSILON )
		{
			// Second segment is degenerate
			s = Saturate( -c / a );
		}
		else
		{
			// General non-degenerate case
			const float b = d1.Dot( d2 );
			const float Denom = ( a * e ) - ( b * b );

			// If segments are not parallel, compute closest point on this line to other
			// line and clamp to this segment. Else pick arbitrary place on line for now,
			// which we'll do by leaving s at 0.
			if( Denom != 0.0f )
			{
				s = Saturate( ( ( b * f ) - ( c * e ) ) / Denom );
			}

			// Now compute point on other line closest to this segment.
			const float tnom = ( b * s ) + f;
			if( tnom < 0.0f )
			{
				t = 0.0f;
				s = Saturate( -c / a );
			}
			else if( tnom > e )
			{
				t = 1.0f;
				s = Saturate( ( b - c ) / a );
			}
			else
			{
				t = tnom / e;
			}
		}
	}

	// Return the t-values if user needs them
	if( pThisTValue )
	{
		*pThisTValue = s;
	}
	if( pOtherTValue )
	{
		*pOtherTValue = t;
	}

	OutPointOnThis	= m_Point1			+ ( d1 * s );
	OutPointOnOther	= Other.m_Point1	+ ( d2 * t );
}

Vector Segment::NearestPointTo( const Vector& Point ) const
{
	const Vector	Extent	= m_Point2 - m_Point1;

	// If segment is zero-length, it is the nearest point
	if( Extent.LengthSquared() == 0.0f )
	{
		return m_Point1;
	}

	const float		T		= Saturate( ( Point - m_Point1 ).Dot( Extent ) / Extent.Dot( Extent ) );
	return m_Point1 + T * Extent;
}

float Segment::DistanceSqTo( const Vector& Point ) const
{
	const Vector NearestPoint = NearestPointTo( Point );
	return ( Point - NearestPoint ).LengthSquared();
}

bool Segment::Intersects( const Triangle& t, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	// Could be optimized
	// See Real-Time Collision Detection p. 191
	Vector v12 = t.m_Vec2 - t.m_Vec1;
	Vector v13 = t.m_Vec3 - t.m_Vec1;
	Vector Direction = m_Point1 - m_Point2;

	Vector TriNormal = v12.Cross( v13 );

	float d = Direction.Dot( TriNormal );

	// If denominator is zero, segment is parallel to triangle
	if( d == 0.0f )
	{
		return false;
	}

	// If denominator is less than zero, segment points away from triangle
	if( d < 0.0f )
	{
		return false;
	}

	Vector v1p = m_Point1 - t.m_Vec1;
	float dt = v1p.Dot( TriNormal );

	if( dt < 0.0f )
	{
		return false;
	}

	if( dt > d )
	{
		return false;
	}

	Vector e = Direction.Cross( v1p );
	float v = v13.Dot( e );

	if( v < 0.0f || v > d )
	{
		return false;
	}

	float w = -( v12.Dot( e ) );

	if( w < 0.0f || v + w > d )
	{
		return false;
	}

	if( pInfo )
	{
		pInfo->m_Out_Collision		= true;
		pInfo->m_Out_HitT			= dt / d;
		pInfo->m_Out_Intersection	= m_Point1 + pInfo->m_Out_HitT * ( m_Point2 - m_Point1 );
		pInfo->m_Out_Plane			= t.GetPlane();
	}

	return true;
}

// Separating-axis test
// See Real-Time Collision Detection p.183
bool Segment::Intersects( const AABB& a ) const
{
	const Vector BoxCenter = ( a.m_Min + a.m_Max ) * 0.5f;
	const Vector BoxHalfLengths = a.m_Max - BoxCenter;
	const Vector SegmentCenterWS = ( ( m_Point1 + m_Point2 ) * 0.5f );
	const Vector SegmentCenter = SegmentCenterWS - BoxCenter;	// Subtraction effectively translates objects to origin
	const Vector SegmentHalfLength = m_Point2 - SegmentCenterWS;

	float AbsHalfX = Abs( SegmentHalfLength.x );
	if( Abs( SegmentCenter.x ) > BoxHalfLengths.x + AbsHalfX )
	{
		return false;
	}

	float AbsHalfY = Abs( SegmentHalfLength.y );
	if( Abs( SegmentCenter.y ) > BoxHalfLengths.y + AbsHalfY )
	{
		return false;
	}

	float AbsHalfZ = Abs( SegmentHalfLength.z );
	if( Abs( SegmentCenter.z ) > BoxHalfLengths.z + AbsHalfZ )
	{
		return false;
	}

	AbsHalfX += EPSILON;
	AbsHalfY += EPSILON;
	AbsHalfZ += EPSILON;

	if( Abs( SegmentCenter.y * SegmentHalfLength.z - SegmentCenter.z * SegmentHalfLength.y ) >
		BoxHalfLengths.y * AbsHalfZ + BoxHalfLengths.z * AbsHalfY )
	{
		return false;
	}

	if( Abs( SegmentCenter.z * SegmentHalfLength.x - SegmentCenter.x * SegmentHalfLength.z ) >
		BoxHalfLengths.z * AbsHalfX + BoxHalfLengths.x * AbsHalfZ )
	{
		return false;
	}

	if( Abs( SegmentCenter.x * SegmentHalfLength.y - SegmentCenter.y * SegmentHalfLength.x ) >
		BoxHalfLengths.x * AbsHalfY + BoxHalfLengths.y * AbsHalfX )
	{
		return false;
	}

	return true;
}

// "Slab" test
// See Real-Time Collision Detection p.180
bool Segment::Intersects( const AABB& a, CollisionInfo* const pInfo ) const
{
	float tMin = 0.0f;
	float tMax = 1.0f;

	int IntersectingAxis = -1;

	const Vector dir = m_Point2 - m_Point1;

	// Maybe an optimization?
	static const Vector kUnits = Vector( 1.0f, 1.0f, 1.0f );
	const Vector OneOverD = kUnits / dir;

	for( uint i = 0; i < 3; ++i )
	{
		if( Abs( dir.v[i] ) < SMALLER_EPSILON )
		{
			// Segment is (close to) parallel to slab in ith dimension. No intersection if origin is outside slab.
			if( m_Point1.v[i] < a.m_Min.v[i] || m_Point1.v[i] > a.m_Max.v[i] )
			{
				// But warn if there might be an intersection that we're ignoring (maybe epsilon is too large)
				DEVASSERT(
					( m_Point1.v[i] < a.m_Min.v[i] && m_Point2.v[i] < a.m_Min.v[i] ) ||
					( m_Point1.v[i] > a.m_Max.v[i] && m_Point2.v[i] > a.m_Max.v[i] ) );
				return false;
			}
		}
		else
		{
			float t1 = ( a.m_Min.v[i] - m_Point1.v[i] ) * OneOverD.v[i];
			float t2 = ( a.m_Max.v[i] - m_Point1.v[i] ) * OneOverD.v[i];
			if( t1 > t2 )
			{
				Swap( t1, t2 );
			}

			tMin = Max( tMin, t1 );
			tMax = Min( tMax, t2 );

			if( tMin == t1 )
			{
				IntersectingAxis = i;
			}

			if( tMin > tMax )
			{
				return false;
			}
		}
	}

	if( pInfo )
	{
		Vector Normal;
		if( IntersectingAxis >= 0 )
		{
			Normal.v[ IntersectingAxis ] = -Sign( dir.v[ IntersectingAxis ] );
		}
		//else
		//{
		//	WARN;
		//}

		pInfo->m_Out_Collision		= true;
		pInfo->m_Out_HitT			= tMin;
		pInfo->m_Out_Intersection	= m_Point1 + pInfo->m_Out_HitT * dir;
		pInfo->m_Out_Plane			= Plane( Normal, pInfo->m_Out_Intersection );
	}

	return true;
}

bool Segment::Intersects( const Sphere& s, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	return s.Intersects( *this, pInfo );
}

bool Segment::Intersects( const Cylinder& c, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	return c.Intersects( *this, pInfo );
}

// Works for either side of a plane; this wasn't always the case
bool Segment::Intersects( const Plane& p, CollisionInfo* const pInfo /*=NULL*/ ) const
{
	const Vector	ab	= m_Point2 - m_Point1;
	const float		d	= ab.Dot( p.m_Normal );
	if( Abs( d ) > EPSILON )	// Is segment not parallel to the plane?
	{
		// Test the t-value where the line crosses the plane to see if the
		// segment intersects or lies completely on one side (accounting
		// for the possibility that the segment faces the other way now).
		const float	t	= -p.DistanceTo( m_Point1 ) / d;

		if( t >= 0.0f && t <= 1.0f )
		{
			if( pInfo )
			{
				pInfo->m_Out_Collision		= true;
				pInfo->m_Out_Intersection	= m_Point1 + t * ab;
				pInfo->m_Out_Plane			= p;
				pInfo->m_Out_HitT			= t;
			}
			return true;
		}
	}
	return false;
}

bool Segment::Intersects( const Ellipsoid& e, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	return e.Intersects( *this, pInfo );
}

Vector Segment::GetPointAt( float TValue ) const
{
	return m_Point1 + ( ( m_Point2 - m_Point1 ) * TValue );
}

Vector Segment::GetDirection() const
{
	return ( m_Point2 - m_Point1 ).GetNormalized();
}
