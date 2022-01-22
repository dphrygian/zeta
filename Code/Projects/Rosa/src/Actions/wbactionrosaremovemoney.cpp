#include "core.h"
#include "wbactionrosaremovemoney.h"
#include "Components/wbcomprosawallet.h"
#include "configmanager.h"
#include "rosagame.h"

WBActionRosaRemoveMoney::WBActionRosaRemoveMoney()
:	m_Amount( 0 )
,	m_AmountPE()
{
}

WBActionRosaRemoveMoney::~WBActionRosaRemoveMoney()
{
}

/*virtual*/ void WBActionRosaRemoveMoney::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Amount );
	m_Amount = ConfigManager::GetInt( sAmount, false, sDefinitionName );

	STATICHASH( AmountPE );
	const SimpleString AmountPEDef = ConfigManager::GetString( sAmountPE, "", sDefinitionName );
	m_AmountPE.InitializeFromDefinition( AmountPEDef );
}

/*virtual*/ void WBActionRosaRemoveMoney::Execute()
{
	WBAction::Execute();

	WBEntity* const			pPlayer	= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaWallet* const	pWallet	= WB_GETCOMP( pPlayer, RosaWallet );
	DEVASSERT( pWallet );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity				= pPlayer;

	m_AmountPE.Evaluate( PEContext );
	const uint				Amount	= m_AmountPE.HasRoot() ? m_AmountPE.GetInt() : m_Amount;

	const bool ShowLogMessage = true;
	pWallet->RemoveMoney( Amount, ShowLogMessage );
}
