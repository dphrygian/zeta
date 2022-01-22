#ifndef RODINBTNODEWAIT_H
#define RODINBTNODEWAIT_H

#include "rodinbtnode.h"
#include "wbparamevaluator.h"
#include "wbeventmanager.h"
#include "iwbeventobserver.h"

class RodinBTNodeWait : public RodinBTNode, public IWBEventObserver
{
public:
	RodinBTNodeWait();
	virtual ~RodinBTNodeWait();

	DEFINE_RODINBTNODE( Wait, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();

	// IWBEventObserver
	virtual void		HandleEvent( const WBEvent& Event );

private:
	WBParamEvaluator	m_TimePE;		// Config
	bool				m_TimerStarted;	// Serialized
	TEventUID			m_EventHandle;	// Serialized
};

#endif // RODINBTNODEWAIT_H
