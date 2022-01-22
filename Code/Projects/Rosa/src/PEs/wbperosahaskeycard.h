#ifndef WBPEROSAHASKEYCARD_H
#define WBPEROSAHASKEYCARD_H

#include "wbpe.h"

class WBPERosaHasKeycard : public WBPE
{
public:
	WBPERosaHasKeycard();
	virtual ~WBPERosaHasKeycard();

	DEFINE_WBPE_FACTORY( RosaHasKeycard );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	HashedString	m_Keycard;
};

#endif // WBPEROSAHASKEYCARD_H
