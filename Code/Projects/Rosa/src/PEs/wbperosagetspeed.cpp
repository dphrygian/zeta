#include "core.h"
#include "wbperosagetspeed.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "Components/wbcomprosatransform.h"

WBPERosaGetSpeed::WBPERosaGetSpeed()
:	m_EntityPE( NULL )
{
}

WBPERosaGetSpeed::~WBPERosaGetSpeed()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetSpeed::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntity, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaGetSpeed::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_EntityPE->Evaluate( Context, Value );

	WBEntity* const pEntity = Value.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
	if( !pTransform )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
	EvaluatedParam.m_Float	= pTransform->GetSpeed();
}
