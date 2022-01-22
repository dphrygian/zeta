#ifndef WBRULE_H
#define WBRULE_H

#include "array.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBRule
{
public:
	WBRule();
	~WBRule();

	// Conditions are for arbitrary logic: any PE that returns a bool can be evaluated.
	// I think this is just wrapping WBParamEvaluator to make it mutable?
	// Probably WBParamEvaluator's m_EvaluatedParam should be mutable instead, oh well.
	struct SCondition
	{
		mutable WBParamEvaluator	m_ConditionPE;
	};

	void	InitializeFromDefinition( const SimpleString& DefinitionName );

	const HashedString&			GetEvent() const		{ return m_Event; }
	const Array<SCondition>&	GetConditions() const	{ return m_Conditions; }
	const uint					GetScore() const		{ return 1 + m_Conditions.Size(); }
	const bool					GetAdditive() const		{ return m_Additive; }

private:
	HashedString		m_Event;
	// DLP 12 Feb 2021: Per-rule control replacing the old m_DoMultiCompare in WBCompReactions.
	// Additive rules always match regardless of their score relative to other rules. This supports
	// the case of a child class adding behavior to a generic rule (e.g., "OnHit") without having
	// to override all the parent's more specific rules ("OnHit" + "WhileInMidair").
	// This also allows attaching to equivalent scored rules, like in the real example of a quest
	// chest marking the objective complete while also granting loot like a normal chest. That case
	// could also be done with composite action arrays, but this lets me do both.
	// And unlike DoMultiCompare, it does not affect the reactions of an entire entity.
	bool				m_Additive;
	Array<SCondition>	m_Conditions;
};

#endif // WBRULE_H
