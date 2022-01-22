#include "core.h"
#include "wbcomprosasensorpoll.h"
#include "configmanager.h"
#include "mathfunc.h"
#include "mathcore.h"
#include "rosaframework.h"
#include "clock.h"

WBCompRosaSensorPoll::WBCompRosaSensorPoll()
:	m_DoPoll( false )
,	m_TickDeltaMin( 0.0f )
,	m_TickDeltaMax( 0.0f )
,	m_NextTickTime( 0.0f )
{
}

WBCompRosaSensorPoll::~WBCompRosaSensorPoll()
{
}

/*virtual*/ void WBCompRosaSensorPoll::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( DoPoll );
	m_DoPoll = ConfigManager::GetInheritedBool( sDoPoll, true, sDefinitionName );

	STATICHASH( TickDeltaMin );
	m_TickDeltaMin = ConfigManager::GetInheritedFloat( sTickDeltaMin, 0.0f, sDefinitionName );

	STATICHASH( TickDeltaMax );
	m_TickDeltaMax = ConfigManager::GetInheritedFloat( sTickDeltaMax, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaSensorPoll::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	if( m_Paused )
	{
		return;
	}

	if( m_DoPoll )
	{
		const float CurrentTime = GetFramework()->GetClock()->GetGameCurrentTime();
		if( CurrentTime >= m_NextTickTime )
		{
			const float NextDeltaTime	= Math::Random( m_TickDeltaMin, m_TickDeltaMax );
			const float PollDeltaTime	= Max( NextDeltaTime, DeltaTime );	// TickDeltaMin/Max could be set to 0 for "run as fast as possible" and that will screw up PollTick
			m_NextTickTime = CurrentTime + NextDeltaTime;
			PollTick( PollDeltaTime );
		}
	}
}

/*virtual*/ void WBCompRosaSensorPoll::PollTick( const float DeltaTime ) const
{
	Unused( DeltaTime );
}
