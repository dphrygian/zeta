#include "core.h"
#include "convexhull.h"
#include "aabb.h"
#include "collisioninfo.h"

ConvexHull::ConvexHull()
:	m_Planes()
,	m_FattenedPlanes()
{
}

ConvexHull::~ConvexHull()
{
}

void ConvexHull::AddPlanes( const Array<Plane>& NewPlanes )
{
	m_Planes.Append( NewPlanes );
	m_FattenedPlanes.Append( NewPlanes );
}

void ConvexHull::TryAddPlane( const Plane& NewPlane, const float Tolerance )
{
	// First, check if NewPlane already exists in the volume (within a tolerance)
	FOR_EACH_ARRAY( PlaneIter, m_Planes, Plane )
	{
		const Plane& ExistingPlane = PlaneIter.GetValue();
		if( ExistingPlane.Equals( NewPlane, Tolerance ) )
		{
			// Plane already exists in volume.
			return;
		}
	}

	// Plane does not yet exist in volume.
	AddPlane( NewPlane );
}

void ConvexHull::AddPlane( const Plane& NewPlane )
{
	m_Planes.PushBack( NewPlane );
	m_FattenedPlanes.PushBack( NewPlane );
}

bool ConvexHull::Sweep( const AABB& Box, const Vector& Travel, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	return Sweep( Box.GetCenter(), Box.GetExtents(), Travel, pInfo );
}

bool ConvexHull::Sweep( const Vector& SweepStart, const Vector& Extents, const Vector& Travel, CollisionInfo* const pInfo /*= NULL*/ ) const
{
	DEVASSERT( m_Planes.Size() );

	// ROSATODO: This is a tentative candidate for caching;
	// if we've got fattened planes for this box's extents,
	// no need to compute them again.
	FattenPlanes( Extents );

	// Now clip the segment of travel by the halfspace of each plane.
	// Based on RTCD p198.
	float			TFirst		= 0.0f;
	float			TLast		= 1.0f;
	int				TFirstPlane	= -1;

	// I'm *not* implementing this using Segment::Intersects( const Plane& p ),
	// because that adds a lot of overhead I don't need.
	const uint NumPlanes = m_FattenedPlanes.Size();
	for( uint PlaneIndex = 0; PlaneIndex < NumPlanes; ++PlaneIndex )
	{
		const Plane& FattenedPlane = m_FattenedPlanes[ PlaneIndex ];

		const float Denominator	= FattenedPlane.m_Normal.Dot( Travel );
		const float Distance	= FattenedPlane.m_Normal.Dot( SweepStart ) + FattenedPlane.m_Distance;

		if( Denominator == 0.0f )	// ROSATODO: Test within a tolerance?
		{
			// Travel is parallel to plane
			// If segment is on front side of plane, there is no possible intersection
			if( Distance > 0.0f )	// ROSATODO: Test within a tolerance?
			{
				// No collision
				return false;
			}
		}
		else
		{
			const float T = -Distance / Denominator;
			if( Denominator < 0.0f )
			{
				// Travel is toward plane normal
				// Update if this is the innermost entry point
				if( T > TFirst )
				{
					TFirst		= T;
					TFirstPlane	= PlaneIndex;
				}
			}
			else
			{
				// Travel is away from plane normal
				// Update if this is the innermost exit point
				if( T < TLast )
				{
					TLast = T;
				}
			}

			if( TFirst > TLast )
			{
				// No intersection; entry point and exit points have crossed
				return false;
			}
		}
	}

	// There was an intersection at TFirst
	if( pInfo )
	{
		pInfo->m_Out_Collision		= true;
		pInfo->m_Out_Intersection	= SweepStart + TFirst * Travel;
		pInfo->m_Out_HitT			= TFirst;

		// It's possible to have an undefined plane of intersection
		// (e.g., if box starts embedded in volume)
		if( TFirstPlane >= 0 )
		{
			// Use the unfattened plane, because it corresponds to the actual point of intersection
			pInfo->m_Out_Plane		= m_Planes[ TFirstPlane ];
		}
	}

	// Intersection!
	return true;
}

bool ConvexHull::Contains( const Vector& Location ) const
{
	FOR_EACH_ARRAY( PlaneIter, m_Planes, Plane )
	{
		const Plane& HullPlane = PlaneIter.GetValue();
		if( HullPlane.DistanceTo( Location ) > 0.0f )
		{
			return false;
		}
	}

	return true;
}

void ConvexHull::FattenPlanes( const Vector& BoxExtents ) const
{
	DEVASSERT( m_FattenedPlanes.Size() == m_Planes.Size() );

	FOR_EACH_ARRAY( PlaneIter, m_Planes, Plane )
	{
		const Plane&	OriginalPlane				= PlaneIter.GetValue();
		const Vector	PlaneAbsNormal				= OriginalPlane.m_Normal.GetAbs();	// ROSATODO: Abs normals are a great candidate for caching
		const float		FattenedDistance			= OriginalPlane.m_Distance - PlaneAbsNormal.Dot( BoxExtents );
		m_FattenedPlanes[ PlaneIter.GetIndex() ]	= Plane( OriginalPlane.m_Normal, FattenedDistance );
	}
}

void ConvexHull::MoveBy( const Vector& Offset, const Angles& OffsetAngles )
{
	FOR_EACH_ARRAY( PlaneIter, m_Planes, Plane )
	{
		Plane&	MovedPlane		= PlaneIter.GetValue();
		MovedPlane.m_Normal		= MovedPlane.m_Normal.RotateBy( OffsetAngles );
		MovedPlane.m_Distance	-= Offset.Dot( MovedPlane.m_Normal );
	}
}

void ConvexHull::MoveBy( const Vector& Offset, const Angles& OffsetAngles, const float Scale )
{
	FOR_EACH_ARRAY( PlaneIter, m_Planes, Plane )
	{
		Plane&	MovedPlane		= PlaneIter.GetValue();
		MovedPlane.m_Normal		= MovedPlane.m_Normal.RotateBy( OffsetAngles );
		MovedPlane.m_Distance	*= Scale;
		MovedPlane.m_Distance	-= Offset.Dot( MovedPlane.m_Normal );
	}
}
