#include "core.h"
#include "wbpeneg.h"
#include "mathcore.h"

WBPENeg::WBPENeg()
{
}

WBPENeg::~WBPENeg()
{
}

void WBPENeg::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_Input->Evaluate( Context, Value );

	ASSERT(
		Value.m_Type == WBParamEvaluator::EPT_Int ||
		Value.m_Type == WBParamEvaluator::EPT_Float ||
		Value.m_Type == WBParamEvaluator::EPT_Vector );

	if( Value.m_Type == WBParamEvaluator::EPT_Int )
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Int;
		EvaluatedParam.m_Int = -Value.m_Int;
	}
	else if( Value.m_Type == WBParamEvaluator::EPT_Float )
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Float;
		EvaluatedParam.m_Float = -Value.m_Float;
	}
	else
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Vector;
		EvaluatedParam.m_Vector = -Value.GetVector();
	}
}
