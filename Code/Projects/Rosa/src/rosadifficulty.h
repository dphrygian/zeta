#ifndef ROSADIFFICULTY_H
#define ROSADIFFICULTY_H

// Difficulty is a tricky system because it is a UI option that
// is also mirrored and serialized in game state, and neither
// version is always authoritative.

// I refer to these as "menu difficulty" and "game difficulty".
// Menu difficulty is a config var.
// Game difficulty is a (serialized) varmap variable, and depends
// on having a player entity with a VariableMap component.

class SimpleString;

namespace RosaDifficulty
{
	void	Initialize();

	uint	GetMenuDifficulty();
	uint	GetGameDifficulty();

	void	SetMenuDifficulty( const uint Difficulty );
	void	SetGameDifficulty( const uint Difficulty );

	void	CycleMenuDifficulty();

	// Ensure that menu and game difficulty are in sync.
	bool	CheckSync();

	void	PushMenuToGame();
	void	PushGameToMenu();

	// Accessors for strings and stuff
	uint			GetNumDifficultyModes();
	HashedString	GetDifficultyHash( const uint Difficulty );
	SimpleString	GetDifficultyString( const uint Difficulty );		// "Tourist"
	SimpleString	GetDifficultyDescString( const uint Difficulty );	// "Enemies will ignore you."
}

#endif // ROSADIFFICULTY_H
