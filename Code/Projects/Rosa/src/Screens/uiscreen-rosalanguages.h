#ifndef UISCREENROSALANGUAGES_H
#define UISCREENROSALANGUAGES_H

#include "uiscreen.h"

class UIScreenRosaLanguages : public UIScreen
{
public:
	UIScreenRosaLanguages();
	virtual ~UIScreenRosaLanguages();

	DEFINE_UISCREEN_FACTORY( RosaLanguages );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Pushed();

private:
	void	InitializeRules();
	void	CreateButtonWidgetDefinition();
	void	CreateSelectLanguageActionDefinition();
	void	CreateButtonWidget();

	// Transient variables used to break initialization into functions
	uint				m_LanguageIndex;
	Array<SimpleString>	m_Languages;
	float				m_Y;
	SimpleString		m_ButtonWidgetDefinitionName;
	SimpleString		m_SelectLanguageActionDefinitionName;

	// Rules variables
	SimpleString	m_Archetype;
	float			m_YBase;
	float			m_YStep;
	float			m_X;
};

#endif // UISCREENROSALANGUAGES_H
