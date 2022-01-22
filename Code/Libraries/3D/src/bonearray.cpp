#include "core.h"
#include "bonearray.h"
#include "animation.h"
#include "simplestring.h"
#include "hashedstring.h"
#include "configmanager.h"

#include <memory.h>
#include <string.h>

BoneArray::BoneArray()
:	m_RefCount( 0 )
,	m_NumFrames( 0 )
,	m_NumBones( 0 )
,	m_NumAnimations( 0 )
,	m_BoneInfos()
,	m_Bones()
,	m_Animations()
{
}

BoneArray::~BoneArray()
{
}

int BoneArray::AddReference()
{
	++m_RefCount;
	return m_RefCount;
}

int BoneArray::Release()
{
	--m_RefCount;
	if( m_RefCount <= 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCount;
}

// Make deep copies so the caller can delete its arrays
// (I chose to do this because it's synonymous with copying
// verts into D3D buffers.)
void BoneArray::Init( const SBoneInfo* const pBoneInfos, const SBone* const pBones, const char* const MeshFilename, const SAnimData* const pAnimData, const int NumFrames, const int NumBones, const int NumAnims )
{
	DEVASSERT( pBoneInfos );
	DEVASSERT( pBones );
	DEVASSERT( pAnimData );

	m_NumFrames				= NumFrames;
	m_NumBones				= NumBones;
	m_NumAnimations			= NumAnims;
	const int TotalBones	= m_NumFrames * m_NumBones;

	m_BoneInfos.Resize( NumBones );
	memcpy_s( m_BoneInfos.GetData(),	sizeof( SBoneInfo )	* NumBones,		pBoneInfos,		sizeof( SBoneInfo )	* NumBones );

	m_Bones.Resize( TotalBones );
	memcpy_s( m_Bones.GetData(),		sizeof( SBone )		* TotalBones,	pBones,			sizeof( SBone )		* TotalBones );

	STATICHASH( AnimationMap );
	MAKEHASH( MeshFilename );
	const SimpleString AnimationMapName = ConfigManager::GetString( sMeshFilename, "", sAnimationMap );

	// Set anim properties from config
	for( int AnimIndex = 0; AnimIndex < NumAnims; ++AnimIndex )
	{
		const SAnimData& AnimData = pAnimData[ AnimIndex ];
		Animation& Anim = m_Animations.PushBack();
		Anim.m_AnimData = AnimData;
		Anim.InitializeFromDefinition( SimpleString::PrintF( "%s:%s", AnimationMapName.CStr(), AnimData.m_Name ) );
	}
}

int BoneArray::GetBoneIndex( const HashedString& Name ) const
{
	DEVASSERT( m_BoneInfos.GetData() );

	for( int BoneIndex = 0; BoneIndex < m_NumBones; ++BoneIndex )
	{
		if( Name == m_BoneInfos[ BoneIndex ].m_Name )
		{
			return BoneIndex;
		}
	}

	return INVALID_INDEX;
}

int BoneArray::GetNumFrames() const
{
	return m_NumFrames;
}

int BoneArray::GetNumBones() const
{
	return m_NumBones;
}

int BoneArray::GetNumAnimations() const
{
	return m_NumAnimations;
}

Animation* BoneArray::GetAnimation( int Index ) const
{
	if( Index == INVALID_INDEX )
	{
		return NULL;
	}

	// This case is sometimes occurring in Eldritch due to hand animation swapping.
	// Until I can isolate a cause, I'm making it more graceful.
	if( Index >= m_NumAnimations )
	{
		WARN;
		return NULL;
	}

	DEVASSERT( m_Animations.GetData() );
	return m_Animations.GetData() + Index;
}

Animation* BoneArray::GetAnimation( const HashedString& Name ) const
{
	DEVASSERT( m_Animations.GetData() );

	for( int i = 0; i < m_NumAnimations; ++i )
	{
		if( Name == m_Animations[i].m_AnimData.m_HashedName )
		{
			return m_Animations.GetData() + i;
		}
	}

	DEVWARNDESC( "BoneArray::GetAnimation: Requested animation not found." );
	return NULL;
}

int BoneArray::GetAnimationIndex( const Animation* pAnimation ) const
{
	for( int i = 0; i < m_NumAnimations; ++i )
	{
		if( m_Animations.GetData() + i == pAnimation )
		{
			return i;
		}
	}

	return INVALID_INDEX;
}
