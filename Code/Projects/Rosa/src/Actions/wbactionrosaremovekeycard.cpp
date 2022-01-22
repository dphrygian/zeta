#include "core.h"
#include "wbactionrosaremovekeycard.h"
#include "Components/wbcomprosakeyring.h"
#include "configmanager.h"
#include "rosagame.h"

WBActionRosaRemoveKeycard::WBActionRosaRemoveKeycard()
:	m_Keycard()
,	m_KeycardPE()
{
}

WBActionRosaRemoveKeycard::~WBActionRosaRemoveKeycard()
{
}

/*virtual*/ void WBActionRosaRemoveKeycard::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Keycard );
	m_Keycard = ConfigManager::GetHash( sKeycard, HashedString::NullString, sDefinitionName );

	STATICHASH( KeycardPE );
	const SimpleString KeycardPEDef = ConfigManager::GetString( sKeycardPE, "", sDefinitionName );
	m_KeycardPE.InitializeFromDefinition( KeycardPEDef );
}

/*virtual*/ void WBActionRosaRemoveKeycard::Execute()
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

	const bool ShowLogMessage = true;
	pKeyRing->RemoveKeycard( Keycard, ShowLogMessage );
}
