#ifndef ROSATARGETMANAGER_H
#define ROSATARGETMANAGER_H

#include "targetmanager.h"

class RosaTargetManager : public TargetManager
{
public:
	RosaTargetManager( IRenderer* const pRenderer );
	virtual ~RosaTargetManager();

	virtual void	CreateTargets( const uint DisplayWidth, const uint DisplayHeight );
};

#endif // ROSATARGETMANAGER_H
