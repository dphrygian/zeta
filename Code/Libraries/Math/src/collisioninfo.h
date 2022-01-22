#ifndef COLLISIONINFO_H
#define COLLISIONINFO_H

#include "vector.h"
#include "plane.h"
#include "hashedstring.h"

class CollisionInfo
{
public:
	CollisionInfo();

	void	ResetInParameters();
	void	ResetOutParameters();

	// Partial assignment
	void	CopyInParametersFrom( const CollisionInfo& OtherInfo );
	void	CopyOutParametersFrom( const CollisionInfo& OtherInfo );

	// In variables
	bool			m_In_CollideWorld;
	bool			m_In_CollideEntities;
	bool			m_In_StopAtAnyCollision;
	void*			m_In_CollidingEntity;
	c_uint32		m_In_UserFlags;				// Defined for any given project

	// Out variables
	bool			m_Out_Collision;
	Plane			m_Out_Plane;
	Vector			m_Out_Intersection;	// Point of intersection
	float			m_Out_HitT;			// Distance (or ratio) along a ray or segment
	void*			m_Out_HitEntity;
	HashedString	m_Out_HitName;		// Description of the hit region. Real strings are too slow to use here.
	c_uint32		m_Out_UserFlags;	// Defined for any given project
};

#endif // COLLISIONINFO_H
