#ifndef WBCOMPROSAFROBBABLE_H
#define WBCOMPROSAFROBBABLE_H

// This also doubles as an optional "aim receiver" because otherwise
// there's a lot of duplicated data (for state, extents, name, etc.)

#include "wbrosacomponent.h"
#include "vector.h"
#include "vector4.h"
#include "simplestring.h"
#include "aabb.h"

class WBEvent;

class WBCompRosaFrobbable : public WBRosaComponent
{
public:
	WBCompRosaFrobbable();
	virtual ~WBCompRosaFrobbable();

	DEFINE_WBCOMP( RosaFrobbable, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool	IsFrobbable() const			{ return m_IsFrobbable; }
	bool	CanBeAimedAt() const		{ return m_CanBeAimedAt; }
	bool	CanBeAutoAimedAt() const	{ return m_CanBeAutoAimedAt; }

	void	SetBoundOffset( const Vector& Offset ) { m_BoundOffset = Offset; }
	Vector	GetBoundOffset() const { return m_BoundOffset; }

	void	SetBoundExtents( const Vector& Extents ) { m_BoundExtents = Extents; }
	Vector	GetBoundExtents() const { return m_BoundExtents; }

	AABB	GetBound() const;

	float	GetCosFrobAngleLow() const { return m_CosFrobAngleLow; }
	float	GetCosFrobAngleHigh() const { return m_CosFrobAngleHigh; }

	int		GetFrobPriority() const { return m_FrobPriority; }
	void	SetFrobPriority( const int FrobPriority ) { m_FrobPriority = FrobPriority; }

	void	SetIsFrobTarget( const bool IsFrobTarget, WBEntity* const pFrobber );
	bool	GetIsFrobTarget() const { return m_IsProbableFrobbable; }

	void	SetIsAimTarget( const bool IsAimTarget );
	bool	GetIsAimTarget() const { return m_IsAimTarget; }

	bool	GetUseMeshExtents() { return m_UseMeshExtents; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	SetHoldReleaseMode( const bool HoldReleaseMode );

	void	MarshalFrob( WBEntity* const pFrobber, const uint Input );
	void	SendOnFrobbedEvent( WBEntity* const pFrobber ) const;
	void	SendOnFrobbedHeldEvent( WBEntity* const pFrobber ) const;

	void	PublishToHUD() const;
	void	SetHUDHidden( const bool Hidden ) const;

	void	SetAimHUD() const;
	void	ResetAimHUD() const;
	void	SetAimHUDHidden( const bool Hidden ) const;
	void	SetCrosshairsHidden( const bool Default, const bool Friendly, const bool Hostile ) const;
	void	SetAimHealthBar() const;

	bool	m_IsFrobbable;			// Serialized
	bool	m_CanBeAimedAt;			// Serialized
	bool	m_CanBeAutoAimedAt;		// Config

	bool	m_IsProbableFrobbable;	// Transient; are we the focus of a frobber?
	bool	m_IsAimTarget;			// Transient; are we the aimtarget of a frobber?

	bool	m_MainFrobDisabled;		// Config/serialized: if we're in hold/release mode, disable the primary verb without disabling the hold verb
	bool	m_MainFrobHidden;		// Config/serialized; hide the main frob prompt but allowing interaction anyway. Hack for locked doors.

	bool	m_HoldReleaseMode;		// Config/serialized: supports two frob reactions, one for held input and one for released
	bool	m_HandleHoldRelease;	// Transient, state that says we've received an OnRise event and will handle a hold or release

	bool	m_UseCollisionExtents;	// Config: if true, uses collisions's extents (else, uses configured extents)
	bool	m_UseMeshExtents;		// Config: if true, uses mesh's AABB
	float	m_ExtentsFatten;		// If using collision or mesh extents, optionally fatten up the bounds

	Vector	m_BoundOffset;			// Config/serialized
	Vector	m_BoundExtents;			// Config/serialized

	AABB	m_OverrideBounds;		// Transient, pushed from ragdoll tick event

	float	m_CosFrobAngleLow;		// Config/serialized, configured as degrees 0-180
	float	m_CosFrobAngleHigh;		// Config/serialized, configured as degrees 0-180

	int		m_FrobPriority;			// Config/serialized. Low priority takes precedence.

	Vector4	m_Highlight;			// Config

	SimpleString	m_FriendlyName;	// Config/serialized
	SimpleString	m_LiteralName;	// Config/serialized; same as above but not localized (HACKHACK for Rosa names)
	SimpleString	m_FrobVerb;		// Config/serialized
	SimpleString	m_HoldVerb;		// Config
};

#endif // WBCOMPROSAFROBBABLE_H
