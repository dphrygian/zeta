#include "core.h"
#include "wbcompreactions.h"
#include "wbpatternmatching.h"
#include "wbaction.h"
#include "configmanager.h"
#include "wbactionfactory.h"
#include "wbrule.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"

WBCompReactions::WBCompReactions()
:	m_Rules()
,	m_Actions()
,	m_ObserveEvents()
{
}

WBCompReactions::~WBCompReactions()
{
	const uint NumReactions = m_Actions.Size();
	FOR_EACH_INDEX( ReactionsIndex, NumReactions )
	{
		TActions& Actions = m_Actions[ ReactionsIndex ];
		WBActionFactory::ClearActionArray( Actions );
	}

	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		FOR_EACH_ARRAY( ObserveEventIter, m_ObserveEvents, HashedString )
		{
			const HashedString ObserveEvent = ObserveEventIter.GetValue();
			pEventManager->RemoveObserver( ObserveEvent, this );
		}
	}
}

/*virtual*/ void WBCompReactions::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NumObserveEvents );
	const uint NumObserveEvents = ConfigManager::GetInheritedInt( sNumObserveEvents, 0, sDefinitionName );
	FOR_EACH_INDEX( ObserveEventIndex, NumObserveEvents )
	{
		const HashedString ObserveEvent = ConfigManager::GetInheritedSequenceHash( "ObserveEvent%d", ObserveEventIndex, HashedString::NullString, sDefinitionName );
		m_ObserveEvents.PushBack( ObserveEvent );
		GetEventManager()->AddObserver( ObserveEvent, this );
	}

	// Add local reactions first, so that we override any reactions in added sets (e.g., parent sets)
	AddReactions( DefinitionName );

	// Optionally add reactions from other sets (or other reaction components, ignoring their other properties)
	STATICHASH( NumReactionSets );
	const uint NumReactionSets = ConfigManager::GetInheritedInt( sNumReactionSets, 0, sDefinitionName );
	FOR_EACH_INDEX( ReactionSetIndex, NumReactionSets )
	{
		const SimpleString ReactionSet = ConfigManager::GetInheritedSequenceString( "ReactionSet%d", ReactionSetIndex, "", sDefinitionName );
		AddReactions( ReactionSet );
	}
}

void WBCompReactions::AddReactions( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumReactions );
	const uint NumReactions = ConfigManager::GetInheritedInt( sNumReactions, 0, sDefinitionName );

	FOR_EACH_INDEX( ReactionIndex, NumReactions )
	{
		const SimpleString ReactionDef = ConfigManager::GetInheritedSequenceString( "Reaction%d", ReactionIndex, "", sDefinitionName );
		MAKEHASH( ReactionDef );
		ASSERT( ReactionDef != "" );

		STATICHASH( Rule );
		const SimpleString Rule = ConfigManager::GetString( sRule, "", sReactionDef );

#if BUILD_DEV
		if( Rule == "" )
		{
			PRINTF( "Reaction %s has no rule.\n", ReactionDef.CStr() );
			WARN;
		}
#endif

		m_Rules.PushBack().InitializeFromDefinition( Rule );

		TActions& Actions = m_Actions.PushBack();

		WBActionFactory::InitializeActionArray( sReactionDef, Actions );
	}
}

/*virtual*/ void WBCompReactions::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Super::HandleEvent( Event );

	// Create a context for pattern matching rules to evaluate their PEs.
	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	uint		MatchingIndex;
	Array<uint>	MatchingAdditiveIndices;
	if( WBPatternMatching::AdditiveCompare( m_Rules, Event, PEContext, MatchingIndex, MatchingAdditiveIndices ) )
	{
		if( INVALID_ARRAY_INDEX != MatchingIndex )
		{
			ExecuteActions( m_Actions[ MatchingIndex ], Event );
		}

		const uint NumMatchingAdditiveIndices = MatchingAdditiveIndices.Size();
		FOR_EACH_INDEX( MatchingAdditiveIndexIndex, NumMatchingAdditiveIndices )
		{
			const uint MatchingAdditiveIndex = MatchingAdditiveIndices[ MatchingAdditiveIndexIndex ];
			ExecuteActions( m_Actions[ MatchingAdditiveIndex ], Event );
		}
	}
}

void WBCompReactions::ExecuteActions( const WBCompReactions::TActions& Actions, const WBEvent& ContextEvent ) const
{
	XTRACE_FUNCTION;

	WBActionFactory::ExecuteActionArray( Actions, ContextEvent, GetEntity() );
}
