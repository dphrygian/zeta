#include "core.h"
#include "wbactionrosaplayhandanim.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "rosamesh.h"
#include "Components/wbcompowner.h"

WBActionRosaPlayHandAnim::WBActionRosaPlayHandAnim()
:	m_AnimationName( "" )
{
}

WBActionRosaPlayHandAnim::~WBActionRosaPlayHandAnim()
{
}

/*virtual*/ void WBActionRosaPlayHandAnim::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Animation );
	m_AnimationName = ConfigManager::GetHash( sAnimation, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaPlayHandAnim::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();

	if( pEntity )
	{
		WB_MAKE_EVENT( PlayHandAnim, pEntity );
		WB_SET_AUTO( PlayHandAnim, Hash, AnimationName, m_AnimationName );
		WB_SET_AUTO( PlayHandAnim, Entity, AnimatingEntity, pEntity );
		WB_DISPATCH_EVENT( GetEventManager(), PlayHandAnim, WBCompOwner::GetTopmostOwner( pEntity ) );
	}
}
