#ifndef WBCOMPROSA_H
#define WBCOMPROSA_H

#include "wbcomponent.h"

class RosaFramework;
class RosaGame;
class RosaWorld;
class RosaCampaign;

class WBRosaComponent : public WBComponent
{
public:
	WBRosaComponent();
	virtual ~WBRosaComponent();

protected:
	RosaFramework*	GetFramework() const;
	RosaGame*		GetGame() const;
	RosaWorld*		GetWorld() const;
	RosaCampaign*	GetCampaign() const;
};

#endif // WBCOMPROSA_H
