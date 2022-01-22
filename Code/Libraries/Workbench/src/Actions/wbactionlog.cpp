#include "core.h"
#include "wbactionlog.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "reversehash.h"
#include "wbevent.h"

WBActionLog::WBActionLog()
:	m_Text( "" )
,	m_TextPE()
,	m_DevOnly( false )
{
}

WBActionLog::~WBActionLog()
{
}

/*virtual*/ void WBActionLog::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Text );
	m_Text = ConfigManager::GetString( sText, "", sDefinitionName );

	STATICHASH( TextPE );
	const SimpleString TextPEDef = ConfigManager::GetString( sTextPE, "", sDefinitionName );
	m_TextPE.InitializeFromDefinition( TextPEDef );

	STATICHASH( DevOnly );
	m_DevOnly = ConfigManager::GetBool( sDevOnly, false, sDefinitionName );
}

/*virtual*/ void WBActionLog::Execute()
{
	WBAction::Execute();

#if BUILD_FINAL
	if( m_DevOnly )
	{
		return;
	}
#endif

	const HashedString EventNameHash	= WBActionStack::TopEvent().GetEventName();
	const SimpleString EventName		= ReverseHash::ReversedHash( EventNameHash );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetEntity();

	m_TextPE.Evaluate( PEContext );
	const SimpleString LogText = ( m_TextPE.GetType() == WBParamEvaluator::EPT_String ) ? m_TextPE.GetString() : m_Text;

	PRINTF( "%s\n", EventName.CStr() );
	PRINTF( "%s\n", LogText.CStr() );
}
