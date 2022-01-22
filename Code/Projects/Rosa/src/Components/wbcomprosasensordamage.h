#ifndef WBCOMPROSASENSORDAMAGE_H
#define WBCOMPROSASENSORDAMAGE_H

#include "wbcomprosasensor.h"

class WBCompRosaSensorDamage : public WBCompRosaSensor
{
public:
	WBCompRosaSensorDamage();
	virtual ~WBCompRosaSensorDamage();

	DEFINE_WBCOMP( RosaSensorDamage, WBCompRosaSensor );

	virtual void	HandleEvent( const WBEvent& Event );

private:
	void	HandleDamage( WBEntity* const pDamager ) const;
};

#endif // WBCOMPROSASENSORDAMAGE_H
