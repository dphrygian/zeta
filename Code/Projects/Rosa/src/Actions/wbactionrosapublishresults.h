#ifndef WBACTIONROSAPUBLISHRESULTS_H
#define WBACTIONROSAPUBLISHRESULTS_H

#include "wbaction.h"

class WBActionRosaPublishResults : public WBAction
{
public:
	WBActionRosaPublishResults();
	virtual ~WBActionRosaPublishResults();

	DEFINE_WBACTION_FACTORY( RosaPublishResults );

	virtual void	Execute();
};

#endif // WBACTIONROSAPUBLISHRESULTS_H
