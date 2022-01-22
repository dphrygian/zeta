#include "core.h"
#include "wbcomprosasensordamage.h"
#include "wbevent.h"
#include "Components/wbcomprodinknowledge.h"
#include "wbcomprosatransform.h"

WBCompRosaSensorDamage::WBCompRosaSensorDamage()
{
}

WBCompRosaSensorDamage::~WBCompRosaSensorDamage()
{
}

/*virtual*/ void WBCompRosaSensorDamage::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnDamaged );
	STATIC_HASHED_STRING( OnDebuffed );
	STATIC_HASHED_STRING( NotifyDamageSensor );	// HACKHACK for all other purposes

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnDamaged || EventName == sOnDebuffed || EventName == sNotifyDamageSensor )
	{
		STATIC_HASHED_STRING( Damager );
		WBEntity* const pDamager = Event.GetEntity( sDamager );
		DEVASSERT( pDamager );

		HandleDamage( pDamager );
	}
}

// TODO: Account for deferred damage like trap bolts.
// Maybe just make that a parameter of damage dealer, that it doesn't trigger sensor, because how could the AI know anything about it?
void WBCompRosaSensorDamage::HandleDamage( WBEntity* const pDamager ) const
{
	XTRACE_FUNCTION;

	if( !pDamager )
	{
		return;
	}

	WBCompRosaTransform* const			pTransform		= pDamager->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRodinKnowledge* const			pKnowledge		= WB_GETCOMP( GetEntity(), RodinKnowledge );
	ASSERT( pKnowledge );

	WBCompRodinKnowledge::TKnowledge&	Knowledge		= pKnowledge->UpdateEntity( pDamager );

	STATIC_HASHED_STRING( RegardAsHostile );
	Knowledge.SetBool( sRegardAsHostile, true );

	STATIC_HASHED_STRING( IsDamager );
	Knowledge.SetBool( sIsDamager, true );

	STATIC_HASHED_STRING( LastKnownLocation );
	Knowledge.SetVector( sLastKnownLocation, pTransform->GetLocation() );
	ASSERT( !pTransform->GetLocation().IsZero() );

	STATIC_HASHED_STRING( KnowledgeType );
	STATIC_HASHED_STRING( Target );
	Knowledge.SetHash( sKnowledgeType, sTarget );
}
