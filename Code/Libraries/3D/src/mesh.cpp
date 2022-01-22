#include "core.h"
#include "mesh.h"
#include "bonearray.h"
#include "ivertexdeclaration.h"
#include "configmanager.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "mathcore.h"

Mesh::Mesh( IVertexBuffer* pVertexBuffer, IVertexDeclaration* pVertexDeclaration, IIndexBuffer* pIndexBuffer, BoneArray* pBones )
:	m_VertexBuffer( NULL )
,	m_VertexDeclaration( NULL )
,	m_IndexBuffer( NULL )
,	m_Bones( NULL )
,	m_Material()
,	m_MultiPassMaterials()
,	m_AllowMaterialOverrides( true )
,	m_Location()
,	m_Scale( 1.0f, 1.0f, 1.0f )
,	m_Rotation()
,	m_CACHED_Location()
,	m_CACHED_Scale()
,	m_CACHED_Rotation()
,	m_PreconcatenatedTransform()
,	m_AABB()
,	m_OriginalAABB()
,	m_AnimationState( NULL )
,	m_ConstantColor( 1.0f, 1.0f, 1.0f, 1.0f )
,	m_ShaderConstants()
#if BUILD_DEV
,	m_DEV_IsDebugMesh( false )
#endif
#if BUILD_DEBUG
,	m_DEBUG_Name( "" )
#endif
{
	Initialize( pVertexBuffer, pVertexDeclaration, pIndexBuffer, pBones );
}

Mesh::~Mesh()
{
	if( IsAnimationOwner() )
	{
		// Just to be safe! The animation state shouldn't ever be doing
		// anything once the owner mesh is deleted, but still.
		m_AnimationState->SetOwnerMesh( NULL );
	}

	SafeRelease( m_VertexBuffer );
	SafeRelease( m_IndexBuffer );
	SafeRelease( m_Bones );
	SafeRelease( m_AnimationState );
}

void Mesh::Initialize( IVertexBuffer* pVertexBuffer, IVertexDeclaration* pVertexDeclaration, IIndexBuffer* pIndexBuffer, BoneArray* pBones )
{
	m_VertexBuffer = pVertexBuffer;
	if( m_VertexBuffer )
	{
		m_VertexBuffer->AddReference();
	}

	m_VertexDeclaration = pVertexDeclaration;

	m_IndexBuffer = pIndexBuffer;
	if( m_IndexBuffer )
	{
		m_IndexBuffer->AddReference();
	}

	m_Bones = pBones;
	if( m_Bones )
	{
		m_Bones->AddReference();

		// Latently create an animation state iff we have bones
		m_AnimationState = new AnimationState;
		m_AnimationState->AddReference();
		m_AnimationState->SetOwnerMesh( this );
	}
}

void Mesh::SetVertexDeclaration( IVertexDeclaration* const pVertexDeclaration )
{
	m_VertexDeclaration = pVertexDeclaration;
}

ITexture* Mesh::GetTexture( unsigned int Stage ) const
{
	DEVASSERT( Stage < MAX_TEXTURE_STAGES );
	return m_Material.GetTexture( Stage );
}

void Mesh::SetTexture( unsigned int Stage, ITexture* const Texture )
{
	DEVASSERT( Stage < MAX_TEXTURE_STAGES );
	m_Material.SetTexture( Stage, Texture );
}

bool Mesh::SupportsTexture( const uint Stage ) const
{
	DEVASSERT( Stage < MAX_TEXTURE_STAGES );
	return m_Material.SupportsTexture( Stage );
}

bool Mesh::SupportsAlphaBlend() const
{
	return m_Material.SupportsAlphaBlend();
}

IShaderProgram* Mesh::GetShaderProgram() const
{
	return m_Material.GetShaderProgram();
}

void Mesh::SetMaterialDefinition( const SimpleString& DefinitionName, IRenderer* const pRenderer )
{
	ASSERT( m_VertexDeclaration );
	m_Material.SetDefinition( DefinitionName, pRenderer, m_VertexDeclaration->GetSignature() );
}

void Mesh::AddMultiPassMaterialDefinition( const SimpleString& DefinitionName, IRenderer* const pRenderer )
{
	Material& MultiPassMaterial = m_MultiPassMaterials.PushBack();

	ASSERT( m_VertexDeclaration );
	MultiPassMaterial.SetDefinition( DefinitionName, pRenderer, m_VertexDeclaration->GetSignature() );
}

uint Mesh::GetMaterialFlags() const
{
	return m_Material.GetFlags();
}

void Mesh::SetMaterialFlags( unsigned int Flags, unsigned int Mask /*= MAT_ALL*/ )
{
	m_Material.SetFlags( Flags, Mask );
}

void Mesh::SetMaterialFlag( unsigned int Flag, bool Set )
{
	m_Material.SetFlag( Flag, Set );
}

Matrix Mesh::GetConcatenatedTransforms()
{
	if( m_CACHED_Location == m_Location &&
		m_CACHED_Rotation == m_Rotation &&
		m_CACHED_Scale == m_Scale )
	{
		return m_PreconcatenatedTransform;
	}

	static const Vector skNoScale = Vector( 1.0f, 1.0f, 1.0f );
	if( m_Scale == skNoScale )
	{
		m_PreconcatenatedTransform =
			m_Rotation.ToMatrix() *
			Matrix::CreateTranslation( m_Location );
	}
	else
	{
		m_PreconcatenatedTransform =
			Matrix::CreateScale( m_Scale ) *
			m_Rotation.ToMatrix() *
			Matrix::CreateTranslation( m_Location );
	}

	m_CACHED_Location	= m_Location;
	m_CACHED_Rotation	= m_Rotation;
	m_CACHED_Scale		= m_Scale;
	return m_PreconcatenatedTransform;
}

void Mesh::Tick( const float DeltaTime )
{
	m_Material.Tick( DeltaTime );

	if( IsAnimationOwner() )
	{
		m_AnimationState->Tick( DeltaTime );
	}
}

// ROSATODO: Also syncs AnimationState; make a separate function for just copying
// bone array, if there's any reason to copy animations but *not* sync the states
// of the two meshes.
void Mesh::CopyAnimationsFrom( Mesh* const pMesh )
{
	DEVASSERT( m_Bones );
	DEVASSERT( pMesh );
	DEVASSERT( pMesh->m_Bones );

#if BUILD_DEV
	const uint	VertexSignature	= m_VertexDeclaration->GetSignature();
	const uint	VertexBones		= VD_BONEINDICES | VD_BONEWEIGHTS;
	const uint	SignatureCheck	= VertexSignature & VertexBones;
	ASSERTDESC( SignatureCheck == VertexBones, "Mesh::CopyAnimationsFrom: target mesh is not rigged" );
#endif

	if( m_Bones != pMesh->m_Bones )
	{
		DEVASSERT( m_Bones->GetNumBones() == pMesh->m_Bones->GetNumBones() );

		SafeRelease( m_Bones );
		m_Bones = pMesh->m_Bones;
		m_Bones->AddReference();
	}

	if( m_AnimationState != pMesh->m_AnimationState )
	{
		SafeRelease( m_AnimationState );
		m_AnimationState = pMesh->m_AnimationState;
		m_AnimationState->AddReference();
	}
}

void Mesh::SuppressAnimEvents( const bool Suppress )
{
	DEBUGASSERT( IsAnimated() );
	m_AnimationState->SuppressAnimEvents( Suppress );
}

void Mesh::PlayAnimation( const HashedString& AnimationName, AnimationState::SPlayAnimationParams& PlayParams )
{
	DEVASSERT( IsAnimated() );

	if( IsAnimated() )
	{
		PlayParams.m_SuppressImmediateAnimEvents = false;
		m_AnimationState->PlayAnimation( m_Bones->GetAnimation( AnimationName ), PlayParams );
	}
}

void Mesh::SetAnimationBlend( const HashedString& AnimationNameA, const HashedString& AnimationNameB, const float BlendAlpha )
{
	DEVASSERT( IsAnimated() );

	if( IsAnimated() )
	{
		m_AnimationState->SetAnimationBlend( m_Bones->GetAnimation( AnimationNameA ), m_Bones->GetAnimation( AnimationNameB ), BlendAlpha );
	}
}

void Mesh::SetAnimation( int AnimationIndex, AnimationState::SPlayAnimationParams& PlayParams )
{
	DEVASSERT( IsAnimated() );

	if( IsAnimated() )
	{
		PlayParams.m_SuppressImmediateAnimEvents = true;
		m_AnimationState->PlayAnimation( m_Bones->GetAnimation( AnimationIndex ), PlayParams );
	}
}

Animation* Mesh::GetAnimation( const SimpleString& Name ) const
{
	DEVASSERT( IsAnimated() );

	if( IsAnimated() )
	{
		return m_Bones->GetAnimation( Name );
	}

	return NULL;
}

void Mesh::AddAnimationListener( const SAnimationListener& AnimationListener )
{
	DEVASSERT( IsAnimated() );

	if( IsAnimated() )
	{
		m_AnimationState->AddAnimationListener( AnimationListener );
	}
}

void Mesh::RemoveAnimationListener( const SAnimationListener& AnimationListener )
{
	DEVASSERT( IsAnimated() );

	if( IsAnimated() )
	{
		m_AnimationState->RemoveAnimationListener( AnimationListener );
	}
}

void Mesh::UpdateBones()
{
	DEVASSERT( IsAnimated() );

	m_AnimationState->UpdateBones( m_Bones );
}

int Mesh::GetBoneIndex( const HashedString& BoneName ) const
{
	if( IsAnimated() )
	{
		return m_Bones->GetBoneIndex( BoneName );
	}
	else
	{
		return INVALID_INDEX;
	}
}

void Mesh::AddBoneModifier( IBoneModifier* pBoneModifier )
{
	DEVASSERT( IsAnimated() );

	m_AnimationState->AddBoneModifier( pBoneModifier );
}

void Mesh::GetAnimationVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity )
{
	DEBUGASSERT( IsAnimated() );
	return m_AnimationState->GetAnimationVelocity( OutVelocity, OutRotationalVelocity );
}

int Mesh::GetAnimationIndex() const
{
	if( m_Bones )
	{
		DEBUGASSERT( m_AnimationState );
		return m_Bones->GetAnimationIndex( m_AnimationState->GetAnimation() );
	}
	else
	{
		return INVALID_INDEX;
	}
}

float Mesh::GetAnimationTime() const
{
	DEBUGASSERT( IsAnimated() );
	return m_AnimationState->GetAnimationTime();
}

void Mesh::SetAnimationTime( const float AnimationTime )
{
	DEBUGASSERT( IsAnimated() );
	m_AnimationState->SetAnimationTime( AnimationTime );
}

float Mesh::GetAnimationPlayRate() const
{
	DEBUGASSERT( IsAnimated() );
	return m_AnimationState->GetAnimationPlayRate();
}

void Mesh::SetAnimationPlayRate( const float AnimationPlayRate )
{
	DEBUGASSERT( IsAnimated() );
	m_AnimationState->SetAnimationPlayRate( AnimationPlayRate );
}

void Mesh::SetAnimationBonesTickRate( const float AnimationBonesTickRate )
{
	DEBUGASSERT( IsAnimated() );
	m_AnimationState->SetAnimationBonesTickRate( AnimationBonesTickRate );
}

AnimationState::EAnimationEndBehavior Mesh::GetAnimationEndBehavior() const
{
	DEBUGASSERT( IsAnimated() );
	return m_AnimationState->GetAnimationEndBehavior();
}

/*virtual*/ Vector Mesh::GetSortLocation()
{
	return m_Location;
}

uint Mesh::GetNumVertices() const
{
	ASSERT( m_VertexBuffer );
	return m_VertexBuffer->GetNumVertices();
}

void Mesh::SetNumVertices( const uint NumVertices )
{
	ASSERT( m_VertexBuffer );
	m_VertexBuffer->SetNumVertices( NumVertices );
}

uint Mesh::GetNumIndices() const
{
	ASSERT( m_IndexBuffer );
	return m_IndexBuffer->GetNumIndices();
}

void Mesh::SetNumIndices( const uint NumIndices )
{
	ASSERT( m_IndexBuffer );
	m_IndexBuffer->SetNumIndices( NumIndices );
}

void Mesh::SetAABB( const AABB& Bounds )
{
	m_AABB			= Bounds;
	m_OriginalAABB	= Bounds;
}

void Mesh::RecomputeAABB()
{
	m_AABB = m_OriginalAABB.GetTransformedBound( m_Location, m_Rotation, m_Scale );
}

const Vector4& Mesh::GetShaderConstant( const HashedString& Name ) const
{
	static const Vector4 skDefaultShaderConstant = Vector4();
	const Map<HashedString, Vector4>::Iterator ConstantIter = m_ShaderConstants.Search( Name );
	return ConstantIter.IsValid() ? ConstantIter.GetValue() : skDefaultShaderConstant;
}

void Mesh::SetShaderConstant( const HashedString& Name, const Vector4& Value )
{
	m_ShaderConstants.Insert( Name, Value );
}
