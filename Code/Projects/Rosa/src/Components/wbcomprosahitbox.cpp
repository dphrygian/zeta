#include "core.h"
#include "wbcomprosahitbox.h"
#include "wbcomprosatransform.h"
#include "wbcomprosamesh.h"
#include "rosaframework.h"
#include "rosamesh.h"
#include "irenderer.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "mathcore.h"

WBCompRosaHitbox::WBCompRosaHitbox()
:	m_DirtyHitbox( false )
,	m_HitboxBounds()
,	m_HitboxBones()
{
}

/*virtual*/ void WBCompRosaHitbox::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaMesh* const		pMesh		= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMesh );

	// HACKHACK to reuse bones on a smaller mesh, for Taboo
	STATICHASH( HitboxScalar );
	const float HitboxScalar = ConfigManager::GetInheritedFloat( sHitboxScalar, 1.0f, sDefinitionName );

	STATICHASH( NumHitboxBones );
	const uint NumHitboxBones = ConfigManager::GetInheritedInt( sNumHitboxBones, 0, sDefinitionName );
	FOR_EACH_INDEX( HitboxBoneIndex, NumHitboxBones )
	{
		SHitboxBone&	HitboxBone		= m_HitboxBones.PushBack();
		HitboxBone.m_BoneName			= ConfigManager::GetInheritedSequenceHash(	"HitboxBone%d",				HitboxBoneIndex,	HashedString::NullString,	sDefinitionName );
		HitboxBone.m_BoneWidth			= ConfigManager::GetInheritedSequenceFloat(	"HitboxBone%dWidth",		HitboxBoneIndex,	0.0f,						sDefinitionName ) * pTransform->GetScale() * HitboxScalar;
		HitboxBone.m_BoneWidthSq		= Square( HitboxBone.m_BoneWidth );
		HitboxBone.m_BoneLengthScalar	= ConfigManager::GetInheritedSequenceFloat(	"HitboxBone%dLengthScalar",	HitboxBoneIndex,	1.0f,						sDefinitionName );
		HitboxBone.m_BoneIndex			= pMesh->GetBoneIndex( HitboxBone.m_BoneName );
	}
	m_DirtyHitbox = true;
}

/*virtual*/ void WBCompRosaHitbox::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnAnimatedMeshTick );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnAnimatedMeshTick )
	{
		m_DirtyHitbox = true;
	}
}

void WBCompRosaHitbox::UpdateHitbox()
{
	if( !m_DirtyHitbox )
	{
		return;
	}

	m_DirtyHitbox = false;

	WBEntity* const			pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaMesh* const	pMeshComponent	= WB_GETCOMP( pEntity, RosaMesh );
	DEVASSERT( pMeshComponent );

	RosaMesh* const			pMesh			= pMeshComponent->GetMesh();

	// Make sure bones are updated first
	pMesh->UpdateBones();

	// Seems like a reasonable assumption that the hitbox bounds will include the mesh location?
	m_HitboxBounds = pMesh->m_Location;

	FOR_EACH_ARRAY( HitboxBoneIter, m_HitboxBones, SHitboxBone )
	{
		SHitboxBone&	HitboxBone	= HitboxBoneIter.GetValue();
		HitboxBone.m_BoneSegment	= pMeshComponent->GetBoneSegment( HitboxBone.m_BoneIndex );

		const Vector BoneOffset				= HitboxBone.m_BoneSegment.m_Point2 - HitboxBone.m_BoneSegment.m_Point1;
		HitboxBone.m_BoneSegment.m_Point2	= HitboxBone.m_BoneSegment.m_Point1 + BoneOffset * HitboxBone.m_BoneLengthScalar;

		Vector			SegmentMin	= HitboxBone.m_BoneSegment.m_Point1;
		Vector			SegmentMax	= HitboxBone.m_BoneSegment.m_Point2;
		Vector::MinMax( SegmentMin, SegmentMax );

		const Vector	BoneSize	= Vector( HitboxBone.m_BoneWidth, HitboxBone.m_BoneWidth, HitboxBone.m_BoneWidth );
		SegmentMin					-= BoneSize;
		SegmentMax					+= BoneSize;

		m_HitboxBounds.ExpandTo( SegmentMin );
		m_HitboxBounds.ExpandTo( SegmentMax );
	}
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaHitbox::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const	pFramework	= GetFramework();
	DEVASSERT( pFramework );

	IRenderer* const		pRenderer	= pFramework->GetRenderer();
	DEVASSERT( pRenderer );

	// Draw hitbox spheres
	if( HasHitbox() )
	{
		pRenderer->DEBUGDrawBox( m_HitboxBounds.m_Min, m_HitboxBounds.m_Max, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
		FOR_EACH_ARRAY( HitboxBoneIter, m_HitboxBones, SHitboxBone )
		{
			const SHitboxBone&	HitboxBone	= HitboxBoneIter.GetValue();
			pRenderer->DEBUGDrawSphere( HitboxBone.m_BoneSegment.m_Point1, HitboxBone.m_BoneWidth, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
			pRenderer->DEBUGDrawSphere( HitboxBone.m_BoneSegment.m_Point2, HitboxBone.m_BoneWidth, ARGB_TO_COLOR( 255, 0, 255, 255 ) );

			// DLP 24 Mar 2020: Adding lines to make the bone widths more apparent
			const Vector X = Vector( HitboxBone.m_BoneWidth, 0.0f, 0.0f );
			const Vector Y = Vector( 0.0f, HitboxBone.m_BoneWidth, 0.0f );
			const Vector Z = Vector( 0.0f, 0.0f, HitboxBone.m_BoneWidth );
			pRenderer->DEBUGDrawLine( HitboxBone.m_BoneSegment.m_Point1 + X, HitboxBone.m_BoneSegment.m_Point2 + X, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
			pRenderer->DEBUGDrawLine( HitboxBone.m_BoneSegment.m_Point1 - X, HitboxBone.m_BoneSegment.m_Point2 - X, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
			pRenderer->DEBUGDrawLine( HitboxBone.m_BoneSegment.m_Point1 + Y, HitboxBone.m_BoneSegment.m_Point2 + Y, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
			pRenderer->DEBUGDrawLine( HitboxBone.m_BoneSegment.m_Point1 - Y, HitboxBone.m_BoneSegment.m_Point2 - Y, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
			pRenderer->DEBUGDrawLine( HitboxBone.m_BoneSegment.m_Point1 + Z, HitboxBone.m_BoneSegment.m_Point2 + Z, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
			pRenderer->DEBUGDrawLine( HitboxBone.m_BoneSegment.m_Point1 - Z, HitboxBone.m_BoneSegment.m_Point2 - Z, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
		}
	}
}
#endif
