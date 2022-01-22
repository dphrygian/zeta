#include "core.h"
#include "wbactionrosasetpersistentvar.h"
#include "configmanager.h"
#include "wbeventmanager.h"

WBActionRosaSetPersistentVar::WBActionRosaSetPersistentVar()
:	m_Key()
,	m_ValuePE()
{
}

WBActionRosaSetPersistentVar::~WBActionRosaSetPersistentVar()
{
}

/*virtual*/ void WBActionRosaSetPersistentVar::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Key );
	m_Key = ConfigManager::GetHash( sKey, HashedString::NullString, sDefinitionName );

	STATICHASH( ValuePE );
	const SimpleString ValuePEDef = ConfigManager::GetString( sValuePE, "", sDefinitionName );
	m_ValuePE.InitializeFromDefinition( ValuePEDef );
}

/*virtual*/ void WBActionRosaSetPersistentVar::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_ValuePE.Evaluate( PEContext );

	WB_MAKE_EVENT( SetPersistentVar, NULL );
	WB_SET_AUTO( SetPersistentVar, Hash, Name, m_Key );
	WB_SET_AUTO( SetPersistentVar, FromPE, Value, m_ValuePE );
	WB_DISPATCH_EVENT( GetEventManager(), SetPersistentVar, NULL );
}
