#ifndef WBACTIONROSACHECKLINE_H
#define WBACTIONROSACHECKLINE_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class Vector;
class Angles;

class WBActionRosaCheckLine : public WBAction
{
public:
	WBActionRosaCheckLine();
	virtual ~WBActionRosaCheckLine();

	DEFINE_WBACTION_FACTORY( RosaCheckLine );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	void			GetLineTransform( WBEntity* const pEntity, Vector& OutLocation, Angles& OutOrientation );
	void			ExecuteSingleTrace();
	void			ExecuteBlastTraces();

	WBParamEvaluator	m_EntityPE;		// Source entity for the check; defaults to topmost owner
	WBParamEvaluator	m_LineStartPE;
	WBParamEvaluator	m_LineDirectionPE;
	float				m_LineLength;
	WBParamEvaluator	m_LineLengthPE;
	HashedString		m_CheckTag;
	bool				m_CollideEntities;

	// Now supporting shotgun style "blast" traces
	WBParamEvaluator	m_BlastNumTracesPE;
	WBParamEvaluator	m_BlastAnglePE;
};

#endif // WBACTIONROSACHECKLINE_H
