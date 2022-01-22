import os
import sys
import time

# The Tools version exists so that other processes like itch butler can reference it
TOOLS_VERSION = '../Tools/version.txt'
GAME_VERSION = '../Raw/Config/version.loom'

currentTime		= time.localtime()
utcOffsetSec	= time.altzone if currentTime.tm_isdst else time.timezone
utcOffsetHour	= utcOffsetSec / 3600
timestr			= time.strftime( '%Y-%m-%dT%H:%M:%S', currentTime ) + '{0:+03d}:00'.format( -utcOffsetHour )

configContent = []
versionNumber = 0

readConfigFile = open( GAME_VERSION, 'r' )
for line in readConfigFile:

	if line.startswith( 'BuildNumber' ):
		# Hacktastic! Python!
		start = line.find( '"' ) + 1
		end = line.rfind( '"' )
		version = line[start:end]
		versionNumber = int( version ) + 1
		configContent.append( line.replace( version, str( versionNumber ) ) )

	elif line.startswith( 'BuildTime' ):
		start = line.find( '"' ) + 1
		end = line.rfind( '"' )
		oldBuildTime = line[start:end]
		configContent.append( line.replace( oldBuildTime, timestr ) )

	else:
		configContent.append( line )

readConfigFile.close()

writeConfigFile = open( GAME_VERSION, 'w' )
for line in configContent:
	writeConfigFile.write( line )
writeConfigFile.close()

writeVersionFile = open( TOOLS_VERSION, 'w' )
writeVersionFile.write( str( versionNumber ) )
writeVersionFile.close()