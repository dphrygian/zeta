#ifndef WBACTIONROSASETLIGHT_H
#define WBACTIONROSASETLIGHT_H

#include "wbaction.h"

class WBActionRosaSetLight : public WBAction
{
public:
	WBActionRosaSetLight();
	virtual ~WBActionRosaSetLight();

	DEFINE_WBACTION_FACTORY( RosaSetLight );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	bool			m_AddLight;
	bool			m_ReAddLight;
};

#endif // WBACTIONROSAPLAYANIM_H
