#include "core.h"
#include "wbperosahasmoney.h"
#include "rosagame.h"
#include "Components/wbcomprosawallet.h"
#include "configmanager.h"

WBPERosaHasMoney::WBPERosaHasMoney()
:	m_Amount( 0 )
,	m_AmountPE()
{
}

WBPERosaHasMoney::~WBPERosaHasMoney()
{
}

/*virtual*/ void WBPERosaHasMoney::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Amount );
	m_Amount = ConfigManager::GetInt( sAmount, 0, sDefinitionName );

	STATICHASH( AmountPE );
	const SimpleString AmountPEDef = ConfigManager::GetString( sAmountPE, "", sDefinitionName );
	m_AmountPE.InitializeFromDefinition( AmountPEDef );
}

/*virtual*/ void WBPERosaHasMoney::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	WBEntity* const			pPlayer		= RosaGame::GetPlayer();
	if( !pPlayer )
	{
		return;
	}

	WBCompRosaWallet* const	pWallet	= WB_GETCOMP( pPlayer, RosaWallet );
	if( !pWallet )
	{
		return;
	}

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity				= pPlayer;

	m_AmountPE.Evaluate( PEContext );
	const uint				Amount	= m_AmountPE.HasRoot() ? m_AmountPE.GetInt() : m_Amount;

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pWallet->HasMoney( Amount );
}
