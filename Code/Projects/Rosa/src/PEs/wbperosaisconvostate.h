#ifndef WBPEROSAISCONVOSTATE_H
#define WBPEROSAISCONVOSTATE_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPERosaIsConvoState : public WBPE
{
public:
	WBPERosaIsConvoState();
	virtual ~WBPERosaIsConvoState();

	DEFINE_WBPE_FACTORY( RosaIsConvoState );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*			m_EntityPE;
	HashedString	m_State;
};

#endif // WBPEROSAISCONVOSTATE_H
