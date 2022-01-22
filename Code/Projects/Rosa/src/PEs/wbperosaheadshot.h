#ifndef WBPEROSAHEADSHOT_H
#define WBPEROSAHEADSHOT_H

// Returns a scalar value from an entity's RosaHeadshot component if appropriate.

#include "wbpe.h"

class WBPERosaHeadshot : public WBPE
{
public:
	WBPERosaHeadshot();
	virtual ~WBPERosaHeadshot();

	DEFINE_WBPE_FACTORY( RosaHeadshot );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*	m_EntityPE;
	WBPE*	m_BonePE;
};

#endif // WBPEROSAHEADSHOT_H
