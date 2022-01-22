#ifndef WBCOMPROSAPOWERTELEPORT_H
#define WBCOMPROSAPOWERTELEPORT_H

#include "wbrosacomponent.h"
#include "wbentityref.h"

class WBCompRosaPowerTeleport : public WBRosaComponent
{
public:
	WBCompRosaPowerTeleport();
	virtual ~WBCompRosaPowerTeleport();

	DEFINE_WBCOMP( RosaPowerTeleport, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

private:
	void			TryTeleport() const;

	WBEntityRef		m_Beacon;
};

#endif // WBCOMPROSAPOWERTELEPORT_H
