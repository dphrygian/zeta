#ifndef WBPEROSAGETCHARACTERVO_H
#define WBPEROSAGETCHARACTERVO_H

#include "wbpe.h"

class WBPERosaGetCharacterVO : public WBPE
{
public:
	WBPERosaGetCharacterVO();
	virtual ~WBPERosaGetCharacterVO();

	DEFINE_WBPE_FACTORY( RosaGetCharacterVO );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	WBPE*			m_EntityPE;
	SimpleString	m_VO;
};

#endif // WBPEROSAGETCHARACTERVO_H
