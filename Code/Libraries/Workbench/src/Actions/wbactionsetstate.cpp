#include "core.h"
#include "wbactionsetstate.h"
#include "configmanager.h"
#include "Components/wbcompstate.h"
#include "wbparamevaluatorfactory.h"

WBActionSetState::WBActionSetState()
:	m_EntityPE()
,	m_State()
{
}

WBActionSetState::~WBActionSetState()
{
}

/*virtual*/ void WBActionSetState::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( State );
	m_State = ConfigManager::GetHash( sState, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionSetState::Execute()
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

	WBCompState* const pState = WB_GETCOMP( pEntity, State );
	if( !pState )
	{
		return;
	}

	pState->SetState( m_State );
}
