#ifndef WBACTIONROSARETURNTOTITLE_H
#define WBACTIONROSARETURNTOTITLE_H

#include "wbaction.h"
#include "simplestring.h"

class WBActionRosaReturnToTitle : public WBAction
{
public:
	WBActionRosaReturnToTitle();
	virtual ~WBActionRosaReturnToTitle();

	DEFINE_WBACTION_FACTORY( RosaReturnToTitle );

	virtual void	Execute();
};

#endif // WBACTIONROSARETURNTOTITLE_H
