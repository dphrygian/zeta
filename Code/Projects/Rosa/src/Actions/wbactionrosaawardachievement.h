#ifndef WBACTIONROSAAWARDACHIEVEMENT_H
#define WBACTIONROSAAWARDACHIEVEMENT_H

#include "wbaction.h"
#include "simplestring.h"

class WBActionRosaAwardAchievement : public WBAction
{
public:
	WBActionRosaAwardAchievement();
	virtual ~WBActionRosaAwardAchievement();

	DEFINE_WBACTION_FACTORY( RosaAwardAchievement );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString	m_AchievementTag;
};

#endif // WBACTIONROSAAWARDACHIEVEMENT_H
