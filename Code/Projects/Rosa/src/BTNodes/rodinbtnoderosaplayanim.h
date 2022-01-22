#ifndef RODINBTNODEROSAPLAYANIM_H
#define RODINBTNODEROSAPLAYANIM_H

#include "rodinbtnode.h"
#include "wbparamevaluator.h"
#include "iwbeventobserver.h"

class RodinBTNodeRosaPlayAnim : public RodinBTNode, public IWBEventObserver
{
public:
	RodinBTNodeRosaPlayAnim();
	virtual ~RodinBTNodeRosaPlayAnim();

	DEFINE_RODINBTNODE( RosaPlayAnim, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );
	virtual void		OnStart();
	virtual void		OnFinish();

	// IWBEventObserver
	virtual void		HandleEvent( const WBEvent& Event );

private:
	HashedString		m_AnimationName;	// Config
	WBParamEvaluator	m_AnimationNamePE;	// Config
	bool				m_Loop;				// Config
	float				m_PlayRate;			// Config
	float				m_BlendTime;		// Config
	bool				m_Layered;			// Config: override with any non-rest-posed bone transforms from this animation, before modifiers like headtracking
	bool				m_Additive;			// Config: add any bone deltas from this animation, after modifiers like headtracking
};

#endif // RODINBTNODEROSAPLAYANIM_H
