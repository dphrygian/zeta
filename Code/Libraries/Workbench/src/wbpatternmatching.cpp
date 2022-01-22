#include "core.h"
#include "wbpatternmatching.h"
#include "wbrule.h"
#include "wbevent.h"
#include "wbactionstack.h"

// TODO: Lots of opportunities for optimization here. See the Valve paper.

bool InternalCompare( const WBRule& Rule, const WBEvent& Event, const WBParamEvaluator::SPEContext& PEContext );

bool WBPatternMatching::AdditiveCompare( const Array<WBRule>& Rules, const WBEvent& Event, const WBParamEvaluator::SPEContext& PEContext, uint& OutIndex, Array<uint>& OutAdditiveIndices )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	uint BestMatch		= INVALID_ARRAY_INDEX;
	uint BestMatchScore	= 0;

	const uint NumRules = Rules.Size();
	for( uint RuleIndex = 0; RuleIndex < NumRules; ++RuleIndex )
	{
		const WBRule& Rule = Rules[ RuleIndex ];

		if( Rule.GetAdditive() )
		{
			if( Compare( Rule, Event, PEContext ) )
			{
				OutAdditiveIndices.PushBack( RuleIndex );
			}
		}
		else
		{
			// Don't bother testing anything that will be a lesser or equal match to our current best match.
			const uint Score = Rule.GetScore();
			if( Score > BestMatchScore )
			{

				if( Compare( Rule, Event, PEContext ) )
				{
					BestMatch		= RuleIndex;
					BestMatchScore	= Score;
				}
			}
		}
	}

	OutIndex = BestMatch;
	return ( BestMatch != INVALID_ARRAY_INDEX ) || ( OutAdditiveIndices.Size() > 0 );
}

bool WBPatternMatching::Compare( const WBRule& Rule, const WBEvent& Event, const WBParamEvaluator::SPEContext& PEContext )
{
	XTRACE_FUNCTION;

	WBActionStack::Push( Event, PEContext.m_Entity );
	const bool RetVal = InternalCompare( Rule, Event, PEContext );
	WBActionStack::Pop();

	return RetVal;
}

bool InternalCompare( const WBRule& Rule, const WBEvent& Event, const WBParamEvaluator::SPEContext& PEContext )
{
	XTRACE_FUNCTION;

	if( Rule.GetEvent() != Event.GetEventName() )
	{
		return false;
	}

	const Array<WBRule::SCondition>& Conditions = Rule.GetConditions();
	const uint NumConditions = Conditions.Size();
	for( uint ConditionIndex = 0; ConditionIndex < NumConditions; ++ConditionIndex )
	{
		const WBRule::SCondition& Condition = Conditions[ ConditionIndex ];

		// TODO: Optimization: For Single/MultiCompare, evaluate this once instead of for each event.
		Condition.m_ConditionPE.Evaluate( PEContext );

		if( !Condition.m_ConditionPE.GetBool() )
		{
			return false;
		}
	}

	return true;
}
