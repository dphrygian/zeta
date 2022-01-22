#ifndef WBCOMPROSALOOTTABLE_H
#define WBCOMPROSALOOTTABLE_H

#include "wbrosacomponent.h"
#include "wbeventmanager.h"

class WBCompRosaLootTable : public WBRosaComponent
{
public:
	WBCompRosaLootTable();
	virtual ~WBCompRosaLootTable();

	DEFINE_WBCOMP( RosaLootTable, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			GiveLoot( const uint Points, const bool GiveBandages, const bool GiveAmmo ) const;

	// Point rates. For ammo prices, points are treated equivalently to dust, just to normalize things.
	// But in the conversion of points to dust, there is a different exchange, which makes it more
	// worthwhile to get ammo in a loot table than to buy it with the dust received in a loot table.
	uint			m_BandagePrice;
	uint			m_DustPrice;
};

#endif // WBCOMPROSALOOTTABLE_H
