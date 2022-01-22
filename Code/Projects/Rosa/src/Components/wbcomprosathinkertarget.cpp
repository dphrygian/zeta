#include "core.h"
#include "wbcomprosathinkertarget.h"
#include "Components/wbcomprodinknowledge.h"
#include "Components/wbcomprodinblackboard.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcomppemap.h"
#include "wbcomprosafaction.h"
#include "wbcomprosahealth.h"
#include "configmanager.h"
#include "rosadifficulty.h"
#include "rosagame.h"
#include "wbeventmanager.h"

WBCompRosaThinkerTarget::WBCompRosaThinkerTarget()
:	m_OutputCombatTargetBlackboardKey()
,	m_OutputSearchTargetBlackboardKey()
,	m_OutputNoticeTargetBlackboardKey()
,	m_CombatTargetScoreThreshold( 0.0f )
,	m_SearchTargetScoreThreshold( 0.0f )
,	m_NoticeTargetScoreThreshold( 0.0f )
,	m_BodyConsiderTimeout()
,	m_AlwaysTargetHostages( false )
{
}

WBCompRosaThinkerTarget::~WBCompRosaThinkerTarget()
{
}

/*virtual*/ void WBCompRosaThinkerTarget::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( OutputCombatTargetBlackboardKey );
	m_OutputCombatTargetBlackboardKey = ConfigManager::GetInheritedHash( sOutputCombatTargetBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( OutputSearchTargetBlackboardKey );
	m_OutputSearchTargetBlackboardKey = ConfigManager::GetInheritedHash( sOutputSearchTargetBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( OutputNoticeTargetBlackboardKey );
	m_OutputNoticeTargetBlackboardKey = ConfigManager::GetInheritedHash( sOutputNoticeTargetBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( CombatTargetScoreThreshold );
	m_CombatTargetScoreThreshold = ConfigManager::GetInheritedFloat( sCombatTargetScoreThreshold, 0.0f, sDefinitionName );

	STATICHASH( SearchTargetScoreThreshold );
	m_SearchTargetScoreThreshold = ConfigManager::GetInheritedFloat( sSearchTargetScoreThreshold, 0.0f, sDefinitionName );

	STATICHASH( NoticeTargetScoreThreshold );
	m_NoticeTargetScoreThreshold = ConfigManager::GetInheritedFloat( sNoticeTargetScoreThreshold, 0.0f, sDefinitionName );

	STATICHASH( BodyConsiderTimeout );
	m_BodyConsiderTimeout = ConfigManager::GetInheritedFloat( sBodyConsiderTimeout, 0.0f, sDefinitionName );

	STATICHASH( AlwaysTargetHostages );
	m_AlwaysTargetHostages = ConfigManager::GetInheritedBool( sAlwaysTargetHostages, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaThinkerTarget::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	if( m_Paused )
	{
		return;
	}

	// ROSAHACK: In tourist mode, never emit a target
	if( RosaDifficulty::GetGameDifficulty() == 0 )
	{
		return;
	}

	// ROSAHACK: Stat mod the score thresholds *by the player* so we can control them based on game difficulty
	WBEntity* const					pPlayer		= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompStatMod* const			pStatMod	= WB_GETCOMP( pPlayer, StatMod );
	DEVASSERT( pStatMod );

	WB_MODIFY_FLOAT( TargetScoreThresholdScalar, 1.0f, pStatMod );
	const float CombatTargetScoreThreshold = m_CombatTargetScoreThreshold * WB_MODDED( TargetScoreThresholdScalar );
	const float SearchTargetScoreThreshold = m_SearchTargetScoreThreshold * WB_MODDED( TargetScoreThresholdScalar );
	const float NoticeTargetScoreThreshold = m_NoticeTargetScoreThreshold * WB_MODDED( TargetScoreThresholdScalar );

	WBEntity* const					pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRodinKnowledge* const		pKnowledge	= WB_GETCOMP( pEntity, RodinKnowledge );
	DEVASSERT( pKnowledge );

	WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
	DEVASSERT( pBlackboard );

	WBCompRosaHealth* const			pHealth		= WB_GETCOMP( pEntity, RosaHealth );
	const bool						NeedsHeal	= pHealth && !pHealth->HasMaxHealth();

	WBEntity*	pSelectedCombatTarget			= NULL;
	float		SelectedCombatTargetScore		= 0.0f;
	int			SelectedCombatTargetPriority	= 0x7ffffff;

	WBEntity*	pSelectedSearchTarget			= NULL;
	float		SelectedSearchTargetScore		= 0.0f;

	WBEntity*	pSelectedNoticeTarget			= NULL;
	float		SelectedNoticeTargetScore		= 0.0f;

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

		// Filter out dead entities (ROSANOTE: unless they're bodies of non-hostile units)
		WBCompRosaHealth* const	pTargetHealth	= WB_GETCOMP( pKnowledgeEntity, RosaHealth );
		const bool				IsDead			= ( pTargetHealth && pTargetHealth->IsDead() );	// NOTE: No health component means entity can never be regarded as dead

		bool					IsHostage		= false;
		WBCompPEMap* const		pPEMap			= WB_GETCOMP( pKnowledgeEntity, PEMap );
		if( pPEMap )
		{
			STATICHASH( IsHostage );
			const WBPE*			pIsHostagePE	= pPEMap->GetPE( sIsHostage );
			if( pIsHostagePE )
			{
				WBParamEvaluator::SPEContext PEContext;
				PEContext.m_Entity = pKnowledgeEntity;

				WBParamEvaluator::SEvaluatedParam Result;
				pIsHostagePE->Evaluate( PEContext, Result );

				IsHostage = Result.GetBool();
			}
		}

		// ROSANOTE: If entity is dead and not hostile, we continue to regard it, but only as a potential search/notice target
		bool IsBodySearchTarget = false;

		// Filter out knowledge entities that are friendly to us (ROSANOTE: unless they're dead)
		const RosaFactions::EFactionCon Con = WBCompRosaFaction::GetCon( pEntity, pKnowledgeEntity );
		if( Con == RosaFactions::EFR_Friendly )
		{
			if( IsDead )
			{
				IsBodySearchTarget = true;
			}
			else
			{
				continue;
			}
		}

		// Filter out knowledge entities that are neutral and not yet regarded as hostile (ROSANOTE: unless they're dead)
		if( Con == RosaFactions::EFR_Neutral )
		{
			STATIC_HASHED_STRING( RegardAsHostile );
			const bool RegardAsHostile = Knowledge.GetBool( sRegardAsHostile );
			if( RegardAsHostile )
			{
				// Do nothing, treat as any other (hostile) target
			}
			else
			{
				if( IsDead )
				{
					IsBodySearchTarget = true;
				}
				else
				{
					continue;
				}
			}
		}

		// Filter out dead entities that aren't body search targets
		if( IsDead && !IsBodySearchTarget )
		{
			continue;
		}

		// HACKHACK: Filter out hostages if we don't need to heal and we don't always target them
		if( IsHostage && !NeedsHeal && !m_AlwaysTargetHostages )
		{
			continue;
		}

		float Score			= 0.0f;
		float HearingScore	= 1.0f;
		float VisionScore	= 1.0f;

		STATIC_HASHED_STRING( HearingCertainty );
		const float HearingCertainty = Knowledge.GetFloat( sHearingCertainty );
		HearingScore *= HearingCertainty;

		Score += HearingScore;

		// Make sure we search for things that have damaged us.
		STATIC_HASHED_STRING( IsDamager );
		if( Knowledge.GetBool( sIsDamager ) )
		{
			// TODO: Configurate
			Score += 1.0f;
		}

		// Prioritize hostages if we need health
		if( IsHostage && NeedsHeal )
		{
			// TODO: Configurate
			Score += 100.0f;
		}

		if( Score >= SearchTargetScoreThreshold && Score > SelectedSearchTargetScore )
		{
			pSelectedSearchTarget		= pKnowledgeEntity;
			SelectedSearchTargetScore	= Score;
		}

		if( Score >= NoticeTargetScoreThreshold && Score > SelectedNoticeTargetScore )
		{
			pSelectedNoticeTarget		= pKnowledgeEntity;
			SelectedNoticeTargetScore	= Score;
		}

		// Filter out knowledge entities that have never been visible.
		STATIC_HASHED_STRING( LastSeenTime );
		if( Knowledge.GetFloat( sLastSeenTime ) <= 0.0f )
		{
			continue;
		}

		// HACKHACK: Mark bodies the first time they are seen *as bodies*, and ignore them subsequently.
		// Violating the implicit rule that knowledge flows from sensors to thinkers.
		if( IsBodySearchTarget )
		{
			STATIC_HASHED_STRING( FirstSeenTime );
			if( Knowledge.HasParameter( sFirstSeenTime ) )
			{
				// Ignore bodies that we've known about for longer than our threshold
				const float TimeSinceFirstSeen = GetTime() - Knowledge.GetFloat( sFirstSeenTime );
				if( TimeSinceFirstSeen > m_BodyConsiderTimeout ||
					TimeSinceFirstSeen < 0.0f )	// HACKHACK: Immediately ignore bodies when a loaded game's timestamps are mismatched
				{
					continue;
				}
			}
			else
			{
				Knowledge.SetFloat( sFirstSeenTime, GetTime() );

				// Set this target to never expire so we never forget the first time we saw it.
				STATIC_HASHED_STRING( NeverExpire );
				Knowledge.SetBool( sNeverExpire, true );

				// HACKHACK: Notify body so it can be counted for results screen.
				WB_MAKE_EVENT( OnBodySeen, NULL );
				WB_DISPATCH_EVENT( GetEventManager(), OnBodySeen, pKnowledgeEntity );
			}
		}

		STATIC_HASHED_STRING( VisionCertainty );
		const float	VisionCertainty	= Knowledge.GetFloat( sVisionCertainty );
		VisionScore *= VisionCertainty;

		STATIC_HASHED_STRING( VisionPriority );
		const int	VisionPriority	= Knowledge.GetInt( sVisionPriority );

		Score += VisionScore;

		// Only consider vision score for selecting combat target.
		// Some HACKHACKs for making hostages easier targets; any non-zero visibility is valid for hostages
		if( !IsDead &&
			( VisionScore >= CombatTargetScoreThreshold ||
			  ( IsHostage && VisionScore > 0.0f ) ) &&
			( VisionScore > SelectedCombatTargetScore ||
			  VisionPriority > SelectedCombatTargetPriority ) )
		{
			pSelectedCombatTarget			= pKnowledgeEntity;
			SelectedCombatTargetScore		= VisionScore;
			SelectedCombatTargetPriority	= VisionPriority;
		}

		if( Score >= SearchTargetScoreThreshold && Score > SelectedSearchTargetScore )
		{
			pSelectedSearchTarget		= pKnowledgeEntity;
			SelectedSearchTargetScore	= Score;
		}

		if( Score >= NoticeTargetScoreThreshold && Score > SelectedNoticeTargetScore )
		{
			pSelectedNoticeTarget		= pKnowledgeEntity;
			SelectedNoticeTargetScore	= Score;
		}
	}

	if( m_OutputCombatTargetBlackboardKey )
	{
		pBlackboard->SetEntity( m_OutputCombatTargetBlackboardKey, pSelectedCombatTarget );
	}

	if( m_OutputSearchTargetBlackboardKey )
	{
		pBlackboard->SetEntity( m_OutputSearchTargetBlackboardKey, pSelectedSearchTarget );
	}

	if( m_OutputNoticeTargetBlackboardKey )
	{
		pBlackboard->SetEntity( m_OutputNoticeTargetBlackboardKey, pSelectedNoticeTarget );
	}
}
