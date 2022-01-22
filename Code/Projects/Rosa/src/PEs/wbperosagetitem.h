#ifndef WBPEROSAGETITEM_H
#define WBPEROSAGETITEM_H

#include "wbpe.h"

class WBPERosaGetItem : public WBPE
{
public:
	WBPERosaGetItem();
	virtual ~WBPERosaGetItem();

	DEFINE_WBPE_FACTORY( RosaGetItem );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*	m_EntityPE;
	WBPE*	m_SlotPE;
};

#endif // WBPEROSAGETITEM_H
