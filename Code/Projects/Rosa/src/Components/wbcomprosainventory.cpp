#include "core.h"
#include "wbcomprosainventory.h"
#include "wbcomprosaitem.h"
#include "wbcomprosaweapon.h"
#include "wbcomprosaammobag.h"
#include "Components/wbcompstatmod.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "rosagame.h"
#include "rosahudlog.h"
#include "mathcore.h"

WBCompRosaInventory::WBCompRosaInventory()
:	m_InventoryMap()
,	m_InitialItems()
,	m_CycleSlots()
,	m_CycleIndex( 0 )
,	m_AvailableCycleSlots( 0 )
{
}

WBCompRosaInventory::~WBCompRosaInventory()
{
}

/*virtual*/ void WBCompRosaInventory::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	// Add local initial items first
	AddInitialItemSet( DefinitionName );

	// Optionally add items from other sets (or other inventory components, ignoring their other properties)
	STATICHASH( NumInitialItemSets );
	const uint NumInitialItemSets = ConfigManager::GetInheritedInt( sNumInitialItemSets, 0, sDefinitionName );
	FOR_EACH_INDEX( InitialItemSetIndex, NumInitialItemSets )
	{
		const SimpleString InitialItemSet = ConfigManager::GetInheritedSequenceString( "InitialItemSet%d", InitialItemSetIndex, "", sDefinitionName );
		AddInitialItemSet( InitialItemSet );
	}

	STATICHASH( NumCycleSlots );
	const uint NumCycleSlots = ConfigManager::GetInheritedInt( sNumCycleSlots, 0, sDefinitionName );
	for( uint CycleSlotIndex = 0; CycleSlotIndex < NumCycleSlots; ++CycleSlotIndex )
	{
		const HashedString CycleSlot = ConfigManager::GetInheritedSequenceHash( "CycleSlot%d", CycleSlotIndex, HashedString::NullString, sDefinitionName );
		m_CycleSlots.PushBack( CycleSlot );
	}

	STATICHASH( AvailableCycleSlots );
	m_AvailableCycleSlots = ConfigManager::GetInheritedInt( sAvailableCycleSlots, 0, sDefinitionName );
}

void WBCompRosaInventory::AddInitialItemSet( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumInitialItems );
	const uint NumInitialItems = ConfigManager::GetInheritedInt( sNumInitialItems, 0, sDefinitionName );

	for( uint InitialItemIndex = 0; InitialItemIndex < NumInitialItems; ++InitialItemIndex )
	{
		SInitialItem& InitialItem	= m_InitialItems.PushBack();
		InitialItem.m_ItemDef		= ConfigManager::GetInheritedSequenceString(	"InitialItem%d",		InitialItemIndex,	"",							sDefinitionName );
		InitialItem.m_Slot			= ConfigManager::GetInheritedSequenceHash(		"InitialItem%dSlot",	InitialItemIndex,	HashedString::NullString,	sDefinitionName );
	}
}

/*virtual*/ void WBCompRosaInventory::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( AddItem );
	STATIC_HASHED_STRING( RemoveItem );
	STATIC_HASHED_STRING( SwapItems );
	STATIC_HASHED_STRING( RequestCyclePrev );
	STATIC_HASHED_STRING( RequestCycleNext );
	STATIC_HASHED_STRING( RequestCycleToSlot );
	STATIC_HASHED_STRING( CycleToSlot );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();

	if( EventName == sOnSpawned )
	{
		// Give initial items
		const bool ShowLogMessage = false;
		FOR_EACH_ARRAY( InitialItemIter, m_InitialItems, SInitialItem )
		{
			const SInitialItem& InitialItem = InitialItemIter.GetValue();
			if( InitialItem.m_Slot )
			{
				AddItem( InitialItem.m_ItemDef, InitialItem.m_Slot, ShowLogMessage );
			}
			else
			{
				AddItem( InitialItem.m_ItemDef, ShowLogMessage );
			}
		}
		m_InitialItems.Clear();

		// "Cycle" to the first item so it will be shown.
		WBEntity* const pCycleItem = GetCycleItem();
		if( pCycleItem )
		{
			OnItemCycled( GetCycleItem() );
		}
	}
	else if( EventName == sAddItem )
	{
		STATIC_HASHED_STRING( ItemDef );
		const SimpleString ItemDef = Event.GetString( sItemDef );

		STATIC_HASHED_STRING( Slot );
		const HashedString Slot = Event.GetHash( sSlot );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		if( Slot == HashedString::NullString )
		{
			AddItem( ItemDef, ShowLogMessage );
		}
		else
		{
			AddItem( ItemDef, Slot, ShowLogMessage );
		}
	}
	else if( EventName == sRemoveItem )
	{
		STATIC_HASHED_STRING( Slot );
		const HashedString Slot = Event.GetHash( sSlot );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		RemoveItemInternal( Slot, ShowLogMessage );
	}
	else if( EventName == sSwapItems )
	{
		STATIC_HASHED_STRING( SlotA );
		const HashedString SlotA = Event.GetHash( sSlotA );

		STATIC_HASHED_STRING( SlotB );
		const HashedString SlotB = Event.GetHash( sSlotB );

		SwapItems( SlotA, SlotB );
	}
	else if( EventName == sRequestCyclePrev || EventName == sRequestCycleNext )
	{
		const bool CycleNext = ( EventName == sRequestCycleNext );
		RequestCycle( CycleNext );
	}
	else if( EventName == sRequestCycleToSlot )
	{
		STATIC_HASHED_STRING( Slot );
		const HashedString Slot = Event.GetHash( sSlot );

		RequestCycle( Slot );
	}
	else if( EventName == sCycleToSlot )
	{
		STATIC_HASHED_STRING( Slot );
		const HashedString Slot = Event.GetHash( sSlot );

		Cycle( Slot );
	}
	else if( EventName == sPushPersistence )
	{
		PushPersistence();
	}
	else if( EventName == sPullPersistence )
	{
		PullPersistence();
	}
}

WBEntity* WBCompRosaInventory::GetItem( const HashedString& Slot ) const
{
	TInventoryMap::Iterator InventoryIter = m_InventoryMap.Search( Slot );
	return InventoryIter.IsValid() ? InventoryIter.GetValue().Get() : NULL;
}

WBEntity* WBCompRosaInventory::GetCycleItem() const
{
	return GetItem( GetCycleSlot() );
}

HashedString WBCompRosaInventory::GetBestCycleSlot() const
{
	// Return the first empty cycle slot
	FOR_EACH_ARRAY( CycleSlotIter, m_CycleSlots, HashedString )
	{
		const HashedString& CycleSlot = CycleSlotIter.GetValue();
		if( m_InventoryMap.Search( CycleSlot ).IsNull() )
		{
			return CycleSlot;
		}
	}

	// Else, return the current cycle slot
	return GetCycleSlot();
}

uint WBCompRosaInventory::GetNumCycleItems() const
{
	uint NumCycleItems = 0;

	FOR_EACH_ARRAY( CycleSlotIter, m_CycleSlots, HashedString )
	{
		const HashedString& CycleSlot = CycleSlotIter.GetValue();
		if( m_InventoryMap.Search( CycleSlot ).IsValid() )
		{
			++NumCycleItems;
		}
	}

	DEVASSERT( NumCycleItems <= GetAvailableCycleSlots() );
	return NumCycleItems;
}

uint WBCompRosaInventory::GetAvailableCycleSlots() const
{
	WBCompStatMod* const	pStatMod		= WB_GETCOMP( GetEntity(), StatMod );
	DEVASSERT( pStatMod );

	const float				DefaultLimit	= static_cast<float>( m_AvailableCycleSlots );
	WB_MODIFY_FLOAT( AvailableCycleSlots, DefaultLimit, pStatMod );
	const uint				AvailableCycleSlots	= RoundToUInt( WB_MODDED( AvailableCycleSlots ) );

	return AvailableCycleSlots;
}

void WBCompRosaInventory::GetEquippedAmmoTypes( Array<SAmmoType>& OutAmmoTypes ) const
{
	WBCompRosaAmmoBag* const	pAmmoBag			= WB_GETCOMP( GetEntity(), RosaAmmoBag );
	DEVASSERT( pAmmoBag );

	Array<HashedString>			EquippedAmmoTypes;
	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const			pItem				= ItemIter.GetValue().Get();
		if( !pItem )
		{
			continue;
		}

		WBCompRosaWeapon* const	pWeapon				= WB_GETCOMP( pItem, RosaWeapon );
		if( !pWeapon )
		{
			continue;
		}

		const Array<SMagazine>&	Magazines			= pWeapon->GetMagazines();
		FOR_EACH_ARRAY( MagazineIter, Magazines, SMagazine )
		{
			SMagazine& Magazine = MagazineIter.GetValue();
			DEVASSERT( Magazine.m_AmmoType != HashedString::NullString );

			// OLDVAMP
			//if( pAmmoBag->IsLockedAmmoType( Magazine.m_AmmoType ) )
			//{
			//	continue;
			//}

			EquippedAmmoTypes.PushBackUnique( Magazine.m_AmmoType );
		}
	}

	FOR_EACH_ARRAY( AmmoTypeIter, EquippedAmmoTypes, HashedString )
	{
		const HashedString AmmoType = AmmoTypeIter.GetValue();
		OutAmmoTypes.PushBack( pAmmoBag->GetAmmoType( AmmoType ) );
	}
}

void WBCompRosaInventory::RefillAmmo()
{
	WBCompRosaAmmoBag* const	pAmmoBag			= WB_GETCOMP( GetEntity(), RosaAmmoBag );
	DEVASSERT( pAmmoBag );

	// HACKHACK: Keep a map of what we've refilled in the magazine so we can
	// pass that to the ammo bag and show an accurate count in the log message.
	Map<HashedString, uint>		RefilledAmounts;

	Array<HashedString>			EquippedAmmoTypes;
	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const			pItem				= ItemIter.GetValue().Get();
		if( !pItem )
		{
			continue;
		}

		WBCompRosaWeapon* const	pWeapon				= WB_GETCOMP( pItem, RosaWeapon );
		if( !pWeapon )
		{
			continue;
		}

		const Array<SMagazine>&	Magazines			= pWeapon->GetMagazines();
		FOR_EACH_ARRAY( MagazineIter, Magazines, SMagazine )
		{
			SMagazine& Magazine = MagazineIter.GetValue();
			DEVASSERT( Magazine.m_AmmoType != HashedString::NullString );

			// OLDVAMP
			//if( pAmmoBag->IsLockedAmmoType( Magazine.m_AmmoType ) )
			//{
			//	continue;
			//}

			// Track how much we're adding of this ammo type
			RefilledAmounts[ Magazine.m_AmmoType ] += ( Magazine.m_AmmoMax - Magazine.m_AmmoCount );

			EquippedAmmoTypes.PushBackUnique( Magazine.m_AmmoType );
			Magazine.m_AmmoCount = Magazine.m_AmmoMax;
		}

		// Notify the weapon that it has been refilled
		// (e.g. hack so Vial can reappear in hand)
		WB_MAKE_EVENT( OnAmmoRefilled, pItem );
		WB_DISPATCH_EVENT( GetEventManager(), OnAmmoRefilled, pItem );
	}

	const bool ShowLogMessage = true;
	FOR_EACH_ARRAY( AmmoTypeIter, EquippedAmmoTypes, HashedString )
	{
		const HashedString&	AmmoTypeName	= AmmoTypeIter.GetValue();
		const uint			MagazineRefill	= RefilledAmounts[ AmmoTypeName ];
		pAmmoBag->RefillAmmo( AmmoTypeName, ShowLogMessage, MagazineRefill );
	}
}

uint WBCompRosaInventory::GetAmmoRefillPrice() const
{
	WBCompRosaAmmoBag* const	pAmmoBag			= WB_GETCOMP( GetEntity(), RosaAmmoBag );
	DEVASSERT( pAmmoBag );

	uint						AmmoRefillPrice		= 0;
	Array<HashedString>			EquippedAmmoTypes;

	// Add the cost to refill each magazine
	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const			pItem				= ItemIter.GetValue().Get();
		if( !pItem )
		{
			continue;
		}

		WBCompRosaWeapon* const	pWeapon				= WB_GETCOMP( pItem, RosaWeapon );
		if( !pWeapon )
		{
			continue;
		}

		const Array<SMagazine>&	Magazines			= pWeapon->GetMagazines();
		FOR_EACH_ARRAY( MagazineIter, Magazines, SMagazine )
		{
			const SMagazine& Magazine = MagazineIter.GetValue();
			DEVASSERT( Magazine.m_AmmoType != HashedString::NullString );

			// OLDVAMP
			//if( pAmmoBag->IsLockedAmmoType( Magazine.m_AmmoType ) )
			//{
			//	continue;
			//}

			EquippedAmmoTypes.PushBackUnique( Magazine.m_AmmoType );

			const SAmmoType&	AmmoType			= pAmmoBag->GetAmmoType( Magazine.m_AmmoType );
			const uint			MagazineSpace		= Magazine.m_AmmoMax - Magazine.m_AmmoCount;
			const uint			MagazineRefillPrice	= MagazineSpace * AmmoType.m_UnitPrice;

			AmmoRefillPrice += MagazineRefillPrice;
		}
	}

	FOR_EACH_ARRAY( AmmoTypeIter, EquippedAmmoTypes, HashedString )
	{
		const HashedString&	AmmoTypeName	= AmmoTypeIter.GetValue();
		const SAmmoType&	AmmoType		= pAmmoBag->GetAmmoType( AmmoTypeName );
		const uint			BagSpace		= pAmmoBag->GetAmmoSpace( AmmoTypeName );
		const uint			BagRefillPrice	= BagSpace * AmmoType.m_UnitPrice;

		AmmoRefillPrice += BagRefillPrice;
	}

	return AmmoRefillPrice;
}

WBEntity* WBCompRosaInventory::AddItem( const SimpleString& DefinitionName, const bool ShowLogMessage )
{
	WBEntity* const pItem = WBWorld::GetInstance()->CreateEntity( DefinitionName );
	if( pItem )
	{
		AddItem( pItem, ShowLogMessage );
	}
	return pItem;
}

void WBCompRosaInventory::AddItem( WBEntity* const pItem, const bool ShowLogMessage )
{
	ASSERT( pItem );

	WBCompRosaItem* const pItemComponent = WB_GETCOMP( pItem, RosaItem );
	ASSERT( pItemComponent );

	AddItemInternal( pItemComponent->GetSlot(), pItem, ShowLogMessage );
}

WBEntity* WBCompRosaInventory::AddItem( const SimpleString& DefinitionName, const HashedString& Slot, const bool ShowLogMessage )
{
	WBEntity* const pItem = WBWorld::GetInstance()->CreateEntity( DefinitionName );
	if( pItem )
	{
		AddItem( pItem, Slot, ShowLogMessage );
	}
	return pItem;
}

void WBCompRosaInventory::AddItem( WBEntity* const pItem, const HashedString& Slot, const bool ShowLogMessage )
{
	ASSERT( pItem );

	WBCompRosaItem* const pItemComponent = WB_GETCOMP( pItem, RosaItem );
	ASSERT( pItemComponent );

	pItemComponent->SetSlot( Slot );

	AddItemInternal( Slot, pItem, ShowLogMessage );
}

void WBCompRosaInventory::RemoveItem( WBEntity* const pItem, const bool ShowLogMessage )
{
	if( !pItem )
	{
		return;
	}

	WBCompRosaItem* const pItemComponent = WB_GETCOMP( pItem, RosaItem );
	ASSERT( pItemComponent );

	RemoveItemInternal( pItemComponent->GetSlot(), ShowLogMessage );
}

void WBCompRosaInventory::AddItemInternal( const HashedString& Slot, WBEntity* const pItem, const bool ShowLogMessage )
{
	ASSERT( pItem );

	RemoveItemInternal( Slot, ShowLogMessage );
	m_InventoryMap[ Slot ] = pItem;

	WBEntity* const pEntity = GetEntity();

	// Set owner immediately
	WB_MAKE_EVENT( SetOwner, pItem );
	WB_SET_AUTO( SetOwner, Entity, Owner, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), SetOwner, pItem );

	// Tell ourselves that the item was equipped (do this *before* the item gets notified; fixes some dependencies)
	WB_MAKE_EVENT( OnItemEquipped, pEntity );
	WB_SET_AUTO( OnItemEquipped, Entity, Item, pItem );
	WB_DISPATCH_EVENT( GetEventManager(), OnItemEquipped, pEntity );

	// Tell the item that it was equipped
	WB_MAKE_EVENT( OnEquipped, pItem );
	WB_LOG_EVENT( OnEquipped );
	WB_DISPATCH_EVENT( GetEventManager(), OnEquipped, pItem );

	// If this item was put in a cycle slot, try to cycle to it
	if( IsCycleSlot( Slot ) )
	{
		if( pItem == GetCycleItem() ||
			NULL == GetCycleItem() )
		{
			// Cycle straight to the thing if it's in our current slot or our current slot is empty
			Cycle( Slot );
		}
		else
		{
			// Else, request a cycle through current cycle item
			RequestCycle( Slot );
		}
	}

	if( ShowLogMessage )
	{
		STATICHASH( ItemPickup );
		STATICHASH( Item );
		ConfigManager::SetString( sItem, pItem->GetName().CStr(), sItemPickup );

		RosaHUDLog::StaticAddDynamicMessage( sItemPickup );
	}
}

void WBCompRosaInventory::RemoveItemInternal( const HashedString& Slot, const bool ShowLogMessage )
{
	WBEntity* const pItem = GetItem( Slot );

	m_InventoryMap.Remove( Slot );

	if( !pItem )
	{
		return;
	}

	WBEntity* const pEntity = GetEntity();

	// NOTE: These events are sent before the item is actually unequipped.
	// There's edge cases either way, don't try to change this.

	// Tell the item that it was unequipped
	WB_MAKE_EVENT( OnUnequipped, pItem );
	WB_LOG_EVENT( OnUnequipped );
	WB_SET_AUTO( OnUnequipped, Entity, Owner, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), OnUnequipped, pItem );

	// Tell ourselves that the item was unequipped
	WB_MAKE_EVENT( OnItemUnequipped, pEntity );
	WB_SET_AUTO( OnItemUnequipped, Entity, Item, pItem );
	WB_DISPATCH_EVENT( GetEventManager(), OnItemUnequipped, pEntity );

	pItem->Destroy();

	if( ShowLogMessage )
	{
		STATICHASH( ItemLost );
		STATICHASH( Item );
		ConfigManager::SetString( sItem, pItem->GetName().CStr(), sItemLost );

		RosaHUDLog::StaticAddDynamicMessage( sItemLost );
	}
}

void WBCompRosaInventory::RemoveAllItems()
{
	const bool ShowLogMessage = false;
	while( m_InventoryMap.Size() )
	{
		RemoveItem( m_InventoryMap.First().GetValue().Get(), ShowLogMessage );
	}
}

void WBCompRosaInventory::RemovePersistentItems()
{
	// Gather persistent items, then remove, so we're not modifying the map while iterating it

	Array<WBEntity*> PersistentItems;
	PersistentItems.Reserve( m_InventoryMap.Size() );

	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const pItem = ItemIter.GetValue().Get();
		DEVASSERT( pItem );

		WBCompRosaItem* const pItemComponent = WB_GETCOMP( pItem, RosaItem );
		DEVASSERT( pItemComponent );

		if( pItemComponent->IsPersistent() )
		{
			PersistentItems.PushBack( pItem );
		}
	}

	const bool ShowLogMessage = false;
	FOR_EACH_ARRAY( ItemIter, PersistentItems, WBEntity* )
	{
		WBEntity* const pItem = ItemIter.GetValue();
		DEVASSERT( pItem );

		RemoveItem( pItem, ShowLogMessage );
	}
}

void WBCompRosaInventory::SwapItems( const HashedString& SlotA, const HashedString& SlotB )
{
	WBEntity* const pItemA = GetItem( SlotA );
	WBEntity* const pItemB = GetItem( SlotB );

	if( pItemA )
	{
		WBCompRosaItem* const pItemAItem = WB_GETCOMP( pItemA, RosaItem );
		ASSERT( pItemAItem );

		pItemAItem->SetSlot( SlotB );
	}

	if( pItemB )
	{
		WBCompRosaItem* const pItemBItem = WB_GETCOMP( pItemB, RosaItem );
		ASSERT( pItemBItem );

		pItemBItem->SetSlot( SlotA );
	}

	// Inventory map needs to be kept clean, so remove slots that will be empty now.
	if( pItemB )
	{
		m_InventoryMap[ SlotA ] = pItemB;
	}
	else
	{
		m_InventoryMap.Remove( SlotA );
	}

	if( pItemA )
	{
		m_InventoryMap[ SlotB ] = pItemA;
	}
	else
	{
		m_InventoryMap.Remove( SlotB );
	}

	// Tell ourselves that the items were swapped
	WB_MAKE_EVENT( OnItemsSwapped, GetEntity() );
	WB_SET_AUTO( OnItemsSwapped, Entity, ItemA, pItemA );
	WB_SET_AUTO( OnItemsSwapped, Entity, ItemB, pItemB );
	WB_DISPATCH_EVENT( GetEventManager(), OnItemsSwapped, GetEntity() );
}

bool WBCompRosaInventory::HasItem( const SimpleString& DefinitionName )
{
	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const pItem = ItemIter.GetValue().Get();
		DEVASSERT( pItem );

		if( pItem->GetName() == DefinitionName )
		{
			return true;
		}
	}

	return false;
}

HashedString WBCompRosaInventory::GetSlotForItem( const SimpleString& DefinitionName )
{
	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const pItem = ItemIter.GetValue().Get();
		DEVASSERT( pItem );

		if( pItem->GetName() == DefinitionName )
		{
			return ItemIter.GetKey();
		}
	}

	return HashedString::NullString;
}

bool WBCompRosaInventory::RequestCycle( const bool Next )
{
	const uint		NumCycleSlots	= m_CycleSlots.Size();
	DEVASSERT( NumCycleSlots > 0 );
	const uint		Offset			= Next ? 1 : ( NumCycleSlots - 1 );
	const uint		OriginalIndex	= m_CycleIndex;
	uint			CycleIndex		= ( OriginalIndex + Offset ) % NumCycleSlots;

	// Iterate until we find a usable item or end up back where we started
	for( ; CycleIndex != OriginalIndex; CycleIndex = ( CycleIndex + Offset ) % NumCycleSlots )
	{
		const HashedString CycleSlot = m_CycleSlots[ CycleIndex ];
		if( RequestCycle( CycleSlot ) )
		{
			return true;
		}
	}

	return false;
}

bool WBCompRosaInventory::RequestCycle( const HashedString& Slot )
{
	// Ignore if this is the current slot
	if( Slot == GetCycleSlot() )
	{
		return false;
	}

	// Ignore if slot is empty
	const TInventoryMap::Iterator SlotIter = m_InventoryMap.Search( Slot );
	if( SlotIter.IsNull() )
	{
		return false;
	}

	// Ignore if item in slot is null
	WBEntity* const pItem = SlotIter.GetValue().Get();
	if( !pItem )
	{
		return false;
	}

	// All checks passed, we've found our desired cycle item
	// Now marshal the request through the hands/weapon system
	WB_MAKE_EVENT( TryCycleToSlot, GetEntity() );
	WB_SET_AUTO( TryCycleToSlot, Hash, Slot, Slot );
	WB_DISPATCH_EVENT( GetEventManager(), TryCycleToSlot, GetEntity() );
	return true;
}

void WBCompRosaInventory::Cycle( const HashedString& Slot )
{
	// Notify the old item that it is no longer cycled
	WBEntity* const	pOldItem = GetCycleItem();
	if( pOldItem )
	{
		OnItemUncycled( pOldItem );
	}

	// Update cycle index so we'll continue to cycle correctly from this slot
	const Array<HashedString>::Iterator CycleSlotIter = m_CycleSlots.Search( Slot );
	DEVASSERT( CycleSlotIter.IsValid() );
	m_CycleIndex = CycleSlotIter.GetIndex();

	// Notify this item it has become the current cycle item
	const TInventoryMap::Iterator SlotIter = m_InventoryMap.Search( Slot );
	WBEntity* const pItem = SlotIter.IsValid() ? SlotIter.GetValue().Get() : NULL;
	if( pItem )
	{
		OnItemCycled( pItem );
	}
}

void WBCompRosaInventory::OnItemCycled( WBEntity* const pItem )
{
	DEVASSERT( pItem );

	WBEntity* const pEntity = GetEntity();

	WB_MAKE_EVENT( OnItemCycled, pEntity );
	WB_SET_AUTO( OnItemCycled, Entity, Item, pItem );
	WB_DISPATCH_EVENT( GetEventManager(), OnItemCycled, pEntity );

	WB_MAKE_EVENT( OnCycled, pItem );
	WB_DISPATCH_EVENT( GetEventManager(), OnCycled, pItem );
}

void WBCompRosaInventory::OnItemUncycled( WBEntity* const pItem )
{
	DEVASSERT( pItem );

	WBEntity* const pEntity = GetEntity();

	WB_MAKE_EVENT( OnUncycled, pItem );
	WB_DISPATCH_EVENT( GetEventManager(), OnUncycled, pItem );

	WB_MAKE_EVENT( OnItemUncycled, pEntity );
	WB_SET_AUTO( OnItemUncycled, Entity, Item, pItem );
	WB_DISPATCH_EVENT( GetEventManager(), OnItemUncycled, pEntity );
}

void WBCompRosaInventory::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	// Push items in order of their UIDs, so we reconstruct them in the same order they were originally created.
	Array<WBEntityRef> PersistentItems;
	PersistentItems.Reserve( m_InventoryMap.Size() );
	FOR_EACH_MAP( ItemIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		WBEntity* const pItem = ItemIter.GetValue().Get();
		DEVASSERT( pItem );

		WBCompRosaItem* const pItemComponent = WB_GETCOMP( pItem, RosaItem );
		DEVASSERT( pItemComponent );

		if( pItemComponent->IsPersistent() )
		{
			PersistentItems.PushBack( ItemIter.GetValue() );
		}
	}
	PersistentItems.QuickSort();

	const uint NumPersistentItems = PersistentItems.Size();

	STATIC_HASHED_STRING( NumPersistentItems );
	Persistence.SetInt( sNumPersistentItems, NumPersistentItems );

	for( uint ItemIndex = 0; ItemIndex < NumPersistentItems; ++ItemIndex )
	{
		WBEntity* const pItem = PersistentItems[ ItemIndex ].Get();
		DEVASSERT( pItem );

		WBCompRosaItem* const pItemComponent = WB_GETCOMP( pItem, RosaItem );
		DEVASSERT( pItemComponent );
		DEVASSERT( pItemComponent->IsPersistent() );

		Persistence.SetHash(		SimpleString::PrintF( "InventoryItem%d",	ItemIndex ),			pItem->GetName() );
		Persistence.SetHash(		SimpleString::PrintF( "InventorySlot%d",	ItemIndex ),			pItemComponent->GetSlot() );

		WBCompRosaWeapon* const pWeaponComponent = WB_GETCOMP( pItem, RosaWeapon );
		if( pWeaponComponent )
		{
			const uint NumMags = pWeaponComponent->GetNumMagazines();
			Persistence.SetInt(		SimpleString::PrintF( "Inventory%dNumMags",	ItemIndex ),			NumMags );
			Persistence.SetInt(		SimpleString::PrintF( "Inventory%dMag",		ItemIndex ),			pWeaponComponent->GetCurrentMagazine() );
			FOR_EACH_INDEX( MagIndex, NumMags )
			{
				Persistence.SetInt(	SimpleString::PrintF( "Inventory%dMag%d",	ItemIndex, MagIndex ),	pWeaponComponent->GetAmmoCount( MagIndex ) );
			}
		}
	}

	STATIC_HASHED_STRING( CycleIndex );
	Persistence.SetInt( sCycleIndex, m_CycleIndex );
}

void WBCompRosaInventory::PullPersistence()
{
	RemovePersistentItems();

	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( NumPersistentItems );
	const uint NumPersistentItems = Persistence.GetInt( sNumPersistentItems );

	const bool ShowLogMessage = false;
	for( uint ItemIndex = 0; ItemIndex < NumPersistentItems; ++ItemIndex )
	{
		const SimpleString		InventoryItem		= Persistence.GetString(	SimpleString::PrintF( "InventoryItem%d",	ItemIndex ) );
		const HashedString		InventorySlot		= Persistence.GetHash(		SimpleString::PrintF( "InventorySlot%d",	ItemIndex ) );

		WBEntity* const			pItem				= AddItem( InventoryItem, InventorySlot, ShowLogMessage );
		DEVASSERT( pItem );

		WBCompRosaWeapon* const	pWeaponComponent	= WB_GETCOMP( pItem, RosaWeapon );
		if( pWeaponComponent )
		{
			const uint			NumMags				= Persistence.GetInt(		SimpleString::PrintF( "Inventory%dNumMags",	ItemIndex ) );
			const uint			CurrentMag			= Persistence.GetInt(		SimpleString::PrintF( "Inventory%dMag",		ItemIndex ) );
			DEVASSERT( NumMags == pWeaponComponent->GetNumMagazines() );
			DEVASSERT( NumMags == 0 || CurrentMag < NumMags );
			pWeaponComponent->SetCurrentMagazine( CurrentMag );
			FOR_EACH_INDEX( MagIndex, NumMags )
			{
				const uint		AmmoCount			= Persistence.GetInt(		SimpleString::PrintF( "Inventory%dMag%d",	ItemIndex, MagIndex ) );
				pWeaponComponent->SetAmmoCount( MagIndex, AmmoCount );
			}
		}
	}

	STATIC_HASHED_STRING( CycleIndex );
	const uint CycleIndex = Persistence.GetInt( sCycleIndex );

	// Notify selected cycle item (refreshes HUD, animations, etc.)
	Cycle( GetCycleSlot( CycleIndex ) );

	// Notify that we've respawned inventory, mainly so character can fix up hand meshes.
	WB_MAKE_EVENT( OnRespawnedInventory, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnRespawnedInventory, GetEntity() );
}

#define VERSION_EMPTY			0
#define VERSION_INVENTORYMAP	1
#define VERSION_CYCLEINDEX		2
#define VERSION_CURRENT			2

/*virtual*/ uint WBCompRosaInventory::GetSerializationSize()
{
	uint Size = 0;

	const uint ItemSize = sizeof( HashedString ) + sizeof( WBEntityRef );

	Size += 4;									// Version

	Size += 4;									// m_InventoryMap.Size()
	Size += ItemSize * m_InventoryMap.Size();	// m_InventoryMap

	Size += 4;									// m_CycleIndex;

	return Size;
}

/*virtual*/ void WBCompRosaInventory::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_InventoryMap.Size() );
	FOR_EACH_MAP( InventoryIter, m_InventoryMap, HashedString, WBEntityRef )
	{
		Stream.WriteHashedString( InventoryIter.GetKey() );
		Stream.Write( sizeof( WBEntityRef ), &InventoryIter.GetValue() );
	}

	Stream.WriteUInt32( m_CycleIndex );
}

/*virtual*/ void WBCompRosaInventory::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_INVENTORYMAP )
	{
		ASSERT( m_InventoryMap.Empty() );
		const uint NumItems = Stream.ReadUInt32();
		for( uint ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex )
		{
			const HashedString Slot = Stream.ReadHashedString();
			WBEntityRef Item;
			Stream.Read( sizeof( WBEntityRef ), &Item );
			m_InventoryMap[ Slot ] = Item;
		}
	}

	if( Version >= VERSION_CYCLEINDEX )
	{
		m_CycleIndex = Stream.ReadUInt32();
	}
}
