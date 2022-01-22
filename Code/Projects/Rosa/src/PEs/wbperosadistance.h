#ifndef WBPEROSADISTANCE_H
#define WBPEROSADISTANCE_H

#include "PEs/wbpebinaryop.h"

class WBPERosaDistance : public WBPEBinaryOp
{
public:
	WBPERosaDistance();
	virtual ~WBPERosaDistance();

	DEFINE_WBPE_FACTORY( RosaDistance );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPEROSADISTANCE_H
