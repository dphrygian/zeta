#ifndef WBCOMPROSASENSORMARKUP_H
#define WBCOMPROSASENSORMARKUP_H

#include "wbcomprosasensorpoll.h"

class WBCompRosaSensorMarkup : public WBCompRosaSensorPoll
{
public:
	WBCompRosaSensorMarkup();
	virtual ~WBCompRosaSensorMarkup();

	DEFINE_WBCOMP( RosaSensorMarkup, WBCompRosaSensorPoll );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	PollTick( const float DeltaTime ) const;

private:
	HashedString	m_Markup;
	float			m_Radius;
};

#endif // WBCOMPROSASENSORMARKUP_H
