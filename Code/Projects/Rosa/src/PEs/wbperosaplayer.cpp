#include "core.h"
#include "wbperosaplayer.h"
#include "rosagame.h"

WBPERosaPlayer::WBPERosaPlayer()
{
}

WBPERosaPlayer::~WBPERosaPlayer()
{
}

/*virtual*/ void WBPERosaPlayer::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	// Warn if we're referencing the player entity before it has been spawned.
	DEVASSERT( RosaGame::GetPlayer() );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Entity;
	EvaluatedParam.m_Entity	= RosaGame::GetPlayer();
}
