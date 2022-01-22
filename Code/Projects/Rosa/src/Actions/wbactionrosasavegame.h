#ifndef WBACTIONROSASAVEGAME_H
#define WBACTIONROSASAVEGAME_H

#include "wbaction.h"
#include "simplestring.h"

class WBActionRosaSaveGame : public WBAction
{
public:
	WBActionRosaSaveGame();
	virtual ~WBActionRosaSaveGame();

	DEFINE_WBACTION_FACTORY( RosaSaveGame );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	void			SaveSlot( const SimpleString& Slot );

	SimpleString			m_SaveSlot;
	WBAction*				m_ConfirmOverwriteAction;
	bool					m_SaveToPendingSlot;
};

#endif // WBACTIONROSASAVEGAME_H
