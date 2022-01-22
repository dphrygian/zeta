#ifndef WBCOMPROSAALARMBOX_H
#define WBCOMPROSAALARMBOX_H

#include "wbrosacomponent.h"
#include "simplestring.h"
#include "rosaworldgen.h"
#include "wbentityref.h"
#include "wbeventmanager.h"

class WBCompRosaAlarmTripper;

class WBCompRosaAlarmBox : public WBRosaComponent
{
public:
	WBCompRosaAlarmBox();
	virtual ~WBCompRosaAlarmBox();

	DEFINE_WBCOMP( RosaAlarmBox, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	void			SetLinkedAlarmTrippers( const Array<WBEntityRef>& LinkedEntities );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			IdleAlarm();
	void			TripAlarm( WBEntity* const pAlarmTarget, const Vector& AlarmTargetLocation );
	void			DisableAlarm();

	void			ConditionalPlayAINoise();

	enum EAlarmState
	{
		EAS_None,
		EAS_Idle,
		EAS_Active,
		EAS_Disabled,
	};

	Array<WBEntityRef>	m_LinkedAlarmTrippers;

	EAlarmState		m_AlarmState;			// Serialized

	WBEntityRef		m_AlarmTarget;			// Serialized
	Vector			m_AlarmTargetLocation;	// Serialized
	float			m_AlarmTargetTimestamp;	// Serialized
	TEventUID		m_AlarmExpireEventUID;	// Serialized
	TEventUID		m_AlarmRepeatEventUID;	// Serialized

	float			m_AlarmDuration;				// Config
	float			m_AlarmRadius;					// Config
	float			m_AlarmNoiseRepeatTime;			// Config
	float			m_AlarmNoiseCertaintyScalar;	// Config

	SimpleString	m_IdleAlbedo;		// Config
	SimpleString	m_IdleSpec;			// Config
	SimpleString	m_ActiveAlbedo;		// Config
	SimpleString	m_ActiveSpec;		// Config
	SimpleString	m_DisabledAlbedo;	// Config
	SimpleString	m_DisabledSpec;		// Config

	SimpleString	m_AlarmSound;		// Config
	SimpleString	m_IdleSound;		// Config
	SimpleString	m_DisableSound;		// Config
};

#endif // WBCOMPROSAALARMBOX_H
