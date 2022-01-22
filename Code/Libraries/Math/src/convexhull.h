#ifndef CONVEXHULL_H
#define CONVEXHULL_H

// A convex polyhedron represented as an unordered set of bounding planes.
// Originally designed for swept AABB intersection tests in Rosa.

#include "array.h"
#include "plane.h"

class AABB;
class Vector;
class CollisionInfo;

class ConvexHull
{
public:
	ConvexHull();
	~ConvexHull();

	uint				GetNumPlanes() const { return m_Planes.Size(); }
	const Array<Plane>&	GetPlanes() const { return m_Planes; }

	// Add complete set of planes without conditions (e.g., for serializing).
	void	AddPlanes( const Array<Plane>& NewPlanes );

	// Add plane
	void	AddPlane( const Plane& NewPlane );

	// Add plane if it does not already exist in the volume (e.g., for building in editor).
	void	TryAddPlane( const Plane& NewPlane, const float Tolerance );

	// TODO: Other intersection tests as needed; but swept AABB should suffice
	// for everything in Rosa including zero-extent ray or point tests.

	// Sweep moving box against stationary volume.
	bool	Sweep( const AABB& Box, const Vector& Travel, CollisionInfo* const pInfo = NULL ) const;
	bool	Sweep( const Vector& Location, const Vector& Extents, const Vector& Travel, CollisionInfo* const pInfo = NULL ) const;

	bool	Contains( const Vector& Location ) const;

	// Assuming hull is currently at the origin, moves it to somewhere else.
	void	MoveBy( const Vector& Offset, const Angles& OffsetAngles );
	void	MoveBy( const Vector& Offset, const Angles& OffsetAngles, const float Scale );

private:
	void	FattenPlanes( const Vector& BoxExtents ) const;

	Array<Plane>			m_Planes;
	mutable Array<Plane>	m_FattenedPlanes;

	// TODO: Future optimizations. Cache things that are frequently computed but rarely.
	// Should probably compute hull's bounding box only once.
	// Should maybe have a helper function to add the axis-aligned planes.
};

#endif // CONVEXHULL_H
