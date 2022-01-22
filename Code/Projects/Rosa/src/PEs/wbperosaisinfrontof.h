#ifndef WBPEROSAISINFRONTOF_H
#define WBPEROSAISINFRONTOF_H

#include "PEs/wbpebinaryop.h"

class WBPERosaIsInFrontOf : public WBPEBinaryOp
{
public:
	WBPERosaIsInFrontOf();
	virtual ~WBPERosaIsInFrontOf();

	DEFINE_WBPE_FACTORY( RosaIsInFrontOf );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPEROSAISINFRONTOF_H
