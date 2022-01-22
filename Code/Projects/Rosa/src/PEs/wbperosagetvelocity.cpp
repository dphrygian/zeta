#include "core.h"
#include "wbperosagetvelocity.h"
#include "Components/wbcomprosatransform.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"

WBPERosaGetVelocity::WBPERosaGetVelocity()
:	m_EntityPE( NULL )
{
}

WBPERosaGetVelocity::~WBPERosaGetVelocity()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetVelocity::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntity, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaGetVelocity::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	if( !Context.m_Entity )
	{
		return;
	}

	WBParamEvaluator::SEvaluatedParam Value;
	m_EntityPE->Evaluate( Context, Value );

	ASSERT( Value.m_Type == WBParamEvaluator::EPT_Entity );

	WBEntity* const pEntity = Value.m_Entity.Get();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
	if( !pTransform )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
	EvaluatedParam.m_Vector	= pTransform->GetVelocity();
}
