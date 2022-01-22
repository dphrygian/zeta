#ifndef WBCOMPROSATHINKERNPCTARGET_H
#define WBCOMPROSATHINKERNPCTARGET_H

#include "wbcomprosathinker.h"

class WBCompRosaThinkerNPCTarget : public WBCompRosaThinker
{
public:
	WBCompRosaThinkerNPCTarget();
	virtual ~WBCompRosaThinkerNPCTarget();

	DEFINE_WBCOMP( RosaThinkerNPCTarget, WBCompRosaThinker );

	virtual void	Tick( const float DeltaTime );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_OutputTargetBlackboardKey;

	float			m_TargetScoreThreshold;
};

#endif // WBCOMPROSATHINKERNPCTARGET_H
