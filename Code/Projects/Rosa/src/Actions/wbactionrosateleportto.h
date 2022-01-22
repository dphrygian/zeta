#ifndef WBACTIONROSATELEPORTTO_H
#define WBACTIONROSATELEPORTTO_H

#include "wbaction.h"
#include "wbparamevaluator.h"

class WBActionRosaTeleportTo : public WBAction
{
public:
	WBActionRosaTeleportTo();
	virtual ~WBActionRosaTeleportTo();

	DEFINE_WBACTION_FACTORY( RosaTeleportTo );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	WBParamEvaluator	m_EntityPE;
	WBParamEvaluator	m_DestinationPE;

	bool				m_SetOrientation;
};

#endif // WBACTIONROSATELEPORTTO_H
