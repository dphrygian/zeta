#ifndef WBPEROSAHASMRUSAVE_H
#define WBPEROSAHASMRUSAVE_H

#include "wbpe.h"

class WBPERosaHasMRUSave : public WBPE
{
public:
	WBPERosaHasMRUSave();
	virtual ~WBPERosaHasMRUSave();

	DEFINE_WBPE_FACTORY( RosaHasMRUSave );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPEROSAHASMRUSAVE_H
