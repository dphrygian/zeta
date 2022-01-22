#!BPY

""" Registration info for Blender menus: <- these words are ignored
Name: 'Project Scaled (Auto)'
Blender: 240
Group: 'UVCalculation'
Tooltip: 'UV Unwrap mesh faces from view projection and scale'
"""

__bpydoc__ = """\
Project face UVs from view, scale to the size of the faces, then scale by a user-defined texture size scalar.
"""

from collections import defaultdict
from Blender import Object, Draw, Window, sys, Mesh, Geometry
from Blender.Mathutils import Matrix, Vector, RotationMatrix
import bpy
from math import cos

def VecToMat(dir):
	fd = dir.__copy__().normalize()
	
	up = Vector(0,0,1)
	if abs(fd.dot(up)) > 0.99:
		up = Vector(0,1,0)
	
	rt = fd.cross(up).normalize()
	up = rt.cross(fd)
	fd = -fd
	return Matrix(
		[rt[0], up[0], fd[0]],
		[rt[1], up[1], fd[1]],
		[rt[2], up[2], fd[2]])

def main():
	scene = bpy.data.scenes.active
	object = scene.objects.active
	if not object or object.type != 'Mesh':
		return

	while Window.GetMouseButtons() != 0:
		sys.sleep(10)

	mesh = object.getData( False, True )

	# PupBlock values
	USER_TEXTURE_SIZE = Draw.Create(1.0)
	USER_NORMAL_FUDGE = Draw.Create(1.0)
	USER_SEPARATE_ISLANDS = Draw.Create(1)

	# PupBlock sequence
	pup_block = [\
	('Texture Size (m):', USER_TEXTURE_SIZE, 0.0, 10.0, 'Size in meters of one texture repetition on the surface.'),\
	('Normal Sim. Fac:', USER_NORMAL_FUDGE, 0.0, 10.0, 'Similarity factor for merging normal groups, higher values merge more easily.'),\
	('Sep. Islands:', USER_SEPARATE_ISLANDS, 'Move UV islands for easier selection.'),\
	]

	if not Draw.PupBlock( 'Project Scaled (Auto)', pup_block ):
		return

	is_editmode = Window.EditMode()
	if is_editmode:
		Window.EditMode(0)

	proj_mat = Window.GetViewMatrix().copy().invert() * object.matrixWorld.copy().invert()

	scale = 1.0
	if USER_TEXTURE_SIZE.val > 0.0:
		scale = 1.0 / USER_TEXTURE_SIZE.val

	simfac = 1.0
	if USER_NORMAL_FUDGE.val > 0.0:
		simfac = 1.0 / USER_NORMAL_FUDGE.val

	if mesh.faceUV == False:
		mesh.faceUV = True

	selectedFaces = [f for f in mesh.faces if f.sel]
	if len(selectedFaces) > 0:

		# Gather faces into buckets by normal
		faceBuckets = defaultdict( list )
		for face in selectedFaces:
			tupleNormal = tuple( [
				round( face.no.x * simfac, 1 ) / simfac,
				round( face.no.y * simfac, 1 ) / simfac,
				round( face.no.z * simfac, 1 ) / simfac ] )
			faceBuckets[ tupleNormal ].append( face )

		offsetUV = Vector( 0, 0 )
		for tupleNormal, faces in faceBuckets.iteritems():
			normal = Vector( tupleNormal )
			proj_mat = VecToMat( -normal ).copy().invert()
			UVsSet = False
			minUV = Vector( 0, 0 )
			maxUV = Vector( 0, 0 )
			for face in faces:
				for i, v in enumerate(face.v):
					face.uv[i][:] = (proj_mat * v.co)[:2]
					if UVsSet:
						minUV[0] = min( minUV[0], face.uv[i][0] )
						minUV[1] = min( minUV[1], face.uv[i][1] )
						maxUV[0] = max( maxUV[0], face.uv[i][0] )
						maxUV[1] = max( maxUV[1], face.uv[i][1] )
					else:
						UVsSet = True
						minUV[0] = face.uv[i][0]
						minUV[1] = face.uv[i][1]
						maxUV[0] = face.uv[i][0]
						maxUV[1] = face.uv[i][1]

			for face in faces:
				for i, v in enumerate(face.v):
					face.uv[i][0] += offsetUV[0] - minUV[0]
					face.uv[i][1] += offsetUV[1] - minUV[1]
					face.uv[i][0] *= scale
					face.uv[i][1] *= scale

			if USER_SEPARATE_ISLANDS.val:
				offsetUV += ( maxUV - minUV )
				offsetUV[0] = round( offsetUV[0] + 1.5 )
				offsetUV[1] = round( offsetUV[1] + 1.5 )

	if is_editmode:
		Window.EditMode(1)
	
	Window.RedrawAll()

if __name__=='__main__':
	main()