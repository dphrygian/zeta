#include "core.h"
#include "wbpegetconfigvar.h"
#include "configmanager.h"

WBPEGetConfigVar::WBPEGetConfigVar()
:	m_VarContext()
,	m_VarContextPE()
,	m_VarName()
,	m_VarNamePE()
,	m_DefaultPE()
{
}

WBPEGetConfigVar::~WBPEGetConfigVar()
{
}

/*virtual*/ void WBPEGetConfigVar::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( VarContext );
	m_VarContext = ConfigManager::GetHash( sVarContext, HashedString::NullString, sDefinitionName );

	STATICHASH( VarContextPE );
	const SimpleString VarContextPEDef = ConfigManager::GetString( sVarContextPE, "", sDefinitionName );
	m_VarContextPE.InitializeFromDefinition( VarContextPEDef );

	STATICHASH( VarName );
	m_VarName = ConfigManager::GetHash( sVarName, HashedString::NullString, sDefinitionName );

	STATICHASH( VarNamePE );
	const SimpleString VarNamePEDef = ConfigManager::GetString( sVarNamePE, "", sDefinitionName );
	m_VarNamePE.InitializeFromDefinition( VarNamePEDef );

	STATICHASH( DefaultPE );
	m_DefaultPE.InitializeFromDefinition( ConfigManager::GetString( sDefaultPE, "", sDefinitionName ) );
}

/*virtual*/ void WBPEGetConfigVar::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	m_VarContextPE.Evaluate( Context );
	const HashedString	VarContext	= m_VarContextPE.HasRoot() ? m_VarContextPE.GetString() : m_VarContext;

	m_VarNamePE.Evaluate( Context );
	const HashedString	VarName		= m_VarNamePE.HasRoot() ? m_VarNamePE.GetString() : m_VarName;

	const ConfigVar::EVarType VarType = ConfigManager::GetType( VarName, VarContext );
	if( ConfigVar::EVT_Bool == VarType )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
		EvaluatedParam.m_Bool	= ConfigManager::GetBool(	VarName, false,	VarContext );
	}
	else if( ConfigVar::EVT_Int == VarType )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Int;
		EvaluatedParam.m_Int	= ConfigManager::GetInt(	VarName, 0,		VarContext );
	}
	else if( ConfigVar::EVT_Float == VarType )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
		EvaluatedParam.m_Float	= ConfigManager::GetFloat(	VarName, 0.0f,	VarContext );
	}
	else if( ConfigVar::EVT_String == VarType )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
		EvaluatedParam.m_String	= ConfigManager::GetString(	VarName, NULL,	VarContext );
	}
	else
	{
		// We only need to evaluate the default if the var isn't present.
		// In all other cases, we know we're not using the default, because
		// we know the var is present, because we had a valid type.
		m_DefaultPE.Evaluate( Context );
		EvaluatedParam.Set( m_DefaultPE );
	}
}
