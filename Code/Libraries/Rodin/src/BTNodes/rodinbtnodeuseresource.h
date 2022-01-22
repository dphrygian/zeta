#ifndef RODINBTNODEUSERESOURCE_H
#define RODINBTNODEUSERESOURCE_H

#include "rodinbtnodedecorator.h"
#include "irodinresourceuser.h"

class RodinBTNodeUseResource : public RodinBTNodeDecorator, public IRodinResourceUser
{
public:
	RodinBTNodeUseResource();
	virtual ~RodinBTNodeUseResource();

	DEFINE_RODINBTNODE( UseResource, RodinBTNodeDecorator );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );
	virtual void		OnFinish();

	// IRodinResourceUser
	virtual bool		OnResourceStolen( const HashedString& Resource );

protected:
	HashedString	m_Resource;		// Config
	bool			m_ForceClaim;	// Config
};

#endif // RODINBTNODEUSERESOURCE_H
