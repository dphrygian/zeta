#include "core.h"
#include "wbcomprosawallet.h"
#include "configmanager.h"
#include "idatastream.h"
#include "mathcore.h"
#include "rosagame.h"
#include "rosahudlog.h"
#include "stringmanager.h"
#include "Achievements/iachievementmanager.h"
#include "rosaframework.h"

WBCompRosaWallet::WBCompRosaWallet()
:	m_Money( 0 )
,	m_Limit( 0 )
{
}

WBCompRosaWallet::~WBCompRosaWallet()
{
}

/*virtual*/ void WBCompRosaWallet::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Limit );
	m_Limit = ConfigManager::GetInheritedInt( sLimit, 0, sDefinitionName );

	STATICHASH( InitialCash );
	m_Money = ConfigManager::GetInheritedInt( sInitialCash, 0, sDefinitionName );
}

/*virtual*/ void WBCompRosaWallet::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( AddMoney );
	STATIC_HASHED_STRING( RemoveMoney );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		PublishToHUD();
	}
	else if( EventName == sAddMoney )
	{
		STATIC_HASHED_STRING( Money );
		const uint Money = Event.GetInt( sMoney );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		AddMoney( Money, ShowLogMessage );
	}
	else if( EventName == sRemoveMoney )
	{
		STATIC_HASHED_STRING( Money );
		const uint Money = Event.GetInt( sMoney );

		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage );

		RemoveMoney( Money, ShowLogMessage );
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

/*virtual*/ void WBCompRosaWallet::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Int, Money, m_Money );
}

void WBCompRosaWallet::AddMoney( const uint Money, const bool ShowLogMessage )
{
	const uint ActualGain = Min( m_Limit - m_Money, Money );
	m_Money += ActualGain;

	PublishToHUD();

	if( ShowLogMessage && ActualGain > 0 )
	{
		STATICHASH( MoneyPickup );
		STATICHASH( Money );
		ConfigManager::SetInt( sMoney, ActualGain, sMoneyPickup );

		RosaHUDLog::StaticAddDynamicMessage( sMoneyPickup );

		//INCREMENT_STAT( "MoneyGained", ActualGain );
	}
}

void WBCompRosaWallet::RemoveMoney( const uint Money, const bool ShowLogMessage )
{
	const uint ActualLoss = Min( m_Money, Money );
	m_Money -= ActualLoss;

	PublishToHUD();

	if( ShowLogMessage && ActualLoss > 0 )
	{
		STATICHASH( MoneyLost );
		STATICHASH( Money );
		ConfigManager::SetInt( sMoney, ActualLoss, sMoneyLost );

		RosaHUDLog::StaticAddDynamicMessage( sMoneyLost );

		//INCREMENT_STAT( "MoneyLost", ActualLoss );
	}
}

void WBCompRosaWallet::PublishToHUD() const
{
	STATICHASH( HUD );
	STATICHASH( Money );
	ConfigManager::SetInt( sMoney, m_Money, sHUD );
}

void WBCompRosaWallet::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Money );
	Persistence.SetInt( sMoney, m_Money );
}

void WBCompRosaWallet::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Money );
	m_Money = Persistence.GetInt( sMoney );

	PublishToHUD();
}

#define VERSION_EMPTY	0
#define VERSION_MONEY	1
#define VERSION_CURRENT	1

uint WBCompRosaWallet::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_Money

	return Size;
}

void WBCompRosaWallet::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_Money );
}

void WBCompRosaWallet::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_MONEY )
	{
		m_Money = Stream.ReadUInt32();
	}
}
