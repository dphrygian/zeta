#!BPY

""" Registration info for Blender menus: <- these words are ignored
Name: 'Project Scaled'
Blender: 240
Group: 'UVCalculation'
Tooltip: 'UV Unwrap mesh faces from view projection and scale'
"""

__bpydoc__ = """\
Project face UVs from view, scale to the size of the faces, then scale by a user-defined texture size scalar.
"""

from Blender import Object, Draw, Window, sys, Mesh, Geometry
from Blender.Mathutils import Matrix, Vector, RotationMatrix
import bpy
from math import cos

def VecToMat(dir):
	fd = dir.__copy__().normalize()
	
	up = Vector(0,0,1)
	if abs(fd.dot(up)) > 0.99:
		print 'VecToMat: Given vector is parallel to up vector, using Y+ as up vector'
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
	USER_PROJECT_SURFACE = Draw.Create(0)

	# PupBlock sequence
	pup_block = [\
	('Texture Size (m):', USER_TEXTURE_SIZE, 0.0, 10.0, 'Size in meters of one texture repetition on the surface.'),\
	('Use Surface Normal:', USER_PROJECT_SURFACE, 'Project onto the average surface normal instead of the view.'),\
	]

	if not Draw.PupBlock( 'Project Scaled', pup_block ):
		return

	is_editmode = Window.EditMode()
	if is_editmode:
		Window.EditMode(0)

	proj_mat = Window.GetViewMatrix().copy().invert() * object.matrixWorld.copy().invert()

	scale = 1.0
	if USER_TEXTURE_SIZE.val > 0.0:
		scale = 1.0 / USER_TEXTURE_SIZE.val

	if mesh.faceUV == False:
		mesh.faceUV = True

	selectedFaces = [f for f in mesh.faces if f.sel]
	if len(selectedFaces) > 0:

		# Change the projection matrix if needed
		if USER_PROJECT_SURFACE.val:
			avg_normal = Vector(0,0,0)
			for face in selectedFaces:
				avg_normal += face.no
			avg_normal /= len(selectedFaces)
			proj_mat = VecToMat(-avg_normal).copy().invert()

		minUVSet = False
		minUV = selectedFaces[0].uv[0].copy()
		for face in selectedFaces:
			for i, v in enumerate(face.v):
				face.uv[i][:] = (proj_mat * v.co)[:2]
				if minUVSet:
					minUV[0] = min( minUV[0], face.uv[i][0] )
					minUV[1] = min( minUV[1], face.uv[i][1] )
				else:
					minUVSet = True
					minUV[0] = face.uv[i][0]
					minUV[1] = face.uv[i][1]

		for face in selectedFaces:
			for i, v in enumerate(face.v):
				face.uv[i][0] -= minUV[0]
				face.uv[i][1] -= minUV[1]
				face.uv[i][0] *= scale
				face.uv[i][1] *= scale

	if is_editmode:
		Window.EditMode(1)
	
	Window.RedrawAll()

if __name__=='__main__':
	main()