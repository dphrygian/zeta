#include "core.h"
#include "wbpexor.h"

WBPEXOR::WBPEXOR()
{
}

WBPEXOR::~WBPEXOR()
{
}

void WBPEXOR::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam ValueA;
	m_InputA->Evaluate( Context, ValueA );

	WBParamEvaluator::SEvaluatedParam ValueB;
	m_InputB->Evaluate( Context, ValueB );

	if( ValueA.m_Type == WBParamEvaluator::EPT_Int && ValueB.m_Type == WBParamEvaluator::EPT_Int )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Int;
		EvaluatedParam.m_Int	= ValueA.GetInt() ^ ValueB.GetInt();
	}
	else
	{
		const bool BoolA = ValueA.GetBool();
		const bool BoolB = ValueB.GetBool();

		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Bool;
		EvaluatedParam.m_Bool = ( BoolA && !BoolB ) || ( !BoolA && BoolB );
	}
}
