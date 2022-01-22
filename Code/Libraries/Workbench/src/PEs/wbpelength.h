#ifndef WBPELENGTH_H
#define WBPELENGTH_H

#include "wbpeunaryop.h"

class WBPELength : public WBPEUnaryOp
{
public:
	WBPELength();
	virtual ~WBPELength();

	DEFINE_WBPE_FACTORY( Length );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPELENGTH_H
