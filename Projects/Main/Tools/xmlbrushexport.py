#!BPY

"""
Name: 'Brush V1 (.brush)...'
Blender: 243
Group: 'Export'
Tooltip: 'Export selected mesh to Engine Brush V1 Format (.brush)'
"""

__bpydoc__ = """\
Intermediate output for Engine brushes.
"""

# This emits a .brush file (in config file format)
# and a .mesh for each object in the scene.

import sys
import Blender
from Blender import Mesh, Modifier, Mathutils
from Blender.Mathutils import *
import BPyMesh
from collections import defaultdict

def safeGetProp( object, name, type, default ):
	try: 
		if not object:
			return default

		prop = object.getProperty( name )
		if not prop:
			return default

		if prop.getType() != type:
			return default

		return prop.getData()
	except:
		return default

def exportMesh( subFilename, objects ):
	subFile = open( subFilename, "wb" )
	subFile.write( '<mesh>\n' )

	# Only export the materials for the first object in the merge group
	firstObject = objects[0]
	firstMesh = firstObject.getData( False, True )
	for mat in firstMesh.materials:
		if mat:
			for mtex in mat.getTextures():
				if mtex and mtex.tex and mtex.tex.image:
					subFile.write( '\t<tex file="%s" />\n' % mtex.tex.image.filename )

	for object in objects:
		# HACKHACK: Modified radial normals for foliage
		radialNormals	= safeGetProp( object, 'RadialNormals', 'BOOL', False )
		radialOffsetZ	= safeGetProp( object, 'RadialOffsetZ', 'FLOAT', 0.0 )
		radialScalarZ	= 1.0 / safeGetProp( object, 'RadialScalarZ', 'FLOAT', 1.0 )	# Divide here, since it's more intuitive to imagine scaling a sphere around the object
		objLoc			= Vector( object.loc )
		objLoc.z		+= radialOffsetZ

		directedNormals	= safeGetProp( object, 'DirectedNormals', 'BOOL', False )
		directedX		= safeGetProp( object, 'DirectedX', 'FLOAT', 0.0 )
		directedY		= safeGetProp( object, 'DirectedY', 'FLOAT', 0.0 )
		directedZ		= safeGetProp( object, 'DirectedZ', 'FLOAT', 0.0 )
		directedNormal	= Vector( directedX, directedY, directedZ )
		directedNormal.normalize()

		normalsB		= safeGetProp( object, 'NormalsB', 'BOOL', False )

		mesh = object.getData( False, True )
		mesh.transform( object.matrixWorld, True )

		for face in mesh.faces:
			subFile.write('\t<face>\n')
			
			idx = 0;
			for vert in face.verts:
				subFile.write( '\t\t<vert>\n' )
				subFile.write( '\t\t\t<pos x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( vert.co ) )
				
				if mesh.faceUV:
					subFile.write( '\t\t\t<uv x="%.6f" y="%.6f" />\n' % tuple( face.uv[ idx ] ) )
				elif mesh.vertexUV:
					subFile.write( '\t\t\t<uv x="%.6f" y="%.6f" />\n' % tuple( vert.uvco ) )

				if radialNormals:
					vertLoc			= vert.co
					vertOffset		= vertLoc - objLoc
					vertOffset.z	*= radialScalarZ
					vertOffset.normalize()
					
					# ZETA: Write the actual normals as norm and the bent normals as normb
					subFile.write( '\t\t\t<norm x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( vert.no ) )
					subFile.write( '\t\t\t<normb x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( vertOffset ) )
				elif directedNormals:
					# ZETA: Write the actual normals as norm and the bent normals as normb
					subFile.write( '\t\t\t<norm x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( vert.no ) )
					subFile.write( '\t\t\t<normb x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( directedNormal ) )
				else:
					subFile.write( '\t\t\t<norm x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( vert.no ) )
					if normalsB:
						# Also write the normals as a second channel, which we need for foliage
						subFile.write( '\t\t\t<normb x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( vert.no ) )

				if mesh.vertexColors:
					col = face.col[ idx ]
					subFile.write( '\t\t\t<col r="%d" g="%d" b="%d" />\n' % ( col.r, col.g, col.b ) )

				subFile.write( '\t\t</vert>\n' )

				idx = idx + 1
			subFile.write( '\t</face>\n' )

		mesh.transform( object.getInverseMatrix(), True )
		
	subFile.write('</mesh>\n')
	subFile.close()

def write( brushFilename ):
	startTime = Blender.sys.time()
	if not brushFilename.lower().endswith('.brush'):
		brushFilename += '.brush'
	
	currentScene = Blender.Scene.GetCurrent()
	
	meshes				= []
	materials			= []
	hulls				= []
	surfaces			= []
	blocksentities		= []
	blockstraces		= []
	blocksocclusions	= []
	blocksaudios		= []
	navignores			= []
	ambients			= []
	cubemaps			= []
	fogmeshes			= []
	fogmeshdefs			= []
	editormeshes		= []
	editorhulls			= []
	editorhiddens		= []
	castsshadows		= []
	
	mergegroups			= defaultdict( list )

	for object in currentScene.objects:
		if safeGetProp( object, 'Ignore', 'BOOL', False ):
			# Do nothing with this mesh!
			continue

		mergeGroupName = safeGetProp( object, 'MergeGroup', 'STRING', object.name )

		subName = '-' + mergeGroupName + '.mesh'
		subFilename = brushFilename.replace( '.brush', subName )
		subFilename = subFilename.replace( 'Raw/', 'Intermediate/' )	# HACKHACK because Blender wants to output beside .blend file
		
		mergegroups[ subFilename ].append( object )

		# "SoftCover" is a shortcut for a hull which only blocks occlusion
		if safeGetProp( object, 'SoftCover', 'BOOL', False ):
			hulls.append(				subFilename )
			surfaces.append(			'' )
			blocksentities.append(		False )
			blockstraces.append(		False )
			blocksocclusions.append(	True )
			blocksaudios.append(		False )
			navignores.append(			True )

		# "Glass" is a shortcut for a hull which blocks everything but occlusion
		elif safeGetProp( object, 'Glass', 'BOOL', False ):
			hulls.append(				subFilename )
			surfaces.append(			safeGetProp( object, 'Surface',			'STRING',	'' ) )
			blocksentities.append(		True )
			blockstraces.append(		True )
			blocksocclusions.append(	False )
			blocksaudios.append(		True )
			navignores.append(			safeGetProp( object, 'NavIgnore',		'BOOL',		False ) )

		elif safeGetProp( object, 'Hull', 'BOOL', False ) or ( safeGetProp( object, 'Surface', 'STRING', '' ) and not safeGetProp( object, 'EditorHull', 'BOOL', False ) ):
			hulls.append(				subFilename )
			surfaces.append(			safeGetProp( object, 'Surface',			'STRING',	'' ) )
			blocksentities.append(		safeGetProp( object, 'BlocksEntities',	'BOOL',		True ) )
			blockstraces.append(		safeGetProp( object, 'BlocksTrace',		'BOOL',		True ) )
			blocksocclusions.append(	safeGetProp( object, 'BlocksOcclusion',	'BOOL',		True ) )
			blocksaudios.append(		safeGetProp( object, 'BlocksAudio',		'BOOL',		True ) )
			navignores.append(			safeGetProp( object, 'NavIgnore',		'BOOL',		False ) )

		elif safeGetProp( object, 'Ambient', 'BOOL', False ) or safeGetProp( object, 'Cubemap', 'STRING', '' ):
			ambients.append( subFilename )
			cubemaps.append( safeGetProp( object, 'Cubemap', 'STRING', '' ) )

		elif safeGetProp( object, 'FogMesh', 'BOOL', False ) or safeGetProp( object, 'FogMeshDef', 'STRING', '' ):
			fogmeshes.append( subFilename )
			fogmeshdefs.append( safeGetProp( object, 'FogMeshDef', 'STRING', '' ) )

		elif safeGetProp( object, 'EditorMesh', 'BOOL', False ):
			editormeshes.append( subFilename )

		elif safeGetProp( object, 'EditorHull', 'BOOL', False ):
			editorhulls.append( subFilename )

		else:
			if subFilename in meshes:
				# We've already added this mesh! Don't add redundant meshes when using MergeGroup
				pass
			else:
				meshes.append( subFilename )
				materials.append( safeGetProp( object, 'Material', 'STRING', '' ) )
				editorhiddens.append( safeGetProp( object, 'EditorHidden', 'BOOL', False ) )
				castsshadows.append( safeGetProp( object, 'CastsShadows', 'BOOL', True ) )

	for filename, objects in mergegroups.iteritems():
		exportMesh( filename, objects )

	brushFile = open( brushFilename, "wb" )

	# HACKHACK: We shouldn't know anything about bake structure here either
	relativeBrushFilename = brushFilename.lstrip( '../Raw/' ).replace( '.brush', '.cbr' ).replace( '\\', '/' )
	brushFile.write( '[%s]\n' % relativeBrushFilename )

	brushFile.write( 'NumMeshes = &\n' )
	brushFile.write( '@ Mesh\n' )
	for meshFilename, materialName, editorHidden, castsShadows in zip( meshes, materials, editorhiddens, castsshadows ):
		# HACKHACK: We shouldn't know anything about bake structure here either
		relativeMeshFilename = meshFilename.lstrip( '../Intermediate/' ).replace( '.mesh', '.cms' ).replace( '\\', '/' )
		brushFile.write( '@@& = "%s"\n' % relativeMeshFilename )
		if materialName != '':
			brushFile.write( '@@^Material = "%s"\n' % materialName )
		if editorHidden:
			brushFile.write( '@@^EditorHidden = true\n' )
		if not castsShadows:
			brushFile.write( '@@^CastsShadows = false\n' )

	brushFile.write( 'NumHulls = &\n' )
	brushFile.write( '@ Hull\n' )
	for hullFilename, surfaceName, blocksEntities, blocksTrace, blocksOcclusion, blocksAudio, navIgnore in zip( hulls, surfaces, blocksentities, blockstraces, blocksocclusions, blocksaudios, navignores ):
		# HACKHACK: We shouldn't know anything about bake structure here either
		relativeHullFilename = hullFilename.lstrip( '../Intermediate/' ).replace( '.mesh', '.cms' ).replace( '\\', '/' )
		brushFile.write( '@@& = "%s"\n' % relativeHullFilename )
		if surfaceName != '':
			brushFile.write( '@@^Surface = "%s"\n' % surfaceName )
		if not blocksEntities:
			brushFile.write( '@@^BlocksEntities = false\n' )
		if not blocksTrace:
			brushFile.write( '@@^BlocksTrace = false\n' )
		if not blocksOcclusion:
			brushFile.write( '@@^BlocksOcclusion = false\n' )
		if not blocksAudio:
			brushFile.write( '@@^BlocksAudio = false\n' )
		if navIgnore:
			brushFile.write( '@@^NavIgnore = true\n' )

	brushFile.write( 'NumAmbientLights = &\n' )
	brushFile.write( '@ AmbientLight\n' )
	for ambientFilename, cubemapName in zip( ambients, cubemaps ):
		# HACKHACK: We shouldn't know anything about bake structure here either
		relativeAmbientFilename = ambientFilename.lstrip( '../Intermediate/' ).replace( '.mesh', '.cms' ).replace( '\\', '/' )
		brushFile.write( '@@& = "%s"\n' % relativeAmbientFilename )
		if cubemapName != '':
			brushFile.write( '@@^Cubemap = "%s"\n' % cubemapName )

	brushFile.write( 'NumFogMeshes = &\n' )
	brushFile.write( '@ FogMesh\n' )
	for fogMeshFilename, fogMeshDefName in zip( fogmeshes, fogmeshdefs ):
		# HACKHACK: We shouldn't know anything about bake structure here either
		relativeFogMeshFilename = fogMeshFilename.lstrip( '../Intermediate/' ).replace( '.mesh', '.cms' ).replace( '\\', '/' )
		brushFile.write( '@@& = "%s"\n' % relativeFogMeshFilename )
		if fogMeshDefName != '':
			brushFile.write( '@@^FogMeshDef = "%s"\n' % fogMeshDefName )

	brushFile.write( 'NumEditorMeshes = &\n' )
	brushFile.write( '@ EditorMesh\n' )
	for meshFilename in editormeshes:
		# HACKHACK: We shouldn't know anything about bake structure here either
		relativeMeshFilename = meshFilename.lstrip( '../Intermediate/' ).replace( '.mesh', '.cms' ).replace( '\\', '/' )
		brushFile.write( '@@& = "%s"\n' % relativeMeshFilename )

	brushFile.write( 'NumEditorHulls = &\n' )
	brushFile.write( '@ EditorHull\n' )
	for hullFilename in editorhulls:
		# HACKHACK: We shouldn't know anything about bake structure here either
		relativeHullFilename = hullFilename.lstrip( '../Intermediate/' ).replace( '.mesh', '.cms' ).replace( '\\', '/' )
		brushFile.write( '@@& = "%s"\n' % relativeHullFilename )

	brushFile.close()

	endTime = Blender.sys.time()
	totalTime = endTime - startTime

	message = 'Successfully exported "%s" in %.4f seconds' % ( Blender.sys.basename( brushFilename ), totalTime )
	print message

def main():
	if Blender.mode == 'interactive':
		Blender.Window.FileSelector( write, 'Brush Export', Blender.sys.makename( ext = '.brush' ) )
	else:
		# Find the blend file argument
		for arg in sys.argv:
			if '.blend' in arg:
				write( Blender.sys.makename( arg, '.brush' ) )

if __name__=='__main__':
	main()