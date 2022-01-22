#ifndef WBACTIONROSALAUNCHWEBSITE_H
#define WBACTIONROSALAUNCHWEBSITE_H

#include "wbaction.h"
#include "simplestring.h"

class WBActionRosaLaunchWebSite : public WBAction
{
public:
	WBActionRosaLaunchWebSite();
	virtual ~WBActionRosaLaunchWebSite();

	DEFINE_WBACTION_FACTORY( RosaLaunchWebSite );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString	m_URL;
};

#endif // WBACTIONROSALAUNCHWEBSITE_H
