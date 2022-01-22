#ifndef WBCOMPROSAAMMOBAG_H
#define WBCOMPROSAAMMOBAG_H

#include "wbrosacomponent.h"
#include "map.h"
#include "array.h"
#include "wbeventmanager.h"
#include "simplestring.h"

struct SAmmoType
{
	SAmmoType()
	:	m_Name()
	,	m_Hash()
	,	m_UnitPrice( 0 )
	,	m_BundleSize( 0 )
	,	m_Limit( 0 )
	{
	}

	SimpleString	m_Name;			// Config
	HashedString	m_Hash;			// Config
	uint			m_UnitPrice;	// Config; how much each unit of ammo costs
	uint			m_BundleSize;	// Config; how many units are sold at once in the shop
	uint			m_Limit;		// Config; how much ammo can go in the bag (does *not* count ammo in weapons) [now can be statmodded!]
};

typedef Map<HashedString, SAmmoType> TAmmoTypeMap;

class WBCompRosaAmmoBag : public WBRosaComponent
{
public:
	WBCompRosaAmmoBag();
	virtual ~WBCompRosaAmmoBag();

	DEFINE_WBCOMP( RosaAmmoBag, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	uint				GetAmmoCount( const HashedString& AmmoType ) const;
	uint				GetAmmoLimit( const HashedString& AmmoType ) const;
	const SimpleString	GetAmmoName( const HashedString& AmmoType ) const;
	const SAmmoType&	GetAmmoType( const HashedString& AmmoType ) const;
	const SAmmoType*	SafeGetAmmoType( const HashedString& AmmoType ) const;
	const TAmmoTypeMap&	GetAmmoTypes() const { return m_AmmoTypes; }
	bool				HasAmmo( const HashedString& AmmoType ) const { return GetAmmoCount( AmmoType ) > 0; }
	bool				HasAmmo( const HashedString& AmmoType, const uint Count ) const { return GetAmmoCount( AmmoType ) >= Count; }
	void				AddAmmo( const HashedString& AmmoType, const uint Count, const bool ShowLogMessage );
	void				RemoveAmmo( const HashedString& AmmoType, const uint Count, const bool ShowLogMessage );
	void				RefillAmmo( const HashedString& AmmoType, const bool ShowLogMessage, const uint MagazineRefill );

	uint				GetAmmoSpace( const HashedString& AmmoType ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PushPersistence() const;
	void			PullPersistence();

	// Parallel maps so we can clear ammo counts without touching immutable limits
	Map<HashedString, uint>	m_AmmoCounts;		// Serialized/persistent
	TAmmoTypeMap			m_AmmoTypes;		// Config

	Array<HashedString>		m_SortedAmmoTypes;	// Config, for listing in UI in the defined order

	// Eldritch-style pickup UI management
	float					m_HidePickupScreenDelay;
	TEventUID				m_HidePickupScreenUID;
};

#endif // WBCOMPROSAAMMOBAG_H
