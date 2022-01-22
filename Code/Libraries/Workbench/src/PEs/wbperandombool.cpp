#include "core.h"
#include "wbperandombool.h"
#include "configmanager.h"
#include "mathfunc.h"

WBPERandomBool::WBPERandomBool()
:	m_Probability( 0.0f )
,	m_ProbabilityPE()
{
}

WBPERandomBool::~WBPERandomBool()
{
}

void WBPERandomBool::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Probability );
	m_Probability = ConfigManager::GetFloat( sProbability, 0.0f, sDefinitionName );

	STATICHASH( ProbabilityPE );
	const SimpleString ProbabilityPEDef = ConfigManager::GetString( sProbabilityPE, "", sDefinitionName );
	m_ProbabilityPE.InitializeFromDefinition( ProbabilityPEDef );
}

void WBPERandomBool::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	m_ProbabilityPE.Evaluate( Context );
	const float Probability = ( m_ProbabilityPE.HasRoot() ) ? m_ProbabilityPE.GetFloat() : m_Probability;

	EvaluatedParam.m_Type = WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool = Math::RandomF( Probability );
}
