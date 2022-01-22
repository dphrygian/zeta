#ifndef WBPATTERNMATCHING_H
#define WBPATTERNMATCHING_H

#include "array.h"
#include "wbparamevaluator.h"

class WBRule;
class WBEvent;

namespace WBPatternMatching
{
	// DLP 12 Feb 2021: This is similar to the old MultiCompare but determined by score *and* per-rule additives.
	// Returns true if any exclusive or additive matches are found.
	// OutIndex is the index into Rules of the best exclusive matching rule, else it is INVALID_ARRAY_INDEX.
	// OutAdditiveIndices is indices into Rules of the matching additive rules if any are found.
	bool	AdditiveCompare( const Array<WBRule>& Rules, const WBEvent& Event, const WBParamEvaluator::SPEContext& PEContext, uint& OutIndex, Array<uint>& OutAdditiveIndices );

	// Returns true if the rule matches.
	bool	Compare( const WBRule& Rule, const WBEvent& Event, const WBParamEvaluator::SPEContext& PEContext );
}

#endif // WBPATTERNMATCHING_H
