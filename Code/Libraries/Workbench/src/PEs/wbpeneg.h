#ifndef WBPENEG_H
#define WBPENEG_H

#include "wbpeunaryop.h"

class WBPENeg : public WBPEUnaryOp
{
public:
	WBPENeg();
	virtual ~WBPENeg();

	DEFINE_WBPE_FACTORY( Neg );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPENEG_H
