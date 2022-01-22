#ifndef WBACTIONROSALOADGAME_H
#define WBACTIONROSALOADGAME_H

#include "wbaction.h"
#include "simplestring.h"

class WBActionRosaLoadGame : public WBAction
{
public:
	WBActionRosaLoadGame();
	virtual ~WBActionRosaLoadGame();

	DEFINE_WBACTION_FACTORY( RosaLoadGame );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	SimpleString	m_SaveSlot;
};

#endif // WBACTIONROSALOADGAME_H
