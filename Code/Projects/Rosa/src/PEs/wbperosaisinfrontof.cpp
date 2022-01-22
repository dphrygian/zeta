#include "core.h"
#include "wbperosaisinfrontof.h"
#include "Components/wbcomprosatransform.h"

WBPERosaIsInFrontOf::WBPERosaIsInFrontOf()
{
}

WBPERosaIsInFrontOf::~WBPERosaIsInFrontOf()
{
}

/*virtual*/ void WBPERosaIsInFrontOf::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam ValueA;
	m_InputA->Evaluate( Context, ValueA );

	WBParamEvaluator::SEvaluatedParam ValueB;
	m_InputB->Evaluate( Context, ValueB );

	DEVASSERT( ValueA.m_Type == WBParamEvaluator::EPT_Entity );
	DEVASSERT( ValueB.m_Type == WBParamEvaluator::EPT_Entity );

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

	const Vector	OffsetBToA		= pTransformA->GetLocation() - pTransformB->GetLocation();
	const Vector	DirectionB		= pTransformB->GetOrientation().ToVector();
	const bool		IsAInFrontOfB	= OffsetBToA.Dot( DirectionB ) > 0.0f;

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= IsAInFrontOfB;
}
