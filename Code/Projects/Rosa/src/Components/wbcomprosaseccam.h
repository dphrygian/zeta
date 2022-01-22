#ifndef WBCOMPROSASECCAM_H
#define WBCOMPROSASECCAM_H

#include "wbrosacomponent.h"
#include "simplestring.h"
#include "vector.h"

class WBCompRosaSecCam : public WBRosaComponent
{
public:
	WBCompRosaSecCam();
	virtual ~WBCompRosaSecCam();

	DEFINE_WBCOMP( RosaSecCam, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:

	void			TickOscillate( const float DeltaTime );

	void			Oscillate();
	void			Track();
	void			Alarm();
	void			Disable();

	void			PlayLoopSound();
	void			StopLoopSound();
	void			PlayStopSound();
	void			PlayTrackSound();

	enum ECameraState
	{
		ECS_None,
		ECS_Oscillate,
		ECS_Track,
		ECS_Alarm,
		ECS_Disabled,
	};

	enum EOscillateState
	{
		EOS_Left,
		EOS_LeftWait,
		EOS_Right,
		EOS_RightWait,
	};

	ECameraState	m_CameraState;				// Serialized
	EOscillateState	m_OscillateState;			// Serialized
	float			m_OscillateStateTime;		// Serialized (as time remaining)

	float			m_FixedPitch;				// Config
	float			m_MaxYaw;					// Config
	float			m_MaxRotation;				// Config
	float			m_OscillateVelocity;		// Config
	float			m_TrackVelocity;			// Config
	float			m_OscillateTurnTime;		// Config
	float			m_OscillateWaitTime;		// Config

	float			m_LightBlendDuration;		// Config

	SimpleString	m_OscillateAlbedo;			// Config
	SimpleString	m_OscillateSpec;			// Config
	Vector			m_OscillateLightHSV;		// Config
	SimpleString	m_TrackAlbedo;				// Config
	SimpleString	m_TrackSpec;				// Config
	Vector			m_TrackLightHSV;			// Config
	SimpleString	m_AlarmAlbedo;				// Config
	SimpleString	m_AlarmSpec;				// Config
	Vector			m_AlarmLightHSV;			// Config
	SimpleString	m_DisabledAlbedo;			// Config
	SimpleString	m_DisabledSpec;				// Config
	Vector			m_DisabledLightHSV;			// Config

	SimpleString	m_LoopSound;				// Config
	SimpleString	m_StopSound;				// Config
	SimpleString	m_TrackSound;				// Config
};

#endif // WBCOMPROSASECCAM_H
