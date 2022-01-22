#include "core.h"
#include "wbactiontriggerstatmod.h"
#include "configmanager.h"
#include "Components/wbcompstatmod.h"
#include "wbentity.h"

WBActionTriggerStatMod::WBActionTriggerStatMod()
:	m_StatModEvent()
,	m_Trigger( false )
,	m_RefCount( false )
{
}

WBActionTriggerStatMod::~WBActionTriggerStatMod()
{
}

/*virtual*/ void WBActionTriggerStatMod::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( StatModEvent );
	m_StatModEvent = ConfigManager::GetHash( sStatModEvent, HashedString::NullString, sDefinitionName );

	STATICHASH( Trigger );
	m_Trigger = ConfigManager::GetBool( sTrigger, true, sDefinitionName );

	STATICHASH( RefCount );
	m_RefCount = ConfigManager::GetBool( sRefCount, true, sDefinitionName );
}

/*virtual*/ void WBActionTriggerStatMod::Execute()
{
	WBAction::Execute();

	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	ASSERT( pStatMod );

	if( m_RefCount )
	{
		if( m_Trigger )
		{
			pStatMod->AddRefEvent( m_StatModEvent );
		}
		else
		{
			pStatMod->ReleaseEvent( m_StatModEvent );
		}
	}
	else
	{
		pStatMod->SetEventActive( m_StatModEvent, m_Trigger );
	}
}
