#include "core.h"
#include "wbpegetvariable.h"
#include "configmanager.h"
#include "Components/wbcompvariablemap.h"
#include "wbparamevaluatorfactory.h"

WBPEGetVariable::WBPEGetVariable()
:	m_EntityPE()
,	m_VariableName()
{
}

WBPEGetVariable::~WBPEGetVariable()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPEGetVariable::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );

	STATICHASH( VariableName );
	m_VariableName = ConfigManager::GetHash( sVariableName, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPEGetVariable::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_EntityPE->Evaluate( Context, Value );

	WBEntity* const pEntity = Value.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompVariableMap* const pVariableMap = WB_GETCOMP( pEntity, VariableMap );
	if( !pVariableMap )
	{
		DEVWARNDESC( "GetVariable PE called on an entity with no VariableMap component." );
		return;
	}

	const WBEvent& Variables = pVariableMap->GetVariables();
	EvaluatedParam.Set( Variables.GetParameter( m_VariableName ) );
}
