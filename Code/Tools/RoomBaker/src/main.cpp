#include "core.h"
#include "roombaker.h"
#include "simplestring.h"
#include <string.h>
#include <stdio.h>

// Syntax: RoomBaker.exe inroom.rosaroom outroom.vrm

int main( int argc, char* argv[] )
{
	if( argc != 3 )
	{
		printf( "Syntax: RoomBaker.exe <infile> <outfile>\n" );
		return 0;
	}

	RoomBaker Baker;
	return Baker.Bake( argv[1], argv[2] );
}
