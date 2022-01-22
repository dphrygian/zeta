#ifndef WBPEROSAISDIFFICULTY_H
#define WBPEROSAISDIFFICULTY_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPERosaIsDifficulty : public WBPE
{
public:
	WBPERosaIsDifficulty();
	virtual ~WBPERosaIsDifficulty();

	DEFINE_WBPE_FACTORY( RosaIsDifficulty );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	uint	m_RangeLo;
	uint	m_RangeHi;
};

#endif // WBPEROSAISDIFFICULTY_H
