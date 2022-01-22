#ifndef WBPEROSADEADRECKONING_H
#define WBPEROSADEADRECKONING_H

#include "wbpe.h"

class WBPERosaDeadReckoning : public WBPE
{
public:
	WBPERosaDeadReckoning();
	virtual ~WBPERosaDeadReckoning();

	DEFINE_WBPE_FACTORY( RosaDeadReckoning );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_LocationPE;
	WBPE*	m_SpeedPE;
	WBPE*	m_TargetLocationPE;
	WBPE*	m_TargetVelocityPE;
};

#endif // WBPEROSADEADRECKONING_H
