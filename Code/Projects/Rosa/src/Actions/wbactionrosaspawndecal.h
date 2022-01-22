#ifndef WBACTIONROSASPAWNDECAL_H
#define WBACTIONROSASPAWNDECAL_H

#include "wbactionrosaspawnentity.h"

class WBActionRosaSpawnDecal : public WBActionRosaSpawnEntity
{
public:
	WBActionRosaSpawnDecal();
	virtual ~WBActionRosaSpawnDecal();

	DEFINE_WBACTION_FACTORY( RosaSpawnDecal );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

protected:
	virtual void	PostSpawn( WBEntity* const pSpawnedEntity );

private:
	WBParamEvaluator	m_NormalBasisPE;
	WBParamEvaluator	m_RollPE;
};

#endif // WBACTIONROSASPAWNDECAL_H
