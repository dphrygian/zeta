#ifndef ACHIEVEMENTMANAGER_STEAM_H
#define ACHIEVEMENTMANAGER_STEAM_H

#include "iachievementmanager.h"
#include "hashedstring.h"
#include "map.h"

class ISteamUserStats;

#if BUILD_STEAM
#include "steam/steam_api.h"
#endif

class AchievementManager_Steam : public IAchievementManager
{
public:
	AchievementManager_Steam();
	virtual ~AchievementManager_Steam();

	virtual void	RequestServerUpdate();

	virtual void	AwardAchievement( const SimpleString& AchievementTag );
	virtual void	IncrementStat( const SimpleString& StatTag, const int Amount );
	virtual void	SetStat( const SimpleString& StatTag, const int Value );
	virtual void	Store();

#if BUILD_DEV
	virtual void	ResetAllStats( const bool ResetAchievements );
#endif

	virtual void	ReportGlobalStat( const SimpleString& StatTag );
	virtual void	ReportGlobalAchievementRate( const SimpleString& AchievementTag );

protected:
	void				RequestUserStats();
	void				RequestGlobalStats();
	void				RequestGlobalAchievementRates();

	ISteamUserStats*	m_UserStats;

public:
#if BUILD_STEAM
	STEAM_CALLBACK( AchievementManager_Steam, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );

	void OnGlobalStatsReceived( GlobalStatsReceived_t* pParam, bool bIOFailure );
	CCallResult<AchievementManager_Steam, GlobalStatsReceived_t> m_CallResultGlobalStatsReceived;

	void OnGlobalAchievementRatesReceived( GlobalAchievementPercentagesReady_t* pParam, bool bIOFailure );
	CCallResult<AchievementManager_Steam, GlobalAchievementPercentagesReady_t> m_CallResultGlobalAchievementRatesReceived;
#endif
};

#endif // ACHIEVEMENTMANAGER_STEAM_H
