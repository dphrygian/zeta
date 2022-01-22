#ifndef WBCOMPROSADOOR_H
#define WBCOMPROSADOOR_H

#include "wbrosacomponent.h"
#include "interpolator.h"
#include "vector.h"
#include "angles.h"
#include "simplestring.h"
#include "wbentityref.h"

class WBCompRosaDoor : public WBRosaComponent
{
public:
	WBCompRosaDoor();
	virtual ~WBCompRosaDoor();

	DEFINE_WBCOMP( RosaDoor, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickSecond; }	// Tick at the same priority as Transform

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsLocked() const { return m_Locked; }
	bool			IsInsideDoor( WBEntity* const pEntity ) const;

	const Array<HashedString>&	GetKeycards() const { return m_Keycards; }

	// HACKHACK for AIs to try locked door handles
	bool			ShouldAITryHandle( WBEntity* const pEntity );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	enum EDoorRotation
	{
		EDR_0,
		EDR_90,
		EDR_180,
		EDR_270,
	};

	void			TryToggle( WBEntity* const pFrobber, const float RetoggleTime, const bool ForceCanOpen );
	void			Toggle( WBEntity* const pFrobber );
	void			Lock();
	void			Unlock();
	void			UpdateFromOpenState( const bool InitialSetup, const bool LoadingSetup );
	bool			CanOpenDoor( WBEntity* const pFrobber );

	EDoorRotation	GetRotation() const;
	void			AdjustForFacing();
	Vector			RotateExtents( const Vector& Extents, const EDoorRotation Rotation ) const;
	Vector			RotateOffset( const Vector& Offset, const EDoorRotation Rotation ) const;
	float			RotateYaw( const float Yaw, const EDoorRotation Rotation ) const;

	bool		m_Open;
	bool		m_Locked;	// Config/serialized

	bool				m_HasKeycode;			// Config
	uint				m_Keycode;				// Config, 0-9999
	Array<HashedString>	m_Keycards;				// Config

	float		m_InterpTime;				// Config
	Vector		m_ClosedOffset;				// Config
	Angles		m_ClosedOrientation;		// Config
	Vector		m_ClosedFrobOffset;			// Config
	Vector		m_ClosedFrobExtents;		// Config
	Vector		m_OpenOffset;				// Config
	Angles		m_OpenOrientation;			// Config
	Vector		m_OpenFrobOffset;			// Config
	Vector		m_OpenFrobExtents;			// Config

	int			m_ClosedFrobPriority;		// Config
	int			m_OpenFrobPriority;			// Config

	Interpolator<Vector>	m_OffsetInterpolator;		// Transient
	Interpolator<Angles>	m_OrientationInterpolator;	// Transient

	bool		m_WaitingToRetoggle;		// Serialized
	float		m_RetoggleTime;				// Serialized (as time remaining)
	WBEntityRef	m_RetoggleEntity;			// Serialized (who requested the retoggle)

	SimpleString	m_UnlockedMesh;			// Config
	SimpleString	m_LockedMesh;			// Config
	SimpleString	m_UnlockedTexture;		// Config
	SimpleString	m_LockedTexture;		// Config
	SimpleString	m_UnlockedFriendlyName;	// Config
	SimpleString	m_LockedFriendlyName;	// Config

	SimpleString	m_ClosedTexture;		// Config
	SimpleString	m_OpenTexture;			// Config

	// HACKHACK
	float			m_NextAITryHandleTime;	// Transient
	float			m_AITryHandleTimeMin;	// Config
	float			m_AITryHandleTimeMax;	// Config
	float			m_AITryHandleDistSq;	// Config
};

#endif // WBCOMPROSADOOR_H
