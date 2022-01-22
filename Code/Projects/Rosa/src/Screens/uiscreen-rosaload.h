#ifndef UISCREENROSALOAD_H
#define UISCREENROSALOAD_H

#include "uiscreen.h"
#include "rosasaveload.h"

class UIScreenRosaLoad : public UIScreen
{
public:
	UIScreenRosaLoad();
	virtual ~UIScreenRosaLoad();

	DEFINE_UISCREEN_FACTORY( RosaLoad );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Pushed();

private:
	void	InitializeRules();
	void	CreateButtonWidgetDefinition();
	void	CreateLoadActionDefinition();
	void	CreateButtonWidget();

	// Transient variables used to break initialization into functions
	uint			m_SaveSlotIndex;
	Array<RosaSaveLoad::SSaveSlotInfo>	m_SaveSlotInfos;
	float			m_Y;
	SimpleString	m_ButtonWidgetDefinitionName;
	SimpleString	m_LoadActionDefinitionName;

	// Rules variables
	SimpleString	m_Archetype;
	float			m_YBase;
	float			m_YStep;
	float			m_X;
};

#endif // UISCREENROSALOAD_H
