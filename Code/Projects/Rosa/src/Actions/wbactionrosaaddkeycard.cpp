#include "core.h"
#include "wbactionrosaaddkeycard.h"
#include "Components/wbcomprosakeyring.h"
#include "configmanager.h"
#include "rosagame.h"

WBActionRosaAddKeycard::WBActionRosaAddKeycard()
:	m_Keycard()
,	m_KeycardPE()
,	m_SuppressLog( false )
{
}

WBActionRosaAddKeycard::~WBActionRosaAddKeycard()
{
}

/*virtual*/ void WBActionRosaAddKeycard::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Keycard );
	m_Keycard = ConfigManager::GetHash( sKeycard, HashedString::NullString, sDefinitionName );

	STATICHASH( KeycardPE );
	const SimpleString KeycardPEDef = ConfigManager::GetString( sKeycardPE, "", sDefinitionName );
	m_KeycardPE.InitializeFromDefinition( KeycardPEDef );

	STATICHASH( SuppressLog );
	m_SuppressLog = ConfigManager::GetBool( sSuppressLog, false, sDefinitionName );
}

/*virtual*/ void WBActionRosaAddKeycard::Execute()
{
	WBAction::Execute();

	WBEntity* const			pPlayer		= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaKeyRing* const	pKeyRing	= WB_GETCOMP( pPlayer, RosaKeyRing );
	DEVASSERT( pKeyRing );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity					= pPlayer;

	m_KeycardPE.Evaluate( PEContext );
	const HashedString		Keycard		= m_KeycardPE.HasRoot() ? m_KeycardPE.GetString() : m_Keycard;

	const bool ShowLogMessage = !m_SuppressLog;
	pKeyRing->AddKeycard( Keycard, ShowLogMessage );
}
