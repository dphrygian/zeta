#!BPY

"""
Name: 'XML Mesh V2 (.mesh)...'
Blender: 243
Group: 'Export'
Tooltip: 'Export selected mesh to Engine XML Mesh V2 Format (.mesh)'
"""

__bpydoc__ = """\
Raw XML mesh output for Engine meshes.
"""

# Bones names must be less than 16 characters (my limit)
# Animation names must be less than 32 characters (my limit)

import sys
import Blender
from Blender import Mesh, Armature, Modifier, Mathutils
from Blender.Mathutils import *
import BPyMesh

# ========================
# === Write XML Format ===
# ========================
def write(filename):
	start = Blender.sys.time()
	if not filename.lower().endswith('.mesh'):
		filename += '.mesh'
	
	scn = Blender.Scene.GetCurrent()
	object = scn.objects.active
	if not object:
		Blender.Draw.PupMenu('Error%t|Select 1 active object')
		return

	meshObj = None
	mesh = None
	armObj = None
	arm = None
	bones = None

	if object.getType() == 'Armature':
		armObj = object
	elif object.getType() == 'Mesh':
		meshObj = object
	else:
		Blender.Draw.PupMenu('Error%t|Active object is not a supported type')
		return

	if meshObj:
		mesh = meshObj.getData(False,True)
		if not mesh:
			Blender.Draw.PupMenu('Error%t|Could not get mesh data from active object')
			return
	
		mesh.transform(meshObj.matrixWorld, True)
	
		# Try to get the armature (and the armature object, which
		# I need to get the pose) through the modifier stack
		for m in meshObj.modifiers:
			if m.type == Modifier.Types['ARMATURE']:
				armObj = m[Modifier.Settings['OBJECT']] # Armature.Get(m.name)
				break
	
	if not armObj:
		# If armature is not applied as a modifier, try to get it as the object's parent
		print 'No armature modifier.'
		armObj = object.parent
		if not armObj:
			print 'No parent object.'
		elif armObj.type != 'Armature':
			print 'Parent object is not an armature.'
			armObj = None
		else:
			print 'Armature found from parent: OB:', armObj.name
			arm = armObj.getData(False,False)
	else:
		print 'Armature found from modifier: OB:', armObj.name
		arm = armObj.getData(False,False)

	if arm:
		bones = arm.bones.values()
		if not arm.vertexGroups:
			print 'Warning: Armature is not using vertex groups.'

	if mesh:
		vgNames = mesh.getVertGroupNames()
		vgroups = [mesh.getVertsFromGroup(vg,1) for vg in vgNames]	# Python is awesome!
	
	file = open(filename, "wb")
	file.write('<mesh>\n')

	if mesh:
		# Output all vertices in all faces--this is redundant, but that's okay,
		# because Blender doesn't have index lists, so I have to do a pass to
		# produce those anyway, and I can cull duplicate vertices then (and if
		# I just iterate on the verts, I only get one UV per vert).
		for face in mesh.faces:
		
			file.write('\t<face>\n')
			
			idx = 0;
			for vert in face.verts:
			
				file.write('\t\t<vert>\n')
				file.write('\t\t\t<pos x="%.6f" y="%.6f" z="%.6f" />\n' % tuple(vert.co))
				
				if mesh.faceUV:
					file.write('\t\t\t<uv x="%.6f" y="%.6f" />\n' % tuple(face.uv[idx]))
				elif mesh.vertexUV:
					file.write('\t\t\t<uv x="%.6f" y="%.6f" />\n' % tuple(vert.uvco))
					
				file.write('\t\t\t<norm x="%.6f" y="%.6f" z="%.6f" />\n' % tuple(vert.no))

				if mesh.vertexColors:
					col = face.col[idx]
					file.write('\t\t\t<col r="%d" g="%d" b="%d" />\n' % (col.r, col.g, col.b))
				
				# Bones--this will get expensive
				vgiter = 0
				numbones = 0
				for vg in vgroups:
					for (vgi,vgw) in vg:
						if vgi == vert.index:
							numbones = numbones + 1
							if numbones > 4:
								print 'Warning: More than 4 bones applied on a vertex (%s)' % vgNames[vgiter]
							else:
								file.write('\t\t\t<bone name="%s" wgt="%.6f" />\n' % (vgNames[vgiter], vgw))
					vgiter = vgiter + 1
					
				file.write('\t\t</vert>\n')
				
				idx = idx + 1
				
			file.write('\t</face>\n')

	if bones:
		curframe = Blender.Get('curframe')
		file.write('\t<armature frames="%d">\n' % (Blender.Get('endframe')-Blender.Get('staframe')+1))
		
		for bone in bones:
			if not bone.hasParent():	# Start with the root bone(s) and recurse
				writeBone(file,bone,armObj,True)
				
		# Write keyframe marker names
		# (When animating, use markers in the timeline to set
		# keyframes at the start of each separate animation in the file
		timeline = Blender.Scene.GetCurrent().getTimeLine();
		if timeline:
			for f in range(Blender.Get('staframe'),Blender.Get('endframe')+1):
				if(timeline.getMarked(f)):
					file.write('\t\t<anim frame="%d" name="%s" />\n' % ( f, timeline.getName(f) ))

		file.write('\t</armature>\n')
		Blender.Set('curframe',curframe)

	if mesh:
		# Material getter stuff
		for mat in mesh.materials:
			if mat:
				for mtex in mat.getTextures():
					if mtex and mtex.tex and mtex.tex.image:
						file.write('\t<tex file="%s" />\n' % mtex.tex.image.filename)
		
	file.write('</mesh>\n')

	if mesh:
		mesh.transform(meshObj.getInverseMatrix(), True)
	
	file.close()
	
	end = Blender.sys.time()
	message = 'Successfully exported "%s" in %.4f seconds' % ( Blender.sys.basename(filename), end-start)
	print message

def writeBone(file,bone,armObj,recurse):
	pose = armObj.getPose()
	parentName = bone.parent.name if bone.parent else ''
	file.write( '\t\t<bonedef name="%s" parent="%s" length="%.6f">\n' % ( bone.name, parentName, bone.length ) )

	# Write inverse bind pose (i.e., the transformation from bone space to armature/object space)
	armMatrix = bone.matrix[ 'ARMATURESPACE' ]
	invBindPoseMatrix = armMatrix.copy().invert()
	file.write( '\t\t\t<invbind\n' )
	file.write( '\t\t\t\tm00="%.6f" m01="%.6f" m02="%.6f" m03="%.6f"\n' % tuple( invBindPoseMatrix[0] ) )
	file.write( '\t\t\t\tm10="%.6f" m11="%.6f" m12="%.6f" m13="%.6f"\n' % tuple( invBindPoseMatrix[1] ) )
	file.write( '\t\t\t\tm20="%.6f" m21="%.6f" m22="%.6f" m23="%.6f"\n' % tuple( invBindPoseMatrix[2] ) )
	file.write( '\t\t\t\tm30="%.6f" m31="%.6f" m32="%.6f" m33="%.6f"\n' % tuple( invBindPoseMatrix[3] ) )
	file.write( '\t\t\t/>\n' )

	# Write each frame
	for f in range(Blender.Get('staframe'),Blender.Get('endframe')+1):
		Blender.Set('curframe',f)
		posebone = pose.bones[bone.name]

		if( posebone.parent ):
			poseMatrix = posebone.poseMatrix * posebone.parent.poseMatrix.copy().invert()
		else:
			poseMatrix = posebone.poseMatrix

		poserot = poseMatrix.toQuat()
		poseloc = poseMatrix.translationPart()

		file.write( '\t\t\t<frame num="%d">\n' % f )
		file.write( '\t\t\t\t<rot w="%.6f" x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( poserot ) )
		file.write( '\t\t\t\t<loc x="%.6f" y="%.6f" z="%.6f" />\n' % tuple( poseloc ) )
		file.write( '\t\t\t</frame>\n' )
	file.write('\t\t</bonedef>\n')

	if recurse and bone.hasChildren():
		for child in bone.getAllChildren():
			writeBone(file,child,armObj,False)


def main():
	if Blender.mode == 'interactive':
		Blender.Window.FileSelector(write, 'XML Mesh Export', Blender.sys.makename(ext='.mesh'))
	else:
		# Find the blend file argument
		for arg in sys.argv:
			if '.blend' in arg:
				write(Blender.sys.makename(arg, '.mesh'))

if __name__=='__main__':
	main()