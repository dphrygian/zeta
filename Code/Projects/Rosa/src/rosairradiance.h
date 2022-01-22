#ifndef ROSAIRRADIANCE_H
#define ROSAIRRADIANCE_H

#include "vector4.h"

// Indicates direction of normal that gets lit; *opposite* of the direction light is traveling through volume
enum EIrradianceDir
{
	IRRDIR_Right,
	IRRDIR_Left,
	IRRDIR_Front,
	IRRDIR_Back,
	IRRDIR_Up,
	IRRDIR_Down
};

struct SVoxelIrradiance
{
	SVoxelIrradiance();

	// Ordered X+, X-, Y+, Y-, Z+, Z-
	Vector4	m_Light[6];
};

#endif // ROSAIRRADIANCE_H
