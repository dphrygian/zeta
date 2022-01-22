#ifndef UISCREENROSASAVE_H
#define UISCREENROSASAVE_H

// ROSATODO: Unify this with RosaLoad? It's almost identical, it just
// filters quicksave and autosave slots and has a different action.

#include "uiscreen.h"
#include "rosasaveload.h"

class UIScreenRosaSave : public UIScreen
{
public:
	UIScreenRosaSave();
	virtual ~UIScreenRosaSave();

	DEFINE_UISCREEN_FACTORY( RosaSave );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Pushed();

	void			RefreshSlots();

private:
	void	InitializeRules();
	void	CreateButtonWidgetDefinition();
	void	CreateSaveActionDefinition();
	void	CreateButtonWidget();

	// Transient variables used to break initialization into functions
	uint			m_SaveSlotIndex;
	Array<RosaSaveLoad::SSaveSlotInfo>	m_SaveSlotInfos;
	float			m_Y;
	SimpleString	m_ButtonWidgetDefinitionName;
	SimpleString	m_SaveActionDefinitionName;

	// Rules variables
	SimpleString	m_Archetype;
	SimpleString	m_ConfirmOverwriteAction;
	float			m_YBase;
	float			m_YStep;
	float			m_X;
};

#endif // UISCREENROSASAVE_H
