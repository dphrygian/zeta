#ifndef WBPERANDOMBOOL_H
#define WBPERANDOMBOOL_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPERandomBool : public WBPE
{
public:
	WBPERandomBool();
	virtual ~WBPERandomBool();

	DEFINE_WBPE_FACTORY( RandomBool );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	float						m_Probability;
	mutable WBParamEvaluator	m_ProbabilityPE;
};

#endif // WBPERANDOMBOOL_H
