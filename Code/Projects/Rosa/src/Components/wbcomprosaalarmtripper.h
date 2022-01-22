#ifndef WBCOMPROSAALARMTRIPPER_H
#define WBCOMPROSAALARMTRIPPER_H

#include "wbrosacomponent.h"
#include "wbentityref.h"

class WBCompRosaAlarmTripper : public WBRosaComponent
{
public:
	WBCompRosaAlarmTripper();
	virtual ~WBCompRosaAlarmTripper();

	DEFINE_WBCOMP( RosaAlarmTripper, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

private:
	WBEntityRef	m_LinkedAlarmBox;
};

#endif // WBCOMPROSAALARMTRIPPER_H
