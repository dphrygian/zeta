#ifndef WBCOMPROSASENSORPATROL_H
#define WBCOMPROSASENSORPATROL_H

#include "wbcomprosasensor.h"

class WBCompRosaSensorPatrol : public WBCompRosaSensor
{
public:
	WBCompRosaSensorPatrol();
	virtual ~WBCompRosaSensorPatrol();

	DEFINE_WBCOMP( RosaSensorPatrol, WBCompRosaSensor );

	virtual void	HandleEvent( const WBEvent& Event );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PromoteCurrentPatrolNode() const;

	HashedString	m_MoveOutputBlackboardKey;
	HashedString	m_TurnOutputBlackboardKey;
};

#endif // WBCOMPROSASENSORPATROL_H
