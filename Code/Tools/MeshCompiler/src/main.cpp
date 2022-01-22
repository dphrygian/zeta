#include "core.h"
#include "meshcompiler.h"
#include <string.h>
#include <stdio.h>

// This tool takes an XML file exported from Blender and does
// all the necessary wrangling to get it ready to load right
// into a game project.

// Syntax: MeshCompiler.exe inmesh.xml outmesh.cms [-l]
// -l specifies long indices; else shorts are used

// Alternative syntax for dependency touching anims:
// MeshCompiler.exe inmesh.xml outfile.unused -t

// Output format: (.cms for compiled mesh)
// Header: 22 bytes
//		4 bytes:	"DCMS" magic ID
//		4 bytes:	n, Number of vertices (unsigned int)
//		4 bytes:	i, Number of indices (unsigned int)
//		4 bytes:	f, Number of frames (unsigned int)
//		1 byte:		b, Number of bones (unsigned char)
//		1 byte:		a, Number of animations (unsigned char)
//		1 byte:		Long indices (bool)
//		1 byte:		Has UVs (bool)
//		1 byte:		Has normals (bool)
//		1 byte:		Has tangents (bool)
//		1 byte:		Has skeleton (bool)
// Array of n positions (Vector)
// Array of n UVs (Vector2) (optional)
// Array of n normals (Vector) (optional)
// Array of n tangents (Vector4) (optional)
// Array of n weights (VertexWeight) (optional)
// Array of i indices (unsigned shorts or unsigned ints)
// Array of b bones per f frames (optional)
//		Bones consist of interlaced Quats and Vectors
//		This is in "frame-major order", or all bones for a frame packed together
// Array of a animations
//		Animations consist of a 32-byte name, an unsigned short start frame, and an unsigned short length

int main( int argc, char* argv[] )
{
	if( argc < 3 || argc > 4 )
	{
		printf( "Syntax: MeshCompiler.exe <infile> <outfile> [-l] (compile [with long indices])\n" );
		printf( "Syntax: MeshCompiler.exe <infile> <outfile> -t (touch dependencies)\n" );
		return 0;
	}

	bool TouchDependencies = false;
	bool LongIndices = false;
	for( int argi = 3; argi < argc; ++argi )
	{
		if( strcmp( argv[ argi ], "-t" ) == 0 )
		{
			TouchDependencies = true;
		}

		if( strcmp( argv[ argi ], "-l" ) == 0 )
		{
			LongIndices = true;
		}
	}

	MeshCompiler Compiler;
	if( TouchDependencies )
	{
		return Compiler.TouchDependencies( argv[1], argv[2] );
	}
	else
	{
		return Compiler.Compile( argv[1], argv[2], LongIndices );
	}
}
