#ifndef WBPEROSAGETSPEED_H
#define WBPEROSAGETSPEED_H

#include "wbpe.h"

class WBPERosaGetSpeed : public WBPE
{
public:
	WBPERosaGetSpeed();
	virtual ~WBPERosaGetSpeed();

	DEFINE_WBPE_FACTORY( RosaGetSpeed );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETSPEED_H
