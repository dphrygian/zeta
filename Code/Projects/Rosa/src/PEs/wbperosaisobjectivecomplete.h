#ifndef WBPEROSAISOBJECTIVECOMPLETE_H
#define WBPEROSAISOBJECTIVECOMPLETE_H

#include "wbpe.h"

class WBPERosaIsObjectiveComplete : public WBPE
{
public:
	WBPERosaIsObjectiveComplete();
	virtual ~WBPERosaIsObjectiveComplete();

	DEFINE_WBPE_FACTORY( RosaIsObjectiveComplete );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	HashedString	m_ObjectiveTag;
	bool			m_RejectFail;	// Failed objective counts as incomplete (default behavior is to count it as completed)
};

#endif // WBPEROSAISOBJECTIVECOMPLETE_H
