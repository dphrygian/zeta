#ifndef WBACTIONROSAPLAYHANDANIM_H
#define WBACTIONROSAPLAYHANDANIM_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaPlayHandAnim : public WBAction
{
public:
	WBActionRosaPlayHandAnim();
	virtual ~WBActionRosaPlayHandAnim();

	DEFINE_WBACTION_FACTORY( RosaPlayHandAnim );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	HashedString	m_AnimationName;
};

#endif // WBACTIONROSAPLAYANIM_H
