#ifndef SEGMENT_H
#define SEGMENT_H

#include "vector.h"

class Triangle;
class AABB;
class CollisionInfo;
class Sphere;
class Cylinder;
class Plane;
class Ellipsoid;

class Segment
{
public:
	Segment();
	Segment( const Vector& Point1, const Vector& Point2 );

	Vector	NearestPointTo( const Segment& Other, float* pTValue = NULL ) const;
	void	NearestPointTo( const Segment& Other, Vector& OutPointOnThis, Vector& OutPointOnOther, float* pThisTValue = NULL, float* pOtherTValue = NULL ) const;
	Vector	NearestPointTo( const Vector& Point ) const;
	float	DistanceSqTo( const Vector& Point ) const;

	bool	Intersects( const Triangle& t, CollisionInfo* const pInfo = NULL ) const;
	bool	Intersects( const AABB& a ) const;	// Cheaper separating axis test
	bool	Intersects( const AABB& a, CollisionInfo* const pInfo ) const;
	bool	Intersects( const Sphere& s, CollisionInfo* const pInfo = NULL ) const;
	bool	Intersects( const Cylinder& c, CollisionInfo* const pInfo = NULL ) const;
	bool	Intersects( const Plane& p, CollisionInfo* const pInfo = NULL ) const;
	bool	Intersects( const Ellipsoid& e, CollisionInfo* const pInfo = NULL ) const;

	Vector	GetPointAt( float TValue ) const;

	Vector	GetDirection() const;

	Vector m_Point1;
	Vector m_Point2;
};

#endif // SEGMENT_H
