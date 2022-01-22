#include "core.h"
#include "wbactionrosabuyammo.h"
#include "Components/wbcomprosaammobag.h"
#include "Components/wbcomprosawallet.h"
#include "Components/wbcomprosainventory.h"
#include "configmanager.h"
#include "rosagame.h"
#include "wbeventmanager.h"

WBActionRosaBuyAmmo::WBActionRosaBuyAmmo()
:	m_AmmoType()
{
}

WBActionRosaBuyAmmo::~WBActionRosaBuyAmmo()
{
}

/*virtual*/ void WBActionRosaBuyAmmo::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AmmoType );
	m_AmmoType = ConfigManager::GetHash( sAmmoType, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaBuyAmmo::Execute()
{
	WBAction::Execute();

	WBEntity* const				pPlayer			= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );
	WBCompRosaAmmoBag* const	pAmmoBag		= WB_GETCOMP( pPlayer, RosaAmmoBag );
	DEVASSERT( pAmmoBag );
	WBCompRosaWallet* const		pWallet			= WB_GETCOMP( pPlayer, RosaWallet );
	DEVASSERT( pWallet );
	WBCompRosaInventory* const	pInventory		= WB_GETCOMP( pPlayer, RosaInventory );
	DEVASSERT( pInventory );

	const SAmmoType* const		pAmmoType		= pAmmoBag->SafeGetAmmoType( m_AmmoType );
	const bool					HasAmmoSpace	= pAmmoType ? ( pAmmoBag->GetAmmoSpace( pAmmoType->m_Hash ) > 0 ) : false;
	const uint					BundlePrice		= HasAmmoSpace ? ( pAmmoType->m_BundleSize * pAmmoType->m_UnitPrice ) : 0;
	const uint					Price			= pAmmoType ? BundlePrice : pInventory->GetAmmoRefillPrice();

	if( Price == 0 )
	{
		// Nothing to buy
		return;
	}

	if( !pWallet->HasMoney( Price ) )
	{
		// Can't afford it! This shouldn't happen because of UI flow, but just to be safe.
		return;
	}

	// All checks passed
	// Pay the cost first
	static const bool sShowLogMessage = true;
	pWallet->RemoveMoney( Price, sShowLogMessage );

	// Then add the bundle amount or refill as needed
	if( pAmmoType )
	{
		pAmmoBag->AddAmmo( m_AmmoType, pAmmoType->m_BundleSize, sShowLogMessage );
	}
	else
	{
		pInventory->RefillAmmo();
	}

	WB_MAKE_EVENT( RefreshWeaponsScreen, NULL );
	WB_DISPATCH_EVENT( GetEventManager(), RefreshWeaponsScreen, NULL );
}
