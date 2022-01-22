#ifndef WBPEROSAGETORIENTATION_H
#define WBPEROSAGETORIENTATION_H

#include "wbpe.h"

class WBPERosaGetOrientation : public WBPE
{
public:
	WBPERosaGetOrientation();
	virtual ~WBPERosaGetOrientation();

	DEFINE_WBPE_FACTORY( RosaGetOrientation );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_EntityPE;
};

#endif // WBPEROSAGETORIENTATION_H
