#include "core.h"
#include "wbpeor.h"

WBPEOR::WBPEOR()
{
}

WBPEOR::~WBPEOR()
{
}

void WBPEOR::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam ValueA;
	m_InputA->Evaluate( Context, ValueA );

	WBParamEvaluator::SEvaluatedParam ValueB;
	m_InputB->Evaluate( Context, ValueB );

	if( ValueA.m_Type == WBParamEvaluator::EPT_Int && ValueB.m_Type == WBParamEvaluator::EPT_Int )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Int;
		EvaluatedParam.m_Int	= ValueA.GetInt() | ValueB.GetInt();
	}
	else
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Bool;
		EvaluatedParam.m_Bool = ValueA.GetBool() || ValueB.GetBool();
	}
}
