#ifndef WBPEROSAHASMONEY_H
#define WBPEROSAHASMONEY_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPERosaHasMoney : public WBPE
{
public:
	WBPERosaHasMoney();
	virtual ~WBPERosaHasMoney();

	DEFINE_WBPE_FACTORY( RosaHasMoney );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	uint						m_Amount;
	mutable WBParamEvaluator	m_AmountPE;
};

#endif // WBPEROSAHASMONEY_H
