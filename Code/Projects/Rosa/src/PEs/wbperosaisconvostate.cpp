#include "core.h"
#include "wbperosaisconvostate.h"
#include "configmanager.h"
#include "Components/wbcompvariablemap.h"
#include "wbparamevaluatorfactory.h"

WBPERosaIsConvoState::WBPERosaIsConvoState()
:	m_EntityPE()
,	m_State()
{
}

WBPERosaIsConvoState::~WBPERosaIsConvoState()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaIsConvoState::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );

	STATICHASH( State );
	m_State = ConfigManager::GetHash( sState, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPERosaIsConvoState::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompVariableMap* const pVariableMap = WB_GETCOMP( pEntity, VariableMap );
	if( !pVariableMap )
	{
		return;
	}

	STATIC_HASHED_STRING( ConvoState );
	WBEvent& Variables = pVariableMap->GetVariables();
	const HashedString ConvoState = Variables.GetHash( sConvoState );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= ( ConvoState == m_State );
}
