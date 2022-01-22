#ifndef WBPEROSAGETFACTION_H
#define WBPEROSAGETFACTION_H

#include "wbpe.h"

class WBPERosaGetFaction : public WBPE
{
public:
	WBPERosaGetFaction();
	virtual ~WBPERosaGetFaction();

	DEFINE_WBPE_FACTORY( RosaGetFaction );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETFACTION_H
