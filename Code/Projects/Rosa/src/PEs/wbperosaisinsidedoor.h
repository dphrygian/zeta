#ifndef WBPEROSAISINSIDEDOOR_H
#define WBPEROSAISINSIDEDOOR_H

#include "wbpe.h"

class WBPERosaIsInsideDoor : public WBPE
{
public:
	WBPERosaIsInsideDoor();
	virtual ~WBPERosaIsInsideDoor();

	DEFINE_WBPE_FACTORY( RosaIsInsideDoor );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*	m_EntityPE;
	WBPE*	m_DoorPE;
};

#endif // WBPEROSAISINSIDEDOOR_H
