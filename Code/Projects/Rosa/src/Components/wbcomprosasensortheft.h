#ifndef WBCOMPROSASENSORTHEFT_H
#define WBCOMPROSASENSORTHEFT_H

#include "wbcomprosasensor.h"

class WBCompRosaSensorTheft : public WBCompRosaSensor
{
public:
	WBCompRosaSensorTheft();
	virtual ~WBCompRosaSensorTheft();

	DEFINE_WBCOMP( RosaSensorTheft, WBCompRosaSensor );

	virtual void	HandleEvent( const WBEvent& Event );

private:
	void	HandleTheft( WBEntity* const pThief ) const;
};

#endif // WBCOMPROSASENSORTHEFT_H
