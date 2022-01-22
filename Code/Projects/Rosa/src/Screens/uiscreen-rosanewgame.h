#ifndef UISCREENROSANEWGAME_H
#define UISCREENROSANEWGAME_H

#include "uiscreen.h"
#include "rosasaveload.h"

class UIScreenRosaNewGame : public UIScreen
{
public:
	UIScreenRosaNewGame();
	virtual ~UIScreenRosaNewGame();

	DEFINE_UISCREEN_FACTORY( RosaNewGame );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Pushed();

private:
	void	InitializeRules();
	void	CreateButtonWidgetDefinition();
	void	CreateNewGameActionDefinition();
	void	CreateButtonWidget();

	// Transient variables used to break initialization into functions
	uint			m_SaveSlotIndex;
	Array<RosaSaveLoad::SSaveSlotInfo>	m_SaveSlotInfos;
	float			m_Y;
	SimpleString	m_ButtonWidgetDefinitionName;
	SimpleString	m_NewGameActionDefinitionName;

	// Rules variables
	SimpleString	m_Archetype;
	SimpleString	m_ConfirmOverwriteAction;
	float			m_YBase;
	float			m_YStep;
	float			m_X;
};

#endif // UISCREENROSANEWGAME_H
