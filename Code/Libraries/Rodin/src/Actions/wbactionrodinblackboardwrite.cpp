#include "core.h"
#include "wbactionrodinblackboardwrite.h"
#include "configmanager.h"
#include "Components/wbcomprodinblackboard.h"
#include "wbactionstack.h"

WBActionRodinBlackboardWrite::WBActionRodinBlackboardWrite()
:	m_BlackboardKey()
,	m_ValuePE()
{
}

WBActionRodinBlackboardWrite::~WBActionRodinBlackboardWrite()
{
}

/*virtual*/ void WBActionRodinBlackboardWrite::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( BlackboardKey );
	m_BlackboardKey = ConfigManager::GetHash( sBlackboardKey, HashedString::NullString, sDefinitionName );

	STATICHASH( ValuePE );
	m_ValuePE.InitializeFromDefinition( ConfigManager::GetString( sValuePE, "", sDefinitionName ) );
}

/*virtual*/ void WBActionRodinBlackboardWrite::Execute()
{
	WBEntity* const					pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRodinBlackboard* const	pBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
	ASSERT( pBlackboard );

	WBParamEvaluator::SPEContext Context;
	Context.m_Entity = pEntity;

	m_ValuePE.Evaluate( Context );
	pBlackboard->Set( m_BlackboardKey, m_ValuePE );
}
