#include "core.h"
#include "wbcomprosathinkernpctarget.h"
#include "Components/wbcomprodinknowledge.h"
#include "Components/wbcomprodinblackboard.h"
#include "configmanager.h"

WBCompRosaThinkerNPCTarget::WBCompRosaThinkerNPCTarget()
:	m_OutputTargetBlackboardKey()
,	m_TargetScoreThreshold( 0.0f )
{
}

WBCompRosaThinkerNPCTarget::~WBCompRosaThinkerNPCTarget()
{
}

/*virtual*/ void WBCompRosaThinkerNPCTarget::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( OutputTargetBlackboardKey );
	m_OutputTargetBlackboardKey = ConfigManager::GetInheritedHash( sOutputTargetBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( TargetScoreThreshold );
	m_TargetScoreThreshold = ConfigManager::GetInheritedFloat( sTargetScoreThreshold, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaThinkerNPCTarget::Tick( const float DeltaTime )
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

	WBCompRodinKnowledge* const		pKnowledge	= WB_GETCOMP( pEntity, RodinKnowledge );
	ASSERT( pKnowledge );

	WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
	ASSERT( pBlackboard );

	WBEntity*	pSelectedTarget		= NULL;
	float		SelectedTargetScore	= 0.0f;

	const WBCompRodinKnowledge::TKnowledgeMap& KnowledgeMap = pKnowledge->GetKnowledgeMap();
	FOR_EACH_MAP( KnowledgeIter, KnowledgeMap, WBEntityRef, WBCompRodinKnowledge::TKnowledge )
	{
		WBEntity* const						pKnowledgeEntity	= KnowledgeIter.GetKey().Get();
		WBCompRodinKnowledge::TKnowledge&	Knowledge			= KnowledgeIter.GetValue();

		if( !pKnowledgeEntity )
		{
			continue;
		}

		// Filter out knowledge entities that aren't potential targets
		STATIC_HASHED_STRING( KnowledgeType );
		STATIC_HASHED_STRING( Target );
		const HashedString KnowledgeType = Knowledge.GetHash( sKnowledgeType );
		if( KnowledgeType != sTarget )
		{
			continue;
		}

		// Filter out knowledge entities that have never been visible.
		STATIC_HASHED_STRING( LastSeenTime );
		if( Knowledge.GetFloat( sLastSeenTime ) <= 0.0f )
		{
			continue;
		}

		STATIC_HASHED_STRING( VisionCertainty );
		const float VisionScore = Knowledge.GetFloat( sVisionCertainty );

		if( VisionScore >= m_TargetScoreThreshold && VisionScore > SelectedTargetScore )
		{
			pSelectedTarget		= pKnowledgeEntity;
			SelectedTargetScore	= VisionScore;
		}
	}

	if( m_OutputTargetBlackboardKey )
	{
		pBlackboard->SetEntity( m_OutputTargetBlackboardKey, pSelectedTarget );
	}
}
