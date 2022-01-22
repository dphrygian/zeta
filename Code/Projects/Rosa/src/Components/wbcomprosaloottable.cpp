#include "core.h"
#include "wbcomprosaloottable.h"
#include "wbcomprosamedkit.h"
#include "wbcomprosaammobag.h"
#include "wbcomprosainventory.h"
#include "Components/wbcompstatmod.h"
#include "wbeventmanager.h"
#include "configmanager.h"
#include "mathcore.h"
#include "mathfunc.h"

#if BUILD_DEV
#include "reversehash.h"
#endif

WBCompRosaLootTable::WBCompRosaLootTable()
:	m_BandagePrice( 0 )
,	m_DustPrice( 0 )
{
}

WBCompRosaLootTable::~WBCompRosaLootTable()
{
}

/*virtual*/ void WBCompRosaLootTable::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( BandagePrice );
	m_BandagePrice = ConfigManager::GetInheritedInt( sBandagePrice, 0, sDefinitionName );

	STATICHASH( DustPrice );
	m_DustPrice = ConfigManager::GetInheritedInt( sDustPrice, 0, sDefinitionName );
}

/*virtual*/ void WBCompRosaLootTable::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( GiveLoot );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sGiveLoot )
	{
		STATIC_HASHED_STRING( Points );
		const uint Points = Event.GetInt( sPoints );

		STATIC_HASHED_STRING( GiveBandages );
		const bool GiveBandages = Event.GetBool( sGiveBandages, true );

		STATIC_HASHED_STRING( GiveAmmo );
		const bool GiveAmmo = Event.GetBool( sGiveAmmo, true );

		GiveLoot( Points, GiveBandages, GiveAmmo );
	}
}

void WBCompRosaLootTable::GiveLoot( const uint Points, const bool GiveBandages, const bool GiveAmmo ) const
{
	WBEventManager* const		pEventManager	= GetEventManager();

	WBEntity* const				pEntity			= GetEntity();

	WBCompRosaMedkit* const		pMedkit			= WB_GETCOMP( pEntity, RosaMedkit );
	DEVASSERT( pMedkit );

	WBCompRosaAmmoBag* const	pAmmoBag		= WB_GETCOMP( pEntity, RosaAmmoBag );
	DEVASSERT( pAmmoBag );

	WBCompRosaInventory* const	pInventory		= WB_GETCOMP( pEntity, RosaInventory );
	DEVASSERT( pInventory );

	WBCompStatMod* const		pStatMod		= WB_GETCOMP( pEntity, StatMod );
	DEVASSERT( pStatMod );

	uint						BandagesNeeded	= pMedkit->GetMaxBandages() - pMedkit->GetBandages();
	uint						BandagesGiven	= 0;

	uint						DustGiven		= 0;

	Array<SAmmoType>			AmmoTypes;
	Map<HashedString, uint>		AmmoNeeded;
	Map<HashedString, uint>		AmmoGiven;

	// DLP 16 Aug 2016: Only give ammo for currently equipped weapons now!
	pInventory->GetEquippedAmmoTypes( AmmoTypes );
	FOR_EACH_ARRAY_REVERSE( AmmoIter, AmmoTypes, SAmmoType )
	{
		const SAmmoType& AmmoType = AmmoIter.GetValue();
		const uint AmmoTypeNeeded = pAmmoBag->GetAmmoLimit( AmmoType.m_Hash ) - pAmmoBag->GetAmmoCount( AmmoType.m_Hash );
		if( AmmoTypeNeeded > 0 )
		{
			AmmoNeeded.Insert( AmmoType.m_Hash, AmmoTypeNeeded );
			AmmoGiven.Insert( AmmoType.m_Hash, 0 );
		}
		else
		{
			AmmoTypes.FastRemove( AmmoIter );
		}
	}

	// Shuffle the needed ammo types and deal them out one at a time;
	// it's more satisfying and logical to get a bunch of units for one
	// or two ammo types instead of one or two units for a bunch of types.
	Math::ArrayShuffle( AmmoTypes );

	const uint					NumElements		= AmmoTypes.Size() + 2;			// All available ammo, plus bandages and dust (regardless of need)
	const float					Weight			= 1.0f / static_cast<float>( NumElements );
	const float					AmmoWeight		= AmmoTypes.Size() * Weight;	// Giving *any* ammo is weighted based on the number of ammo types needed

	WB_MODIFY_FLOAT( LootTablePoints, static_cast<float>( Points ), pStatMod );
	uint						PointsRemaining	= static_cast<uint>( WB_MODDED( LootTablePoints ) );
	while( PointsRemaining > 0 )
	{
		// Weighted random select from bandages, ammo, or dust we can afford with points remaining
		// Subtract the point cost and continue

		// First, try to give bandages
		if( GiveBandages &&
			BandagesNeeded > 0 &&
			PointsRemaining >= m_BandagePrice &&
			Math::RandomF( Weight ) )
		{
			BandagesGiven	+= 1;
			BandagesNeeded	-= 1;
			PointsRemaining	-= m_BandagePrice;
			continue;
		}

		// Drop ammo types until we get to one we can afford
		while( AmmoTypes.Size() )
		{
			const SAmmoType&	AmmoType		= AmmoTypes.Last();
			const uint&			AmmoUnitPrice	= AmmoType.m_UnitPrice;
			if( PointsRemaining < AmmoUnitPrice )
			{
				AmmoTypes.PopBack();
			}
			else
			{
				break;
			}
		}

		// Next, try to give ammo (in units, not bundles)
		if( GiveAmmo &&
			AmmoTypes.Size() > 0 &&
			Math::RandomF( AmmoWeight ) )
		{
			const SAmmoType&	AmmoType		= AmmoTypes.Last();
			uint&				AmmoTypeNeeded	= AmmoNeeded[ AmmoType.m_Hash ];
			uint&				AmmoTypeGiven	= AmmoGiven[ AmmoType.m_Hash ];
			const uint&			AmmoUnitPrice	= AmmoType.m_UnitPrice;	// ROSANOTE: Points are treated equivalently to dust for this conversion, to normalize point values

			DEVASSERT( AmmoTypeNeeded > 0 );
			DEVASSERT( PointsRemaining >= AmmoUnitPrice );

			AmmoTypeGiven	+= 1;
			AmmoTypeNeeded	-= 1;
			PointsRemaining	-= AmmoUnitPrice;

			if( AmmoTypeNeeded == 0 )
			{
				AmmoTypes.PopBack();
			}

			continue;
		}

		// Finally, convert points to dust
		DustGiven		+= 1;
		PointsRemaining	-= Min( PointsRemaining, m_DustPrice );
	}

	if( BandagesGiven > 0 )
	{
		DEVPRINTF( "Loot table: %d bandages (%d points)\n", BandagesGiven, BandagesGiven * m_BandagePrice );
		WB_MAKE_EVENT( AddBandages, pEntity );
		WB_SET_AUTO( AddBandages, Int, Bandages, BandagesGiven );
		WB_SET_AUTO( AddBandages, Bool, ShowLogMessage, true );
		WB_DISPATCH_EVENT( pEventManager, AddBandages, pEntity );
	}

	if( DustGiven > 0 )
	{
		DEVPRINTF( "Loot table: %d dust (%d points)\n", DustGiven, DustGiven );
		WB_MAKE_EVENT( AddMoney, pEntity );
		WB_SET_AUTO( AddMoney, Int, Money, DustGiven );
		WB_SET_AUTO( AddMoney, Bool, ShowLogMessage, true );
		WB_DISPATCH_EVENT( pEventManager, AddMoney, pEntity );
	}

	FOR_EACH_MAP( AmmoGivenIter, AmmoGiven, HashedString, uint )
	{
		const HashedString&	AmmoType	= AmmoGivenIter.GetKey();
		const uint			AmmoCount	= AmmoGivenIter.GetValue();
		if( AmmoCount > 0 )
		{
			DEVPRINTF( "Loot table: %d %s (%d points)\n", AmmoCount, ReverseHash::ReversedHash( AmmoType ).CStr(), AmmoCount * pAmmoBag->GetAmmoType( AmmoType ).m_UnitPrice );
			WB_MAKE_EVENT( AddAmmo, pEntity );
			WB_SET_AUTO( AddAmmo, Hash, AmmoType, AmmoType );
			WB_SET_AUTO( AddAmmo, Int, AmmoCount, AmmoCount );
			WB_SET_AUTO( AddAmmo, Bool, ShowLogMessage, true );
			WB_DISPATCH_EVENT( pEventManager, AddAmmo, pEntity );
		}
	}
}
