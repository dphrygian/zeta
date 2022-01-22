#ifndef WBACTIONROSACHECKSPHERE_H
#define WBACTIONROSACHECKSPHERE_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class Vector;

class WBActionRosaCheckSphere : public WBAction
{
public:
	WBActionRosaCheckSphere();
	virtual ~WBActionRosaCheckSphere();

	DEFINE_WBACTION_FACTORY( RosaCheckSphere );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	void			GetSphereTransform( WBEntity* const pEntity, Vector& OutLocation ) const;

	WBParamEvaluator	m_EntityPE;		// Source entity for the check; defaults to topmost owner
	float				m_RadiusSq;
	float				m_RadiusRcp;
	bool				m_TraceCenter;	// If true, trace to the center of the entity's bounds (typically the same as transform location) instead of nearest point to source
	HashedString		m_CheckTag;
};

#endif // WBACTIONROSACHECKSPHERE_H
