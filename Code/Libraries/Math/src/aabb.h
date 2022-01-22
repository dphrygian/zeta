#ifndef AABB_H
#define AABB_H

#include "vector.h"

class Frustum;
class Segment;
class Plane;
class Ray;
class Sphere;
class Matrix;
class Angles;

class AABB
{
public:
	AABB();
	AABB( const Vector& Point );
	AABB( const Vector& MinCorner, const Vector& MaxCorner );

	Vector	GetClosestPoint( const Vector& v ) const;
	Vector	GetExtents() const { return 0.5f * ( m_Max - m_Min ); }
	Vector	GetCenter() const { return m_Min + GetExtents(); }
	float	GetSquaredDistance( const Vector& v ) const;

	bool	Equals( const AABB& a ) const;

	float	GetVolume() const;
	bool	IsZero() const;

	// ROSANOTE: Inlined since it's called a lot from collision code
	FORCE_INLINE bool Intersects( const AABB& a ) const
	{
		return (
			a.m_Max.x >= m_Min.x &&
			a.m_Max.y >= m_Min.y &&
			a.m_Max.z >= m_Min.z &&
			a.m_Min.x <= m_Max.x &&
			a.m_Min.y <= m_Max.y &&
			a.m_Min.z <= m_Max.z
			);
	}

	bool	Intersects( const Segment& s ) const;
	bool	Intersects( const Vector& v ) const;	// Point
	bool	Intersects( const Frustum& f ) const;
	bool	IsOnFrontSide( const Plane& p ) const;
	bool	IsOnBackSide( const Plane& p ) const;
	bool	Intersects( const Plane& p ) const;
	bool	Intersects( const Ray& r ) const;
	bool	Intersects( const Sphere& s ) const;

	void	ExpandTo( const Vector& v );
	void	ExpandBy( const Vector& v );
	void	ExpandTo( const AABB& a );
	void	ContractTo( const AABB& Other );

	void	MoveBy( const Vector& v );

	AABB	GetTransformedBound( const Vector& Translation, const Angles& Rotation ) const;
	AABB	GetTransformedBound( const Vector& Translation, const Angles& Rotation, const Vector& Scale ) const;
	AABB	GetTransformedBound( const Matrix& m ) const;
	void	TransformBound( const Matrix& m );

	Vector m_Min;
	Vector m_Max;

	static inline AABB CreateFromCenterAndExtents( const Vector& Center, const Vector& HalfExtents )
	{
		return AABB( Center - HalfExtents, Center + HalfExtents );
	}
};

#endif // AABB_H
