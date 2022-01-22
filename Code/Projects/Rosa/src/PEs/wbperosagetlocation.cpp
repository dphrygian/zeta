#include "core.h"
#include "wbperosagetlocation.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "Components/wbcomprosatransform.h"

WBPERosaGetLocation::WBPERosaGetLocation()
:	m_EntityPE( NULL )
{
}

WBPERosaGetLocation::~WBPERosaGetLocation()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetLocation::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntity, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaGetLocation::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
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

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
	EvaluatedParam.m_Vector	= pTransform->GetLocation();
}
