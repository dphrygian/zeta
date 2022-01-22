#ifndef WBPEROSAGETLOCATION_H
#define WBPEROSAGETLOCATION_H

#include "wbpe.h"

class WBPERosaGetLocation : public WBPE
{
public:
	WBPERosaGetLocation();
	virtual ~WBPERosaGetLocation();

	DEFINE_WBPE_FACTORY( RosaGetLocation );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETLOCATION_H
