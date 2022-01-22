#include "core.h"
#include "uiscreen-rosacampaign.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosacampaign.h"
#include "configmanager.h"
#include "mathcore.h"
#include "uifactory.h"

UIScreenRosaCampaign::UIScreenRosaCampaign()
:	m_GeoImageArchetype()
,	m_ScenarioButtonArchetype()
,	m_GeoImageWidgetDef()
{
}

UIScreenRosaCampaign::~UIScreenRosaCampaign()
{
}

/*virtual*/ void UIScreenRosaCampaign::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Flush();

	UIScreen::InitializeFromDefinition( DefinitionName );

	// CAMTODO: Make a bunch of widget definitions.
	// Also maybe find a better way to do this, it's my least favorite pattern in this codebase.
	// Widget defs for geo terrain tiles, with random offsets from campaign
	// Widget defs for at least the next 3 scenarios if not the 5 beyond that
	// Focused action defs for those
	// Widgets for the completed scenarios (and any others?)
	// Widgets for the dotted line trail
	// etc.

	InitializeRules();

	// We can't initialize until the campaign exists. No need to assert, it's expected.
	RosaCampaign* const pCampaign = GetCampaign();
	if( NULL == pCampaign )
	{
		return;
	}

	const uint	CurrentGeoGridX	= pCampaign->GetGeoGridX( pCampaign->m_CurrentGeoGridIndex );
	const uint	CurrentGeoGridY	= pCampaign->GetGeoGridY( pCampaign->m_CurrentGeoGridIndex );

	// CAMTODO: Get the geogrids relative to m_CurrentGeoGridIndex!
	FOR_EACH_INDEX( MapGridY, pCampaign->m_GeoGridMapSize )
	{
		FOR_EACH_INDEX( MapGridX, pCampaign->m_GeoGridMapSize )
		{
			const uint	GeoGridIndex	= pCampaign->GetGeoGridIndex( MapGridX + CurrentGeoGridX, MapGridY + CurrentGeoGridY );
			CreateGeoImageWidgetDefinition( MapGridX, MapGridY, GeoGridIndex );
			CreateGeoImageWidget();

			const RosaCampaign::SGeoGrid&	GeoGrid	= pCampaign->m_GeoGrids[ GeoGridIndex ];
			if( GeoGrid.m_Active )
			{
				// CAMTODO: Make a scenario button
			}
			else if( GeoGrid.m_Completed )
			{
				// CAMTODO: Make an inactive scenario button? Or just an image with a different archetype? X-ed out in the image?
			}
			// CAMTODO: What's the case for something on the eastmost side that's not active yet but we can see coming?
		}
	}

	UpdateRender();
	// CAMTODO: Is there a way to retain the focused element instead of resetting?
	// Set focus (with effects) to whatever the widget for the campaign-selected scenario is?
	// Do we need to set focus with effects to trigger the focus action?
	// Oh yeah dur I can just call SetFocus on the widget I want to focus. Or ForceSetFocus if I need to invoke the focused actions.
	ResetFocus();
	RefreshWidgets();
}

RosaCampaign* UIScreenRosaCampaign::GetCampaign() const
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	if( NULL == pFramework )
	{
		return NULL;
	}

	RosaGame* const			pGame		= pFramework->GetGame();
	if( NULL == pGame )
	{
		return NULL;
	}

	RosaCampaign* const		pCampaign	= pGame->GetCampaign();
	return pCampaign;
}

void UIScreenRosaCampaign::InitializeRules()
{
	MAKEHASH( m_Name );

	STATICHASH( Rules );
	const SimpleString	UsingRules	= ConfigManager::GetString( sRules, "", sm_Name );
	MAKEHASH( UsingRules );

	STATICHASH( GeoImageArchetype );
	m_GeoImageArchetype				= ConfigManager::GetString(	sGeoImageArchetype,			"",		sUsingRules );

	STATICHASH( ScenarioButtonArchetype );
	m_ScenarioButtonArchetype		= ConfigManager::GetString(	sScenarioButtonArchetype,	"",		sUsingRules );
}

void UIScreenRosaCampaign::CreateGeoImageWidgetDefinition( const uint MapGridX, const uint MapGridY, const uint GeoGridIndex )
{
	RosaCampaign* const				pCampaign	= GetCampaign();
	DEVASSERT( pCampaign );

	DEVASSERT( pCampaign->m_GeoGrids.IsValidIndex( GeoGridIndex ) );
	const RosaCampaign::SGeoGrid&	GeoGrid		= pCampaign->m_GeoGrids[ GeoGridIndex ];

	Unused( GeoGrid );

	// Get offsets from center, and convert to parent coordinates (making assumptions about the underlying map image)
	const uint						GridCenter		= pCampaign->m_GeoGridMapSize / 2;
	const int						OffsetX			= MapGridX - GridCenter;
	const int						OffsetY			= MapGridY - GridCenter;
	const float						GridStep		= 1.0f / static_cast<float>( 2 + pCampaign->m_GeoGridMapSize );	// Assumption: the map image is sized to 7x7 for a 5x5 map, so add 2.
	const float						ParentWX		= 0.5f + OffsetX * GridStep;
	const float						ParentHY		= 0.5f + OffsetY * GridStep;
	// CAMTODO: Add some (rolled-once) random offsets from campaign info
	const uint						MapGridIndex	= ( pCampaign->m_GeoGridMapSize * MapGridY ) + MapGridX;

	m_GeoImageWidgetDef								= SimpleString::PrintF( "_CampaignGeoImage%d", MapGridIndex );
	MAKEHASH( m_GeoImageWidgetDef );

	STATICHASH( Extends );
	ConfigManager::SetString(	sExtends,	m_GeoImageArchetype.CStr(),	sm_GeoImageWidgetDef );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat(	sParentWX,	ParentWX,					sm_GeoImageWidgetDef );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat(	sParentHY,	ParentHY,					sm_GeoImageWidgetDef );
}

void UIScreenRosaCampaign::CreateGeoImageWidget()
{
	UIWidget* const pGeoImageWidget = UIFactory::CreateWidget( m_GeoImageWidgetDef, this /*pOwnerScreen*/, NULL /*pParentWidget*/ );
	DEVASSERT( pGeoImageWidget );

	AddWidget( pGeoImageWidget );
}

/*virtual*/ void UIScreenRosaCampaign::Pushed()
{
	UIScreen::Pushed();

	// Reinitialize whenever this screen is pushed to make sure we've got the latest campaign info.
	// I think the UI will be initialized before the campaign has a chance to generate scenarios,
	// so this would be the first chance to do it right.
	InitializeFromDefinition( m_Name );
}
