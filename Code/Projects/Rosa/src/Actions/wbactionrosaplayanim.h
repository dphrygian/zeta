#ifndef WBACTIONROSAPLAYANIM_H
#define WBACTIONROSAPLAYANIM_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaPlayAnim : public WBAction
{
public:
	WBActionRosaPlayAnim();
	virtual ~WBActionRosaPlayAnim();

	DEFINE_WBACTION_FACTORY( RosaPlayAnim );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	HashedString		m_AnimationName;
	bool				m_Loop;
	float				m_PlayRate;
	WBParamEvaluator	m_PlayRatePE;
	float				m_BlendTime;
	bool				m_Layered;
	bool				m_Additive;
};

#endif // WBACTIONROSAPLAYANIM_H
