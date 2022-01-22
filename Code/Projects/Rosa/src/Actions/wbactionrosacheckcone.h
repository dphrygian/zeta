#ifndef WBACTIONROSACHECKCONE_H
#define WBACTIONROSACHECKCONE_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class Vector;
class Angles;

class WBActionRosaCheckCone : public WBAction
{
public:
	WBActionRosaCheckCone();
	virtual ~WBActionRosaCheckCone();

	DEFINE_WBACTION_FACTORY( RosaCheckCone );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	void			GetConeTransform( WBEntity* const pEntity, Vector& OutLocation, Angles& OutOrientation ) const;

	WBParamEvaluator	m_EntityPE;		// Source entity for the check; defaults to topmost owner
	float				m_ConeCosTheta;
	float				m_ConeLengthSq;
	bool				m_TraceCenter;	// If true, trace to the center of the entity's bounds (typically the same as transform location) instead of nearest point to source
	HashedString		m_CheckTag;
};

#endif // WBACTIONROSACHECKCONE_H
