#include "core.h"
#include "wbactionrosaaddmoney.h"
#include "Components/wbcomprosawallet.h"
#include "configmanager.h"
#include "rosagame.h"

WBActionRosaAddMoney::WBActionRosaAddMoney()
:	m_Amount( 0 )
,	m_AmountPE()
,	m_ShowLogMessage( false )
{
}

WBActionRosaAddMoney::~WBActionRosaAddMoney()
{
}

/*virtual*/ void WBActionRosaAddMoney::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Amount );
	m_Amount = ConfigManager::GetInt( sAmount, false, sDefinitionName );

	STATICHASH( AmountPE );
	const SimpleString AmountPEDef = ConfigManager::GetString( sAmountPE, "", sDefinitionName );
	m_AmountPE.InitializeFromDefinition( AmountPEDef );

	STATICHASH( ShowLogMessage );
	m_ShowLogMessage = ConfigManager::GetBool( sShowLogMessage, true, sDefinitionName );
}

/*virtual*/ void WBActionRosaAddMoney::Execute()
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

	pWallet->AddMoney( Amount, m_ShowLogMessage );
}
