#ifndef WBACTIONROSASPAWNENTITY_H
#define WBACTIONROSASPAWNENTITY_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class WBActionRosaSpawnEntity : public WBAction
{
public:
	WBActionRosaSpawnEntity();
	virtual ~WBActionRosaSpawnEntity();

	DEFINE_WBACTION_FACTORY( RosaSpawnEntity );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

protected:
	virtual void	PostSpawn( WBEntity* const pSpawnedEntity );

private:
	void			GetSpawnTransform( WBEntity* const pSpawnedEntity, Vector& OutLocation, Vector& OutImpulse, Angles& OutOrientation );

	SimpleString		m_EntityDef;
	WBParamEvaluator	m_EntityDefPE;

	// If overrides are not present, the active entity's transform is used.
	WBParamEvaluator	m_LocationPE;
	WBParamEvaluator	m_OrientationPE;

	bool				m_UseHeadTracker;
	bool				m_YawOnly;

	float				m_SpawnImpulseZ;
	float				m_SpawnImpulse;
	WBParamEvaluator	m_SpawnImpulsePE;

	float				m_SpawnOffsetZ;
};

#endif // WBACTIONROSASPAWNENTITY_H
