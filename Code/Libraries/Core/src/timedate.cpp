#include "core.h"
#include "timedate.h"

timedate_tm_t TimeDate::GetLocalTime()
{
	timedate_time_t	Time;
	timedate_tm_t	TimeStruct;

#if BUILD_WINDOWS
	_tzset();	// Apply environment variables so that _localtime_s will convert properly
	_time64( &Time );
	_localtime64_s( &TimeStruct, &Time );
#elif BUILD_LINUX || BUILD_MAC
    tzset();
    time( &Time );
    localtime_r( &Time, &TimeStruct );
#else
	#error timedate.cpp not updated for this platform
#endif

	return TimeStruct;
}

int TimeDate::GetHours12( const timedate_tm_t& CurrentLocalTime )
{
	int Hours = CurrentLocalTime.tm_hour % 12;
	if( Hours == 0 )
	{
		Hours = 12;
	}
	return Hours;
}

int TimeDate::GetHours24( const timedate_tm_t& CurrentLocalTime )
{
	return CurrentLocalTime.tm_hour;
}

int TimeDate::GetAMPM( const timedate_tm_t& CurrentLocalTime )
{
	int Hours = CurrentLocalTime.tm_hour;
	if( Hours < 12 )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int TimeDate::GetMinutes( const timedate_tm_t& CurrentLocalTime )
{
	return CurrentLocalTime.tm_min;
}

int TimeDate::GetSeconds( const timedate_tm_t& CurrentLocalTime )
{
	return CurrentLocalTime.tm_sec;
}

int TimeDate::GetDay( const timedate_tm_t& CurrentLocalTime )
{
	return CurrentLocalTime.tm_mday;
}

int TimeDate::GetMonth( const timedate_tm_t& CurrentLocalTime )
{
	return CurrentLocalTime.tm_mon;
}

int TimeDate::GetYear( const timedate_tm_t& CurrentLocalTime )
{
	return CurrentLocalTime.tm_year + 1900;
}

// If a localized month is needed, pass the return
// value to ConfigManager::GetLocalizedString
SimpleString TimeDate::GetMonthName( int Month )
{
	switch( Month )
	{
	case 0:
		return "January";
	case 1:
		return "February";
	case 2:
		return "March";
	case 3:
		return "April";
	case 4:
		return "May";
	case 5:
		return "June";
	case 6:
		return "July";
	case 7:
		return "August";
	case 8:
		return "September";
	case 9:
		return "October";
	case 10:
		return "November";
	case 11:
		return "December";
	default:
		WARNDESC( "Invalid month." );
		return "";
	}
}

SimpleString TimeDate::GetTimeDateString()
{
	const timedate_tm_t CurrentLocalTime = GetLocalTime();
	return SimpleString::PrintF(
		"%04d-%02d-%02d-%02d-%02d-%02d",
		GetYear( CurrentLocalTime ),
		GetMonth( CurrentLocalTime ) + 1,
		GetDay( CurrentLocalTime ),
		GetHours24( CurrentLocalTime ),
		GetMinutes( CurrentLocalTime ),
		GetSeconds( CurrentLocalTime ) );
}
