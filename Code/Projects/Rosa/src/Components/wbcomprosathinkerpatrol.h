#ifndef WBCOMPROSATHINKERPATROL_H
#define WBCOMPROSATHINKERPATROL_H

#include "wbcomprosathinker.h"

class WBCompRosaThinkerPatrol : public WBCompRosaThinker
{
public:
	WBCompRosaThinkerPatrol();
	virtual ~WBCompRosaThinkerPatrol();

	DEFINE_WBCOMP( RosaThinkerPatrol, WBCompRosaThinker );

	virtual void	Tick( const float DeltaTime );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_OutputBlackboardKey;
};

#endif // WBCOMPROSATHINKERPATROL_H
