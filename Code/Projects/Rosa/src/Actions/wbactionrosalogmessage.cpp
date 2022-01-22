#include "core.h"
#include "wbactionrosalogmessage.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "wbeventmanager.h"
#include "rosagame.h"
#include "rosahudlog.h"
#include "stringmanager.h"

WBActionRosaLogMessage::WBActionRosaLogMessage()
:	m_String()
,	m_StringPE()
,	m_IsDynamic( false )
{
}

WBActionRosaLogMessage::~WBActionRosaLogMessage()
{
}

/*virtual*/ void WBActionRosaLogMessage::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( String );
	m_String = ConfigManager::GetString( sString, "", sDefinitionName );

	STATICHASH( StringPE );
	const SimpleString StringPE = ConfigManager::GetString( sStringPE, "", sDefinitionName );
	m_StringPE.InitializeFromDefinition( StringPE );

	STATICHASH( IsDynamic );
	m_IsDynamic = ConfigManager::GetBool( sIsDynamic, false, sDefinitionName );
}

/*virtual*/ void WBActionRosaLogMessage::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_StringPE.Evaluate( PEContext );
	const SimpleString String = ( m_StringPE.GetType() == WBParamEvaluator::EPT_String ) ? m_StringPE.GetString() : m_String;

	if( m_IsDynamic )
	{
		RosaHUDLog::StaticAddDynamicMessage( String );
	}
	else
	{
		RosaHUDLog::StaticAddMessage( String );
	}
}
