#ifndef WBACTIONROSAOPENUSERDATAPATH_H
#define WBACTIONROSAOPENUSERDATAPATH_H

#include "wbaction.h"

class WBActionRosaOpenUserDataPath : public WBAction
{
public:
	WBActionRosaOpenUserDataPath();
	virtual ~WBActionRosaOpenUserDataPath();

	DEFINE_WBACTION_FACTORY( RosaOpenUserDataPath );

	virtual void	Execute();
};

#endif // WBACTIONROSAOPENUSERDATAPATH_H
