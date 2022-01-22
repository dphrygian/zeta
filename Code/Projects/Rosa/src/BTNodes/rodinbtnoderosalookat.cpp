#include "core.h"
#include "rodinbtnoderosalookat.h"
#include "configmanager.h"
#include "Components/wbcomprodinblackboard.h"
#include "wbeventmanager.h"

RodinBTNodeRosaLookAt::RodinBTNodeRosaLookAt()
:	m_LookTargetBlackboardKey()
{
}

RodinBTNodeRosaLookAt::~RodinBTNodeRosaLookAt()
{
}

void RodinBTNodeRosaLookAt::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( LookTargetBlackboardKey );
	m_LookTargetBlackboardKey = ConfigManager::GetHash( sLookTargetBlackboardKey, HashedString::NullString, sDefinitionName );
}

RodinBTNode::ETickStatus RodinBTNodeRosaLookAt::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	WBEntity* const		pEntity		= GetEntity();

	WBCompRodinBlackboard* const	pAIBlackboard	= WB_GETCOMP( pEntity, RodinBlackboard );
	ASSERT( pAIBlackboard );

	const WBEvent::EType			TargetType		= pAIBlackboard->GetType( m_LookTargetBlackboardKey );

	if( TargetType == WBEvent::EWBEPT_Vector )
	{
		const Vector LookTarget = pAIBlackboard->GetVector( m_LookTargetBlackboardKey );

		WB_MAKE_EVENT( LookAt, pEntity );
		WB_SET_AUTO( LookAt, Vector, LookAtLocation, LookTarget );
		WB_DISPATCH_EVENT( GetEventManager(), LookAt, pEntity );

		return ETS_Success;
	}
	else if( TargetType == WBEvent::EWBEPT_Entity )
	{
		WBEntity* const pLookTargetEntity = pAIBlackboard->GetEntity( m_LookTargetBlackboardKey );
		if( !pLookTargetEntity )
		{
			return ETS_Fail;
		}

		WB_MAKE_EVENT( LookAt, pEntity );
		WB_SET_AUTO( LookAt, Entity, LookAtEntity, pLookTargetEntity );
		WB_DISPATCH_EVENT( GetEventManager(), LookAt, pEntity );

		return ETS_Success;
	}

	return ETS_Fail;
}
