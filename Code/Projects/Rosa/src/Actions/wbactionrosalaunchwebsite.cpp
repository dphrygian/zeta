#include "core.h"
#include "wbactionrosalaunchwebsite.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "wbeventmanager.h"

WBActionRosaLaunchWebSite::WBActionRosaLaunchWebSite()
:	m_URL()
{
}

WBActionRosaLaunchWebSite::~WBActionRosaLaunchWebSite()
{
}

/*virtual*/ void WBActionRosaLaunchWebSite::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( URL );
	m_URL = ConfigManager::GetString( sURL, "", sDefinitionName );
}

/*virtual*/ void WBActionRosaLaunchWebSite::Execute()
{
	WBAction::Execute();

	RosaGame* const			pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( LaunchWebSite, NULL );
	WB_LOG_EVENT( LaunchWebSite );
	WB_SET_AUTO( LaunchWebSite, Hash, URL, m_URL );
	WB_DISPATCH_EVENT( pEventManager, LaunchWebSite, pGame );
}
