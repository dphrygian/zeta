#include "core.h"
#include "wbpeisstate.h"
#include "Components/wbcompstate.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "reversehash.h"

WBPEIsState::WBPEIsState()
:	m_EntityPE( NULL )
,	m_State()
{
}

WBPEIsState::~WBPEIsState()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPEIsState::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );

	STATICHASH( State );
	m_State = ConfigManager::GetHash( sState, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPEIsState::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompState* const pState = WB_GETCOMP( pEntity, State );
	if( !pState )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pState->GetState() == m_State;
}
