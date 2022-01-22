#include "core.h"
#include "wbcomprosaammobag.h"
#include "wbcomprosaitem.h"
#include "Components/wbcompstatmod.h"
#include "configmanager.h"
#include "idatastream.h"
#include "mathcore.h"
#include "rosagame.h"
#include "rosacampaign.h"
#include "reversehash.h"
#include "rosahudlog.h"

WBCompRosaAmmoBag::WBCompRosaAmmoBag()
:	m_AmmoCounts()
,	m_AmmoTypes()
,	m_SortedAmmoTypes()
,	m_HidePickupScreenDelay( 0.0f )
,	m_HidePickupScreenUID( 0 )
{
}

WBCompRosaAmmoBag::~WBCompRosaAmmoBag()
{
}

/*virtual*/ void WBCompRosaAmmoBag::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NumAmmoTypes );
	const uint NumAmmoTypes = ConfigManager::GetInheritedInt( sNumAmmoTypes, 0, sDefinitionName );
	FOR_EACH_INDEX( AmmoTypeIndex, NumAmmoTypes )
	{
		const SimpleString	AmmoTypeName	= ConfigManager::GetInheritedSequenceString( "AmmoType%d", AmmoTypeIndex, "", sDefinitionName );
		const HashedString	AmmoTypeHash	= AmmoTypeName;

		SAmmoType&			AmmoType		= m_AmmoTypes[ AmmoTypeHash ];

		AmmoType.m_Name						= AmmoTypeName;
		AmmoType.m_Hash						= AmmoTypeHash;

		STATICHASH( UnitPrice );
		AmmoType.m_UnitPrice				= ConfigManager::GetInheritedInt( sUnitPrice,	0,	AmmoTypeHash );

		STATICHASH( BundleSize );
		AmmoType.m_BundleSize				= ConfigManager::GetInheritedInt( sBundleSize,	0,	AmmoTypeHash );

		STATICHASH( Limit );
		AmmoType.m_Limit					= ConfigManager::GetInheritedInt( sLimit,		0,	AmmoTypeHash );

		m_SortedAmmoTypes.PushBack( AmmoTypeName );
	}

	STATICHASH( HidePickupScreenDelay );
	m_HidePickupScreenDelay = ConfigManager::GetInheritedFloat( sHidePickupScreenDelay, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaAmmoBag::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( AddAmmo );
	STATIC_HASHED_STRING( RemoveAmmo );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sAddAmmo )
	{
		STATIC_HASHED_STRING( AmmoType );
		const HashedString AmmoType = Event.GetHash( sAmmoType );

		STATIC_HASHED_STRING( AmmoCount );
		const uint AmmoCount = Event.GetInt( sAmmoCount );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		AddAmmo( AmmoType, AmmoCount, ShowLogMessage );
	}
	else if( EventName == sRemoveAmmo )
	{
		STATIC_HASHED_STRING( AmmoType );
		const HashedString AmmoType = Event.GetHash( sAmmoType );

		STATIC_HASHED_STRING( AmmoCount );
		const uint AmmoCount = Event.GetInt( sAmmoCount );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		RemoveAmmo( AmmoType, AmmoCount, ShowLogMessage );
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

uint WBCompRosaAmmoBag::GetAmmoCount( const HashedString& AmmoType ) const
{
	Map<HashedString, uint>::Iterator	AmmoIter	= m_AmmoCounts.Search( AmmoType );
	const uint							AmmoCount	= AmmoIter.IsValid() ? AmmoIter.GetValue() : 0;
	return AmmoCount;
}

uint WBCompRosaAmmoBag::GetAmmoLimit( const HashedString& AmmoType ) const
{
	Map<HashedString, SAmmoType>::Iterator	AmmoIter	= m_AmmoTypes.Search( AmmoType );
	if( AmmoIter.IsNull() )
	{
		return 0;
	}

	WBCompStatMod* const					pStatMod		= WB_GETCOMP( GetEntity(), StatMod );
	const float								DefaultLimit	= static_cast<float>( AmmoIter.GetValue().m_Limit );
	WB_MODIFY_FLOAT( AmmoCapacity, DefaultLimit, pStatMod );
	const uint								AmmoCapacity	= RoundToUInt( WB_MODDED( AmmoCapacity ) );

	return AmmoCapacity;
}

const SimpleString WBCompRosaAmmoBag::GetAmmoName( const HashedString& AmmoType ) const
{
	Map<HashedString, SAmmoType>::Iterator	AmmoIter	= m_AmmoTypes.Search( AmmoType );
	const SimpleString						AmmoName	= AmmoIter.IsValid() ? AmmoIter.GetValue().m_Name : "";
	return AmmoName;
}

const SAmmoType& WBCompRosaAmmoBag::GetAmmoType( const HashedString& AmmoType ) const
{
	DEVASSERT( m_AmmoTypes.Search( AmmoType ).IsValid() );
	return m_AmmoTypes[ AmmoType ];
}

const SAmmoType* WBCompRosaAmmoBag::SafeGetAmmoType( const HashedString& AmmoType ) const
{
	TAmmoTypeMap::Iterator AmmoTypeIter = m_AmmoTypes.Search( AmmoType );
	return AmmoTypeIter.IsValid() ? &AmmoTypeIter.GetValue() : NULL;
}

void WBCompRosaAmmoBag::RefillAmmo( const HashedString& AmmoType, const bool ShowLogMessage, const uint MagazineRefill )
{
	DEVASSERT( SafeGetAmmoType( AmmoType ) );
	uint&		AmmoCount	= m_AmmoCounts[ AmmoType ];
	const uint	AmmoLimit	= GetAmmoLimit( AmmoType );
	DEVASSERT( AmmoLimit > 0 );
	const uint	ActualGain	= ( AmmoLimit - AmmoCount ) + MagazineRefill;	// HACKHACK: Add the magazine refill amount so the log message is correct
	AmmoCount				= AmmoLimit;

	if( ShowLogMessage && ActualGain > 0 )
	{
		STATICHASH( AmmoPickup );
		STATICHASH( Count );
		ConfigManager::SetInt( sCount, ActualGain, sAmmoPickup );

		STATICHASH( Type );
		ConfigManager::SetString( sType, ReverseHash::ReversedHash( AmmoType ).CStr(), sAmmoPickup );

		RosaHUDLog::StaticAddDynamicMessage( sAmmoPickup );
	}

	WB_MAKE_EVENT( OnAmmoBagUpdated, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnAmmoBagUpdated, GetEntity() );
}

void WBCompRosaAmmoBag::AddAmmo( const HashedString& AmmoType, const uint Count, const bool ShowLogMessage )
{
	uint&		AmmoCount	= m_AmmoCounts[ AmmoType ];
	const uint	AmmoLimit	= GetAmmoLimit( AmmoType );
	const uint	ActualGain	= ( AmmoLimit > 0 ) ? Min( AmmoLimit - AmmoCount, Count ) : Count;
	AmmoCount += ActualGain;

	if( ShowLogMessage )
	{
		if( ActualGain > 0 )
		{
			STATICHASH( AmmoPickup );
			STATICHASH( Count );
			ConfigManager::SetInt( sCount, ActualGain, sAmmoPickup );

			STATICHASH( Type );
			ConfigManager::SetString( sType, ReverseHash::ReversedHash( AmmoType ).CStr(), sAmmoPickup );

			STATICHASH( AmmoPickupSwitch );
			ConfigManager::SetString( sAmmoPickupSwitch, "AmmoPickup", sAmmoPickup );

			//RosaHUDLog::StaticAddDynamicMessage( sAmmoPickup );

			// Eldritch-style pickup screen
			{
				STATIC_HASHED_STRING( AmmoPickupScreen );

				{
					WB_MAKE_EVENT( PushUIScreen, NULL );
					WB_SET_AUTO( PushUIScreen, Hash, Screen, sAmmoPickupScreen );
					WB_DISPATCH_EVENT( GetEventManager(), PushUIScreen, NULL );
				}

				{
					// Remove previously queued hide event if any
					GetEventManager()->UnqueueEvent( m_HidePickupScreenUID );

					WB_MAKE_EVENT( RemoveUIScreen, NULL );
					WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sAmmoPickupScreen );
					m_HidePickupScreenUID = WB_QUEUE_EVENT_DELAY( GetEventManager(), RemoveUIScreen, NULL, m_HidePickupScreenDelay );
				}
			}

			{
				// HACKHACK for Zeta
				WB_MAKE_EVENT( OnAmmoAdded, GetEntity() );
				WB_DISPATCH_EVENT( GetEventManager(), OnAmmoAdded, GetEntity() );
			}
		}
		else
		{
			// HACKHACK: Show Found Nothing dialog

			STATICHASH( AmmoPickup );
			STATICHASH( AmmoPickupSwitch );
			ConfigManager::SetString( sAmmoPickupSwitch, "FoundNothing", sAmmoPickup );

			//RosaHUDLog::StaticAddDynamicMessage( sFoundNothing );

			// Eldritch-style pickup screen
			{
				STATIC_HASHED_STRING( AmmoPickupScreen );

				{
					WB_MAKE_EVENT( PushUIScreen, NULL );
					WB_SET_AUTO( PushUIScreen, Hash, Screen, sAmmoPickupScreen );
					WB_DISPATCH_EVENT( GetEventManager(), PushUIScreen, NULL );
				}

				{
					// Remove previously queued hide event if any
					GetEventManager()->UnqueueEvent( m_HidePickupScreenUID );

					WB_MAKE_EVENT( RemoveUIScreen, NULL );
					WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sAmmoPickupScreen );
					m_HidePickupScreenUID = WB_QUEUE_EVENT_DELAY( GetEventManager(), RemoveUIScreen, NULL, m_HidePickupScreenDelay );
				}
			}
		}
	}

	WB_MAKE_EVENT( OnAmmoBagUpdated, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnAmmoBagUpdated, GetEntity() );
}

void WBCompRosaAmmoBag::RemoveAmmo( const HashedString& AmmoType, const uint Count, const bool ShowLogMessage )
{
	ASSERT( HasAmmo( AmmoType, Count ) );

	uint&		AmmoCount	= m_AmmoCounts[ AmmoType ];
	const uint	ActualLoss	= Min( AmmoCount, Count );
	AmmoCount -= ActualLoss;

	if( ShowLogMessage && ActualLoss > 0 )
	{
		STATICHASH( AmmoLost );
		STATICHASH( Count );
		ConfigManager::SetInt( sCount, ActualLoss, sAmmoLost );

		STATICHASH( Type );
		ConfigManager::SetString( sType, ReverseHash::ReversedHash( AmmoType ).CStr(), sAmmoLost );

		RosaHUDLog::StaticAddDynamicMessage( sAmmoLost );
	}

	WB_MAKE_EVENT( OnAmmoBagUpdated, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnAmmoBagUpdated, GetEntity() );
}

uint WBCompRosaAmmoBag::GetAmmoSpace( const HashedString& AmmoType ) const
{
	const uint AmmoCount = GetAmmoCount( AmmoType );
	const uint AmmoLimit = GetAmmoLimit( AmmoType );

	if( AmmoCount > AmmoLimit )
	{
		// Prevent overfilling ammo bag if limits are decreased and game is serialized with more than the new limits
		return 0;
	}
	else
	{
		return AmmoLimit - AmmoCount;
	}
}

void WBCompRosaAmmoBag::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	const uint NumAmmoTypes = m_AmmoCounts.Size();

	STATIC_HASHED_STRING( NumAmmoTypes );
	Persistence.SetInt( sNumAmmoTypes, NumAmmoTypes );

	uint AmmoIndex = 0;
	FOR_EACH_MAP( AmmoIter, m_AmmoCounts, HashedString, uint )
	{
		Persistence.SetHash( SimpleString::PrintF( "Ammo%dType", AmmoIndex ), AmmoIter.GetKey() );
		Persistence.SetInt( SimpleString::PrintF( "Ammo%dCount", AmmoIndex ), AmmoIter.GetValue() );
		++AmmoIndex;
	}
}

void WBCompRosaAmmoBag::PullPersistence()
{
	m_AmmoCounts.Clear();

	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( NumAmmoTypes );
	const uint NumAmmoTypes = Persistence.GetInt( sNumAmmoTypes );

	for( uint AmmoIndex = 0; AmmoIndex < NumAmmoTypes; ++AmmoIndex )
	{
		const HashedString	AmmoKey		= Persistence.GetHash(	SimpleString::PrintF( "Ammo%dType",		AmmoIndex ) );
		const uint			AmmoCount	= Persistence.GetInt(	SimpleString::PrintF( "Ammo%dCount",	AmmoIndex ) );
		m_AmmoCounts[ AmmoKey ]			= AmmoCount;
	}

	WB_MAKE_EVENT( OnAmmoBagUpdated, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnAmmoBagUpdated, GetEntity() );
}

#define VERSION_EMPTY	0
#define VERSION_AMMO	1
#define VERSION_CURRENT	1

uint WBCompRosaAmmoBag::GetSerializationSize()
{
	uint Size = 0;

	const uint AmmoSize = sizeof( HashedString ) + sizeof( uint );

	Size += 4;								// Version
	Size += 4;								// m_AmmoCounts.Size()
	Size += AmmoSize * m_AmmoCounts.Size();	// m_AmmoCounts

	return Size;
}

void WBCompRosaAmmoBag::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_AmmoCounts.Size() );
	FOR_EACH_MAP( AmmoIter, m_AmmoCounts, HashedString, uint )
	{
		Stream.WriteHashedString(	AmmoIter.GetKey() );
		Stream.WriteUInt32(			AmmoIter.GetValue() );
	}
}

void WBCompRosaAmmoBag::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_AMMO )
	{
		ASSERT( m_AmmoCounts.Empty() );
		const uint NumAmmoTypes = Stream.ReadUInt32();
		for( uint AmmoIndex = 0; AmmoIndex < NumAmmoTypes; ++AmmoIndex )
		{
			const HashedString	AmmoType	= Stream.ReadHashedString();
			const uint			AmmoValue	= Stream.ReadUInt32();
			m_AmmoCounts[ AmmoType ] = AmmoValue;
		}
	}
}
