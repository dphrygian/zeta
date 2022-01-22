#include "core.h"
#include "rosafactions.h"
#include "map.h"
#include "hashedstring.h"
#include "configmanager.h"

typedef Map<HashedString, RosaFactions::EFactionCon> TFactionConMap;
typedef Map<HashedString, TFactionConMap> TFactionMap;

static TFactionMap	sFactionMap;
static int			sRefCount = 0;

static void StaticInitialize();
static void StaticShutDown();
RosaFactions::EFactionCon StaticGetCon( const HashedString& Con );

void RosaFactions::AddRef()
{
	if( sRefCount++ == 0 )
	{
		StaticInitialize();
	}
}

void RosaFactions::Release()
{
	if( --sRefCount == 0 )
	{
		StaticShutDown();
	}
}

RosaFactions::EFactionCon RosaFactions::GetCon( const HashedString& FactionA, const HashedString& FactionB )
{
	// If faction relationship isn't described, default to being friendly to same faction and neutral to others
	const EFactionCon DefaultCon = ( FactionA == FactionB ) ? EFR_Friendly : EFR_Neutral;

	const TFactionMap::Iterator FactionAIter = sFactionMap.Search( FactionA );
	if( FactionAIter.IsNull() )
	{
		return DefaultCon;
	}

	const TFactionConMap& FactionConMap = FactionAIter.GetValue();
	const TFactionConMap::Iterator FactionBIter = FactionConMap.Search( FactionB );
	if( FactionBIter.IsNull() )
	{
		return DefaultCon;
	}

	return FactionBIter.GetValue();
}

void StaticInitialize()
{
	STATICHASH( RosaFactions );

	STATICHASH( NumFactionCons );
	const uint NumFactionCons = ConfigManager::GetInt( sNumFactionCons, 0, sRosaFactions );

	for( uint FactionConIndex = 0; FactionConIndex < NumFactionCons; ++FactionConIndex )
	{
		const HashedString FactionA = ConfigManager::GetSequenceHash( "FactionCon%dA", FactionConIndex, HashedString::NullString, sRosaFactions );
		const HashedString FactionB = ConfigManager::GetSequenceHash( "FactionCon%dB", FactionConIndex, HashedString::NullString, sRosaFactions );
		const HashedString FactionC = ConfigManager::GetSequenceHash( "FactionCon%dC", FactionConIndex, HashedString::NullString, sRosaFactions );
		RosaFactions::EFactionCon Con = StaticGetCon( FactionC );

		TFactionConMap& FactionConMap = sFactionMap[ FactionA ];
		FactionConMap[ FactionB ] = Con;
	}
}

void StaticShutDown()
{
	FOR_EACH_MAP( FactionAIter, sFactionMap, HashedString, TFactionConMap )
	{
		TFactionConMap& FactionConMap = FactionAIter.GetValue();
		FactionConMap.Clear();
	}
	sFactionMap.Clear();
}

RosaFactions::EFactionCon StaticGetCon( const HashedString& Con )
{
	STATIC_HASHED_STRING( Hostile );
	STATIC_HASHED_STRING( Neutral );
	STATIC_HASHED_STRING( Friendly );

	if( Con == sHostile )
	{
		return RosaFactions::EFR_Hostile;
	}
	else if( Con == sNeutral )
	{
		return RosaFactions::EFR_Neutral;
	}
	else if( Con == sFriendly )
	{
		return RosaFactions::EFR_Friendly;
	}
	else
	{
		WARN;
		return RosaFactions::EFR_Neutral;
	}
}
