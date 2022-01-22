#include "core.h"
#include "wbpecross.h"

WBPECross::WBPECross()
{
}

WBPECross::~WBPECross()
{
}

void WBPECross::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam ValueA;
	m_InputA->Evaluate( Context, ValueA );

	WBParamEvaluator::SEvaluatedParam ValueB;
	m_InputB->Evaluate( Context, ValueB );

	ASSERT( ValueA.m_Type == WBParamEvaluator::EPT_Vector && ValueB.m_Type == WBParamEvaluator::EPT_Vector );

	if( ValueA.m_Type == WBParamEvaluator::EPT_Vector && ValueB.m_Type == WBParamEvaluator::EPT_Vector )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
		EvaluatedParam.m_Vector	= ValueA.m_Vector.Cross( ValueB.m_Vector );
	}
}
