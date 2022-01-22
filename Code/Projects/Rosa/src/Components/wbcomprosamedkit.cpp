#include "core.h"
#include "wbcomprosamedkit.h"
#include "wbcomprosahealth.h"
#include "Components/wbcompstatmod.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "configmanager.h"
#include "rosahudlog.h"
#include "rosagame.h"
#include "Common/uimanagercommon.h"
#include "rosaframework.h"
#include "mathcore.h"

WBCompRosaMedkit::WBCompRosaMedkit()
:	m_Bandages( 0 )
,	m_MaxBandages( 0 )
{
}

WBCompRosaMedkit::~WBCompRosaMedkit()
{
}

/*virtual*/ void WBCompRosaMedkit::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( MaxBandages );
	m_MaxBandages = ConfigManager::GetInheritedInt( sMaxBandages, 0, sDefinitionName );
}

/*virtual*/ void WBCompRosaMedkit::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( OnHealthChanged );
	STATIC_HASHED_STRING( RefillBandages );
	STATIC_HASHED_STRING( AddBandages );
	STATIC_HASHED_STRING( TryUseBandage );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		PublishToHUD();
	}
	else if( EventName == sOnHealthChanged )
	{
		PublishToHUD();
	}
	else if( EventName == sRefillBandages )
	{
		AddBandages( GetMaxBandages(), false );
	}
	else if( EventName == sAddBandages )
	{
		STATIC_HASHED_STRING( ShowLogMessage );
		const bool ShowLogMessage = Event.GetBool( sShowLogMessage, true );

		STATIC_HASHED_STRING( Bandages );
		const uint Bandages = Event.GetInt( sBandages, 1 );

		AddBandages( Bandages, ShowLogMessage );
	}
	else if( EventName == sTryUseBandage )
	{
		TryUseBandage();
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

/*virtual*/ void WBCompRosaMedkit::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Int, Bandages, m_Bandages );
}

uint WBCompRosaMedkit::GetMaxBandages() const
{
	WBCompStatMod* const	pStatMod		= WB_GETCOMP( GetEntity(), StatMod );
	DEVASSERT( pStatMod );

	const float				DefaultLimit	= static_cast<float>( m_MaxBandages );
	WB_MODIFY_FLOAT( MaxBandages, DefaultLimit, pStatMod );
	const uint				MaxBandages	= RoundToUInt( WB_MODDED( MaxBandages ) );

	return MaxBandages;
}

void WBCompRosaMedkit::AddBandages( const uint Bandages, const bool ShowLogMessage )
{
	const uint MaxBandages		= GetMaxBandages();
	DEVASSERT( m_Bandages <= MaxBandages );

	const uint ActualBandages	= Min( Bandages, MaxBandages - m_Bandages );
	m_Bandages += ActualBandages;

	PublishToHUD();

	if( ShowLogMessage && ActualBandages > 0 )
	{
		STATICHASH( BandagePickup );
		STATICHASH( Bandages );
		ConfigManager::SetInt( sBandages, ActualBandages, sBandagePickup );

		RosaHUDLog::StaticAddDynamicMessage( sBandagePickup );
	}
}

void WBCompRosaMedkit::TryUseBandage()
{
	if( !HasBandages() )
	{
		// No bandages to use!
		return;
	}

	WBCompRosaHealth* const pHealth = WB_GETCOMP( GetEntity(), RosaHealth );
	DEVASSERT( pHealth );

	if( pHealth->HasMaxHealth() )
	{
		// Don't waste a bandage!
		return;
	}

	WBEntity* const			pEntity			= GetEntity();
	WBEventManager* const	pEventManager	= GetEventManager();

	WB_MAKE_EVENT( RestoreHealth, pEntity );
	WB_DISPATCH_EVENT( pEventManager, RestoreHealth, pEntity );

	WB_MAKE_EVENT( OnBandaged, pEntity );
	WB_DISPATCH_EVENT( pEventManager, OnBandaged, pEntity );

	m_Bandages -= 1;

	PublishToHUD();
}

void WBCompRosaMedkit::PublishToHUD() const
{
	WBCompRosaHealth* const pHealth = WB_GETCOMP( GetEntity(), RosaHealth );
	DEVASSERT( pHealth );

	UIManager* const pUIManager = GetFramework()->GetUIManager();
	DEVASSERT( pUIManager );

	STATICHASH( HUD );
	STATICHASH( Bandages );
	ConfigManager::SetInt( sBandages, m_Bandages, sHUD );

	STATICHASH( MaxBandages );
	ConfigManager::SetInt( sMaxBandages, GetMaxBandages(), sHUD );

	const bool BandagesHelpHidden = pHealth->HasMaxHealth() || ( m_Bandages == 0 );

	{
		STATIC_HASHED_STRING( BandagesHelp );
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sBandagesHelp );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, BandagesHelpHidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}
}

void WBCompRosaMedkit::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Bandages );
	Persistence.SetInt( sBandages, m_Bandages );
}

void WBCompRosaMedkit::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Bandages );
	m_Bandages = Persistence.GetInt( sBandages );

	PublishToHUD();
}

#define VERSION_EMPTY				0
#define VERSION_BANDAGES			1
#define VERSION_MAXBANDAGES_DEPR	3
#define VERSION_CURRENT				3

uint WBCompRosaMedkit::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 4;	// m_Bandages

	return Size;
}

void WBCompRosaMedkit::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.WriteUInt32( m_Bandages );
}

void WBCompRosaMedkit::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_BANDAGES )
	{
		m_Bandages = Stream.ReadUInt32();
	}

	if( Version < VERSION_MAXBANDAGES_DEPR )
	{
		Stream.ReadUInt32();
	}
}
