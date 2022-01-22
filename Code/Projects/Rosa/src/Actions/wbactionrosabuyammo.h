#ifndef WBACTIONROSABUYAMMO_H
#define WBACTIONROSABUYAMMO_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaBuyAmmo : public WBAction
{
public:
	WBActionRosaBuyAmmo();
	virtual ~WBActionRosaBuyAmmo();

	DEFINE_WBACTION_FACTORY( RosaBuyAmmo );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString	m_AmmoType;	// If null, this action should attempt to refill ammo
};

#endif // WBACTIONROSABUYAMMO_H
