#ifndef DYNAMICMESHMANAGER_H
#define DYNAMICMESHMANAGER_H

#include "map.h"
#include "hashedstring.h"
#include "material.h"
#include "meshfactory.h"

class IVertexBuffer;
class IIndexBuffer;
class BoneArray;
class IVertexDeclaration;
class MeshFactory;
class Mesh;

// DynamicMeshManager keeps an unused copy of each dynamic mesh
// around so that the vertex buffers can be shared and aren't
// automatically freed when every instance is deleted.

class DynamicMeshManager
{
private:
	DynamicMeshManager();
	~DynamicMeshManager();

public:
	static DynamicMeshManager*	GetInstance();
	static void					DeleteInstance();

	void						FreeMeshes();
	Mesh*						GetOrCreateMesh( const char* Filename, MeshFactory* pFactory, MeshFactory::SReadMeshCallback Callback = MeshFactory::SReadMeshCallback() );
	Mesh*						GetMesh( const char* Filename ) const;

	Map<HashedString, Mesh*>	m_Meshes;

	static DynamicMeshManager*	m_Instance;
};

#endif // DYNAMICMESHMANAGER_H
