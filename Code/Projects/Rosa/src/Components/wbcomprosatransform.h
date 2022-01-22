#ifndef WBCOMPROSATRANSFORM_H
#define WBCOMPROSATRANSFORM_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "angles.h"
#include "matrix.h"
#include "array.h"
#include "interpolator.h"

class WBCompRosaTransform : public WBRosaComponent
{
public:
	WBCompRosaTransform();
	virtual ~WBCompRosaTransform();

	DEFINE_WBCOMP( RosaTransform, WBRosaComponent );

	virtual void	Initialize();

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickSecond; }

	void	OnTeleport() const;

	void	SetInitialTransform( const Vector& Location, const Angles& Orientation );

	Vector	GetLocation() const { return m_Location; }
	void	SetLocation( const Vector& NewLocation );
	void	MoveBy( const Vector& Offset );

	float	GetSpeedLimit() const { return m_SpeedLimit; }
	float	GetStatModdedSpeedLimit() const;
	float	GetSpeed() const { return m_Velocity.Length(); }

	Vector	GetVelocity() const { return m_Velocity; }
	void	SetVelocity( const Vector& NewVelocity ) { m_Velocity = NewVelocity; }
	void	ApplyImpulse( const Vector& Impulse ) { m_Velocity += Impulse; }

	Vector	GetAcceleration() const { return m_Acceleration; }
	void	SetAcceleration( const Vector& NewAcceleration ) { m_Acceleration = NewAcceleration; }

	Angles	GetOrientation() const { return m_Orientation; }
	void	SetOrientation( const Angles& NewOrientation );

	Angles	GetRotationalVelocity() const { return m_RotationalVelocity; }
	void	SetRotationalVelocity( const Angles& NewRotationalVelocity ) { m_RotationalVelocity = NewRotationalVelocity; }
	void	ApplyRotationalImpulse( const Angles& RotationalImpulse ) { m_RotationalVelocity += RotationalImpulse; }

	float	GetGravity() const { return m_Gravity; }
	void	SetGravity( const float Gravity ) { m_Gravity = Gravity; }
	void	SetDefaultGravity();

	bool	GetCanMove() const { return m_CanMove; }
	void	SetCanMove( const bool CanMove ) { m_CanMove = CanMove; }

	void	CopyOwnerOffset( const WBEntity* const pSourceEntity );

	void	TeleportTo( WBEntity* const pDestination, const bool ShouldSetOrientation );

	float	GetScale() const { return m_Scale; }

	// Helper for consistency
	static Angles	ClampPitch( const Angles& InOrientation );

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

#if BUILD_DEV
	virtual void	Report() const;
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	TickMotion( const float DeltaTime );
	void	TickAcceleration( const float DeltaTime );
	void	MoveWithOwner();

	Vector		m_Location;				// Serialized
	Vector		m_Velocity;				// Serialized
	Vector		m_Acceleration;			// Serialized

	float		m_Gravity;				// Config/serialized

	bool		m_UseSpeedLimit;		// Transient
	float		m_SpeedLimit;			// Config, applied only to acceleration

	bool		m_AllowImpulses;		// Config

	Angles		m_Orientation;			// Serialized
	Angles		m_RotationalVelocity;	// Serialized

	bool		m_CanMove;				// Config/serialized

	bool		m_IsAttachedToOwner;	// Config
	Vector		m_OwnerOffset;			// Config
	Matrix		m_OwnerOffsetMatrix;	// Config

	bool		m_IsSettled;			// Serialized

	float		m_Scale;				// Config/serialized

	// HACKHACK for title screen, maybe I'll find other places for it too
	Interpolator<Vector>	m_InterpMoveToLocation;
	Interpolator<Angles>	m_InterpMoveToOrientation;
};

#endif // WBCOMPROSATRANSFORM_H
