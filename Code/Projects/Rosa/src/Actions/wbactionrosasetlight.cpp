#include "core.h"
#include "wbactionrosasetlight.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"

WBActionRosaSetLight::WBActionRosaSetLight()
:	m_AddLight( false )
,	m_ReAddLight( false )
{
}

WBActionRosaSetLight::~WBActionRosaSetLight()
{
}

/*virtual*/ void WBActionRosaSetLight::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AddLight );
	m_AddLight = ConfigManager::GetBool( sAddLight, false, sDefinitionName );

	STATICHASH( ReAddLight );
	m_ReAddLight = ConfigManager::GetBool( sReAddLight, false, sDefinitionName );
}

/*virtual*/ void WBActionRosaSetLight::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();

	if( pEntity )
	{
		if( m_AddLight )
		{
			// Queue because if we do this during spawning, the world won't accept a light yet.
			WB_MAKE_EVENT( AddLight, pEntity );
			WB_QUEUE_EVENT( GetEventManager(), AddLight, pEntity );
		}
		else if( m_ReAddLight )
		{
			WB_MAKE_EVENT( ReAddLight, pEntity );
			WB_QUEUE_EVENT( GetEventManager(), ReAddLight, pEntity );
		}
		else
		{
			WB_MAKE_EVENT( RemoveLight, pEntity );
			WB_DISPATCH_EVENT( GetEventManager(), RemoveLight, pEntity );
		}
	}
}
