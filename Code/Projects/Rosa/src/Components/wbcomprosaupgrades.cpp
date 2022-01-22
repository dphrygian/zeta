#include "core.h"
#include "wbcomprosaupgrades.h"
#include "Components/wbcompstatmod.h"
#include "wbcomprosawallet.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "rosahudlog.h"
#include "rosagame.h"
#include "simplestring.h"
#include "Achievements/iachievementmanager.h"
#include "set.h"

WBCompRosaUpgrades::WBCompRosaUpgrades()
:	m_Upgrades()
{
}

WBCompRosaUpgrades::~WBCompRosaUpgrades()
{
}

/*virtual*/ void WBCompRosaUpgrades::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NumUpgrades );
	const uint NumUpgrades = ConfigManager::GetInheritedInt( sNumUpgrades, 0, sDefinitionName );
	for( uint UpgradeIndex = 0; UpgradeIndex < NumUpgrades; ++UpgradeIndex )
	{
		const SimpleString	UpgradeTag	= ConfigManager::GetInheritedSequenceString(	"Upgrade%d",		UpgradeIndex,	"",							sDefinitionName );
		SUpgrade&			Upgrade		= m_Upgrades.Insert( UpgradeTag );
		Upgrade.m_Name					= UpgradeTag;
		Upgrade.m_Hash					= UpgradeTag;
		Upgrade.m_Icon					= ConfigManager::GetInheritedSequenceString(	"Upgrade%dIcon",		UpgradeIndex,	"",							sDefinitionName );
		Upgrade.m_StatModEvent			= ConfigManager::GetInheritedSequenceHash(		"Upgrade%dStatMod",		UpgradeIndex,	HashedString::NullString,	sDefinitionName );
		Upgrade.m_Cost					= ConfigManager::GetInheritedSequenceInt(		"Upgrade%dCost",		UpgradeIndex,	0,							sDefinitionName );
		Upgrade.m_Prereq				= ConfigManager::GetInheritedSequenceHash(		"Upgrade%dPrereq",		UpgradeIndex,	HashedString::NullString,	sDefinitionName );
		Upgrade.m_PostEvent				= ConfigManager::GetInheritedSequenceHash(		"Upgrade%dPostEvent",	UpgradeIndex,	HashedString::NullString,	sDefinitionName );
		Upgrade.m_Track					= ConfigManager::GetInheritedSequenceHash(		"Upgrade%dTrack",		UpgradeIndex,	HashedString::NullString,	sDefinitionName );
		Upgrade.m_Status				= ( Upgrade.m_Prereq == HashedString::NullString ) ? EUS_Unlocked : EUS_Locked;
	}
}

/*virtual*/ void WBCompRosaUpgrades::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( TryPurchaseUpgrade );
	STATIC_HASHED_STRING( GiveUpgrade );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sTryPurchaseUpgrade )
	{
		STATIC_HASHED_STRING( UpgradeTag );
		const HashedString UpgradeTag = Event.GetHash( sUpgradeTag );

		TryPurchase( UpgradeTag );
	}
	else if( EventName == sGiveUpgrade )
	{
		STATIC_HASHED_STRING( UpgradeTag );
		const HashedString UpgradeTag = Event.GetHash( sUpgradeTag );

		Give( UpgradeTag );
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

void WBCompRosaUpgrades::TriggerStatMods() const
{
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	DEVASSERT( pStatMod );

	FOR_EACH_MAP( UpgradeIter, m_Upgrades, HashedString, SUpgrade )
	{
		const SUpgrade& Upgrade = UpgradeIter.GetValue();
		if( Upgrade.m_Status == EUS_Purchased )
		{
			pStatMod->TriggerEvent( Upgrade.m_StatModEvent );
		}
	}
}

void WBCompRosaUpgrades::TryPurchase( const HashedString& UpgradeTag )
{
	TUpgradeMap::Iterator UpgradeIter = m_Upgrades.Search( UpgradeTag );
	if( UpgradeIter.IsNull() )
	{
		return;
	}

	SUpgrade& Upgrade = UpgradeIter.GetValue();
	if( Upgrade.m_Status != EUS_Unlocked )
	{
		return;
	}

	WBCompRosaWallet* const pWallet = WB_GETCOMP( GetEntity(), RosaWallet );
	DEVASSERT( pWallet );
	if( !pWallet->HasMoney( Upgrade.m_Cost ) )
	{
		return;
	}

	// All checks cleared, purchase and activate

	// Pay the cost first
	const bool ShowLogMessage = true;
	pWallet->RemoveMoney( Upgrade.m_Cost, ShowLogMessage );

	// Then give the upgrade
	Give( Upgrade );
}

void WBCompRosaUpgrades::Give( const HashedString& UpgradeTag )
{
	TUpgradeMap::Iterator UpgradeIter = m_Upgrades.Search( UpgradeTag );
	if( UpgradeIter.IsNull() )
	{
		return;
	}

	SUpgrade& Upgrade = UpgradeIter.GetValue();
	Give( Upgrade );
}

void WBCompRosaUpgrades::Give( SUpgrade& Upgrade )
{
	// Set the upgrade status
	Upgrade.m_Status = EUS_Purchased;

	// Unlock anything that has this upgrade as a prereq
	FOR_EACH_MAP( UnlockUpgradeIter, m_Upgrades, HashedString, SUpgrade )
	{
		SUpgrade& UnlockUpgrade = UnlockUpgradeIter.GetValue();
		if( UnlockUpgrade.m_Prereq == Upgrade.m_Hash )
		{
			if( UnlockUpgrade.m_Status == EUS_Locked )
			{
				UnlockUpgrade.m_Status = EUS_Unlocked;
			}
		}
	}

	// Activate the stat mod
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	DEVASSERT( pStatMod );
	pStatMod->TriggerEvent( Upgrade.m_StatModEvent );

	// Finally, send an event to the entity if required
	if( Upgrade.m_PostEvent )
	{
		WBEvent PostEvent;
		PostEvent.SetEventName( Upgrade.m_PostEvent );
		GetEntity()->AddContextToEvent( PostEvent );
		GetEventManager()->DispatchEvent( PostEvent, GetEntity() );
	}

	// Award achievements for all completed tracks
	Set<HashedString> Tracks;
	Set<HashedString> IncompleteTracks;
	FOR_EACH_MAP( UpgradeIter, m_Upgrades, HashedString, SUpgrade )
	{
		const SUpgrade& IterUpgrade = UpgradeIter.GetValue();
		Tracks.Insert( IterUpgrade.m_Track );
		if( IterUpgrade.m_Status != EUS_Purchased )
		{
			IncompleteTracks.Insert( IterUpgrade.m_Track );
		}
	}

	// ROSATODO: Re-enable if desired
	//const uint NumCompleteTracks = Tracks.Size() - IncompleteTracks.Size();
	//if( NumCompleteTracks == 1 ) { AWARD_ACHIEVEMENT( "ACH_Skills_1" ) };
	//if( NumCompleteTracks == 2 ) { AWARD_ACHIEVEMENT( "ACH_Skills_2" ) };
	//if( NumCompleteTracks == 3 ) { AWARD_ACHIEVEMENT( "ACH_Skills_3" ) };
	//if( NumCompleteTracks == 4 ) { AWARD_ACHIEVEMENT( "ACH_Skills_4" ) };
}

const SUpgrade* WBCompRosaUpgrades::SafeGetUpgrade( const HashedString& Upgrade ) const
{
	TUpgradeMap::Iterator UpgradeIter = m_Upgrades.Search( Upgrade );
	if( UpgradeIter.IsNull() )
	{
		return NULL;
	}

	return &UpgradeIter.GetValue();
}

void WBCompRosaUpgrades::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( NumUpgrades );
	Persistence.SetInt( sNumUpgrades, m_Upgrades.Size() );

	uint UpgradeIndex = 0;
	FOR_EACH_MAP( UpgradeIter, m_Upgrades, HashedString, SUpgrade )
	{
		const HashedString	UpgradeTag	= UpgradeIter.GetKey();
		const SUpgrade&		Upgrade		= UpgradeIter.GetValue();

		Persistence.SetHash(	SimpleString::PrintF( "Upgrade%dTag",		UpgradeIndex ),	UpgradeTag );
		Persistence.SetInt(		SimpleString::PrintF( "Upgrade%dStatus",	UpgradeIndex ),	Upgrade.m_Status );

		++UpgradeIndex;
	}
}

void WBCompRosaUpgrades::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( NumUpgrades );
	const uint NumUpgrades = Persistence.GetInt( sNumUpgrades );

	// We're just persisting upgrade *status*, not the upgrades themselves.
	// In fact, pushing and pulling the upgrade tags is just a precaution;
	// they should remain ordered the same.
	DEVASSERT( NumUpgrades == m_Upgrades.Size() );

	for( uint UpgradeIndex = 0; UpgradeIndex < NumUpgrades; ++UpgradeIndex )
	{
		const HashedString		UpgradeTag	= Persistence.GetHash( SimpleString::PrintF( "Upgrade%dTag", UpgradeIndex ) );
		TUpgradeMap::Iterator	UpgradeIter	= m_Upgrades.Search( UpgradeTag );
		if( UpgradeIter.IsNull() )
		{
			continue;
		}

		SUpgrade&				Upgrade		= UpgradeIter.GetValue();
		Upgrade.m_Status					= static_cast<EUpgradeStatus>( Persistence.GetInt( SimpleString::PrintF( "Upgrade%dStatus", UpgradeIndex ) ) );
	}

	// Reactivate stat mods after persistence
	TriggerStatMods();
}

#define VERSION_EMPTY			0
#define VERSION_UPGRADE_STATUS	1
#define VERSION_CURRENT			1

uint WBCompRosaUpgrades::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;											// m_Upgrades.Size()
	Size += m_Upgrades.Size() * sizeof( HashedString );	// Upgrade tag
	Size += m_Upgrades.Size() * 4;						// SUpgrade::m_Status

	return Size;
}

void WBCompRosaUpgrades::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_Upgrades.Size() );
	FOR_EACH_MAP( UpgradeIter, m_Upgrades, HashedString, SUpgrade )
	{
		const HashedString	UpgradeTag	= UpgradeIter.GetKey();
		const SUpgrade&		Upgrade		= UpgradeIter.GetValue();

		Stream.WriteHashedString(	UpgradeTag );
		Stream.WriteUInt32(			Upgrade.m_Status );
	}
}

void WBCompRosaUpgrades::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_UPGRADE_STATUS )
	{
		const uint NumUpgrades = Stream.ReadUInt32();
		for( uint UpgradeIndex = 0; UpgradeIndex < NumUpgrades; ++UpgradeIndex )
		{
			const HashedString		UpgradeTag	= Stream.ReadHashedString();
			TUpgradeMap::Iterator	UpgradeIter	= m_Upgrades.Search( UpgradeTag );

			const EUpgradeStatus	Status		= Stream.Read<EUpgradeStatus>();
			if( UpgradeIter.IsValid() )
			{
				SUpgrade&			Upgrade		= UpgradeIter.GetValue();
				Upgrade.m_Status				= Status;
			}
		}
	}
}
