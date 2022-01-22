#ifndef WBCOMPROSATHINKERTARGET_H
#define WBCOMPROSATHINKERTARGET_H

#include "wbcomprosathinker.h"

class WBCompRosaThinkerTarget : public WBCompRosaThinker
{
public:
	WBCompRosaThinkerTarget();
	virtual ~WBCompRosaThinkerTarget();

	DEFINE_WBCOMP( RosaThinkerTarget, WBCompRosaThinker );

	virtual void	Tick( const float DeltaTime );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_OutputCombatTargetBlackboardKey;
	HashedString	m_OutputSearchTargetBlackboardKey;
	HashedString	m_OutputNoticeTargetBlackboardKey;

	float			m_CombatTargetScoreThreshold;
	float			m_SearchTargetScoreThreshold;
	float			m_NoticeTargetScoreThreshold;

	float			m_BodyConsiderTimeout;

	bool			m_AlwaysTargetHostages;	// Config
};

#endif // WBCOMPROSATHINKERTARGET_H
