#ifndef WBPEROSAGETSLOT_H
#define WBPEROSAGETSLOT_H

#include "wbpe.h"

class WBPERosaGetSlot : public WBPE
{
public:
	WBPERosaGetSlot();
	virtual ~WBPERosaGetSlot();

	DEFINE_WBPE_FACTORY( RosaGetSlot );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETSLOT_H
