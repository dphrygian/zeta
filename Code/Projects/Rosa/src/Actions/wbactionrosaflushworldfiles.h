#ifndef WBACTIONROSAFLUSHWORLDFILES_H
#define WBACTIONROSAFLUSHWORLDFILES_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaFlushWorldFiles : public WBAction
{
public:
	WBActionRosaFlushWorldFiles();
	virtual ~WBActionRosaFlushWorldFiles();

	DEFINE_WBACTION_FACTORY( RosaFlushWorldFiles );

	virtual void	Execute();
};

#endif // WBACTIONROSAFLUSHWORLDFILES_H
