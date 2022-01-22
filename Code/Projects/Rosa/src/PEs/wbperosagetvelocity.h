#ifndef WBPEROSAGETVELOCITY_H
#define WBPEROSAGETVELOCITY_H

#include "wbpe.h"

class WBPERosaGetVelocity : public WBPE
{
public:
	WBPERosaGetVelocity();
	virtual ~WBPERosaGetVelocity();

	DEFINE_WBPE_FACTORY( RosaGetVelocity );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETVELOCITY_H
