import os
import sys

LINUX = sys.platform.startswith( 'linux' )
WINDOWS = sys.platform.startswith( 'win' )

SILENT = False
VERBOSE = False
REBUILD = False

# The first value in each entry is the description that will be
# shown to the user if they try an invalid ruleset.

BAKE_RULES = {	'all': ( 'Bake everything.', 'blender_meshes', 'blender_brushes', 'fonts', 'font_textures', 'textures', 'shaders', 'meshes', 'brushes', 'loom', 'config', 'audio', 'rooms', 'worlds', 'exe', 'etc' ),
				'meshes': ( 'Export and bake Blender meshes.', 'blender_meshes', 'meshes' ),
				'brushes': ( 'Export and bake Blender brushes.', 'blender_brushes', 'brushes' ),
				'blender': ( 'Export Blender assets.', 'blender_meshes', 'blender_brushes' ),
				'compilemeshes': ( 'Bake intermediate meshes.', 'meshes' ),
				'fonts': ( 'Generate font textures and bake definitions.', 'fonts', 'font_textures' ),
				'textures': ( 'Bake all textures.', 'textures', 'font_textures' ),
				'shaders': ( 'Bake shaders.', 'shaders' ),
				'loom': ( 'Bake Loom files only.', 'loom' ),
				'config': ( 'Bake configuration files.', 'loom', 'config' ),
				'audio': ( 'Bake all audio.', 'audio' ),
				'rooms': ( 'Bake room files.', 'rooms' ),
				'worlds': ( 'Bake world files.', 'worlds' ),
				'exe': ( 'Bake executables.', 'exe' ),
				'etc': ( 'Bake everything else.', 'etc' ),
				'clean': ( 'Clean Intermediate and Baked folders.', 'clean' ) }

# Asset folders
RAW					= '../Raw/'
BAKED				= '../Baked/'
INTERMEDIATE		= '../Intermediate/'
RAW_TEXTURES		= RAW + 'Textures/'
INTER_TEXTURES		= INTERMEDIATE + 'Textures/'
INTER_FONT_TEXTURES	= INTER_TEXTURES + 'Fonts/'
RAW_MESHES			= RAW + 'Meshes/'
INTER_MESHES		= INTERMEDIATE + 'Meshes/'
RAW_ANIMS			= RAW + 'Anims/'
INTER_ANIMS			= INTERMEDIATE + 'Anims/'
RAW_BRUSHES			= RAW + 'Brushes/'
INTER_BRUSHES		= INTERMEDIATE + 'Brushes/'
FONTS				= RAW + 'Fonts/'
SHADERS				= RAW + 'Shaders/'
RAW_CONFIG			= RAW + 'Config/'
INTER_CONFIG		= INTERMEDIATE + 'Config/'
AUDIO				= RAW + 'Audio/'
ROOMS				= RAW + 'Rooms/'
WORLDS				= RAW + 'Rooms/'	# Moved from /Worlds/ so save/load dialogs can just stay in the most recently used folder
MISC				= RAW + 'Misc/'
RAW_CLOUD			= RAW + 'Cloud/'	# Cloud content could contain anything! See below for what actually gets baked (currently config, rooms, and worlds)
INTER_CLOUD			= INTERMEDIATE + 'Cloud/'
RAW_DLC				= RAW + 'DLC/'		# DLC content could contain anything, same as Cloud
INTER_DLC			= INTERMEDIATE + 'DLC/'

# Environment
if WINDOWS:
	pass
elif LINUX:
	# For Blender
	os.environ[ 'PYTHONHOME' ] = '/usr/local/bin/python2.6'
	os.environ[ 'PYTHONPATH' ] = '/usr/local/lib/python2.6'

# Tools folders
if WINDOWS:
	CYGWIN_BIN = 'C:/cygwin64/bin/'										# Updated for Karras
	TOOLS_DIR = './'
	BLENDER_DIR = 'C:/Program Files (x86)/Blender Foundation/Blender/'
	#BLENDER_DIR = 'C:/Program Files/Blender Foundation/Blender/'		# Updated for Karras (modern Blender, not currently used!)
	PYTHON_DIR = 'C:/Program Files/Python27/'							# Updated for Karras
elif LINUX:
	COMMON_BIN = '/bin/'
	TOOLS_DIR = './'
	BLENDER_DIR = '/home/david/Dev/blender-2.49b-linux-glibc236-py26-x86_64/'
	PYTHON_DIR = '/usr/local/bin/'

# Tools
if WINDOWS:
	COPY			= CYGWIN_BIN + 'cp'
	MOVE			= CYGWIN_BIN + 'mv'
	MESH_COMPILER	= TOOLS_DIR + 'MeshCompiler.exe'
	CONFIG_COMPILER	= TOOLS_DIR + 'ConfigCompiler.exe'
	FONT_GENERATOR	= TOOLS_DIR + 'FontGenerator.exe'
	ROOM_BAKER		= TOOLS_DIR + 'RoomBaker.exe'
	WORLD_BAKER		= TOOLS_DIR + 'RoomBaker.exe'
	MESH_EXPORT		= TOOLS_DIR + 'xmlmeshexport.py'
	BRUSH_EXPORT	= TOOLS_DIR + 'xmlbrushexport.py'
	PYTHON			= PYTHON_DIR + 'python.exe'
	LOOM			= TOOLS_DIR + 'loom.py'
	BLENDER			= BLENDER_DIR + 'blender.exe'
	DXTEX			= TOOLS_DIR + 'DxTex.exe'	# Custom version to do normal hack
	FXC				= TOOLS_DIR + 'fxc.exe'		# Moved to work around Program Files weirdness or something
elif LINUX:
	COPY			= COMMON_BIN + 'cp'
	MOVE			= COMMON_BIN + 'mv'
	MESH_COMPILER	= TOOLS_DIR + 'MeshCompiler.exe'
	CONFIG_COMPILER	= TOOLS_DIR + 'ConfigCompiler'
	FONT_GENERATOR	= TOOLS_DIR + 'FontGenerator.exe'
	ROOM_BAKER		= TOOLS_DIR + 'RoomBaker.exe'
	WORLD_BAKER		= TOOLS_DIR + 'RoomBaker.exe'
	MESH_EXPORT		= 'xmlmeshexport.py'
	BRUSH_EXPORT	= 'xmlbrushexport.py'
	PYTHON			= PYTHON_DIR + 'python'
	LOOM			= TOOLS_DIR + 'loom.py'
	BLENDER			= BLENDER_DIR + 'blender'
	DXTEX			= TOOLS_DIR + 'DxTex.exe'	# Custom version to do normal hack
	FXC				= TOOLS_DIR + 'fxc.exe'		# Moved to work around Program Files weirdness or something

NUMBAKED = 0
NUMSKIPPED = 0

#-----------------------------------------------------
def bake( rules ):
	if 'blender_meshes' in rules:
		blenddir( MESH_EXPORT, RAW_MESHES, RAW, INTERMEDIATE, '.mesh' )
		blenddir( MESH_EXPORT, RAW_ANIMS, RAW, INTERMEDIATE, '.mesh' )
	if 'blender_brushes' in rules:
		blenddir( BRUSH_EXPORT, RAW_BRUSHES, RAW, INTERMEDIATE, '.brush' )

	if 'fonts' in rules:
		generatefonts( FONT_GENERATOR, FONTS )
	if 'textures' in rules:
		converttextures( DXTEX, RAW_TEXTURES, RAW, BAKED );
		bakedir( COPY, RAW_TEXTURES, RAW, BAKED, '.bmp', '.bmp' )
	if 'font_textures' in rules:
		converttextures( DXTEX, INTER_TEXTURES, INTERMEDIATE, BAKED );
	if 'shaders' in rules:
		compileshaders( FXC, COPY, SHADERS );

	if 'loom' in rules:
		bakedir( PYTHON, RAW_CONFIG, RAW, INTERMEDIATE, '.loom', '.config', True, [ LOOM ] )
		bakedir( PYTHON, RAW_CLOUD, RAW, INTERMEDIATE, '.loom', '.config', True, [ LOOM ] )
		bakedir( PYTHON, RAW_DLC, RAW, INTERMEDIATE, '.loom', '.config', True, [ LOOM ] )
	if 'config' in rules:
		bakedir( CONFIG_COMPILER, RAW_CONFIG, RAW, BAKED, '.config', '.ccf' )
		bakedir( CONFIG_COMPILER, INTER_CONFIG, INTERMEDIATE, BAKED, '.config', '.ccf' )
		bakedir( COPY, RAW_CONFIG, RAW, BAKED, '.cfg', '.cfg' )
		bakedir( CONFIG_COMPILER, RAW_CLOUD, RAW, BAKED, '.config', '.ccf' )
		bakedir( CONFIG_COMPILER, INTER_CLOUD, INTERMEDIATE, BAKED, '.config', '.ccf' )
		bakedir( COPY, RAW_CLOUD, RAW, BAKED, '.cfg', '.cfg' )
		bakedir( CONFIG_COMPILER, RAW_DLC, RAW, BAKED, '.config', '.ccf' )
		bakedir( CONFIG_COMPILER, INTER_DLC, INTERMEDIATE, BAKED, '.config', '.ccf' )
		bakedir( COPY, RAW_DLC, RAW, BAKED, '.cfg', '.cfg' )

	if 'brushes' in rules:
		bakedir( CONFIG_COMPILER, INTER_BRUSHES, INTERMEDIATE, BAKED, '.brush', '.cbr' )
		bakedir( MESH_COMPILER, INTER_BRUSHES, INTERMEDIATE, BAKED, '.mesh', '.cms' )

	if 'meshes' in rules:
		bakedir( MESH_COMPILER, INTER_ANIMS, INTERMEDIATE, INTERMEDIATE, '.mesh', '.unused', True, [], [ '-t' ]  )	# HACKHACK to touch files with anim dependencies
		bakedir( MESH_COMPILER, INTER_MESHES, INTERMEDIATE, BAKED, '.mesh', '.cms' )

	if 'audio' in rules:
		bakedir( COPY, AUDIO, RAW, BAKED, '.mp3', '.mp3' )
		bakedir( COPY, AUDIO, RAW, BAKED, '.ogg', '.ogg' )
		bakedir( COPY, AUDIO, RAW, BAKED, '.wav', '.wav' )

	# Rooms must be baked near the end, because RoomBaker loads other content.
	if 'rooms' in rules:
		bakedir( ROOM_BAKER, ROOMS, RAW, BAKED, '.rosaroom', '.rrm' )
		bakedir( ROOM_BAKER, RAW_CLOUD, RAW, BAKED, '.rosaroom', '.rrm' )
		bakedir( ROOM_BAKER, RAW_DLC, RAW, BAKED, '.rosaroom', '.rrm' )

	# Worlds can probably be baked anywhere, but bake them after rooms to be safe
	if 'worlds' in rules:
		bakedir( WORLD_BAKER, WORLDS, RAW, BAKED, '.rosaworld', '.rwd' )
		bakedir( WORLD_BAKER, RAW_CLOUD, RAW, BAKED, '.rosaworld', '.rwd' )
		bakedir( WORLD_BAKER, RAW_DLC, RAW, BAKED, '.rosaworld', '.rwd' )

	if 'exe' in rules:
		bakedir( COPY, RAW, RAW, BAKED, '.exe', '.exe', False )
		bakedir( COPY, RAW, RAW, BAKED, '.pdb', '.pdb', False )
		bakedir( COPY, RAW, RAW, BAKED, '.dll', '.dll', False )

	if 'etc' in rules:
		bakedir( COPY, RAW, RAW, BAKED, '.txt', '.txt', False )
		bakedir( COPY, RAW, RAW, BAKED, '.html', '.html', False )
		bakedir( COPY, MISC, RAW, BAKED, '.bmp', '.bmp', False )

#-----------------------------------------------------
def runtool( args ):
	trueexe = args[0]

	if ' ' in args[0] and not args[0].startswith( '"' ):
		args[0] = '"' + args[0] + '"'

	if not SILENT:
		for arg in args:
			print arg,
		print

	os.spawnv( os.P_WAIT, trueexe, args )

#-----------------------------------------------------
# Timestamp checking for simple improvement on build time
def shouldbuild( rawfile, bakedfiles ):
	global NUMBAKED
	global NUMSKIPPED

	if REBUILD:
		NUMBAKED = NUMBAKED + 1
		return True

	rawstat = os.stat( rawfile )
	for bakedfile in bakedfiles:
		bakedfileexists = os.access( bakedfile, os.F_OK )
		if bakedfileexists:
			bakedstat = os.stat( bakedfile )
			if bakedstat.st_mtime < rawstat.st_mtime:
				NUMBAKED = NUMBAKED + 1
				return True
		else:
			NUMBAKED = NUMBAKED + 1
			return True

	if VERBOSE:
		print 'Skipping ' + rawfile

	NUMSKIPPED = NUMSKIPPED + 1
	return False

#-----------------------------------------------------
# If rawext is specified, only files matching that extension are baked
# If rawext isn't specified, all files in the folder are baked
# This will recurse into any subfolders of the given path
def bakedir( tool, rawdir, rawpath, bakedpath, rawext = '', bakedext = '', recursive = True, prefixargs = [], postfixargs = [] ):
	for path, dirs, files in os.walk( rawdir ):
		if '.svn' in dirs:
			dirs.remove( '.svn' )	# Ignore SVN folders
		if not recursive:
			del dirs[:]				# Empty the list so we just examine this path
		if not os.path.exists( path.replace( rawpath, bakedpath ) ):
			os.mkdir( path.replace( rawpath, bakedpath ) )
		for file in files:
			if( ( not rawext ) or ( rawext in file ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( rawpath, bakedpath ).replace( rawext, bakedext )
				if shouldbuild( infile, [ outfile ] ):
					allargs = [ tool ] + prefixargs + [ infile, outfile ] + postfixargs
					runtool( allargs )

#-----------------------------------------------------
# Create font property and image files
def generatefonts( tool, rawdir ):
	for path, dirs, files in os.walk( rawdir ):
		if '.svn' in dirs:
			dirs.remove( '.svn' )	# Ignore SVN folders
		if not os.path.exists( path.replace( FONTS, INTER_TEXTURES ) ):
			os.mkdir( path.replace( FONTS, INTER_TEXTURES ) )
		if not os.path.exists( path.replace( FONTS, INTER_FONT_TEXTURES ) ):
			os.mkdir( path.replace( FONTS, INTER_FONT_TEXTURES ) )
		if not os.path.exists( path.replace( RAW, BAKED ) ):
			os.mkdir( path.replace( RAW, BAKED ) )
		for file in files:
			if( file.endswith( '.font' ) ):
				infile = os.path.join( path, file )
				outfontfile = infile.replace( RAW, BAKED ).replace( '.font', '.fnp' )
				outimagefile = infile.replace( FONTS, INTER_FONT_TEXTURES ).replace( '.font', '.tga' )
				# Don't test shouldbuild(outimagefile), because it's unnecessary and the name will be modified by FontGenerator
				if shouldbuild( infile, [ outfontfile ] ):
					runtool( [ tool, infile, outfontfile, outimagefile ] )

#-----------------------------------------------------
# Create font property and image files
def converttextures( tool, rawdir, rawpath, bakedpath ):
	for path, dirs, files in os.walk( rawdir ):
		if '.svn' in dirs:
			dirs.remove( '.svn' )	# Ignore SVN folders
		if not os.path.exists( path.replace( rawpath, bakedpath ) ):
			os.mkdir( path.replace( rawpath, bakedpath ) )
		for file in files:
			if( '.tga' in file ):
				# For Rosa, I'm changing these rules so I don't need the _NODXT suffix everywhere
				if( '_DXT' in file ):
					compression = 'DXT1'
					normalhack = ''	# Use -n with custom dxtex to swizzle R into A for quality (no longer used)
					createmips = '-m'
					if( '_NOMIPS' in file ):
						createmips = ''
					if( '_DXT2' in file ):
						compression = 'DXT2'
					if( '_DXT3' in file ):
						compression = 'DXT3'
					if( '_DXT4' in file ):
						compression = 'DXT4'
					if( '_DXT5' in file ):
						compression = 'DXT5'
					infile = os.path.join( path, file )
					outfile = infile.replace( rawpath, bakedpath ).replace( '.tga', '.dds' )
					if shouldbuild( infile, [ outfile ] ):
						runtool( [ tool, infile, createmips, normalhack, compression, outfile ] )
				else:
					infile = os.path.join( path, file )
					outfile = infile.replace( rawpath, bakedpath )
					if shouldbuild( infile, [ outfile ] ):
						runtool( [ COPY, infile, outfile ] )

#-----------------------------------------------------
# Create font property and image files
def compileshaders( compiler, copy, rawdir ):
	for path, dirs, files in os.walk( rawdir ):
		if '.svn' in dirs:
			dirs.remove( '.svn' )	# Ignore SVN folders
		if not os.path.exists( path.replace( RAW, BAKED ) ):
			os.mkdir( path.replace( RAW, BAKED ) )
		for file in files:

			if( file.endswith( '.fx' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.fx', '.cfx' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ compiler, '/T', 'fx_2_0', '/Fo', outfile, infile ] )

			elif( file.endswith( '.hlsl_vs2' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.hlsl_vs2', '.chv2' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ compiler, '/T', 'vs_2_0', '/E', 'Main', '/Fo', outfile, infile ] )

			elif( file.endswith( '.hlsl_ps3' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.hlsl_ps3', '.chp2' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ compiler, '/T', 'ps_3_0', '/E', 'Main', '/Fo', outfile, infile ] )

			elif( file.endswith( '.hlsl_ps2b' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.hlsl_ps2b', '.chp2' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ compiler, '/T', 'ps_2_b', '/E', 'Main', '/Fo', outfile, infile ] )

			elif( file.endswith( '.hlsl_ps2a' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.hlsl_ps2a', '.chp2' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ compiler, '/T', 'ps_2_a', '/E', 'Main', '/Fo', outfile, infile ] )

			elif( file.endswith( '.hlsl_ps2' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.hlsl_ps2', '.chp2' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ compiler, '/T', 'ps_2_0', '/E', 'Main', '/Fo', outfile, infile ] )

			elif( file.endswith( '.glsl_vs120' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.glsl_vs120', '.gv12' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ copy, infile, outfile ] )

			elif( file.endswith( '.glsl_fs120' ) ):
				infile = os.path.join( path, file )
				outfile = infile.replace( RAW, BAKED ).replace( '.glsl_fs120', '.gf12' )
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ copy, infile, outfile ] )

#-----------------------------------------------------
# Call Blender in background mode with a given script for each blend file
def blenddir( script, sourcedir, rawpath, bakedpath, bakedext ):
	for path, dirs, files in os.walk( sourcedir ):
		if '.svn' in dirs:
			dirs.remove( '.svn' )	# Ignore SVN folders
		if not os.path.exists( path.replace( rawpath, bakedpath ) ):
			os.mkdir( path.replace( rawpath, bakedpath ) )
		for file in files:
			if( file.endswith('.blend') ): # Cull .blend1, etc.
				infile = os.path.join( path, file )
				exportedfile = infile.replace( '.blend', bakedext )
				outfile = exportedfile.replace( rawpath, bakedpath )
				# Because I can't pass another parameter to Blender, I have to assume the name and move it
				if shouldbuild( infile, [ outfile ] ):
					runtool( [ BLENDER, '-b', infile, '-P', script ] )
					runtool( [ MOVE, '-f', exportedfile, outfile ] )

#-----------------------------------------------------
# Recursively delete a directory
def clean( cleanpath, ext = '' ):
	if os.path.exists( cleanpath ):
		for path, dirs, files in os.walk( cleanpath, False ):
			for file in files:
				if ext == '' or ext in file:
					os.remove( path + '/' + file )
			if ext == '':
				os.rmdir( path )

#-----------------------------------------------------
# Ensure a directory exists by making it if it does not
def makeifnotexists( path ):
	while not os.path.exists( path ):
		try:
			os.mkdir( path )
		except:
			pass

#-----------------------------------------------------
# Entry point
#-----------------------------------------------------

# TODO: Add exception handling (that also catches bad tool exit codes)
# NOTE: Python uses try/except/raise instead of try/catch/throw

# Ruleset must be the first parameter, if it's used at all
if len( sys.argv ) > 1:
	if sys.argv[1] in BAKE_RULES:
		ruleset = sys.argv[1]
	else:
		ruleset = None
else:
	ruleset = 'all'

for arg in sys.argv:
	if arg == '-r':
		REBUILD = True

if not ruleset:
	print 'Specified ruleset not found. Valid rulesets are:'
	for k, v in BAKE_RULES.iteritems():
		print k + ': ' + v[0]
else:
	rules = BAKE_RULES[ ruleset ]
	
	# Only clean the Intermediate/Baked folders if we're rebuilding all
	if( ( ruleset == 'all' and REBUILD ) or 'clean' in rules ):
		print 'Deleting Intermediate and Baked folders...'
		clean( INTERMEDIATE )
		clean( BAKED )

	# Clean up Blender junk
	clean( RAW, '.blend1' )

	makeifnotexists( INTERMEDIATE )
	makeifnotexists( BAKED )
	
	print 'Baking assets...'
	
	try:
		bake( rules )
	except:
		print 'Exception while baking: {0}'.format( sys.exc_info() )
		sys.exit(1)
	
	print 'Baking done!'
	print 'Baked %d files, skipped %d files.' % ( NUMBAKED, NUMSKIPPED )
	print 'Total %d files processed.' % ( NUMBAKED + NUMSKIPPED )
