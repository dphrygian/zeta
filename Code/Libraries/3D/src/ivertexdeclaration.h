#ifndef IVERTEXDECLARATION_H
#define IVERTEXDECLARATION_H

#include "3d.h"

#define VD_POSITIONS		0x0001
#define VD_COLORS			0x0002
#define VD_FLOATCOLORS		0x0004	// Same meaning as what was formerly VD_FLOATCOLORS_SM2; float colors using a texture coordinate
#define VD_UVS				0x0008
#define VD_NORMALS			0x0010
#define VD_NORMALS_B		0x0020	// Second set of normals, for foliage that needs bent normals and actual ones
#define VD_TANGENTS			0x0040
#define VD_BONEINDICES		0x0080
#define VD_BONEWEIGHTS		0x0100

class IVertexDeclaration
{
public:
	virtual ~IVertexDeclaration() {}

	virtual void	Initialize( uint VertexSignature ) = 0;
	virtual void*	GetDeclaration() = 0;
	virtual uint	GetSignature() = 0;
};

#endif // IVERTEXDECLARATION_H
