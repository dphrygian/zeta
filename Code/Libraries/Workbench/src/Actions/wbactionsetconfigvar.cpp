#include "core.h"
#include "wbactionsetconfigvar.h"
#include "configmanager.h"

WBActionSetConfigVar::WBActionSetConfigVar()
:	m_VarContext()
,	m_VarName()
,	m_ValuePE()
{
}

WBActionSetConfigVar::~WBActionSetConfigVar()
{
}

/*virtual*/ void WBActionSetConfigVar::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( VarContext );
	m_VarContext = ConfigManager::GetHash( sVarContext, HashedString::NullString, sDefinitionName );

	STATICHASH( VarName );
	m_VarName = ConfigManager::GetHash( sVarName, HashedString::NullString, sDefinitionName );

	STATICHASH( ValuePE );
	m_ValuePE.InitializeFromDefinition( ConfigManager::GetString( sValuePE, "", sDefinitionName ) );
}

/*virtual*/ void WBActionSetConfigVar::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext Context;
	Context.m_Entity = GetEntity();
	m_ValuePE.Evaluate( Context );

	if( m_ValuePE.GetType() == WBParamEvaluator::EPT_Bool )
	{
		ConfigManager::SetBool( m_VarName, m_ValuePE.GetBool(), m_VarContext );
	}
	else if( m_ValuePE.GetType() == WBParamEvaluator::EPT_Int )
	{
		ConfigManager::SetInt( m_VarName, m_ValuePE.GetInt(), m_VarContext );
	}
	else if( m_ValuePE.GetType() == WBParamEvaluator::EPT_Float )
	{
		ConfigManager::SetFloat( m_VarName, m_ValuePE.GetFloat(), m_VarContext );
	}
	else if( m_ValuePE.GetType() == WBParamEvaluator::EPT_String )
	{
		ConfigManager::SetString( m_VarName, m_ValuePE.GetString().CStr(), m_VarContext );
	}
}
