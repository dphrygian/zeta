#ifndef WBACTIONROSABINDINPUT_H
#define WBACTIONROSABINDINPUT_H

#include "wbaction.h"
#include "simplestring.h"

class WBActionRosaBindInput : public WBAction
{
public:
	WBActionRosaBindInput();
	virtual ~WBActionRosaBindInput();

	DEFINE_WBACTION_FACTORY( RosaBindInput );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	SimpleString	m_Input;
};

#endif // WBACTIONROSABINDINPUT_H
