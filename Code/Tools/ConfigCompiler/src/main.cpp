#include "core.h"
#include "configparser.h"
#include "configmanager.h"
#include "filestream.h"
#include "printmanager.h"
#include <string.h>
#include <stdio.h>

// Extensions:
// .cfg: Plaintext config used at runtime
// .config: Plaintext config compiled offline into .ccf
// .ccf: Compiled config baked from .config

// Syntax: ConfigCompiler.exe inconfig.config outconfig.ccf

int main( int argc, char* argv[] )
{
	// DLP 12 May 2019: I don't want this output anymore.
	//SETPRINTLEVEL( PRINTLEVEL_Spam );

	if( argc != 3 )
	{
		printf( "Syntax: ConfigCompiler.exe <infile> <outfile>\n" );
		return 0;
	}

	static const bool skWarnIfOverwriting = true;
	ConfigParser::Parse( FileStream( argv[1], FileStream::EFM_Read ), skWarnIfOverwriting );

	ConfigManager::Report();

	ConfigManager::Save( FileStream( argv[2], FileStream::EFM_Write ) );

	return 0;
}
