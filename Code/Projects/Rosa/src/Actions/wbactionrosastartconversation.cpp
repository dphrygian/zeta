#include "core.h"
#include "wbactionrosastartconversation.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaStartConversation::WBActionRosaStartConversation()
:	m_Convo()
,	m_ConvoPE()
{
}

WBActionRosaStartConversation::~WBActionRosaStartConversation()
{
}

/*virtual*/ void WBActionRosaStartConversation::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Convo );
	m_Convo = ConfigManager::GetHash( sConvo, HashedString::NullString, sDefinitionName );

	STATICHASH( ConvoPE );
	const SimpleString ConvoPE = ConfigManager::GetString( sConvoPE, "", sDefinitionName );
	m_ConvoPE.InitializeFromDefinition( ConvoPE );
}

/*virtual*/ void WBActionRosaStartConversation::Execute()
{
	WBAction::Execute();

	RosaGame* const		pGame			= RosaFramework::GetInstance()->GetGame();
	ASSERT( pGame );

	WBEventManager* const	pEventManager	= GetEventManager();
	ASSERT( pEventManager );

	WBEntity* const			pEntity			= GetEntity();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;

	m_ConvoPE.Evaluate( PEContext );
	const HashedString Conversation = ( m_ConvoPE.GetType() == WBParamEvaluator::EPT_String ) ? m_ConvoPE.GetString() : m_Convo;

	WB_MAKE_EVENT( StartConversation, NULL );
	WB_LOG_EVENT( StartConversation );
	WB_SET_AUTO( StartConversation, Hash, Conversation, Conversation );
	WB_SET_AUTO( StartConversation, Entity, ConvoTarget, pEntity );
	WB_DISPATCH_EVENT( pEventManager, StartConversation, pGame );
}
