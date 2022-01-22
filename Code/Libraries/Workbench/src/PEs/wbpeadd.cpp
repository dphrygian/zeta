#include "core.h"
#include "wbpeadd.h"

WBPEAdd::WBPEAdd()
{
}

WBPEAdd::~WBPEAdd()
{
}

void WBPEAdd::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam ValueA;
	m_InputA->Evaluate( Context, ValueA );

	WBParamEvaluator::SEvaluatedParam ValueB;
	m_InputB->Evaluate( Context, ValueB );

	if( ValueA.m_Type == WBParamEvaluator::EPT_String && ValueB.m_Type == WBParamEvaluator::EPT_String )
	{
		// Special case: String concatenation!
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
		EvaluatedParam.m_String	= ValueA.m_String + ValueB.m_String;
	}
	else if( ValueA.m_Type == WBParamEvaluator::EPT_Vector && ValueB.m_Type == WBParamEvaluator::EPT_Vector )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
		EvaluatedParam.m_Vector	= ValueA.m_Vector + ValueB.m_Vector;
	}
	else if( WBParamEvaluator::IsIntOp( ValueA, ValueB ) )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Int;
		EvaluatedParam.m_Int	= ValueA.m_Int + ValueB.m_Int;
	}
	else
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
		EvaluatedParam.m_Float	= ValueA.GetFloat() + ValueB.GetFloat();
	}
}
