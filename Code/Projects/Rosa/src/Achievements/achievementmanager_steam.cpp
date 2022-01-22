#include "core.h"
#include "achievementmanager_steam.h"
#include "simplestring.h"

AchievementManager_Steam::AchievementManager_Steam()
:	m_UserStats( NULL )
#if BUILD_STEAM
,	m_CallbackUserStatsReceived( this, &AchievementManager_Steam::OnUserStatsReceived )
,	m_CallResultGlobalStatsReceived()
,	m_CallResultGlobalAchievementRatesReceived()
#endif // BUILD_STEAM
{
#if BUILD_STEAM
	m_UserStats = SteamUserStats();
#endif // BUILD_STEAM
	RequestServerUpdate();
}

/*virtual*/ AchievementManager_Steam::~AchievementManager_Steam()
{
}

/*virtual*/ void AchievementManager_Steam::RequestServerUpdate()
{
#if BUILD_STEAM
	RequestUserStats();
	RequestGlobalStats();
	RequestGlobalAchievementRates();
#endif // BUILD_STEAM
}

/* virtual*/ void AchievementManager_Steam::AwardAchievement( const SimpleString& AchievementTag )
{
	Unused( AchievementTag );

#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	const bool AchievementSet	= m_UserStats->SetAchievement( AchievementTag.CStr() );
	ASSERT( AchievementSet );

	if( !AchievementSet )
	{
		return;
	}

	Store();
#endif // BUILD_STEAM
}

/*virtual*/ void AchievementManager_Steam::IncrementStat( const SimpleString& StatTag, const int Amount )
{
	Unused( StatTag );
	Unused( Amount );

#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	int32 CurrentStatValue;
	const bool GotStat = m_UserStats->GetStat( StatTag.CStr(), &CurrentStatValue );
	ASSERT( GotStat );

	if( !GotStat )
	{
		return;
	}

	const int32 NewStatValue = CurrentStatValue + Amount;
	const bool SetStat	= m_UserStats->SetStat( StatTag.CStr(), NewStatValue );
	ASSERT( SetStat );
#endif // BUILD_STEAM
}

/*virtual*/ void AchievementManager_Steam::SetStat( const SimpleString& StatTag, const int Value )
{
	Unused( StatTag );
	Unused( Value );

#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	const int32 NewStatValue = Value;
	const bool SetStat	= m_UserStats->SetStat( StatTag.CStr(), NewStatValue );
	ASSERT( SetStat );
#endif // BUILD_STEAM
}

/*virtual*/ void AchievementManager_Steam::Store()
{
#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	const bool StatsStored	= m_UserStats->StoreStats();
	ASSERT( StatsStored );
#endif // BUILD_STEAM
}

#if BUILD_DEV
/*virtual*/ void AchievementManager_Steam::ResetAllStats( const bool ResetAchievements )
{
	Unused( ResetAchievements );

#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	const bool StatsReset	= m_UserStats->ResetAllStats( ResetAchievements );
	ASSERT( StatsReset );
#endif
}
#endif

/*virtual*/ void AchievementManager_Steam::ReportGlobalStat( const SimpleString& StatTag )
{
	Unused( StatTag );

#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	int64 GlobalStatValue;
	const bool GotStat = m_UserStats->GetGlobalStat( StatTag.CStr(), &GlobalStatValue );

	if( !GotStat )
	{
		PRINTF( "  %s: failed get\n", StatTag.CStr() );
		return;
	}

	// Convert to long long for consistency in printing. On Windows, Steam declares
	// int64 as __int64, which should be printed with %I64d, but that is nonstandard.
	const long long LongStat = GlobalStatValue;
	PRINTF( "  %s: %lld\n", StatTag.CStr(), LongStat );
#endif // BUILD_STEAM
}

/*virtual*/ void AchievementManager_Steam::ReportGlobalAchievementRate( const SimpleString& AchievementTag )
{
	Unused( AchievementTag );

#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	float AchievementRate;
	const bool GotAchievementRate = m_UserStats->GetAchievementAchievedPercent( AchievementTag.CStr(), &AchievementRate );

	if( !GotAchievementRate )
	{
		PRINTF( "  %s: failed get\n", AchievementTag.CStr() );
		return;
	}

	PRINTF( "  %s: %.2f\n", AchievementTag.CStr(), AchievementRate );
#endif // BUILD_STEAM
}

void AchievementManager_Steam::RequestUserStats()
{
#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	const bool RequestedStats	= m_UserStats->RequestCurrentStats();
	ASSERT( RequestedStats );
#endif // BUILD_STEAM
}

void AchievementManager_Steam::RequestGlobalStats()
{
#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	SteamAPICall_t APICall = m_UserStats->RequestGlobalStats( 0 );
	m_CallResultGlobalStatsReceived.Set( APICall, this, &AchievementManager_Steam::OnGlobalStatsReceived );
#endif // BUILD_STEAM
}

void AchievementManager_Steam::RequestGlobalAchievementRates()
{
#if BUILD_STEAM
	if( !m_UserStats )
	{
		return;
	}

	SteamAPICall_t APICall = m_UserStats->RequestGlobalAchievementPercentages();
	m_CallResultGlobalAchievementRatesReceived.Set( APICall, this, &AchievementManager_Steam::OnGlobalAchievementRatesReceived );
#endif // BUILD_STEAM
}

#if BUILD_STEAM
void AchievementManager_Steam::OnUserStatsReceived( UserStatsReceived_t* pParam )
{
	if( !m_UserStats )
	{
		return;
	}

	ISteamUtils* const	pSteamUtils = SteamUtils();
	const uint32		AppID		= pSteamUtils ? pSteamUtils->GetAppID() : k_uAppIdInvalid;
	if( AppID != pParam->m_nGameID )	// Make sure the stats are for this game
	{
		return;
	}

	if( k_EResultOK != pParam->m_eResult )
	{
		PRINTF( "Steam RequestUserStats failed.\n" );
	}
}
#endif // BUILD_STEAM

#if BUILD_STEAM
void AchievementManager_Steam::OnGlobalStatsReceived( GlobalStatsReceived_t* pParam, bool bIOFailure )
{
	if( !m_UserStats )
	{
		return;
	}

	if( bIOFailure )
	{
		return;
	}

	ISteamUtils* const	pSteamUtils = SteamUtils();
	const uint32		AppID		= pSteamUtils ? pSteamUtils->GetAppID() : k_uAppIdInvalid;
	if( AppID != pParam->m_nGameID )	// Make sure the stats are for this game
	{
		return;
	}

	if( k_EResultOK != pParam->m_eResult )
	{
		PRINTF( "Steam RequestGlobalStats failed.\n" );
	}
}
#endif // BUILD_STEAM

#if BUILD_STEAM
void AchievementManager_Steam::OnGlobalAchievementRatesReceived( GlobalAchievementPercentagesReady_t* pParam, bool bIOFailure )
{
	if( !m_UserStats )
	{
		return;
	}

	if( bIOFailure )
	{
		return;
	}

	ISteamUtils* const	pSteamUtils = SteamUtils();
	const uint32		AppID		= pSteamUtils ? pSteamUtils->GetAppID() : k_uAppIdInvalid;
	if( AppID != pParam->m_nGameID )	// Make sure the stats are for this game
	{
		return;
	}

	if( k_EResultOK != pParam->m_eResult )
	{
		PRINTF( "Steam RequestGlobalAchievementPercentages failed.\n" );
	}
}
#endif // BUILD_STEAM
