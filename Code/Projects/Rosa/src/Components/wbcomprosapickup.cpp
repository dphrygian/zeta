#include "core.h"
#include "wbcomprosapickup.h"
#include "wbcomprosainventory.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "wbcomprosawallet.h"
#include "idatastream.h"

/*static*/ WBCompRosaPickup*	WBCompRosaPickup::sm_PurchasePickup	= NULL;
/*static*/ WBEntity*		WBCompRosaPickup::sm_Purchaser		= NULL;

WBCompRosaPickup::WBCompRosaPickup()
:	m_GiveItemDef()
,	m_Price( 0 )
,	m_FriendlyName()
,	m_FriendlyDesc()
{
	RegisterForPurchaseEvents();
}

WBCompRosaPickup::~WBCompRosaPickup()
{
	UnregisterForPurchaseEvents();
}

/*virtual*/ void WBCompRosaPickup::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( GiveItem );
	m_GiveItemDef = ConfigManager::GetInheritedString( sGiveItem, "", sDefinitionName );

	STATICHASH( Price );
	m_Price = ConfigManager::GetInheritedInt( sPrice, 0, sDefinitionName );

	STATICHASH( FriendlyName );
	m_FriendlyName = ConfigManager::GetInheritedString( sFriendlyName, "", sDefinitionName );

	STATICHASH( FriendlyDesc );
	m_FriendlyDesc = ConfigManager::GetInheritedString( sFriendlyDesc, "", sDefinitionName );
}

/*virtual*/ void WBCompRosaPickup::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnFrobbed );
	STATIC_HASHED_STRING( OnPurchaseBuy );
	STATIC_HASHED_STRING( OnPurchaseSteal );
	STATIC_HASHED_STRING( OnShopkeeperDied );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnShopkeeperDied )
	{
		m_Price = 0;
	}
	else if( EventName == sOnFrobbed )
	{
		STATIC_HASHED_STRING( Frobber );
		WBEntity* const pFrobber = Event.GetEntity( sFrobber );
		ASSERT( pFrobber );

		if( m_Price > 0 )
		{
			SellItemTo( pFrobber );
		}
		else
		{
			GiveItemTo( pFrobber );
		}
	}
	else if( EventName == sOnPurchaseBuy )
	{
		if( this == sm_PurchasePickup )
		{
			ASSERT( sm_Purchaser );
			AcceptPaymentFrom( sm_Purchaser );
			NotifyPurchasedBy( sm_Purchaser );
			GiveItemTo( sm_Purchaser );
		}
	}
	else if( EventName == sOnPurchaseSteal )
	{
		if( this == sm_PurchasePickup )
		{
			ASSERT( sm_Purchaser );
			NotifyTheftBy( sm_Purchaser );
			GiveItemTo( sm_Purchaser );
		}
	}
}

void WBCompRosaPickup::GiveItemTo( WBEntity* const pEntity ) const
{
	// Because Pickup is now unfortunately doubling as a Purchaseable.
	// This is such a hack.
	// TODO: Make a separate Purchaseable component.
	if( m_GiveItemDef == "" )
	{
		// OnObtained is fired whether purchased, stolen, or given
		WB_MAKE_EVENT( OnObtained, GetEntity() );
		WB_SET_AUTO( OnObtained, Entity, Buyer, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnObtained, GetEntity() );

		return;
	}

	DEVASSERT( pEntity );

	WBCompRosaInventory* const	pInventory		= WB_GETCOMP( pEntity, RosaInventory );
	ASSERT( pInventory );

	WBEntity* const				pGivenEntity	= WBWorld::GetInstance()->CreateEntity( m_GiveItemDef );
	ASSERT( pGivenEntity );

	const bool ShowLogMessage = true;
	pInventory->AddItem( pGivenEntity, ShowLogMessage );

	WB_MAKE_EVENT( OnItemGiven, GetEntity() );
	WB_SET_AUTO( OnItemGiven, Entity, GivenTo, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnItemGiven, GetEntity() );

	GetEntity()->Destroy();
}

void WBCompRosaPickup::SellItemTo( WBEntity* const pEntity )
{
	DEVASSERT( pEntity );

	sm_PurchasePickup	= this;
	sm_Purchaser		= pEntity;

	WBCompRosaWallet* const pWallet = WB_GETCOMP( pEntity, RosaWallet );
	ASSERT( pWallet );

	const bool CanAfford = ( pWallet->GetMoney() >= m_Price );

	STATICHASH( Purchase );

	STATICHASH( NameTag );
	ConfigManager::SetString( sNameTag, m_FriendlyName.CStr(), sPurchase );

	STATICHASH( ItemDesc );
	ConfigManager::SetString( sItemDesc, m_FriendlyDesc.CStr(), sPurchase );

	STATICHASH( PriceTag );
	ConfigManager::SetInt( sPriceTag, m_Price, sPurchase );

	STATIC_HASHED_STRING( PurchaseScreen );
	STATIC_HASHED_STRING( PurchaseBuyButton );

	// Disable the buy button if the price is too high
	{
		WB_MAKE_EVENT( SetWidgetDisabled, GetEntity() );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sPurchaseScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sPurchaseBuyButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, !CanAfford );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetDisabled, NULL );
	}

	// Show the purchase screen
	{
		WB_MAKE_EVENT( PushUIScreen, GetEntity() );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sPurchaseScreen );
		WB_DISPATCH_EVENT( GetEventManager(), PushUIScreen, NULL );
	}
}

void WBCompRosaPickup::AcceptPaymentFrom( WBEntity* const pEntity ) const
{
	DEVASSERT( pEntity );

	WBCompRosaWallet* const pWallet = WB_GETCOMP( pEntity, RosaWallet );
	ASSERT( pWallet );

	ASSERT( pWallet->GetMoney() >= m_Price );

	const bool ShowLogMessage = true;
	pWallet->RemoveMoney( m_Price, ShowLogMessage );
}

void WBCompRosaPickup::NotifyPurchasedBy( WBEntity* const pEntity ) const
{
	DEVASSERT( pEntity );

	{
		WB_MAKE_EVENT( OnPurchased, GetEntity() );
		WB_SET_AUTO( OnPurchased, Entity, Buyer, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnPurchased, GetEntity() );
	}

	{
		// OnObtained is fired whether purchased, stolen, or given
		WB_MAKE_EVENT( OnObtained, GetEntity() );
		WB_SET_AUTO( OnObtained, Entity, Buyer, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnObtained, GetEntity() );
	}
}

void WBCompRosaPickup::NotifyTheftBy( WBEntity* const pEntity ) const
{
	DEVASSERT( pEntity );

	{
		WB_MAKE_EVENT( OnTheft, GetEntity() );
		WB_SET_AUTO( OnTheft, Entity, Thief, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnTheft, GetEntity() );
	}

	{
		// OnObtained is fired whether purchased, stolen, or given
		WB_MAKE_EVENT( OnObtained, GetEntity() );
		WB_SET_AUTO( OnObtained, Entity, Buyer, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), OnObtained, GetEntity() );
	}
}

void WBCompRosaPickup::RegisterForPurchaseEvents()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnPurchaseBuy );
		pEventManager->AddObserver( sOnPurchaseBuy, this, NULL );

		STATIC_HASHED_STRING( OnPurchaseSteal );
		pEventManager->AddObserver( sOnPurchaseSteal, this, NULL );

		STATIC_HASHED_STRING( OnShopkeeperDied );
		pEventManager->AddObserver( sOnShopkeeperDied, this, NULL );
	}
}

void WBCompRosaPickup::UnregisterForPurchaseEvents()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnPurchaseBuy );
		pEventManager->RemoveObserver( sOnPurchaseBuy, this, NULL );

		STATIC_HASHED_STRING( OnPurchaseSteal );
		pEventManager->RemoveObserver( sOnPurchaseSteal, this, NULL );

		STATIC_HASHED_STRING( OnShopkeeperDied );
		pEventManager->RemoveObserver( sOnShopkeeperDied, this, NULL );
	}
}

#define VERSION_EMPTY	0
#define VERSION_PRICE	1
#define VERSION_CURRENT	1

uint WBCompRosaPickup::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 4;	// m_Price

	return Size;
}

void WBCompRosaPickup::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_Price );
}

void WBCompRosaPickup::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_PRICE )
	{
		m_Price = Stream.ReadUInt32();
	}
}
