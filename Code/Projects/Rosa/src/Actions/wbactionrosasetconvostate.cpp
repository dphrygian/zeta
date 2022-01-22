#include "core.h"
#include "wbactionrosasetconvostate.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "Components/wbcompvariablemap.h"

WBActionRosaSetConvoState::WBActionRosaSetConvoState()
:	m_EntityPE()
,	m_State()
{
}

WBActionRosaSetConvoState::~WBActionRosaSetConvoState()
{
}

/*virtual*/ void WBActionRosaSetConvoState::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( State );
	m_State = ConfigManager::GetHash( sState, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaSetConvoState::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_EntityPE.Evaluate( PEContext );

	WBEntity* const pEntity = m_EntityPE.GetEntity();
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
	Variables.SetHash( sConvoState, m_State );
}
