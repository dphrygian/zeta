#ifndef WBACTIONROSACLEAROBJECTIVES_H
#define WBACTIONROSACLEAROBJECTIVES_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaClearObjectives : public WBAction
{
public:
	WBActionRosaClearObjectives();
	virtual ~WBActionRosaClearObjectives();

	DEFINE_WBACTION_FACTORY( RosaClearObjectives );

	virtual void	Execute();
};

#endif // WBACTIONROSACLEAROBJECTIVES_H
