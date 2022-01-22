#ifndef CLOCK_H
#define CLOCK_H

#include "list.h"

#if BUILD_SDL
#include "SDL2/SDL.h"
#endif

#if BUILD_WINDOWS_NO_SDL
#define CLOCK_T	__int64
#elif BUILD_SDL
#define CLOCK_T	Uint64
#endif

// Static functions to provide simple non-object oriented timing functions
// Usage:
// CLOCK_T c = 0;
// START_CLOCK( c );
// /* do work here */
// STOP_CLOCK( c );
// float s = GET_CLOCK( c );
// float ms = GET_CLOCK_MS( c );
#define DECLARE_CLOCK( c )		CLOCK_T c = 0
#define RESET_CLOCK( c )		( c ) = 0
#define START_CLOCK( c )		do { ( c ) -= Clock::GetCurrentTimeCounter(); } while( 0 )
#define STOP_CLOCK( c )			do { ( c ) += Clock::GetCurrentTimeCounter(); } while( 0 )
#define GET_CLOCK( c )			( Clock::GetDeltaTimeSeconds( ( c ) ) )
#define GET_CLOCK_MS( c )		( 1000.0f * Clock::GetDeltaTimeSeconds( ( c ) ) )
#define REPORT_CLOCK( c, pf )	do { PRINTF( pf #c ": %.3fs\n", GET_CLOCK( c ) ); } while( 0 )
#define DECLARE_AND_START_CLOCK( c )	\
	CLOCK_T c = 0;						\
	START_CLOCK( c )

#if BUILD_DEV
#define DEV_DECLARE_CLOCK( c )				CLOCK_T c = 0
#define DEV_RESET_CLOCK( c )				( c ) = 0
#define DEV_START_CLOCK( c )				START_CLOCK( c )
#define DEV_STOP_CLOCK( c )					STOP_CLOCK( c )
#define DEV_REPORT_CLOCK( c, pf )			REPORT_CLOCK( c, pf )
#define DEV_DECLARE_AND_START_CLOCK( c )	\
	CLOCK_T c = 0;							\
	START_CLOCK( c )
#define DEV_STOP_AND_REPORT_CLOCK( c, pf )	\
	STOP_CLOCK( c );						\
	REPORT_CLOCK( c, pf )
#else
#define DEV_DECLARE_CLOCK( c )				DoNothing
#define DEV_RESET_CLOCK( c )				DoNothing
#define DEV_START_CLOCK( c )				DoNothing
#define DEV_STOP_CLOCK( c )					DoNothing
#define DEV_REPORT_CLOCK( c, pf )			DoNothing
#define DEV_DECLARE_AND_START_CLOCK( c )	DoNothing
#define DEV_STOP_AND_REPORT_CLOCK( c, pf )	DoNothing
#endif

// Returns the total running time, current time, or delta time
// for either the game (subject to slow/stop effects) or the
// machine (independent of effects), or the physical clock (the
// integral counters, not in seconds).

// This class is not intended to be a singleton; while the engine
// should primarily use a single clock, everything works fine if
// more than one is used (and the profiler depends on it, because
// it uses its own clock).

class SimpleString;

class Clock
{
public:
	struct MultiplierRequest
	{
		float	m_Duration;		// In machine time; <= 0.0 implies infinite request
		float	m_Multiplier;
	};

	Clock();
	~Clock();

	// Clocks are automatically initialized and shut down on construction/destruction;
	// these just exist to allow reinitialization or deletion of memory when needed.
	void	Initialize();
	void	ShutDown();
	void	Tick( bool GamePaused = false, const bool DoGameTick = true );

	// A bit of a hack; my needs for this class have changed a lot in 6 years.
	void	GameTick( const bool GamePaused, const float DeltaTime );

	void	SetGameCurrentTime( const float NewCurrentTime );
	float	GetGameCurrentTime() const;
	float	GetGameDeltaTime() const;

	// HACKHACK for input system; these will diverge from game time if scalars are present
	// (or when loading a saved game, because this will not be set), but will also only tick
	// while the game is unpaused (unlike machine time). Use caution!
	float	GetRealUnpausedCurrentTime() const;
	float	GetRealUnpausedDeltaTime() const;

	float	GetMachineCurrentTime() const;
	float	GetMachineDeltaTime() const;

	CLOCK_T	GetPhysicalCurrentTime() const;
	CLOCK_T	GetPhysicalDeltaTime() const;

	// Used by profiler, could probably be compiled out for Release
	double	GetResolution() const;

	uint	GetTickCount() const;

	void	Report() const;

	MultiplierRequest*	AddMultiplierRequest( const SimpleString& DefinitionName );
	MultiplierRequest*	AddMultiplierRequest( float Duration, float Multiplier );	// Set Duration to 0.0f to make it infinite
	void				RemoveMultiplierRequest( MultiplierRequest** pMultiplierRequest );
	void				ClearMultiplierRequests();
	float				GetMultiplier() const;

	// Used to clamp the game delta time to a reasonable maximum so
	// that hitches don't interfere with the sim and cause chaos.
	// Set to zero to disable the limit.
	void	SetGameDeltaTimeMax( float Limit );

	void	UseFixedMachineDeltaTime( float FixedDeltaTime );
	void	UseActualMachineDeltaTime();

	static CLOCK_T	GetCurrentTimeCounter();
	static float	GetDeltaTimeSeconds( const CLOCK_T& TimeCounter );

private:
	float	CounterToSeconds( const CLOCK_T Counter ) const;
	void	TickMultiplierRequests( const float DeltaTime );

	// NOTE: It might be a bit redundant, but I should probably precompute and store all
	// the various things this class can return, for two reasons: 1) to prevent recomputing
	// things I don't need to, and 2) so there's no way I could possibly get two different
	// results in a given frame (e.g., by changing the multiplier mid-frame).
	CLOCK_T	m_PhysicalBaseTime;
	CLOCK_T	m_PhysicalCurrentTime;
	CLOCK_T	m_PhysicalDeltaTime;

	float	m_MachineCurrentTime;
	float	m_MachineDeltaTime;

	float	m_GameCurrentTime;
	float	m_GameDeltaTime;

	// HACKHACK for input system; these will diverge from game time if scalars are present
	// (or when loading a saved game, because this will not be set), but will also only tick
	// while the game is unpaused (unlike machine time). Use caution!
	float	m_RealUnpausedCurrentTime;
	float	m_RealUnpausedDeltaTime;

	uint	m_TickCount;

	// Used by profiler, could probably be compiled out for Release
	double	m_Resolution;

	List< MultiplierRequest* >	m_MultiplierRequests;
	float	m_GameDeltaTimeMax;

	// Hack to make FRAPS captures smoother
	bool	m_UseFixedMachineDeltaTime;
	float	m_FixedMachineDeltaTime;
};

#endif // CLOCK_H
