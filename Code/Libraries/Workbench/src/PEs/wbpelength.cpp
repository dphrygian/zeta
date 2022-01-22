#include "core.h"
#include "wbpelength.h"
#include "mathcore.h"

WBPELength::WBPELength()
{
}

WBPELength::~WBPELength()
{
}

void WBPELength::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_Input->Evaluate( Context, Value );

	DEVASSERT( Value.m_Type == WBParamEvaluator::EPT_Vector );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
	EvaluatedParam.m_Float	= Value.GetVector().Length();
}
