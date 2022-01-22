#ifndef WBCOMPROSASPAWNMANAGER_H
#define WBCOMPROSASPAWNMANAGER_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "angles.h"
#include "array.h"
#include "wbentityref.h"

class WBCompRosaSpawnManager : public WBRosaComponent
{
public:
	WBCompRosaSpawnManager();
	virtual ~WBCompRosaSpawnManager();

	DEFINE_WBCOMP( RosaSpawnManager, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	struct SSpawnPoint
	{
		SSpawnPoint()
		:	m_Location()
		,	m_Orientation()
		,	m_SpawnTime( 0.0f )
		{
		}

		Vector	m_Location;
		Angles	m_Orientation;
		float	m_SpawnTime;	// The next time this spawn point can be used (not serialized, it really doesn't matter)
	};

	uint			GetTargetPopulation() const;

	void			RegisterSpawnPoint( const Vector& Location, const Angles& Orientation );
	void			RemoveManagedEntity( WBEntity* const pEntity );
	void			SpawnInitialEntities();
	uint			SpawnEntities( const uint NumEntities );
	SimpleString	PickMiniboss() const;
	bool			SpawnMiniboss( const SimpleString& Entity, const bool ManageEntity );
	bool			SpawnNear( const SimpleString& Entity, const Vector& TargetLocation, const float MinSpawnDist, const float MaxSpawnDist, const bool ManageEntity );	// HACKHACK for Ritual mission
	void			SpawnEntityAt( SSpawnPoint& SpawnPoint, const SimpleString& EntityDef, const bool ManageEntity );
	void			SpawnEntityAt( const Vector& SpawnLocation, const Angles& SpawnOrientation, const SimpleString& EntityDef, const bool ManageEntity );
	void			DestroyIrrelevantEntities();
	bool			IsMiniboss( const WBEntity* const pEntity ) const;

	struct SMiniboss
	{
		SMiniboss()
		:	m_Entity()
		,	m_Weight( 0.0f )
		,	m_MinSeason( 0 )
		{
		}

		SimpleString	m_Entity;
		float			m_Weight;
		uint			m_MinSeason;
	};

	SimpleString		m_SpawnEntity;					// Config; main thing to spawn (Minions)
	SimpleString		m_BigBadEntity;					// Config
	Array<SMiniboss>	m_Minibosses;					// Config

	Vector				m_ExtentsMax;					// From config
	float				m_MinSpawnDistSq;				// Config
	float				m_MaxSpawnDistSq;				// Config

	float				m_TargetPopulationLowThreat;	// Config
	float				m_TargetPopulationHighThreat;	// Config
	uint				m_MaxMinibosses;				// Config

	float				m_RelevanceCheckRate;			// Config
	float				m_NextRelevanceCheckTime;		// Serialized (as time remaining)
	float				m_MaxRelevanceDistSq;			// Config

	bool				m_IsSpawning;					// Serialized; is the manager actively spawning new things
	float				m_SpawnRateMin;					// Config
	float				m_SpawnRateMax;					// Config
	float				m_SpawnPointTimeout;			// Config; how soon a specific spawn point can be reused
	float				m_NextSpawnTime;				// Serialized (as time remaining)
	uint				m_SpawnPhase;					// Serialized; how many spawns we've done
	bool				m_CanSpawnBigBad;				// Config/serialized; initialized by campaign as needed, and set false after spawning once
	bool				m_CanSpawnMiniboss;				// Serialized; so we don't spawn multiple minibosses during the phase
	float				m_BigBadScoreThreshold;			// Config; at what point during a mission can a Big Bad be spawned
	uint				m_MinibossSpawnPhase;			// Config; how many spawns must elapse between miniboss spawns
	bool				m_DeferSpawning;				// Config/serialized; for waiting until combat to start spawning
	bool				m_StealthMode;					// Config/serialized; alternative to defer spawning that uses a lower target population
	float				m_StealthPopulationPercent;		// Config; percent of target population to use in stealth mode

	Array<SSpawnPoint>	m_SpawnPoints;					// Transient, registered on initialization
	Array<WBEntityRef>	m_ManagedEntities;				// Serialized
	uint				m_NumManagedMinibosses;			// Serialized
};

#endif // WBCOMPROSASPAWNMANAGER_H
