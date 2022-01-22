#include "core.h"
#include "wbactionrosasetlanguage.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaSetLanguage::WBActionRosaSetLanguage()
:	m_Language()
{
}

WBActionRosaSetLanguage::~WBActionRosaSetLanguage()
{
}

/*virtual*/ void WBActionRosaSetLanguage::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Language );
	m_Language = ConfigManager::GetString( sLanguage, "", sDefinitionName );
}

/*virtual*/ void WBActionRosaSetLanguage::Execute()
{
	WBEventManager* const pEventManager = GetEventManager();

	WBAction::Execute();

	STATICHASH( Language );
	ConfigManager::SetString( sLanguage, m_Language.CStr() );

	WB_MAKE_EVENT( RepublishObjectives, NULL );
	WB_DISPATCH_EVENT( pEventManager, RepublishObjectives, RosaGame::GetPlayer() );

	WB_MAKE_EVENT( ReinitializeUI, NULL );
	WB_DISPATCH_EVENT( pEventManager, ReinitializeUI, NULL );

	WB_MAKE_EVENT( PopUIScreen, NULL );
	WB_DISPATCH_EVENT( pEventManager, PopUIScreen, NULL );
}
