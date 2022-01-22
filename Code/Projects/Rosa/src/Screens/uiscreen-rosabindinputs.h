#ifndef UISCREENROSABINDINPUTS_H
#define UISCREENROSABINDINPUTS_H

#include "uiscreen.h"

class UIScreenRosaBindInputs : public UIScreen
{
public:
	UIScreenRosaBindInputs();
	virtual ~UIScreenRosaBindInputs();

	DEFINE_UISCREEN_FACTORY( RosaBindInputs );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	InitializeRules();
	void	CreateLabelWidgetDefinition();
	void	CreateKeyboardWidgetDefinition();
	void	CreateMouseWidgetDefinition();
	void	CreateControllerWidgetDefinition();
	void	CreateBindingActionDefinition();
	void	CreateCompositeWidgetDefinition();
	void	CreateCompositeWidget();

	// Transient variables used to break initialization into functions
	uint			m_ExposedInputIndex;
	SimpleString	m_ExposedInput;
	float			m_Y;
	float			m_YBase;
	float			m_YStep;
	float			m_Column0X;
	float			m_Column1X;
	float			m_Column2X;
	float			m_Column3X;
	SimpleString	m_ArchetypeName;
	SimpleString	m_ControllerArchetypeName;
	SimpleString	m_Parent;
	SimpleString	m_LabelWidgetDefinitionName;
	SimpleString	m_KeyboardWidgetDefinitionName;
	SimpleString	m_MouseWidgetDefinitionName;
	SimpleString	m_ControllerWidgetDefinitionName;
	SimpleString	m_BindActionDefinitionName;
	SimpleString	m_CompositeWidgetDefinitionName;
};

#endif // UISCREENROSABINDINPUTS_H
