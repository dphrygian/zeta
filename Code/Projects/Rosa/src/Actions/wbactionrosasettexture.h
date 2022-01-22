#ifndef WBACTIONROSASETTEXTURE_H
#define WBACTIONROSASETTEXTURE_H

#include "wbaction.h"

class WBActionRosaSetTexture : public WBAction
{
public:
	WBActionRosaSetTexture();
	virtual ~WBActionRosaSetTexture();

	DEFINE_WBACTION_FACTORY( RosaSetTexture );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString	m_Texture;
};

#endif // WBACTIONROSASETTEXTURE_H
