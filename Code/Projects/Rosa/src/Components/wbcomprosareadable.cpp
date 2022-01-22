#include "core.h"
#include "wbcomprosareadable.h"
#include "configmanager.h"
#include "wbentity.h"
#include "wbevent.h"
#include "idatastream.h"
#include "Actions/wbactionrosashowbook.h"

WBCompRosaReadable::WBCompRosaReadable()
:	m_String()
,	m_StringPE()
,	m_IsDynamic( false )
{
}

WBCompRosaReadable::~WBCompRosaReadable()
{
}

/*virtual*/ void WBCompRosaReadable::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( String );
	m_String = ConfigManager::GetInheritedString( sString, "", sDefinitionName );

	STATICHASH( StringPE );
	const SimpleString StringPEDef = ConfigManager::GetInheritedString( sStringPE, "", sDefinitionName );
	m_StringPE.InitializeFromDefinition( StringPEDef );

	STATICHASH( IsDynamic );
	m_IsDynamic = ConfigManager::GetInheritedBool( sIsDynamic, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaReadable::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnFrobbed );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnFrobbed )
	{
		WBParamEvaluator::SPEContext PEContext;
		PEContext.m_Entity = GetEntity();

		m_StringPE.Evaluate( PEContext );
		const SimpleString UseString = ( m_StringPE.GetType() == WBParamEvaluator::EPT_String ) ? m_StringPE.GetString() : m_String;

		const HashedString BookScreenOverride = HashedString::NullString;
		WBActionRosaShowBook::ShowBookScreen( UseString, m_IsDynamic, BookScreenOverride );
	}
}
