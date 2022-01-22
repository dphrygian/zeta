#include "core.h"
#include "wbrosacomponent.h"
#include "rosaframework.h"
#include "rosacampaign.h"
#include "wbworld.h"

WBRosaComponent::WBRosaComponent()
{
}

WBRosaComponent::~WBRosaComponent()
{
}

RosaFramework* WBRosaComponent::GetFramework() const
{
	return RosaFramework::GetInstance();
}

RosaGame* WBRosaComponent::GetGame() const
{
	return RosaFramework::GetInstance()->GetGame();
}

RosaWorld* WBRosaComponent::GetWorld() const
{
	return RosaFramework::GetInstance()->GetWorld();
}

RosaCampaign* WBRosaComponent::GetCampaign() const
{
	return RosaCampaign::GetCampaign();
}
