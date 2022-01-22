#include "core.h"
#include "wbactionrosaawardachievement.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "Achievements/iachievementmanager.h"

WBActionRosaAwardAchievement::WBActionRosaAwardAchievement()
:	m_AchievementTag()
{
}

WBActionRosaAwardAchievement::~WBActionRosaAwardAchievement()
{
}

/*virtual*/ void WBActionRosaAwardAchievement::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AchievementTag );
	m_AchievementTag = ConfigManager::GetString( sAchievementTag, "", sDefinitionName );
}

/*virtual*/ void WBActionRosaAwardAchievement::Execute()
{
	WBAction::Execute();

	// ROSATODO: Do some validation so users can't easily make mods to award achievements.
	AWARD_ACHIEVEMENT( m_AchievementTag );
}
