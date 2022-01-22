#include "core.h"
#include "rosadifficulty.h"
#include "configmanager.h"
#include "rosagame.h"
#include "wbentity.h"
#include "Components/wbcompvariablemap.h"
#include "Components/wbcompstatmod.h"

void RosaDifficulty::Initialize()
{
	SetMenuDifficulty( GetMenuDifficulty() );
}

uint RosaDifficulty::GetMenuDifficulty()
{
	STATICHASH( Difficulty );
	return ConfigManager::GetInt( sDifficulty, 0 );
}

uint RosaDifficulty::GetGameDifficulty()
{
	WBEntity* const				pPlayer		= RosaGame::GetPlayer();
	WBCompVariableMap* const	pVarMap		= WB_GETCOMP( pPlayer, VariableMap );
	const WBEvent&				Variables	= pVarMap->GetVariables();

	STATICHASH( Difficulty );
	return Variables.GetInt( sDifficulty );
}

void RosaDifficulty::SetMenuDifficulty( const uint Difficulty )
{
	STATICHASH( Difficulty );
	ConfigManager::SetInt( sDifficulty, Difficulty );

	// Side effects like setting UI strings
	STATICHASH( HUD );
	STATICHASH( DifficultyDesc );
	ConfigManager::SetString( sDifficulty,		GetDifficultyString( Difficulty ).CStr(),		sHUD );
	ConfigManager::SetString( sDifficultyDesc,	GetDifficultyDescString( Difficulty ).CStr(),	sHUD );
}

void RosaDifficulty::SetGameDifficulty( const uint Difficulty )
{
	WBEntity* const				pPlayer		= RosaGame::GetPlayer();
	WBCompVariableMap* const	pVarMap		= WB_GETCOMP( pPlayer, VariableMap );
	WBEvent&					Variables	= pVarMap->GetVariables();

	STATICHASH( Difficulty );
	Variables.SetInt( sDifficulty, Difficulty );

	// Side effects like activating/deactivating the appropriate stat mods
	WBCompStatMod* const		pStatMod	= WB_GETCOMP( pPlayer, StatMod );

	// Deactivate all difficulty stat mods
	const uint NumDifficulties = GetNumDifficultyModes();
	for( uint DifficultyIndex = 0; DifficultyIndex < NumDifficulties; ++DifficultyIndex )
	{
		const HashedString DifficultyHash = GetDifficultyHash( DifficultyIndex );
		pStatMod->UnTriggerEvent( DifficultyHash );
	}

	// Activate the relevant stat mod
	const HashedString DifficultyHash = GetDifficultyHash( Difficulty );
	pStatMod->TriggerEvent( DifficultyHash );
}

void RosaDifficulty::CycleMenuDifficulty()
{
	const uint NumDifficultyModes		= GetNumDifficultyModes();
	const uint CurrentMenuDifficulty	= GetMenuDifficulty();
	const uint NewMenuDifficulty		= ( CurrentMenuDifficulty + 1 ) % NumDifficultyModes;

	SetMenuDifficulty( NewMenuDifficulty );
}

bool RosaDifficulty::CheckSync()
{
	return GetMenuDifficulty() == GetGameDifficulty();
}

void RosaDifficulty::PushMenuToGame()
{
	SetGameDifficulty( GetMenuDifficulty() );
}

void RosaDifficulty::PushGameToMenu()
{
	SetMenuDifficulty( GetGameDifficulty() );
}

uint RosaDifficulty::GetNumDifficultyModes()
{
	STATICHASH( RosaDifficulty );
	STATICHASH( NumDifficulties );
	return ConfigManager::GetInt( sNumDifficulties, 0, sRosaDifficulty );
}

HashedString RosaDifficulty::GetDifficultyHash( const uint Difficulty )
{
	STATICHASH( RosaDifficulty );
	return ConfigManager::GetSequenceHash( "Difficulty%d", Difficulty, HashedString::NullString, sRosaDifficulty );
}

SimpleString RosaDifficulty::GetDifficultyString( const uint Difficulty )
{
	STATICHASH( RosaDifficulty );
	return ConfigManager::GetSequenceString( "Difficulty%d", Difficulty, "", sRosaDifficulty );
}

SimpleString RosaDifficulty::GetDifficultyDescString( const uint Difficulty )
{
	const SimpleString DifficultyString = GetDifficultyString( Difficulty );
	return SimpleString::PrintF( "%s_Desc", DifficultyString.CStr() );
}
