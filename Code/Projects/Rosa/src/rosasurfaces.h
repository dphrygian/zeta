#ifndef ROSASURFACES_H
#define ROSASURFACES_H

#include "simplestring.h"

class HashedString;

namespace RosaSurfaces
{
	// For managing statically allocated memory
	void			AddRef();
	void			Release();

	struct SSurface
	{
		SSurface()
		:	m_Sound()
		,	m_VolumeScalar( 0.0f )
		,	m_RadiusScalar( 0.0f )
		{
		}

		SimpleString	m_Sound;
		float			m_VolumeScalar;
		float			m_RadiusScalar;
	};

	const SSurface&	GetSurface( const HashedString& Surface );
	SimpleString	GetSound( const HashedString& Surface );
	float			GetVolumeScalar( const HashedString& Surface );
	float			GetRadiusScalar( const HashedString& Surface );
}

#endif // ROSASURFACES_H
