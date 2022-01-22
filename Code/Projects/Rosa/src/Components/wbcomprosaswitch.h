#ifndef WBCOMPROSASWITCH_H
#define WBCOMPROSASWITCH_H

#include "wbrosacomponent.h"
#include "array.h"
#include "wbeventmanager.h"
#include "rosaworldgen.h"
#include "wbentityref.h"

class WBCompRosaSwitchable;

class WBCompRosaSwitch : public WBRosaComponent
{
public:
	WBCompRosaSwitch();
	virtual ~WBCompRosaSwitch();

	DEFINE_WBCOMP( RosaSwitch, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	void			SetLinkedSwitchables( const Array<WBEntityRef>& LinkedEntities );

private:
	void			Switch( WBEntity* const pFrobber ) const;

	Array<WBEntityRef>	m_LinkedSwitchables;
};

#endif // WBCOMPROSASWITCH_H
