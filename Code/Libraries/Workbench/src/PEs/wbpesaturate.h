#ifndef WBPESATURATE_H
#define WBPESATURATE_H

#include "wbpeunaryop.h"

class WBPESaturate : public WBPEUnaryOp
{
public:
	WBPESaturate();
	virtual ~WBPESaturate();

	DEFINE_WBPE_FACTORY( Saturate );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPESATURATE_H
