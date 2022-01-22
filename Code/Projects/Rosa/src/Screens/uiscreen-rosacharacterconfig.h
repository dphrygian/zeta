#ifndef UISCREENROSACHARACTERCONFIG_H
#define UISCREENROSACHARACTERCONFIG_H

#include "uiscreen.h"
#include "wbeventmanager.h"

class WBCompRosaCharacter;

class UIScreenRosaCharacterConfig : public UIScreen, IWBEventObserver
{
public:
	UIScreenRosaCharacterConfig();
	virtual ~UIScreenRosaCharacterConfig();

	DEFINE_UISCREEN_FACTORY( RosaCharacterConfig );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	RegisterForEvents();

	virtual void	Pushed();

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

private:
	WBCompRosaCharacter*	GetCharacterComponent() const;

	void	InitializeRules();
	void	CreateSkinPresetActionDefinition();
	void	CreateSkinPresetButtonWidgetDefinition();
	void	CreateNailsPresetActionDefinition();
	void	CreateNailsPresetButtonWidgetDefinition();
	void	CreatePresetButtonWidget();

	void	HandleUISliderEvent( const HashedString& SliderName );

	// Transient variables used to break initialization into functions
	uint			m_PresetButtonIndex;
	SimpleString	m_ButtonWidgetDefinitionName;
	SimpleString	m_ActionDefinitionName;
	float			m_ParentWX;

	// Rules variables
	SimpleString	m_SkinPresetButtonArchetype;
	SimpleString	m_NailsPresetButtonArchetype;
	float			m_PresetButtonParentWXSpacing;
};

#endif // UISCREENROSACHARACTERCONFIG_H
