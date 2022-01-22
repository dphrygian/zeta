#ifndef UISCREENROSACAMPAIGN_H
#define UISCREENROSACAMPAIGN_H

#include "uiscreen.h"

class UIScreenRosaCampaign : public UIScreen
{
public:
	UIScreenRosaCampaign();
	virtual ~UIScreenRosaCampaign();

	DEFINE_UISCREEN_FACTORY( RosaCampaign );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Pushed();

private:
	class RosaCampaign* GetCampaign() const;

	void	InitializeRules();

	void	CreateGeoImageWidgetDefinition( const uint MapGridX, const uint MapGridY, const uint GeoGridIndex );
	void	CreateGeoImageWidget();

	// Config variables
	SimpleString	m_GeoImageArchetype;
	SimpleString	m_ScenarioButtonArchetype;

	// Transient initialization variables to synchronize between init functions
	SimpleString	m_GeoImageWidgetDef;
};

#endif // UISCREENROSACAMPAIGN_H
