#ifndef WBPEISSTATE_H
#define WBPEISSTATE_H

#include "wbpe.h"

class WBPEIsState : public WBPE
{
public:
	WBPEIsState();
	virtual ~WBPEIsState();

	DEFINE_WBPE_FACTORY( IsState );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*			m_EntityPE;
	HashedString	m_State;
};

#endif // WBPEISSTATE_H
