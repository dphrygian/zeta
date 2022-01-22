#include "core.h"
#include "wbactionrosaselectconversationchoice.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaSelectConversationChoice::WBActionRosaSelectConversationChoice()
:	m_ChoiceIndex( 0 )
{
}

WBActionRosaSelectConversationChoice::~WBActionRosaSelectConversationChoice()
{
}

/*virtual*/ void WBActionRosaSelectConversationChoice::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( ChoiceIndex );
	m_ChoiceIndex = ConfigManager::GetInt( sChoiceIndex, 0, sDefinitionName );
}

/*virtual*/ void WBActionRosaSelectConversationChoice::Execute()
{
	WBAction::Execute();

	RosaGame* const		pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= GetEventManager();
	ASSERT( pEventManager );

	WB_MAKE_EVENT( SelectConversationChoice, NULL );
	WB_LOG_EVENT( SelectConversationChoice );
	WB_SET_AUTO( SelectConversationChoice, Int, ChoiceIndex, m_ChoiceIndex );
	WB_DISPATCH_EVENT( pEventManager, SelectConversationChoice, pGame );
}
