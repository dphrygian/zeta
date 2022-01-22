#include "core.h"
#include "wbactionrosaselectcharacterpreset.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosagame.h"

WBActionRosaSelectCharacterPreset::WBActionRosaSelectCharacterPreset()
:	m_SkinPreset( 0 )
,	m_NailsPreset( 0 )
{
}

WBActionRosaSelectCharacterPreset::~WBActionRosaSelectCharacterPreset()
{
}

/*virtual*/ void WBActionRosaSelectCharacterPreset::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( SkinPreset );
	m_SkinPreset = ConfigManager::GetInt( sSkinPreset, -1, sDefinitionName );

	STATICHASH( NailsPreset );
	m_NailsPreset = ConfigManager::GetInt( sNailsPreset, -1, sDefinitionName );
}

/*virtual*/ void WBActionRosaSelectCharacterPreset::Execute()
{
	WBAction::Execute();

	WB_MAKE_EVENT( SelectCharacterPreset, NULL );
	WB_SET_AUTO( SelectCharacterPreset, Int, SkinPresetIndex, m_SkinPreset );
	WB_SET_AUTO( SelectCharacterPreset, Int, NailsPresetIndex, m_NailsPreset );
	WB_DISPATCH_EVENT( GetEventManager(), SelectCharacterPreset, RosaGame::GetPlayer() );
}
