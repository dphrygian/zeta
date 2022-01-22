#ifndef WBPEROSAGETSPEEDLIMIT_H
#define WBPEROSAGETSPEEDLIMIT_H

#include "wbpe.h"

class WBPERosaGetSpeedLimit : public WBPE
{
public:
	WBPERosaGetSpeedLimit();
	virtual ~WBPERosaGetSpeedLimit();

	DEFINE_WBPE_FACTORY( RosaGetSpeedLimit );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETSPEEDLIMIT_H
