#ifndef IACHIEVEMENTMANAGER_H
#define IACHIEVEMENTMANAGER_H

#include "rosaframework.h"

class SimpleString;

#define DO_ACHIEVEMENTS_IN_DEV	1
#define DO_ACHIEVEMENTS			DO_ACHIEVEMENTS_IN_DEV || BUILD_FINAL

#if DO_ACHIEVEMENTS
#define AWARD_ACHIEVEMENT( ach ) \
	IAchievementManager* const pAchievementManager = RosaFramework::GetInstance()->GetAchievementManager(); \
	ASSERTDESC( NULL != pAchievementManager, "AchievementManager did not exist when achievement was awarded!" ); \
	if( pAchievementManager ) { pAchievementManager->AwardAchievement( ( ach ) ); }
#else
#define AWARD_ACHIEVEMENT( ach ) DoNothing
#endif

#if DO_ACHIEVEMENTS
#define INCREMENT_STAT( stat, amount ) \
	IAchievementManager* const pAchievementManager = RosaFramework::GetInstance()->GetAchievementManager(); \
	DEVASSERTDESC( NULL != pAchievementManager, "AchievementManager did not exist when stat was incremented!" ); \
	if( pAchievementManager ) { pAchievementManager->IncrementStat( ( stat ), ( amount ) ); }
#define SET_STAT( stat, value ) \
	IAchievementManager* const pAchievementManager = RosaFramework::GetInstance()->GetAchievementManager(); \
	DEVASSERTDESC( NULL != pAchievementManager, "AchievementManager did not exist when stat was set!" ); \
	if( pAchievementManager ) { pAchievementManager->SetStat( ( stat ), ( value ) ); }
#else
#define INCREMENT_STAT( stat, amount ) DoNothing
#define SET_STAT( stat, value ) DoNothing
#endif

class IAchievementManager
{
public:
	virtual ~IAchievementManager()
	{
	}

	virtual void	RequestServerUpdate() = 0;

	virtual void	AwardAchievement( const SimpleString& AchievementTag ) = 0;
	virtual void	IncrementStat( const SimpleString& StatTag, const int Amount ) = 0;
	virtual void	SetStat( const SimpleString& StatTag, const int Value ) = 0;
	virtual void	Store() = 0;

#if BUILD_DEV
	virtual void	ResetAllStats( const bool ResetAchievements ) = 0;
#endif

	virtual void	ReportGlobalAchievementRate( const SimpleString& AchievementTag ) = 0;
	virtual void	ReportGlobalStat( const SimpleString& StatTag ) = 0;
};

#endif // IACHIEVEMENTMANAGER_H
