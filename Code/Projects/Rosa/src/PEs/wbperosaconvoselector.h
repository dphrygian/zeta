#ifndef WBPEROSACONVOSELECTOR_H
#define WBPEROSACONVOSELECTOR_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPERosaConvoSelector : public WBPE
{
public:
	WBPERosaConvoSelector();
	virtual ~WBPERosaConvoSelector();

	DEFINE_WBPE_FACTORY( RosaConvoSelector );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	Map<HashedString, SimpleString>	m_Selections;
};

#endif // WBPEROSACONVOSELECTOR_H
