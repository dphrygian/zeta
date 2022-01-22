#ifndef WBPEROSAGETPERSISTENTVAR_H
#define WBPEROSAGETPERSISTENTVAR_H

#include "wbpe.h"

class WBPERosaGetPersistentVar : public WBPE
{
public:
	WBPERosaGetPersistentVar();
	virtual ~WBPERosaGetPersistentVar();

	DEFINE_WBPE_FACTORY( RosaGetPersistentVar );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	HashedString	m_Key;
};

#endif // WBPEROSAGETFACTION_H
