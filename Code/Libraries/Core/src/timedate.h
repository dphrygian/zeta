#ifndef TIMEDATE_H
#define TIMEDATE_H

#include "simplestring.h"

#if BUILD_WINDOWS
	#include <time.h>
	typedef __time64_t	timedate_time_t;
	typedef struct tm	timedate_tm_t;
#elif BUILD_LINUX || BUILD_MAC
    #include <time.h>
	typedef time_t	    timedate_time_t;
	typedef struct tm	timedate_tm_t;
#else
	#error timedate.h not updated for this platform
#endif

namespace TimeDate
{
	timedate_tm_t GetLocalTime();

	int	GetHours12( const timedate_tm_t& CurrentLocalTime );	// 0-11
	int	GetHours24( const timedate_tm_t& CurrentLocalTime );	// 0-23
	int	GetAMPM( const timedate_tm_t& CurrentLocalTime );		// 0 = AM, 1 = PM
	int	GetMinutes( const timedate_tm_t& CurrentLocalTime );	// 0-59
	int	GetSeconds( const timedate_tm_t& CurrentLocalTime );	// 0-59
	int GetDay( const timedate_tm_t& CurrentLocalTime );		// 0-31
	int GetMonth( const timedate_tm_t& CurrentLocalTime );		// 0-11
	int GetYear( const timedate_tm_t& CurrentLocalTime );		// Actual year AD

	SimpleString GetMonthName( int Month );

	SimpleString GetTimeDateString();	// Format: yyyy-mm-dd-HH-MM-SS
}

#endif // TIMEDATE_H
