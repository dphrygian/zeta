#ifndef WBCOMPROSASTIM_H
#define WBCOMPROSASTIM_H

#include "wbrosacomponent.h"
#include "wbeventmanager.h"

class WBCompRosaStim : public WBRosaComponent
{
public:
	WBCompRosaStim();
	virtual ~WBCompRosaStim();

	DEFINE_WBCOMP( RosaStim, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			Activate();
	void			Deactivate();

	float			m_Duration;				// If 0, this is a transient stim and the end event will not be queued
	bool			m_IsActive;
	TEventUID		m_DeactivateEventUID;

	// Effects, wrapped up to reduce scripting overhead
	HashedString	m_StatModEvent;
};

#endif // WBCOMPROSASTIM_H
