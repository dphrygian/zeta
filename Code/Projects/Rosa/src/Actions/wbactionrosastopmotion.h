#ifndef WBACTIONROSASTOPMOTION_H
#define WBACTIONROSASTOPMOTION_H

#include "wbaction.h"

class WBActionRosaStopMotion : public WBAction
{
public:
	WBActionRosaStopMotion();
	virtual ~WBActionRosaStopMotion();

	DEFINE_WBACTION_FACTORY( RosaStopMotion );

	virtual void	Execute();
};

#endif // WBACTIONROSASTOPMOTION_H
