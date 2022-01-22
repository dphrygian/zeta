#ifndef ANIMEVENTROSAEXECUTEACTION_H
#define ANIMEVENTROSAEXECUTEACTION_H

#include "animevent.h"
#include "wbaction.h"
#include "array.h"

class AnimEventRosaExecuteAction : public AnimEvent
{
public:
	AnimEventRosaExecuteAction();
	virtual ~AnimEventRosaExecuteAction();

	DEFINE_ANIMEVENT( RosaExecuteAction );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Call( Mesh* pMesh, Animation* pAnimation );

private:
	Array<WBAction*>	m_Actions;
};

#endif // ANIMEVENTROSAEXECUTEACTION_H
