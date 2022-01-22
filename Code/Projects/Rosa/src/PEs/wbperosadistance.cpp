#include "core.h"
#include "wbperosadistance.h"
#include "Components/wbcomprosatransform.h"

WBPERosaDistance::WBPERosaDistance()
{
}

WBPERosaDistance::~WBPERosaDistance()
{
}

/*virtual*/ void WBPERosaDistance::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam ValueA;
	m_InputA->Evaluate( Context, ValueA );

	WBParamEvaluator::SEvaluatedParam ValueB;
	m_InputB->Evaluate( Context, ValueB );

	ASSERT( ValueA.m_Type == WBParamEvaluator::EPT_Entity );
	ASSERT( ValueB.m_Type == WBParamEvaluator::EPT_Entity );

	if( ValueA.m_Type != WBParamEvaluator::EPT_Entity || ValueB.m_Type != WBParamEvaluator::EPT_Entity )
	{
		return;
	}

	WBEntity* const pEntityA = ValueA.m_Entity.Get();
	WBEntity* const pEntityB = ValueB.m_Entity.Get();

	if( !pEntityA || !pEntityB )
	{
		return;
	}

	WBCompRosaTransform* const pTransformA = pEntityA->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaTransform* const pTransformB = pEntityB->GetTransformComponent<WBCompRosaTransform>();

	ASSERT( pTransformA );
	ASSERT( pTransformB );

	const float Distance = ( pTransformB->GetLocation() - pTransformA->GetLocation() ).Length();

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
	EvaluatedParam.m_Float	= Distance;
}
