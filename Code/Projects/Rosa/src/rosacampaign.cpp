#include "core.h"
#include "rosacampaign.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "idatastream.h"
#include "packstream.h"
#include "configmanager.h"
#include "fileutil.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "Components/wbcomprosacampaignartifact.h"
#include "wbcomponentarrays.h"
#include "Common/uimanagercommon.h"
#include "wbactionfactory.h"
#include "reversehash.h"

/*static*/ RosaCampaign* RosaCampaign::GetCampaign()
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pGame );

	RosaCampaign* const		pCampaign	= pGame->GetCampaign();
	DEVASSERT( pCampaign );

	return pCampaign;
}

RosaCampaign::RosaCampaign()
:	m_GeoGridMapSize( 0 )
,	m_NumScenarios( 0 )
,	m_GeoGridsW( 0 )
,	m_GeoGridsH( 0 )
,	m_GeoGrids()
,	m_RegionTypes()
,	m_MissionTypes()
,	m_QueueResultsDelay( 0.0f )
,	m_CurrentGeoGridIndex( 0 )
,	m_SelectedGeoGridIndex( 0 )
,	m_ScenarioCompleted( false )
,	m_ScenarioSucceeded( false )
,	m_QueueResultsEventUID( 0 )
,	m_ArtifactsTarget( 0 )
,	m_ArtifactsResolved( 0 )
,	m_ArtifactsCapacity( 0 )
,	m_ResetActions()
,	m_TurnActions()
,	m_TurnStartActions()
,	m_PostGenStartActions()
{
	RegisterForEvents();

	// Maybe later, support reinitializing from different campaigns, for major expansions.
	InitializeFromDefinition( "RosaCampaign" );
}

RosaCampaign::~RosaCampaign()
{
	// I don't unregister for events here because world has already been destroyed. Assumptions!

	WBActionFactory::ClearActionArray( m_ResetActions );
	WBActionFactory::ClearActionArray( m_TurnActions );
	WBActionFactory::ClearActionArray( m_TurnStartActions );
	WBActionFactory::ClearActionArray( m_PostGenStartActions );
}

void RosaCampaign::RegisterForEvents()
{
	STATIC_HASHED_STRING( Campaign_TakePostGenTurn );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCampaign_TakePostGenTurn, this, NULL );

	STATIC_HASHED_STRING( Campaign_GenerateScenarios );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCampaign_GenerateScenarios, this, NULL );

	STATIC_HASHED_STRING( Campaign_StartScenario );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCampaign_StartScenario, this, NULL );

	STATIC_HASHED_STRING( Campaign_ShowResults );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCampaign_ShowResults, this, NULL );

	STATIC_HASHED_STRING( Campaign_FailScenario );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCampaign_FailScenario, this, NULL );

	STATIC_HASHED_STRING( Campaign_SucceedScenario );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCampaign_SucceedScenario, this, NULL );

	STATIC_HASHED_STRING( OnArtifactResolved );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sOnArtifactResolved, this, NULL );

	STATIC_HASHED_STRING( OnDied );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sOnDied, this, NULL );

	STATIC_HASHED_STRING( PostWorldGen );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sPostWorldGen, this, NULL );

	STATIC_HASHED_STRING( OnMasterFileLoaded );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sOnMasterFileLoaded, this, NULL );
}

void RosaCampaign::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( QueueResultsDelay );
	m_QueueResultsDelay = ConfigManager::GetInheritedFloat( sQueueResultsDelay, 0.0f, sDefinitionName );

	STATICHASH( GeoGridMapSize );
	m_GeoGridMapSize = ConfigManager::GetInheritedInt( sGeoGridMapSize, 0, sDefinitionName );
	DEVASSERT( m_GeoGridMapSize == 3 || m_GeoGridMapSize == 5 );	// There are reasons these are the only values that I'd want to use.

	STATICHASH( NumScenarios );
	m_NumScenarios = ConfigManager::GetInheritedInt( sNumScenarios, 0, sDefinitionName );
	DEVASSERT( m_NumScenarios > 0 );
	// See also RosaCampaign::Load
	m_GeoGridsW = 5 + m_NumScenarios;			// 2 buffer tiles on either side + 1 starting tile + n scenario tiles
	m_GeoGridsH = 3 + ( 2 * m_NumScenarios );	// 2 buffer tiles on either side + 2n-1 mission tiles at penultimate mission

	DEVASSERT( m_GeoGrids.Empty() );
	// Push back individually since Resize doesn't construct elements
	FOR_EACH_INDEX( GeoGridIndex, m_GeoGridsW * m_GeoGridsH )
	{
		m_GeoGrids.PushBack();
	}

	STATICHASH( NumRegionTypes );
	const uint NumRegionTypes = ConfigManager::GetInheritedInt( sNumRegionTypes, 0, sDefinitionName );
	FOR_EACH_INDEX( RegionTypeIndex, NumRegionTypes )
	{
		SRegionType& RegionType = m_RegionTypes.PushBack();
		RegionType.m_Hash = RegionType.m_Name = ConfigManager::GetInheritedSequenceString( "RegionType%d", RegionTypeIndex, "", sDefinitionName );

		STATICHASH( Weight );
		RegionType.m_Weight = ConfigManager::GetInheritedFloat( sWeight, 1.0f, RegionType.m_Hash );

		STATICHASH( Level );
		RegionType.m_Level = ConfigManager::GetInheritedHash( sLevel, HashedString::NullString, RegionType.m_Hash );
	}

	STATICHASH( NumMissionTypes );
	const uint NumMissionTypes = ConfigManager::GetInheritedInt( sNumMissionTypes, 0, sDefinitionName );
	FOR_EACH_INDEX( MissionTypeIndex, NumMissionTypes )
	{
		SMissionType& MissionType = m_MissionTypes.PushBack();
		MissionType.m_Hash = MissionType.m_Name = ConfigManager::GetInheritedSequenceString( "MissionType%d", MissionTypeIndex, "", sDefinitionName );

		STATICHASH( Weight );
		MissionType.m_Weight = ConfigManager::GetInheritedFloat( sWeight, 1.0f, MissionType.m_Hash );

		STATICHASH( Priority );
		MissionType.m_Priority = ConfigManager::GetInheritedInt( sPriority, 0, MissionType.m_Hash );

		STATICHASH( NumObjectiveTags );
		const uint NumObjectiveTags = ConfigManager::GetInheritedInt( sNumObjectiveTags, 0, MissionType.m_Hash );
		FOR_EACH_INDEX( ObjectiveTagIndex, NumObjectiveTags )
		{
			SObjectiveTag& ObjectiveTag = MissionType.m_ObjectiveTags.PushBack();

			ObjectiveTag.m_Tag		= ConfigManager::GetInheritedSequenceHash( "ObjectiveTag%d",			ObjectiveTagIndex, HashedString::NullString,	MissionType.m_Hash );
			ObjectiveTag.m_Optional	= ConfigManager::GetInheritedSequenceBool( "ObjectiveTag%dOptional",	ObjectiveTagIndex, false,						MissionType.m_Hash );
		}

		STATICHASH( NumArtifactTags );
		const uint NumArtifactTags = ConfigManager::GetInheritedInt( sNumArtifactTags, 0, MissionType.m_Hash );
		FOR_EACH_INDEX( ArtifactTagIndex, NumArtifactTags )
		{
			const HashedString ArtifactTag = ConfigManager::GetInheritedSequenceHash( "ArtifactTag%d", ArtifactTagIndex, HashedString::NullString, MissionType.m_Hash );
			MissionType.m_ArtifactTags.PushBack( ArtifactTag );
		}

		STATICHASH( TargetArtifactPercent );
		MissionType.m_TargetArtifactPercent = ConfigManager::GetInheritedFloat( sTargetArtifactPercent, 1.0f, MissionType.m_Hash );

		STATICHASH( WinIfArtifactsResolved );
		MissionType.m_WinIfArtifactsResolved = ConfigManager::GetInheritedBool( sWinIfArtifactsResolved, false, MissionType.m_Hash );

		STATICHASH( FailIfPlayerDies );
		MissionType.m_FailIfPlayerDies = ConfigManager::GetInheritedBool( sFailIfPlayerDies, false, MissionType.m_Hash );

		STATICHASH( FailIfArtifactsReduced );
		MissionType.m_FailIfArtifactsReduced = ConfigManager::GetInheritedBool( sFailIfArtifactsReduced, false, MissionType.m_Hash );
	}

	WBActionFactory::InitializeActionArray( sDefinitionName, "Reset",			m_ResetActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "Turn",			m_TurnActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "TurnStart",		m_TurnStartActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "PostGenStart",	m_PostGenStartActions );
}

// CAMTODO: Adapt more from Vamp as needed.
/*virtual*/ void RosaCampaign::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( Campaign_TakePostGenTurn );
	STATIC_HASHED_STRING( Campaign_GenerateScenarios );
	STATIC_HASHED_STRING( Campaign_StartScenario );
	STATIC_HASHED_STRING( Campaign_ShowResults );
	STATIC_HASHED_STRING( Campaign_FailScenario );
	STATIC_HASHED_STRING( Campaign_SucceedScenario );
	STATIC_HASHED_STRING( OnArtifactResolved );
	STATIC_HASHED_STRING( OnDied );
	STATIC_HASHED_STRING( PostWorldGen );
	STATIC_HASHED_STRING( OnMasterFileLoaded );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sCampaign_TakePostGenTurn )
	{
		TakePostGenTurn();
	}
	else if( EventName == sCampaign_GenerateScenarios )
	{
		GenerateScenarios();
		PublishToHUD();
	}
	else if( EventName == sCampaign_StartScenario )
	{
		StartScenario();
	}
	else if( EventName == sCampaign_ShowResults )
	{
		DEVASSERT( m_ScenarioCompleted );

		static const float sDelay = 0.0f;
		QueueResults( sDelay );
	}
	else if( EventName == sCampaign_FailScenario )
	{
		STATIC_HASHED_STRING( AllowContinue );
		const bool AllowContinue = Event.GetBool( sAllowContinue );

		FailScenario( AllowContinue );
	}
	else if( EventName == sCampaign_SucceedScenario )
	{
		SucceedScenario();
	}
	else if( EventName == sOnArtifactResolved )
	{
		STATIC_HASHED_STRING( Tag );
		const HashedString Tag = Event.GetHash( sTag );

		if( GetMissionType().m_ArtifactTags.Find( Tag ) )
		{
			DEVASSERT( m_ArtifactsResolved < m_ArtifactsCapacity );
			m_ArtifactsResolved++;

			if( m_ArtifactsResolved == m_ArtifactsCapacity )
			{
				WB_MAKE_EVENT( CompleteObjective, NULL );
				WB_SET_AUTO( CompleteObjective, Hash, ObjectiveTag, Tag );
				WB_SET_AUTO( CompleteObjective, Bool, Fail, false );
				WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), CompleteObjective, RosaGame::GetPlayer() );
			}

			PublishToHUD();

			//CheckWinLoseConditions();
		}
	}
	else if( EventName == sOnDied )
	{
		// ZETA: We don't want any of this.
		//if( IsInHub() )
		//{
		//	// Do nothing, we shouldn't die in the hub (and if we do, the campaign doesn't handle it)
		//}
		//else
		//{
		//	STATIC_HASHED_STRING( EventOwner );
		//	if( Event.GetEntity( sEventOwner ) == RosaGame::GetPlayer() )
		//	{
		//		if( m_ScenarioCompleted )
		//		{
		//			// HACKHACK: Player died after scenario was previously succeeded

		//			QueueResults( m_QueueResultsDelay );
		//		}
		//		else
		//		{
		//			CheckWinLoseConditions();
		//		}
		//	}
		//}
	}
	else if( EventName == sPostWorldGen )
	{
		// NOTE: I changed this from PostLevelTransition so it
		// will work when I regenerate the level in dev builds.
		OnPostWorldGen();
	}
	else if( EventName == sOnMasterFileLoaded )
	{
		// Update UI to reflect loaded state
		PublishToHUD();

		// Reshow the victory/defeat titles as needed
		if( m_ScenarioCompleted )
		{
			WB_MAKE_EVENT( ShowScenarioResultTitles, NULL );
			WB_SET_AUTO( ShowScenarioResultTitles, Bool, Success, m_ScenarioSucceeded );
			WB_SET_AUTO( ShowScenarioResultTitles, Bool, AllowContinue, true );
			WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), ShowScenarioResultTitles, RosaGame::GetPlayer() );
		}
	}
}

// This is pared down from the massive function in Vamp.
// CAMTODO: Adapt more from Vamp as needed.
void RosaCampaign::PublishToHUD()
{
	PROFILE_FUNCTION;

	STATICHASH( Campaign );

	// ****************************************************************
	// Set objective numbers
	STATICHASH( ArtifactsTarget );
	ConfigManager::SetInt( sArtifactsTarget, m_ArtifactsTarget, sCampaign );

	STATICHASH( ArtifactsResolved );
	ConfigManager::SetInt( sArtifactsResolved, m_ArtifactsResolved, sCampaign );

	STATICHASH( ArtifactsCapacity );
	ConfigManager::SetInt( sArtifactsCapacity, m_ArtifactsCapacity, sCampaign );

	// ****************************************************************
	// Set campaign screen side panel mission information

	// This is also used for the title card
	STATICHASH( RegionType );
	ConfigManager::SetString( sRegionType, GetRegionType().m_Name.CStr(), sCampaign );

	// This is also used for the title card
	STATICHASH( MissionType );
	ConfigManager::SetString( sMissionType, GetMissionType().m_Name.CStr(), sCampaign );

	// Assemble and publish mission info string
	{
		SimpleString MissionInfoString;

		STATICHASH( MissionInfo_Base );
		MissionInfoString += ConfigManager::GetLocalizedString( sMissionInfo_Base, "" );

		STATICHASH( MissionInfo );
		ConfigManager::SetString( sMissionInfo, MissionInfoString.CStr(), sCampaign );
	}

	// ****************************************************************
	// Set results screen
	STATICHASH( ResultsHeader );
	ConfigManager::SetString( sResultsHeader, m_ScenarioSucceeded ? "Results_Success" : "Results_Failure", sCampaign );

	SimpleString ResultsString;

	STATICHASH( ObjectivesListLeft );
	const SimpleString ObjectivesString = ConfigManager::GetLocalizedString( sObjectivesListLeft, "" );

	ResultsString += ObjectivesString;

	if( !IsWon() )
	{
		STATICHASH( ResultsThreatUp );
		STATICHASH( ResultsThreatDown );
		const SimpleString ThreatString = ConfigManager::GetLocalizedString( m_ScenarioSucceeded ? sResultsThreatDown : sResultsThreatUp, "" );

		ResultsString += ThreatString;
	}

	STATICHASH( ResultsText );
	ConfigManager::SetString( sResultsText, ResultsString.CStr(), sCampaign );
}

// This is true if we're in any level whose def is tagged as IsHub
// (including the title screen, which is why I've kept this around in Zeta)
bool RosaCampaign::IsInHub() const
{
	RosaGame* const pGame = RosaFramework::GetInstance()->GetGame();
	return pGame->IsInHub();
}

void RosaCampaign::OnPostWorldGen()
{
	// Reset state in new level
	m_ScenarioCompleted		= false;
	m_ScenarioSucceeded		= false;
	m_QueueResultsEventUID	= 0;

	// Get initial artifact counts for scoring and display on HUD
	m_ArtifactsResolved					= 0;
	m_ArtifactsCapacity					= GetArtifactCapacity();
	const float fTargetArtifacts		= 0.0f;
	m_ArtifactsTarget					= static_cast<uint>( fTargetArtifacts );
	PublishToHUD();

	// Clear and set objective text.
	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	WB_MAKE_EVENT( ClearObjectives, NULL );
	WB_DISPATCH_EVENT( pEventManager, ClearObjectives, RosaGame::GetPlayer() );

	if( !IsInHub() )
	{
		FOR_EACH_ARRAY( ObjectiveTagIter, GetMissionType().m_ObjectiveTags, SObjectiveTag )
		{
			const SObjectiveTag& ObjectiveTag = ObjectiveTagIter.GetValue();

			WB_MAKE_EVENT( AddObjective, NULL );
			WB_SET_AUTO( AddObjective, Hash, ObjectiveTag, ObjectiveTag.m_Tag );
			WB_DISPATCH_EVENT( pEventManager, AddObjective, RosaGame::GetPlayer() );
		}
	}
}

void RosaCampaign::CheckWinLoseConditions()
{
	if( m_ScenarioCompleted )
	{
		return;
	}

	if( IsInHub() )
	{
		return;
	}

	const SMissionType& MissionType = GetMissionType();
	if( MissionType.m_FailIfPlayerDies && !RosaGame::IsPlayerAlive() )
	{
		static const bool skAllowContinue = false;
		FailScenario( skAllowContinue );
	}
	else if( MissionType.m_FailIfArtifactsReduced && m_ArtifactsCapacity < m_ArtifactsTarget )
	{
		static const bool skAllowContinue = true;
		FailScenario( skAllowContinue );
	}
	else if( MissionType.m_WinIfArtifactsResolved && m_ArtifactsResolved >= m_ArtifactsTarget )
	{
		SucceedScenario();
	}
}

void RosaCampaign::FailScenario( const bool AllowContinue )
{
	if( IsInHub() )
	{
		return;
	}

	if( m_ScenarioCompleted )
	{
		return;
	}

	m_ScenarioCompleted	= true;
	m_ScenarioSucceeded	= false;

	WB_MAKE_EVENT( SpawnManager_Stop, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SpawnManager_Stop, RosaGame::GetPlayer() );

	FOR_EACH_ARRAY( ObjectiveTagIter, GetMissionType().m_ObjectiveTags, SObjectiveTag )
	{
		const SObjectiveTag& ObjectiveTag = ObjectiveTagIter.GetValue();

		if( ObjectiveTag.m_Optional )
		{
			// Don't explicitly fail optional objectives
			continue;
		}

		WB_MAKE_EVENT( CompleteObjective, NULL );
		WB_SET_AUTO( CompleteObjective, Hash, ObjectiveTag, ObjectiveTag.m_Tag );
		WB_SET_AUTO( CompleteObjective, Bool, Fail, true );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), CompleteObjective, RosaGame::GetPlayer() );
	}

	if( !AllowContinue )
	{
		QueueResults( m_QueueResultsDelay );
	}

	// Update results screen
	PublishToHUD();

	WB_MAKE_EVENT( ShowScenarioResultTitles, NULL );
	WB_SET_AUTO( ShowScenarioResultTitles, Bool, Success, false );
	WB_SET_AUTO( ShowScenarioResultTitles, Bool, AllowContinue, AllowContinue );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), ShowScenarioResultTitles, RosaGame::GetPlayer() );
}

void RosaCampaign::SucceedScenario()
{
	if( IsInHub() )
	{
		return;
	}

	if( m_ScenarioCompleted )
	{
		return;
	}

	m_ScenarioCompleted	= true;
	m_ScenarioSucceeded	= true;

	// Stop the spawn manager so we can't keep farming chumps
	WB_MAKE_EVENT( SpawnManager_Stop, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SpawnManager_Stop, RosaGame::GetPlayer() );

	FOR_EACH_ARRAY( ObjectiveTagIter, GetMissionType().m_ObjectiveTags, SObjectiveTag )
	{
		const SObjectiveTag& ObjectiveTag = ObjectiveTagIter.GetValue();

		WB_MAKE_EVENT( CompleteObjective, NULL );
		WB_SET_AUTO( CompleteObjective, Hash, ObjectiveTag, ObjectiveTag.m_Tag );
		WB_SET_AUTO( CompleteObjective, Bool, Fail, false );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), CompleteObjective, RosaGame::GetPlayer() );
	}

	// Update results screen
	PublishToHUD();

	WB_MAKE_EVENT( ShowScenarioResultTitles, NULL );
	WB_SET_AUTO( ShowScenarioResultTitles, Bool, Success, true );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), ShowScenarioResultTitles, RosaGame::GetPlayer() );
}

void RosaCampaign::QueueResults( const float Delay )
{
	DEVASSERT( !IsInHub() );

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	if( Delay == 0.0f )
	{
		// Cancel any previous event, queue a new one to force it to show sooner.
		pEventManager->UnqueueEvent( m_QueueResultsEventUID );
	}

	STATIC_HASHED_STRING( ResultsScreen );
	WB_MAKE_EVENT( RepushUIScreen, NULL );
	WB_SET_AUTO( RepushUIScreen, Hash, Screen, sResultsScreen );
	m_QueueResultsEventUID = WB_QUEUE_EVENT_DELAY( pEventManager, RepushUIScreen, NULL, Delay );
}

uint RosaCampaign::GetArtifactCapacity() const
{
	uint OutCapacity = 0;

	const Array<HashedString>&					ArtifactTags	= GetMissionType().m_ArtifactTags;
	const Array<WBCompRosaCampaignArtifact*>*	pArtifacts		= WBComponentArrays::GetComponents<WBCompRosaCampaignArtifact>();
	if( pArtifacts )
	{
		FOR_EACH_ARRAY( ArtifactIter, *pArtifacts, WBCompRosaCampaignArtifact* )
		{
			const WBCompRosaCampaignArtifact* const pArtifact	= ArtifactIter.GetValue();
			const HashedString						ArtifactTag	= pArtifact->GetTag();
			if( ArtifactTags.Find( ArtifactTag ) )
			{
				const uint							Capacity	= pArtifact->GetCapacity();
				OutCapacity										+= Capacity;
			}
		}
	}

	return OutCapacity;
}

// This is invoked from RosaGame::GoToLevel() when m_IsRestarting (either starting initial level or returning to title).
// That's probably fine, though I'll have to see where campaign vs. persistence stuff lives and how we restart the roguelike.
void RosaCampaign::Reset()
{
	m_CurrentGeoGridIndex	= GetGeoGridIndex( 2, m_NumScenarios + 1 );
	m_SelectedGeoGridIndex	= GetGeoGridIndex( 3, m_NumScenarios );		// This is somewhat arbitrary and might be selected by opening the screen and setting focus anyway.

	WBActionFactory::ExecuteActionArray( m_ResetActions, WBEvent(), NULL );
}

// CAMTODO: The entire concept of "turns" probably doesn't map to this campaign. (Actually, it does now!)
// VAMPNOTE: TakePreGenTurn happens *before* world generation. For side effects
// that require a player entity, use TakePostGenTurn.
void RosaCampaign::TakePreGenTurn()
{
	// CAMTODO: This happens every turn, which means when returning to the hub.
	// The meaning of returning to the hub is gonna change, so... figure it out later.
	//if( m_Legacy == 0 )
	{
		// Initialize campaign if we've just started
		//m_Legacy = 1;
		//m_Season = 1;
		//m_Episode = 1;

		WBActionFactory::ExecuteActionArray( m_TurnStartActions, WBEvent(), NULL );
	}

	WBActionFactory::ExecuteActionArray( m_TurnActions, WBEvent(), NULL );

	PublishToHUD();
}

// CAMTODO: The entire concept of "turns" probably doesn't map to this campaign. (Actually, it does now!)
// VAMPNOTE: TakePostGenTurn happens *after* world generation; its side effects
// may depend on there being a player entity. For pre-generation effects,
// use TakePreGenTurn.
void RosaCampaign::TakePostGenTurn()
{
	DEVASSERT( IsInHub() );

	// CAMTODO: This happens every turn, which means when returning to the hub.
	// The meaning of returning to the hub is gonna change, so... figure it out later.
	// (This currently causes the player to get their starting items again.)
	//if( m_Season == 1 && m_Episode == 1 )
	{
		WBActionFactory::ExecuteActionArray( m_PostGenStartActions, WBEvent(), NULL );
	}
}

// CAMTODO: Somewhere (not here!) there will be a need to generate region types for newly-visited geo;
// it is important that I save the region type for each location at least until it scrolls off the west
// edge of the map. By expanding the map from 3x3 to 5x5, I've introduced the possibility that something
// is generated, scrolled off the north or south edge of the map, and then scrolled back onto the map
// by an opposite move. I want the terrain to be stable in that case (both the region type and any random
// offset applied to its UI image). Which means I need to store stuff not just in m_GeoGrids but in a
// map of locations to geo or something. Or, maybe the more obvious way, knowing the campaign duration,
// generate the full 2D map including all the places you'll never be able to see. Only the mission types
// will need to be generated just-in-time (or, like, one turn ahead if I want them to be visible there).
//
// Before I do that, I should probably revisit the turn actions stuff because I want to use those properly.
// There will be stuff I do at the start (to include after a death) and other stuff I do each turn (after
// a successful scenario). And I should bring back the "episode" counter to indicate progress in days or
// whatever, which can also be indicated on the save file where it used to show legacy/season/episode.
// (See RosaSaveLoad stuff for that.)
//
// Incidentally, if I'm generating one turn ahead, then I can do interesting things like putting the
// most rare/desirable missions in the northmost OR southmost positions, because there's only one
// path to each of those, whereas there are two paths to the next positions and three to the center.
//   4 | 0-1-4
//  15 | 0-1-5 | 0-2-6
// 026 | 0-1-6 | 0-2-6 | 0-3-6
//  37 | 0-2-7 | 0-3-7
//   8 | 0-3-8
// It's maybe a little weird that there would be a rare AND easy path, but it doesn't seem like a big deal.
//
// The full campaign map size should be (5+number of missions)x(3+2*number of missions), because:
// X: 2 buffer tiles on the west side; 1 starting tile; n mission tiles; 2 buffer tiles on the east side
// Y: 2n-1 mission tiles at penultimate mission; 2 buffer tiles each on north and south.
//
// 1234567890 = 5+n for n=5
// .......... 1
// .......... 2
// ......4... 3
// .....34... 4
// ....234... 5
// ...1234... 6
// ..S1234E.. 7
// ...1234... 8
// ....234... 9
// .....34... 10
// ......4... 11
// .......... 12
// .......... 13 = 2n+3
//
// Starting location = (2,n+1)
// Destination column = n+2

void RosaCampaign::GenerateScenarios()
{
	DEVASSERT( IsInHub() );

	// DLP 16 Oct 2021: We don't have neighborhoods like in Vamp, so missions are just missions.
	// But with some sort of digraph linkage.
	// Maybe instead of calling them missions, it should be "scenarios" or something, which
	// encapsulates a mission at a region or whatever.

	// First, mark all geogrids as inactive
	FOR_EACH_ARRAY( GeoGridIter, m_GeoGrids, SGeoGrid )
	{
		SGeoGrid& GeoGrid = GeoGridIter.GetValue();
		GeoGrid.m_Active = false;
	}

	// CAMTODO: Advance the grid? Or do that somewhere else? Where do turns fit in?

	// Mark the next three geogrids as active and roll their mission types if they don't already have them?
	// CAMTODO: Where should regions be rolled?
	const uint MiddleGeoGridIndex		= ( m_GeoGridMapSize * m_GeoGridMapSize ) / 2;
	const uint NortheastGeoGridIndex	= ( MiddleGeoGridIndex + 1 ) - m_GeoGridMapSize;
	const uint EastGeoGridIndex			= ( MiddleGeoGridIndex + 1 );
	const uint SoutheastGeoGridIndex	= ( MiddleGeoGridIndex + 1 ) + m_GeoGridMapSize;

	GetGeoGrid( NortheastGeoGridIndex ).m_MissionTypeIndex	= Math::ArrayWeightedRandom( m_MissionTypes, []( const SMissionType& MissionType ) { return MissionType.m_Weight; } );
	GetGeoGrid( NortheastGeoGridIndex ).m_Active			= true;
	GetGeoGrid( EastGeoGridIndex ).m_MissionTypeIndex		= Math::ArrayWeightedRandom( m_MissionTypes, []( const SMissionType& MissionType ) { return MissionType.m_Weight; } );
	GetGeoGrid( EastGeoGridIndex ).m_Active					= true;
	GetGeoGrid( SoutheastGeoGridIndex ).m_MissionTypeIndex	= Math::ArrayWeightedRandom( m_MissionTypes, []( const SMissionType& MissionType ) { return MissionType.m_Weight; } );
	GetGeoGrid( SoutheastGeoGridIndex ).m_Active			= true;
}

// CAMTODO: This can probably go away. Or it'll be a check from progress on the scenario graph?
bool RosaCampaign::CanStartScenario()
{
	return true;
}

void RosaCampaign::StartScenario()
{
	DEVASSERT( IsInHub() );

	ASSERT( CanStartScenario() );
	if( !CanStartScenario() )
	{
		return;
	}

	const SRegionType&		RegionType		= GetRegionType();

	RosaGame* const			pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( GoToLevel, NULL );
	WB_LOG_EVENT( GoToLevel );
	WB_SET_AUTO( GoToLevel, Hash, Level, RegionType.m_Level );
	WB_DISPATCH_EVENT( pEventManager, GoToLevel, pGame );
}

// CAMTODO: Is this still used for anything?
// HACKHACK for some hub overrides
HashedString RosaCampaign::GetCampaignStatus() const
{
	if( IsLost() )
	{
		STATIC_HASHED_STRING( Campaign_Lost );
		return sCampaign_Lost;
	}
	else if( IsWon() )
	{
		STATIC_HASHED_STRING( Campaign_Won );
		return sCampaign_Won;
	}
	else
	{
		STATIC_HASHED_STRING( Campaign_InProgress );
		return sCampaign_InProgress;
	}
}

bool RosaCampaign::CanModify( const bool ForceModify ) const
{
	// ROSANOTE: I'm listing this here in case I ever create any rules that should supersede a force
	if( ForceModify )
	{
		return true;
	}

	if( IsInHub() )
	{
		return false;
	}

	return true;
}

// CAMTODO: Update the rules for all these overrides/modifies
bool RosaCampaign::OverrideBool( const HashedString& Name, const bool Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	bool RetVal = Default;

	// Hard-coded priority: campaign status > plot points > opportunities > twists > mission types > neighborhoods > SE > LSE > acts
	RetVal = ConfigManager::GetInheritedBool( Name, RetVal, GetMissionType().m_Hash );
	RetVal = ConfigManager::GetInheritedBool( Name, RetVal, GetCampaignStatus() );

	return RetVal;
}

int RosaCampaign::ModifyInt( const HashedString& Name, const int Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	float RetVal = static_cast<float>( Default );

	RetVal *= ConfigManager::GetInheritedFloat( Name, 1.0f, GetMissionType().m_Hash );
	RetVal *= ConfigManager::GetInheritedFloat( Name, 1.0f, GetCampaignStatus() );

	return static_cast<int>( RetVal );
}

int RosaCampaign::OverrideInt( const HashedString& Name, const int Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	int RetVal = Default;

	// Hard-coded priority: campaign status > plot points > opportunities > twists > mission types > neighborhoods > SE > LSE > acts
	RetVal = ConfigManager::GetInheritedInt( Name, RetVal, GetMissionType().m_Hash );
	RetVal = ConfigManager::GetInheritedInt( Name, RetVal, GetCampaignStatus() );

	return RetVal;
}

float RosaCampaign::ModifyFloat( const HashedString& Name, const float Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	float RetVal = Default;

	RetVal *= ConfigManager::GetInheritedFloat( Name, 1.0f, GetMissionType().m_Hash );
	RetVal *= ConfigManager::GetInheritedFloat( Name, 1.0f, GetCampaignStatus() );

	return RetVal;
}

float RosaCampaign::OverrideFloat( const HashedString& Name, const float Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	float RetVal = Default;

	// Hard-coded priority: campaign status > plot points > opportunities > twists > mission types > neighborhoods > SE > LSE > acts
	RetVal = ConfigManager::GetInheritedFloat( Name, RetVal, GetMissionType().m_Hash );
	RetVal = ConfigManager::GetInheritedFloat( Name, RetVal, GetCampaignStatus() );

	return RetVal;
}

SimpleString RosaCampaign::AppendString( const HashedString& Name, const SimpleString& Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	SimpleString RetVal = Default;

	// Hard-coded order (reverse of priority): acts -> LSE -> SE -> neighborhoods -> mission types -> twists -> opportunities -> plot points -> campaign status
	RetVal += ConfigManager::GetInheritedString( Name, "", GetMissionType().m_Hash );
	RetVal += ConfigManager::GetInheritedString( Name, "", GetCampaignStatus() );

	return RetVal;
}

SimpleString RosaCampaign::OverrideString( const HashedString& Name, const SimpleString& Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	SimpleString RetVal = Default;

	// Hard-coded priority: campaign status > plot points > opportunities > twists > mission types > neighborhoods > SE > LSE > acts
	RetVal = ConfigManager::GetInheritedString( Name, RetVal.CStr(), GetMissionType().m_Hash );
	RetVal = ConfigManager::GetInheritedString( Name, RetVal.CStr(), GetCampaignStatus() );

	return RetVal;
}

HashedString RosaCampaign::OverrideHash( const HashedString& Name, const HashedString& Default, const bool ForceModify /*= false*/ ) const
{
	if( !CanModify( ForceModify ) )
	{
		return Default;
	}

	HashedString RetVal = Default;

	// Hard-coded priority: campaign status > plot points > opportunities > twists > mission types > neighborhoods > SE > LSE > acts
	RetVal = ConfigManager::GetInheritedHash( Name, RetVal, GetMissionType().m_Hash );
	RetVal = ConfigManager::GetInheritedHash( Name, RetVal, GetCampaignStatus() );

	return RetVal;
}

// Keep this in sync with the list of prioritized config modifiers in the functions above.
uint RosaCampaign::GetElementCount( const bool ForceModify /*= false*/ ) const
{
	return CanModify( ForceModify ) ? 2 : 0;
}

// When using this for e.g. appending resolve groups, it creates a hard-coded priority:
// campaign status > plot points > opportunities > twists > mission types > neighborhoods > SE > LSE > acts
SimpleString RosaCampaign::GetElementName( const uint Element ) const
{
	switch( Element )
	{
	case 0:
		return GetMissionType().m_Name;
	case 1:
		return ReverseHash::ReversedHash( GetCampaignStatus() );
	}

	WARN;
	return "";
}

bool RosaCampaign::IsWon() const
{
	return false;
}

bool RosaCampaign::IsLost() const
{
	return false;
}

// CAMTODO: Is this used anywhere?
RosaGame* RosaCampaign::GetGame() const
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pGame );

	return pGame;
}

uint RosaCampaign::GetRegionTypeIndex( const HashedString& Name ) const
{
	FOR_EACH_ARRAY( RegionTypeIter, m_RegionTypes, SRegionType )
	{
		if( RegionTypeIter.GetValue().m_Hash == Name )
		{
			return RegionTypeIter.GetIndex();
		}
	}

	DEVWARNDESC( "Could not find index for region type name." );
	return INVALID_ARRAY_INDEX;
}

uint RosaCampaign::GetMissionTypeIndex( const HashedString& Name ) const
{
	FOR_EACH_ARRAY( MissionTypeIter, m_MissionTypes, SMissionType )
	{
		if( MissionTypeIter.GetValue().m_Hash == Name )
		{
			return MissionTypeIter.GetIndex();
		}
	}

	DEVWARNDESC( "Could not find index for mission type name." );
	return INVALID_ARRAY_INDEX;
}

void RosaCampaign::Report() const
{
	PRINTF( "RosaCampaign:\n" );
	PRINTF( "  Geogrids Width:  %d\n", m_GeoGridsW );
	PRINTF( "  Geogrids Height: %d\n", m_GeoGridsH );
	PRINTF( "  Current geogrid index: %d (%d, %d)\n", m_CurrentGeoGridIndex, GetGeoGridX( m_CurrentGeoGridIndex ), GetGeoGridY( m_CurrentGeoGridIndex ) );
	PRINTF( "  Selected geogrid index: %d (%d, %d)\n", m_SelectedGeoGridIndex, GetGeoGridX( m_SelectedGeoGridIndex ), GetGeoGridY( m_SelectedGeoGridIndex ) );

	// Print map
	PRINTF( "    " );
	FOR_EACH_INDEX( GeoGridX, m_GeoGridsW )
	{
		const uint OnesX = GeoGridX % 10;
		PRINTF( "%d ", OnesX );
	}
	PRINTF( "\n" );
	FOR_EACH_INDEX( GeoGridY, m_GeoGridsH )
	{
		const uint OnesY = GeoGridY % 10;
		PRINTF( "  %d ", OnesY );
		FOR_EACH_INDEX( GeoGridX, m_GeoGridsW )
		{
			const uint		GeoGridIndex	= GetGeoGridIndex( GeoGridX, GeoGridY );
			if( m_CurrentGeoGridIndex == GeoGridIndex )
			{
				PRINTF(  "X " );
			}
			else
			{
				const SGeoGrid&	GeoGrid			= m_GeoGrids[ GeoGridIndex ];
				PRINTF( "%d ", GeoGrid.m_RegionTypeIndex );
			}
		}
		PRINTF( "\n" );
	}

	//FOR_EACH_ARRAY( GeoGridIter, m_GeoGrids, SGeoGrid )
	//{
	//	const SGeoGrid& GeoGrid = GeoGridIter.GetValue();
	//	if( GeoGrid.m_Active )
	//	{
	//		PRINTF( "  %d:\n", GeoGridIter.GetIndex() );
	//		PRINTF( "    Region type: %s\n", GetRegionType( GeoGrid.m_RegionTypeIndex ).m_Name.CStr() );
	//		PRINTF( "    Mission type: %s\n", GetMissionType( GeoGrid.m_MissionTypeIndex ).m_Name.CStr() );
	//	}
	//}
}

#define VERSION_EMPTY			0
#define VERSION_BASE			1
#define VERSION_REBASE			4
#define VERSION_SCENARIOS		5
#define VERSION_GEOGRIDS		6
#define VERSION_GEOGRIDINDEXES	7
#define VERSION_CURRENT			7

void RosaCampaign::Save( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_NumScenarios );

	Stream.WriteUInt32( m_GeoGrids.Size() );	// This should be implicit from m_NumScenarios, I'm just being safe.
	FOR_EACH_ARRAY( GeoGridIter, m_GeoGrids, SGeoGrid )
	{
		const SGeoGrid& GeoGrid = GeoGridIter.GetValue();
		Stream.WriteBool( GeoGrid.m_Active );
		Stream.WriteBool( GeoGrid.m_Completed );
		// Following the pattern from Vamp, I'm writing hashes instead of indices,
		// which I think was to let me fix things up if the order of elements changed.
		Stream.WriteHashedString( GetRegionType( GeoGrid.m_RegionTypeIndex ).m_Hash );
		Stream.WriteHashedString( GetMissionType( GeoGrid.m_MissionTypeIndex ).m_Hash );
	}

	Stream.WriteUInt32( m_CurrentGeoGridIndex );
	Stream.WriteUInt32( m_SelectedGeoGridIndex );

	Stream.WriteBool( m_ScenarioCompleted );
	Stream.WriteBool( m_ScenarioSucceeded );

	Stream.Write<TEventUID>( m_QueueResultsEventUID );

	Stream.WriteUInt32( m_ArtifactsTarget );
	Stream.WriteUInt32( m_ArtifactsResolved );
	Stream.WriteUInt32( m_ArtifactsCapacity );
}

void RosaCampaign::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();
	if( Version < VERSION_BASE )
	{
		return;
	}

	if( Version >= VERSION_SCENARIOS )
	{
		m_NumScenarios	= Stream.ReadUInt32();
		// See also InitializeFromDefinition
		m_GeoGridsW		= 5 + m_NumScenarios;
		m_GeoGridsH		= 3 + ( 2 * m_NumScenarios );
		const uint	NumGeoGrids			= m_GeoGridsW * m_GeoGridsH;

		// Clearing and pushing back elements instead of resizing just in case any elements
		// in SGeoGrid end up being transient and aren't serialized. Doesn't really matter.
		m_GeoGrids.Clear();
		FOR_EACH_INDEX( GeoGridIndex, NumGeoGrids )
		{
			m_GeoGrids.PushBack();
		}

		const uint	SerializedGeoGrids	= Stream.ReadUInt32();
		ASSERT( SerializedGeoGrids == NumGeoGrids );

		const uint	UsingNumGeoGrids	= Min( SerializedGeoGrids, NumGeoGrids );
		FOR_EACH_INDEX( GeoGridIndex, UsingNumGeoGrids )
		{
			SGeoGrid&	GeoGrid			= m_GeoGrids[ GeoGridIndex ];
			GeoGrid.m_Active			= Stream.ReadBool();
			GeoGrid.m_Completed			= Stream.ReadBool();
			GeoGrid.m_RegionTypeIndex	= GetRegionTypeIndex(	Stream.ReadHashedString() );
			GeoGrid.m_MissionTypeIndex	= GetMissionTypeIndex(	Stream.ReadHashedString() );

			// If these fail, we've removed an element entirely and need to fix things up for it.
			ASSERT( m_RegionTypes.IsValidIndex( GeoGrid.m_RegionTypeIndex ) );
			ASSERT( m_MissionTypes.IsValidIndex( GeoGrid.m_MissionTypeIndex ) );
		}
	}

	if( Version >= VERSION_GEOGRIDINDEXES )
	{
		m_CurrentGeoGridIndex	= Stream.ReadUInt32();
		m_SelectedGeoGridIndex	= Stream.ReadUInt32();
	}

	if( Version >= VERSION_REBASE )
	{
		m_ScenarioCompleted		= Stream.ReadBool();
		m_ScenarioSucceeded		= Stream.ReadBool();

		m_QueueResultsEventUID	= Stream.Read<TEventUID>();

		m_ArtifactsTarget		= Stream.ReadUInt32();
		m_ArtifactsResolved		= Stream.ReadUInt32();
		m_ArtifactsCapacity		= Stream.ReadUInt32();
	}
}
