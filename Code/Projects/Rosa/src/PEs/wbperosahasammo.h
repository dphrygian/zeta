#ifndef WBPEROSAHASAMMO_H
#define WBPEROSAHASAMMO_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPERosaHasAmmo : public WBPE
{
public:
	WBPERosaHasAmmo();
	virtual ~WBPERosaHasAmmo();

	DEFINE_WBPE_FACTORY( RosaHasAmmo );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	HashedString				m_Type;
	mutable WBParamEvaluator	m_TypePE;
	uint						m_Count;
	mutable WBParamEvaluator	m_CountPE;
};

#endif // WBPEROSAHASAMMO_H
