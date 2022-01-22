#ifndef WBPECROSS_H
#define WBPECROSS_H

#include "wbpebinaryop.h"

class WBPECross : public WBPEBinaryOp
{
public:
	WBPECross();
	virtual ~WBPECross();

	DEFINE_WBPE_FACTORY( Cross );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPECROSS_H
