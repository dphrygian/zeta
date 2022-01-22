#ifndef WBPEROSAGETBESTCYCLESLOT_H
#define WBPEROSAGETBESTCYCLESLOT_H

// Returns the first available cycle slot in the player's inventory,
// or the current cycle slot if none are available. (When used to
// put a new weapon into a slot, this will roughly mimic the behavior
// of Eldritch.)

#include "wbpe.h"

class WBPERosaGetBestCycleSlot : public WBPE
{
public:
	WBPERosaGetBestCycleSlot();
	virtual ~WBPERosaGetBestCycleSlot();

	DEFINE_WBPE_FACTORY( RosaGetBestCycleSlot );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPEROSAGETBESTCYCLESLOT_H
