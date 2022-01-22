#ifndef WBACTIONROSAFOOTSTEP_H
#define WBACTIONROSAFOOTSTEP_H

#include "wbaction.h"

class WBActionRosaFootstep : public WBAction
{
public:
	WBActionRosaFootstep();
	virtual ~WBActionRosaFootstep();

	DEFINE_WBACTION_FACTORY( RosaFootstep );

	virtual void	Execute();
};

#endif // WBACTIONROSAFOOTSTEP_H
