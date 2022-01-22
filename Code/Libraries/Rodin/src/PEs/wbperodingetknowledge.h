#ifndef WBPERODINGETKNOWLEDGE_H
#define WBPERODINGETKNOWLEDGE_H

#include "wbpe.h"

class WBPERodinGetKnowledge : public WBPE
{
public:
	WBPERodinGetKnowledge();
	virtual ~WBPERodinGetKnowledge();

	DEFINE_WBPE_FACTORY( RodinGetKnowledge );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*			m_EntityPE;
	HashedString	m_Key;
};

#endif // WBPERODINGETKNOWLEDGE_H
