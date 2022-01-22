#ifndef WBCOMPROSASWITCHABLE_H
#define WBCOMPROSASWITCHABLE_H

#include "wbrosacomponent.h"

class WBCompRosaSwitchable : public WBRosaComponent
{
public:
	WBCompRosaSwitchable();
	virtual ~WBCompRosaSwitchable();

	DEFINE_WBCOMP( RosaSwitchable, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }
};

#endif // WBCOMPROSASWITCHABLE_H
