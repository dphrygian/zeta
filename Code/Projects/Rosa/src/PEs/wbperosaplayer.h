#ifndef WBPEROSAPLAYER_H
#define WBPEROSAPLAYER_H

#include "wbpe.h"

class WBPERosaPlayer : public WBPE
{
public:
	WBPERosaPlayer();
	virtual ~WBPERosaPlayer();

	DEFINE_WBPE_FACTORY( RosaPlayer );

	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;
};

#endif // WBPEROSAGETITEM_H
