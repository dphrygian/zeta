#ifndef WBCOMPROSASENSORBUMP_H
#define WBCOMPROSASENSORBUMP_H

#include "wbcomprosasensor.h"

class WBCompRosaSensorBump : public WBCompRosaSensor
{
public:
	WBCompRosaSensorBump();
	virtual ~WBCompRosaSensorBump();

	DEFINE_WBCOMP( RosaSensorBump, WBCompRosaSensor );

	virtual void	HandleEvent( const WBEvent& Event );

private:
	void	HandleBump( WBEntity* const pCollidedEntity ) const;
};

#endif // WBCOMPROSASENSORBUMP_H
