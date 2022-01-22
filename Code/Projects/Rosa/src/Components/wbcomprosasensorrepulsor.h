#ifndef WBCOMPROSASENSORREPULSOR_H
#define WBCOMPROSASENSORREPULSOR_H

#include "wbcomprosasensor.h"

class WBCompRosaSensorRepulsor : public WBCompRosaSensor
{
public:
	WBCompRosaSensorRepulsor();
	virtual ~WBCompRosaSensorRepulsor();

	DEFINE_WBCOMP( RosaSensorRepulsor, WBCompRosaSensor );

	virtual void	HandleEvent( const WBEvent& Event );

private:
	void	HandleRepulsed( WBEntity* const pRepulsorEntity ) const;
};

#endif // WBCOMPROSASENSORREPULSOR_H
