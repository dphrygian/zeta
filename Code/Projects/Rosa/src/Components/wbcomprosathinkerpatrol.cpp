#include "core.h"
#include "wbcomprosathinkerpatrol.h"
#include "Components/wbcomprodinknowledge.h"
#include "Components/wbcomprodinblackboard.h"
#include "Components/wbcomprosatransform.h"
#include "configmanager.h"

WBCompRosaThinkerPatrol::WBCompRosaThinkerPatrol()
:	m_OutputBlackboardKey()
{
}

WBCompRosaThinkerPatrol::~WBCompRosaThinkerPatrol()
{
}

/*virtual*/ void WBCompRosaThinkerPatrol::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( OutputBlackboardKey );
	m_OutputBlackboardKey = ConfigManager::GetInheritedHash( sOutputBlackboardKey, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBCompRosaThinkerPatrol::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	if( m_Paused )
	{
		return;
	}

	WBEntity* const					pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const		pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRodinKnowledge* const		pKnowledge	= WB_GETCOMP( pEntity, RodinKnowledge );
	ASSERT( pKnowledge );

	WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
	ASSERT( pBlackboard );

	const Vector	CurrentLocation	= pTransform->GetLocation();

	// Select the furthest patrol point that we know about.
	WBEntity*		pFurthestPatrol	= NULL;
	float			FurthestDistSq	= -1.0f;

	const WBCompRodinKnowledge::TKnowledgeMap& KnowledgeMap = pKnowledge->GetKnowledgeMap();
	FOR_EACH_MAP( KnowledgeIter, KnowledgeMap, WBEntityRef, WBCompRodinKnowledge::TKnowledge )
	{
		WBEntity*								pKnowledgeEntity	= KnowledgeIter.GetKey().Get();
		const WBCompRodinKnowledge::TKnowledge&	Knowledge			= KnowledgeIter.GetValue();

		if( !pKnowledgeEntity )
		{
			continue;
		}

		// Filter out knowledge entities that aren't patrol markup.
		STATIC_HASHED_STRING( KnowledgeType );
		STATIC_HASHED_STRING( Patrol );
		if( Knowledge.GetHash( sKnowledgeType ) != sPatrol )
		{
			continue;
		}

		WBCompRosaTransform* const				pKnowledgeTransform	= pKnowledgeEntity->GetTransformComponent<WBCompRosaTransform>();
		ASSERT( pKnowledgeTransform );

		const float DistSq = ( pKnowledgeTransform->GetLocation() - CurrentLocation ).LengthSquared();
		if( DistSq > FurthestDistSq )
		{
			FurthestDistSq = DistSq;
			pFurthestPatrol = pKnowledgeEntity;
		}
	}

	pBlackboard->SetEntity( m_OutputBlackboardKey, pFurthestPatrol );
}
