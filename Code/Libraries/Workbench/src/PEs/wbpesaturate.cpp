#include "core.h"
#include "wbpesaturate.h"
#include "mathcore.h"

WBPESaturate::WBPESaturate()
{
}

WBPESaturate::~WBPESaturate()
{
}

void WBPESaturate::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_Input->Evaluate( Context, Value );

	ASSERT( Value.m_Type == WBParamEvaluator::EPT_Float );

	EvaluatedParam.m_Type = WBParamEvaluator::EPT_Float;
	EvaluatedParam.m_Float = Saturate( Value.m_Float );
}
