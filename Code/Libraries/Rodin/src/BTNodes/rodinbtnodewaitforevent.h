#ifndef RODINBTNODEWAITFOREVENT_H
#define RODINBTNODEWAITFOREVENT_H

#include "rodinbtnode.h"
#include "wbrule.h"
#include "iwbeventobserver.h"

class RodinBTNodeWaitForEvent : public RodinBTNode, public IWBEventObserver
{
public:
	RodinBTNodeWaitForEvent();
	virtual ~RodinBTNodeWaitForEvent();

	DEFINE_RODINBTNODE( WaitForEvent, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();

	// IWBEventObserver
	virtual void		HandleEvent( const WBEvent& Event );

protected:
	WBRule		m_Rule;	// Config
};

#endif // RODINBTNODEWAITFOREVENT_H
