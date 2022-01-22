#ifndef WBACTIONROSASETLIGHTCOLOR_H
#define WBACTIONROSASETLIGHTCOLOR_H

#include "wbaction.h"

class WBActionRosaSetLightColor : public WBAction
{
public:
	WBActionRosaSetLightColor();
	virtual ~WBActionRosaSetLightColor();

	DEFINE_WBACTION_FACTORY( RosaSetLightColor );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	float	m_ColorH;
	float	m_ColorS;
	float	m_ColorV;
};

#endif // WBACTIONROSASETLIGHTCOLOR_H
