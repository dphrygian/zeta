#ifndef ROSACAMPAIGN_H
#define ROSACAMPAIGN_H

#include "array.h"
#include "simplestring.h"
#include "iwbeventobserver.h"
#include "wbeventmanager.h"

class IDataStream;
class RosaGame;
class WBAction;

// CAMTODO: Look for things tagged with OLDVAMP comments to find where I previously unhooked some campaign stuff.
// CAMTODO: Rename "missions" to "scenarios" where appropriate.

class RosaCampaign : public IWBEventObserver
{
public:
	RosaCampaign();
	~RosaCampaign();

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

	void	RegisterForEvents();

	void	InitializeFromDefinition( const SimpleString& DefinitionName );

	void	Reset();					// Reset campaign to zero for starting a new game (CAMTODO: Where would this be used now? What lives in campaign vs. persistence?)

	// CAMTODO: The entire concept of "turns" probably doesn't map to this campaign. (Actually, it does now!)
	void	TakePreGenTurn();			// Pre-world generation
	void	TakePostGenTurn();			// Post-world generation

	void	GenerateScenarios();		// Randomly roll the scenarios for the whole campaign

	bool	CanStartScenario();			// Validate currently selected scenario
	void	StartScenario();			// Start scenario if possible

	void	Report() const;

	void	Save( const IDataStream& Stream );
	void	Load( const IDataStream& Stream );

	// DLP 16 Oct 2021: CanModify currently only returns false in the hub. ForceModify is used to override that to modify the hub in a few places. Maybe this could be cleaned up.
	bool			CanModify( const bool ForceModify ) const;
	bool			OverrideBool( const HashedString& Name, const bool Default, const bool ForceModify = false ) const;
	int				ModifyInt( const HashedString& Name, const int Default, const bool ForceModify = false ) const;
	int				OverrideInt( const HashedString& Name, const int Default, const bool ForceModify = false ) const;
	float			ModifyFloat( const HashedString& Name, const float Default, const bool ForceModify = false ) const;
	float			OverrideFloat( const HashedString& Name, const float Default, const bool ForceModify = false ) const;
	SimpleString	AppendString( const HashedString& Name, const SimpleString& Default, const bool ForceModify = false ) const;
	SimpleString	OverrideString( const HashedString& Name, const SimpleString& Default, const bool ForceModify = false ) const;
	HashedString	OverrideHash( const HashedString& Name, const HashedString& Default, const bool ForceModify = false ) const;

	uint			GetElementCount( const bool ForceModify = false ) const;
	SimpleString	GetElementName( const uint Element ) const;

	bool			IsScenarioCompleted() const	{ return m_ScenarioCompleted; }

	bool			IsFinished() const		{ return IsWon() || IsLost(); }
	bool			IsWon() const;
	bool			IsLost() const;

	static RosaCampaign* GetCampaign();

private:
	friend class UIScreenRosaCampaign;	// Let the UI screen have access to all internals that no one else should need.

	// Region types have no serialized properties, they just define the type of level where the scenario takes place.
	struct SRegionType
	{
		SRegionType()
		:	m_Name()
		,	m_Hash()
		,	m_Weight( 0.0f )
		,	m_Level()
		{
		}

		SimpleString	m_Name;
		HashedString	m_Hash;
		// CAMTODO: Region types should be based on progression, maybe with some randomness and
		// the occasional special level (like Spelunky's worm level). Not weighted random.
		// See https://trello.com/c/fS65nh22/1270-region-types-should-probably-not-be-11-with-progression-phases
		float			m_Weight;	// Config; weighted chance of this region appearing under normal circumstances
		HashedString	m_Level;	// Config
	};

	// Tags for the objectives system (i.e., what you do in a scenario).
	// CAMTODO: This may go away or evolve depending on what I do with missions.
	struct SObjectiveTag
	{
		SObjectiveTag()
		:	m_Tag()
		,	m_Optional( false )
		{
		}

		HashedString	m_Tag;
		bool			m_Optional;
	};

	// CAMTODO: Review this, I'm importing it wholesale from Vamp and there's probably a lot I don't need.
	// CAMTODO: Also, I probably want to do a more sophisticated TBC-esque mission system, so this may all go away.
	struct SMissionType
	{
		SMissionType()
		:	m_Name()
		,	m_Hash()
		,	m_Weight( 0.0f )
		,	m_Priority( 0 )
		,	m_ObjectiveTags()
		,	m_ArtifactTags()
		,	m_TargetArtifactPercent( 0.0f )
		,	m_WinIfArtifactsResolved( false )
		,	m_FailIfPlayerDies( false )
		,	m_FailIfArtifactsReduced( false )
		{
		}

		SimpleString			m_Name;
		HashedString			m_Hash;
		float					m_Weight;		// Config; weighted chance of this mission appearing under normal circumstances
		uint					m_Priority;		// Config; sorting order for required missions, low number takes priority

		// Mission parameters
		Array<SObjectiveTag>	m_ObjectiveTags;			// Config; tags for the objective system (i.e., what you do in a mission) (CAMTODO: This is likely to be refactored with a new mission system)
		Array<HashedString>		m_ArtifactTags;				// Config; what type of artifacts (enemies, hostages, etc.) we're counting for this mission
		float					m_TargetArtifactPercent;	// Config; percent of artifact capacity required to win
		bool					m_WinIfArtifactsResolved;	// Config
		bool					m_FailIfPlayerDies;			// Config
		bool					m_FailIfArtifactsReduced;	// Config; fail if reduced to the point that the mission cannot be won (for e.g. hostage rescue missions, where hostages can get killed off)
	};

	// This replaces "neighborhoods" from Vamp. It also encapsulates what I was calling a "scenario" before.
	struct SGeoGrid
	{
		SGeoGrid()
		:	m_Active( false )
		,	m_Completed( false )
		,	m_RegionTypeIndex( 0 )
		,	m_MissionTypeIndex( 0 )
		{
		}

		// Current state of scenario
		bool			m_Active;			// Serialized; is this geogrid available as a scenario
		bool			m_Completed;		// Serialized; was this geogrid completed as a scenario (for displaying with an X over it or whatever)
		//uint			m_ThreatLevel;		// Serialized (CAMTODO: I'll need some version of "threat level" to escalate over the course of the campaign, or maybe just progression)
		uint			m_RegionTypeIndex;	// Serialized
		uint			m_MissionTypeIndex;	// Serialized
		//uint			m_TwistIndex;		// Serialized (CAMTODO: I'll want to bring twists back eventually)
	};

	uint					m_GeoGridMapSize;		// Config; the size of the UI map (should only ever be 3x3 or 5x5)
	uint					m_NumScenarios;			// Config/serialized (to be safe in case it changes)
	uint					m_GeoGridsW;			// Config-ish, always 5+m_NumMissions
	uint					m_GeoGridsH;			// Config-ish, always 3+2*m_NumMissions
	Array<SGeoGrid>			m_GeoGrids;				// Serialized
	Array<SRegionType>		m_RegionTypes;			// Config
	Array<SMissionType>		m_MissionTypes;			// Config

	float					m_QueueResultsDelay;	// Config

	uint					m_CurrentGeoGridIndex;	// Serialized; where you are (where the map camera is centered)
	uint					m_SelectedGeoGridIndex;	// Serialized; currently selected/active geogrid (or scenario)
	// CAMTODO: I may be able to collapse these, there will be no notion of a failed scenario except for player death.
	bool					m_ScenarioCompleted;	// Serialized; the scenario has been failed or succeeded
	bool					m_ScenarioSucceeded;	// Serialized; the scenario has been succeeded (i.e., Failed == Completed && !Succeeded)
	TEventUID				m_QueueResultsEventUID;	// Serialized; so we don't double up on requests to queue the results screen

	// CAMTODO: Revisit this later for new mission system (is the campaign even the proper place to track this sort of thing?)
	uint					m_ArtifactsTarget;		// Serialized; how many artifacts we need to resolve for current mission
	uint					m_ArtifactsResolved;	// Serialized; redundant with world state, be sure to keep in sync
	uint					m_ArtifactsCapacity;	// Serialized; redundant with world state, be sure to keep in sync

	// CAMTODO: Revisit these (and others from Vamp), the concept of turns WILL apply now.
	Array<WBAction*>		m_ResetActions;				// Actions which are performed when the campaign is reset
	Array<WBAction*>		m_TurnActions;				// Actions which are performed on any turn (start, next episode, next season, or next generation)
	Array<WBAction*>		m_TurnStartActions;			// Actions which are performed on the first turn (i.e., for a new game, before the world is generated)
	Array<WBAction*>		m_PostGenStartActions;		// Actions which are performed on the postgen first turn (i.e., for a new game, after the world is generated)

	// Helper functions
	RosaGame*				GetGame() const;

	HashedString			GetCampaignStatus() const;

	uint					GetGeoGridIndex( const uint GeoGridX, const uint GeoGridY ) const	{ return ( GeoGridY * m_GeoGridsW ) + GeoGridX; }
	uint					GetGeoGridX( const uint GeoGridIndex ) const						{ return GeoGridIndex % m_GeoGridsW; }
	uint					GetGeoGridY( const uint GeoGridIndex ) const						{ return GeoGridIndex / m_GeoGridsW; }

	// CAMTODO: Are these accessors needed? Can I rename them for clarity that they're the selected geo grid?
	SGeoGrid&				GetGeoGrid()			{ return m_GeoGrids[ m_SelectedGeoGridIndex ]; }
	const SGeoGrid&			GetGeoGrid() const		{ return m_GeoGrids[ m_SelectedGeoGridIndex ]; }
	SRegionType&			GetRegionType()			{ return m_RegionTypes[ GetGeoGrid().m_RegionTypeIndex ]; }
	const SRegionType&		GetRegionType() const	{ return m_RegionTypes[ GetGeoGrid().m_RegionTypeIndex ]; }
	SMissionType&			GetMissionType()		{ return m_MissionTypes[ GetGeoGrid().m_MissionTypeIndex ]; }
	const SMissionType&		GetMissionType() const	{ return m_MissionTypes[ GetGeoGrid().m_MissionTypeIndex ]; }

	void					PublishToHUD();

	bool					IsInHub() const;	// Also includes dream sequence or other hub-like levels (i.e., is not a mission)

	void					OnPostWorldGen();
	void					CheckWinLoseConditions();

	// Artifacts are things used for mission objectives: enemies, hostages, relics, etc.
	// Because these entities may be destroyed during play, we should not iterate over
	// the entities in the world to count these except at mission start. Anything that
	// may spawn other artifacts should consider this in its initial capacity.
	uint					GetArtifactCapacity() const;

	void					QueueResults( const float Delay );
	void					FailScenario( const bool AllowContinue );
	void					SucceedScenario();

	uint					GetRegionTypeIndex( const HashedString& Name ) const;
	uint					GetMissionTypeIndex( const HashedString& Name ) const;

	const SGeoGrid&			GetGeoGrid( const uint GeoGridIndex ) const				{ return m_GeoGrids[ GeoGridIndex ]; }
	SGeoGrid&				GetGeoGrid( const uint GeoGridIndex )					{ return m_GeoGrids[ GeoGridIndex ]; }
	const SGeoGrid*			SafeGetGeoGrid( const uint GeoGridIndex ) const			{ return m_GeoGrids.IsValidIndex( GeoGridIndex ) ? &m_GeoGrids[ GeoGridIndex ] : NULL; }
	SGeoGrid*				SafeGetGeoGrid( const uint GeoGridIndex )				{ return m_GeoGrids.IsValidIndex( GeoGridIndex ) ? &m_GeoGrids[ GeoGridIndex ] : NULL; }

	const SRegionType&		GetRegionType( const uint RegionTypeIndex ) const		{ return m_RegionTypes[ RegionTypeIndex ]; }
	SRegionType&			GetRegionType( const uint RegionTypeIndex )				{ return m_RegionTypes[ RegionTypeIndex ]; }
	const SRegionType*		SafeGetRegionType( const uint RegionTypeIndex ) const	{ return m_RegionTypes.IsValidIndex( RegionTypeIndex ) ? &m_RegionTypes[ RegionTypeIndex ] : NULL; }
	SRegionType*			SafeGetRegionType( const uint RegionTypeIndex )			{ return m_RegionTypes.IsValidIndex( RegionTypeIndex ) ? &m_RegionTypes[ RegionTypeIndex ] : NULL; }

	const SMissionType&		GetMissionType( const uint MissionTypeIndex ) const		{ return m_MissionTypes[ MissionTypeIndex ]; }
	SMissionType&			GetMissionType( const uint MissionTypeIndex )			{ return m_MissionTypes[ MissionTypeIndex ]; }
	const SMissionType*		SafeGetMissionType( const uint MissionTypeIndex ) const	{ return m_MissionTypes.IsValidIndex( MissionTypeIndex ) ? &m_MissionTypes[ MissionTypeIndex ] : NULL; }
	SMissionType*			SafeGetMissionType( const uint MissionTypeIndex )		{ return m_MissionTypes.IsValidIndex( MissionTypeIndex ) ? &m_MissionTypes[ MissionTypeIndex ] : NULL; }
};

#endif // ROSACAMPAIGN_H
