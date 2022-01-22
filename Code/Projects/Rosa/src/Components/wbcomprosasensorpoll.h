#ifndef WBCOMPROSASENSORPOLL_H
#define WBCOMPROSASENSORPOLL_H

#include "wbcomprosasensor.h"

class WBCompRosaSensorPoll : public WBCompRosaSensor
{
public:
	WBCompRosaSensorPoll();
	virtual ~WBCompRosaSensorPoll();

	DEFINE_WBCOMP( RosaSensorPoll, WBCompRosaSensor );

	virtual void	Tick( const float DeltaTime );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	PollTick( const float DeltaTime ) const;

private:
	bool	m_DoPoll;			// Config
	float	m_TickDeltaMin;		// Config
	float	m_TickDeltaMax;		// Config
	float	m_NextTickTime;		// Transient
};

#endif // WBCOMPROSASENSORPOLL_H
