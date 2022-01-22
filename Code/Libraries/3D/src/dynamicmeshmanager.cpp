#include "core.h"
#include "3d.h"
#include "dynamicmeshmanager.h"
#include "mesh.h"
#include "irenderer.h"
#include "packstream.h"
#include "meshfactory.h"
#include "fileutil.h"

#include <memory.h>

DynamicMeshManager* DynamicMeshManager::m_Instance = NULL;

DynamicMeshManager::DynamicMeshManager()
:	m_Meshes() {}

DynamicMeshManager::~DynamicMeshManager()
{
	FreeMeshes();
}

DynamicMeshManager* DynamicMeshManager::GetInstance()
{
	if( !m_Instance )
	{
		m_Instance = new DynamicMeshManager;
	}
	return m_Instance;
}

void DynamicMeshManager::DeleteInstance()
{
	SafeDelete( m_Instance );
}

void DynamicMeshManager::FreeMeshes()
{
	FOR_EACH_MAP( Iter, m_Meshes, HashedString, Mesh* )
	{
		SafeDelete( *Iter );
	}
	m_Meshes.Clear();
}

Mesh* DynamicMeshManager::GetOrCreateMesh( const char* Filename, MeshFactory* pFactory, MeshFactory::SReadMeshCallback Callback /*= MeshFactory::SReadMeshCallback()*/ )
{
	const HashedString HashedFilename( Filename );
	Map<HashedString, Mesh*>::Iterator MeshIter = m_Meshes.Search( HashedFilename );
	if( MeshIter.IsNull() )
	{
		MeshIter = m_Meshes.Insert( HashedFilename, pFactory->Read( PackStream( Filename ), Filename, NULL, NULL, Callback ) );
	}
	return ( MeshIter.IsValid() ) ? MeshIter.GetValue() : NULL;
}

Mesh* DynamicMeshManager::GetMesh( const char* Filename ) const
{
	const HashedString HashedFilename( Filename );
	Map<HashedString, Mesh*>::Iterator MeshIter = m_Meshes.Search( Filename );
	return ( MeshIter.IsValid() ) ? MeshIter.GetValue() : NULL;
}
