#ifndef WBCOMPROSAINVENTORY_H
#define WBCOMPROSAINVENTORY_H

#include "wbrosacomponent.h"
#include "map.h"
#include "hashedstring.h"
#include "wbentityref.h"
#include "simplestring.h"

struct SAmmoType;

class WBCompRosaInventory : public WBRosaComponent
{
public:
	WBCompRosaInventory();
	virtual ~WBCompRosaInventory();

	DEFINE_WBCOMP( RosaInventory, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	WBEntity*		GetItem( const HashedString& Slot ) const;
	WBEntity*		GetCycleItem() const;
	HashedString	GetCycleSlot() const						{ return m_CycleSlots[ m_CycleIndex ]; }
	HashedString	GetCycleSlot( const uint CycleIndex ) const	{ return m_CycleSlots[ CycleIndex ]; }
	HashedString	GetBestCycleSlot() const;
	bool			IsCycleSlot( const HashedString& Slot ) const { return m_CycleSlots.Search( Slot ).IsValid(); }
	uint			GetNumCycleItems() const;	// Returns number of filled slots
	uint			GetAvailableCycleSlots() const;

	void			GetEquippedAmmoTypes( Array<SAmmoType>& OutAmmoTypes ) const;	// Get unlocked ammo types for currently equipped weapons
	uint			GetAmmoRefillPrice() const;
	void			RefillAmmo();

	WBEntity*		AddItem( const SimpleString& DefinitionName, const bool ShowLogMessage );
	void			AddItem( WBEntity* const pItem, const bool ShowLogMessage );
	WBEntity*		AddItem( const SimpleString& DefinitionName, const HashedString& Slot, const bool ShowLogMessage );
	void			AddItem( WBEntity* const pItem, const HashedString& Slot, const bool ShowLogMessage );
	void			RemoveItem( WBEntity* const pItem, const bool ShowLogMessage );
	void			RemoveAllItems();
	void			RemovePersistentItems();
	void			SwapItems( const HashedString& SlotA, const HashedString& SlotB );
	bool			HasItem( const SimpleString& DefinitionName );
	HashedString	GetSlotForItem( const SimpleString& DefinitionName );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			AddInitialItemSet( const SimpleString& DefinitionName );

	void			AddItemInternal( const HashedString& Slot, WBEntity* const pItem, const bool ShowLogMessage );
	void			RemoveItemInternal( const HashedString& Slot, const bool ShowLogMessage );

	void			PushPersistence() const;
	void			PullPersistence();

	// RequestCycle() goes through the hands to the weapon, to wait for the equipped weapon to be lowered;
	// it should not be used if there is no weapon currently equipped. (ZETATODO: Make it Cycle() immediately
	// in that case?). See WBCompRosaWeapon::TryPutDown and WBCompRosaWeapon::EndPutDown for how it comes
	// back here. Cycle() actually does the cycling and may be used directly when needed.
	bool			RequestCycle( const bool Next );
	bool			RequestCycle( const HashedString& Slot );
	void			Cycle( const HashedString& Slot );

	void			OnItemCycled( WBEntity* const pItem );
	void			OnItemUncycled( WBEntity* const pItem );

	typedef Map<HashedString, WBEntityRef> TInventoryMap;
	TInventoryMap	m_InventoryMap;	// Serialized, maps slots/categories to inventory in those slots

	struct SInitialItem
	{
		SInitialItem()
		:	m_ItemDef()
		,	m_Slot()
		{
		}

		SimpleString	m_ItemDef;
		HashedString	m_Slot;
	};

	Array<SInitialItem>	m_InitialItems;			// Config

	Array<HashedString>	m_CycleSlots;			// Config, ordered list of item slots to cycle through
	uint				m_CycleIndex;			// Serialized, currently readied slot in ordered list
	uint				m_AvailableCycleSlots;	// Config, how many cycle slots are unlocked by default (statmodded)

	// ROSANOTE: AvailableCycleSlots doesn't strictly forbid items being placed in those slots.
	// It is only used by the weapon equip screen to disable assigning to that slot, and so
	// nothing *should* be able to be equipped there until more cycle slots are available.
};

#endif // WBCOMPROSAINVENTORY_H
