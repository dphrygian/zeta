#include "core.h"
#include "wbcomprosacollectible.h"
#include "wbevent.h"
#include "rosahudlog.h"
#include "wbcomponentarrays.h"
#include "configmanager.h"
#include "stringmanager.h"
#include "rosagame.h"
#include "Components/wbcompvariablemap.h"
#include "Achievements/iachievementmanager.h"
#include "rosaframework.h"

WBCompRosaCollectible::WBCompRosaCollectible()
{
}

WBCompRosaCollectible::~WBCompRosaCollectible()
{
}

/*virtual*/ void WBCompRosaCollectible::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnFrobbed );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnFrobbed )
	{
		// Increment local collectibles count and notify player of how many caches are left to find.
		// Global collectibles count is stored in RosaPersistence so it persists between levels;
		// that's done in script (see [IncrementGeocacheCount]).
		WBEntity* const		pPlayer				= RosaGame::GetPlayer();
		WBCompVariableMap*	pVarMap				= WB_GETCOMP( pPlayer, VariableMap );
		WBEvent&			VarMap				= pVarMap->GetVariables();

		STATIC_HASHED_STRING( GeocacheCount );
		const uint			FoundCollectibles	= VarMap.GetInt( sGeocacheCount ) + 1;
		VarMap.SetInt( sGeocacheCount, FoundCollectibles );

		const Array<WBCompRosaCollectible*>* pCollectibleComponents = WBComponentArrays::GetComponents<WBCompRosaCollectible>();
		ASSERT( pCollectibleComponents );

		const uint TotalCollectibles = pCollectibleComponents->Size();

		STATICHASH( GeocachePickup );
		STATICHASH( Found );
		ConfigManager::SetInt( sFound, FoundCollectibles, sGeocachePickup );

		STATICHASH( Total );
		ConfigManager::SetInt( sTotal, TotalCollectibles, sGeocachePickup );

		STATICHASH( Remaining );
		ConfigManager::SetInt( sRemaining, TotalCollectibles - FoundCollectibles, sGeocachePickup );

		RosaHUDLog::StaticAddDynamicMessage( sGeocachePickup );

		INCREMENT_STAT( "NumGeocachesSigned", 1 );
	}
}
