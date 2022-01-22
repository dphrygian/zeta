#include "core.h"
#include "wbperosaisdifficulty.h"
#include "rosadifficulty.h"
#include "configmanager.h"

WBPERosaIsDifficulty::WBPERosaIsDifficulty()
:	m_RangeLo( 0 )
,	m_RangeHi( 0 )
{
}

WBPERosaIsDifficulty::~WBPERosaIsDifficulty()
{
}

/*virtual*/ void WBPERosaIsDifficulty::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( RangeLo );
	m_RangeLo = ConfigManager::GetInt( sRangeLo, 0, sDefinitionName );

	STATICHASH( RangeHi );
	m_RangeHi = ConfigManager::GetInt( sRangeHi, m_RangeLo, sDefinitionName );

	DEVASSERT( m_RangeHi >= m_RangeLo );
}

/*virtual*/ void WBPERosaIsDifficulty::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	const uint	Difficulty	= RosaDifficulty::GetGameDifficulty();
	const bool	InRange		= ( Difficulty >= m_RangeLo && Difficulty <= m_RangeHi );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= InRange;
}
