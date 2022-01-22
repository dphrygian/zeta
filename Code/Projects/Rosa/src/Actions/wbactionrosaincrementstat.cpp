#include "core.h"
#include "wbactionrosaincrementstat.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "Achievements/iachievementmanager.h"

WBActionRosaIncrementStat::WBActionRosaIncrementStat()
:	m_StatTag()
,	m_Amount( 0 )
,	m_AmountPE()
{
}

WBActionRosaIncrementStat::~WBActionRosaIncrementStat()
{
}

/*virtual*/ void WBActionRosaIncrementStat::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( StatTag );
	m_StatTag = ConfigManager::GetString( sStatTag, "", sDefinitionName );

	STATICHASH( Amount );
	m_Amount = ConfigManager::GetInt( sAmount, 1, sDefinitionName );

	STATICHASH( AmountPE );
	const SimpleString AmountPEDef = ConfigManager::GetString( sAmountPE, "", sDefinitionName );
	m_AmountPE.InitializeFromDefinition( AmountPEDef );
}

// ROSATODO: Do some validation so users can't easily make mods to increment stats.
/*virtual*/ void WBActionRosaIncrementStat::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_AmountPE.Evaluate( PEContext );
	const uint Amount = m_AmountPE.HasRoot() ? m_AmountPE.GetInt() : m_Amount;

	Unused( Amount );
	INCREMENT_STAT( m_StatTag, Amount );
}
