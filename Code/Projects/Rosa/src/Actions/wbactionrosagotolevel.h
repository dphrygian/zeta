#ifndef WBACTIONROSAGOTOLEVEL_H
#define WBACTIONROSAGOTOLEVEL_H

#include "wbaction.h"
#include "simplestring.h"
#include "rosagame.h"

class WBActionRosaGoToLevel : public WBAction
{
public:
	WBActionRosaGoToLevel();
	virtual ~WBActionRosaGoToLevel();

	DEFINE_WBACTION_FACTORY( RosaGoToLevel );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	bool			m_GoToNextLevel;
	bool			m_GoToPrevLevel;
	bool			m_GoToHubLevel;

	bool			m_NewGame;

#if ROSA_USE_ACTIVESAVESLOT
	SimpleString	m_NewGameSaveSlot;
	WBAction*		m_NewGameConfirmOverwriteAction;
	bool			m_NewGameUseActiveSlot;
#endif

	bool			m_Immediate;		// Go to level immediately. Only safe to call outside world tick (e.g., from UI tick).

	SimpleString	m_Level;
	HashedString	m_TeleportLabel;	// Optional teleport target after level transition
};

#endif // WBACTIONROSAGOTOLEVEL_H
