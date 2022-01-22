#include "core.h"
#include "filepacker.h"
#include "printmanager.h"

#include <string.h>

void PrintManual()
{
	PRINTF( "Syntax:\n\tFilePacker.exe <packfile> <infile> [-c]\n" );
	PRINTF( "\tFilePacker.exe -m <packfile> <manifestfile>\n" );
	PRINTF( "\tFilePacker.exe -u <packfile>\n" );
	PRINTF( "\tFilePacker.exe -r <packfile>\n" );
	PRINTF( "\nOptions:\n" );
	PRINTF( "\t-c: Compress file in package\n" );
	PRINTF( "\t-m: Pack from manifest\n" );
	PRINTF( "\t-u: Unpack package file\n" );
	PRINTF( "\t-r: Report package file stats\n" );
}

int main( int argc, char* argv[] )
{
	if( argc < 3 || argc > 4 )
	{
		PrintManual();
		return 0;
	}

	FilePacker Packer;
	if( 0 == strcmp( "-u", argv[1] ) )
	{
		return Packer.UnpackFile( argv[2] );
	}
	else if( 0 == strcmp( "-r", argv[1] ) )
	{
		return Packer.Report( argv[2] );
	}
	else if( 0 == strcmp( "-m", argv[1] ) )
	{
		if( argc < 4 )
		{
			PrintManual();
			return 0;
		}
		else
		{
			return Packer.PackManifest( argv[2], argv[3] );
		}
	}
	else
	{
		bool Compress = false;
		if( argc == 4 && 0 == strcmp( "-c", argv[3] ) )
		{
			Compress = true;
		}
		return Packer.PackFile( argv[2], argv[1], Compress );
	}
}
