import os
import sys

MOVEPACKAGES	= True	# Move (instead of copy) to BakedContent after baking
VERBOSE			= True
PACKTESTASSETS	= False	# Enabled with -t
PACKMAINASSETS	= True	# Disabled with -c or -d
PACKCLOUDASSETS	= True	# Disabled with -d (mutually exclusive with -c)
PACKDLCASSETS	= True	# Disabled with -c (mutually exclusive with -d)

# Assets
BAKED			= '../Baked/'
BAKEDCONTENT	= '../BakedContent'
TEXTURES		= 'Textures/'
MESHES			= 'Meshes/'
BRUSHES			= 'Brushes/'
FONTS			= 'Fonts/'
SHADERS			= 'Shaders/'
CONFIG			= 'Config/'
AUDIO			= 'Audio/'
ROOMS			= 'Rooms/'
WORLDS			= 'Rooms/'
MISC			= 'Misc/'
CLOUD			= 'Cloud/'
DLC				= 'DLC/'

BASEPACKFILE		= 'zeta-base.cpk'
TEXTURESPACKFILE	= 'zeta-textures.cpk'
MESHESPACKFILE		= 'zeta-meshes.cpk'
AUDIOPACKFILE		= 'zeta-audio.cpk'
WORLDPACKFILE		= 'zeta-world.cpk'

# Tools folders
CYGWIN_BIN	= 'C:/cygwin64/bin/'	# Updated for Karras
TOOLS_DIR	= '../Tools/'	# Relative to Baked directory

# Tools
FILE_PACKER		= TOOLS_DIR + 'FilePacker.exe'
COPY			= CYGWIN_BIN + 'cp'
MOVE			= CYGWIN_BIN + 'mv'
PACKAGESTORE	= MOVE if MOVEPACKAGES else COPY

#-----------------------------------------------------
def pack():
	if PACKTESTASSETS:
		print 'ALERT: Packing test assets!'

	if PACKMAINASSETS:
		packdir( CONFIG,	True,	'.ccf',		BASEPACKFILE )
		packdir( FONTS,		True,	'.fnp',		BASEPACKFILE )
		packdir( MESHES,	True,	'.cms',		MESHESPACKFILE )
		packdir( BRUSHES,	True,	'.cbr',		BASEPACKFILE )
		packdir( BRUSHES,	True,	'.cms',		MESHESPACKFILE )
		packdir( SHADERS,	True,	'.cfx',		BASEPACKFILE )
		packdir( SHADERS,	True,	'.chv2',	BASEPACKFILE )
		packdir( SHADERS,	True,	'.chp2',	BASEPACKFILE )
		packdir( SHADERS,	True,	'.gv12',	BASEPACKFILE )
		packdir( SHADERS,	True,	'.gf12',	BASEPACKFILE )
		packdir( TEXTURES,	True,	'.dds',		TEXTURESPACKFILE )
		packdir( TEXTURES,	True,	'.bmp',		TEXTURESPACKFILE )
		packdir( TEXTURES,	True,	'.tga',		TEXTURESPACKFILE )
		packdir( ROOMS,		True,	'.rrm',		WORLDPACKFILE )
		packdir( WORLDS,	True,	'.rwd',		WORLDPACKFILE )
		packdir( AUDIO,		False,	'.wav',		AUDIOPACKFILE )	# DLP 28 Nov 2021: Don't compress WAVs anymore either, I'd only ever use them for short streams to avoid Vorbis decode times.
		packdir( AUDIO,		False,	'.ogg',		AUDIOPACKFILE )	# Don't compress .ogg files, so we can stream direct from pack file.
		packdir( MISC,		True,	'.bmp',		BASEPACKFILE )

	# Make separate cloud packages for each folder in CLOUD
	if PACKCLOUDASSETS:
		packclouddir( CLOUD, True )

	if PACKDLCASSETS:
		packdlcdir( DLC, True )

	if PACKMAINASSETS:
		storefiles( '.', '.cpk', PACKAGESTORE )
		storefiles( '.', '.html', COPY )
		storefiles( '.', '.txt', COPY )				# NOTE: This copies any logs in the Baked folder, whatever.

		# These aren't distributed with the game, but are stored in BakedContent for versioning with each shipping build number
		# (These should be the PDBs for the Steam build, since the intent is to use with Steam minidumps)
		storefiles( '.', '.pdb', COPY )
		storefiles( '.', '.exe', COPY )				# Might as well store the binaries here too, makes copying to SteamPipe easier.
	
	if PACKCLOUDASSETS:
		storefiles( CLOUD, '.cpk', PACKAGESTORE )
	
	if PACKDLCASSETS:
		storefiles( DLC, '.cpk', PACKAGESTORE )

#-----------------------------------------------------
def runtool( args ):
	if VERBOSE:
		for arg in args:
			print arg,
		print
	os.spawnv( os.P_WAIT, args[0], args )

#-----------------------------------------------------
def storefiles( root, ext, storecmd ):
	for path, dirs, files in os.walk( root ):
		del dirs[:]	# Don't recurse into any folders

		for file in files:
			if( ext in file ):
				storefile( os.path.join( root, file ), storecmd )

#-----------------------------------------------------
def storefile( packfile, storecmd ):
	copypackfile = os.path.join( BAKEDCONTENT, packfile ).replace( '\\', '/' )
	if not os.path.exists( os.path.dirname( copypackfile ) ):
		os.mkdir( os.path.dirname( copypackfile ) )
	runtool( [ storecmd, packfile, copypackfile ] )

#-----------------------------------------------------
# If ext is specified, only files matching that extension are baked
# If ext isn't specified, all files in the folder are baked
# This will recurse into any subfolders of the given path
def packdir( dir, compress, ext, packfile ):
	for path, dirs, files in os.walk( dir ):

		# Ignore source control and test content
		if '.svn' in dirs:
			dirs.remove( '.svn' )

		# Ignore test content if we're not building a test package
		if( ( not PACKTESTASSETS ) and ( 'Test' in dirs ) ):
			dirs.remove( 'Test' )

		usepackfile = packfile

		for file in files:
			if( ( not ext ) or ( ext in file ) ):
				infile = os.path.join( path, file )
				compressflag = ''
				if compress:
					compressflag = '-c'
				# NOTE: Switched the order of parameters from the way FilePacker used to work!
				runtool( [ FILE_PACKER, usepackfile, infile, compressflag ] )

def packclouddir( clouddir, compress ):
	# Reorganizing Cloud folder so this can also pack whole seasons for DLC.
	# The assumption at the time was that each top-level Cloud folder represented
	# a season which would later be bundled as DLC; each mid-level folder within
	# the season represents a scenario. In the future, I may revisit this because
	# I'm doing away with scenarios and don't expect to bundle cloud stuff as DLC.
	for season in os.listdir( clouddir ):
		# First, pack whole seasons as DLC
		seasonfullname = os.path.join( clouddir, season )
		if os.path.isdir( seasonfullname ):
			seasonpackname = os.path.join( clouddir, 'zeta-' + season + '.cpk' )
			packdir( seasonfullname, compress, '', seasonpackname )
		
		# Second, pack individual scenarios for cloud
		for scenario in os.listdir( seasonfullname ):
			scenariofullname = os.path.join( seasonfullname, scenario )
			if os.path.isdir( scenariofullname ):
				scenariopackname = os.path.join( clouddir, scenario + '.cpk' )
				packdir( scenariofullname, compress, '', scenariopackname )

def packdlcdir( dlcdir, compress ):
	for dlc in os.listdir( dlcdir ):
		dlcfullname = os.path.join( dlcdir, dlc )
		if os.path.isdir( dlcfullname ):
			dlcpackname = os.path.join( dlcdir, 'zeta-' + dlc + '.cpk' )
			packdir( dlcfullname, compress, '', dlcpackname )

#-----------------------------------------------------
# Entry point
#-----------------------------------------------------

for arg in sys.argv:
	if arg == '-t':
		PACKTESTASSETS = True
	if arg == '-c':
		PACKMAINASSETS = False
		PACKDLCASSETS = False
	if arg == '-d':
		PACKMAINASSETS = False
		PACKCLOUDASSETS = False

print 'Deleting pack files...'

os.chdir( BAKED )

if PACKMAINASSETS:
	if os.path.exists( BASEPACKFILE ):
		os.remove( BASEPACKFILE )

	if os.path.exists( TEXTURESPACKFILE ):
		os.remove( TEXTURESPACKFILE )

	if os.path.exists( MESHESPACKFILE ):
		os.remove( MESHESPACKFILE )

	if os.path.exists( AUDIOPACKFILE ):
		os.remove( AUDIOPACKFILE )

	if os.path.exists( WORLDPACKFILE ):
		os.remove( WORLDPACKFILE )

if PACKCLOUDASSETS:
	for cloudpath, clouddirs, cloudfiles in os.walk( CLOUD ):
		del clouddirs[:]
		for cloudfile in cloudfiles:
			if cloudfile.endswith( '.cpk' ):
				os.remove( os.path.join( CLOUD, cloudfile ) )

if PACKDLCASSETS:
	for dlcpath, dlcdirs, dlcfiles in os.walk( DLC ):
		del dlcdirs[:]
		for dlcfile in dlcfiles:
			if dlcfile.endswith( '.cpk' ):
				os.remove( os.path.join( DLC, dlcfile ) )

print 'Packing assets...'

try:
	pack()
except:
	sys.exit(1)
	
print 'Packing done!'