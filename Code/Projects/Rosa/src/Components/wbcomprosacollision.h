#ifndef WBCOMPROSACOLLISION_H
#define WBCOMPROSACOLLISION_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "array.h"
#include "wbentityref.h"
#include "aabb.h"
#include "map.h"
#include "rosaworld.h"

class CollisionInfo;

class WBCompRosaCollision : public WBRosaComponent
{
public:
	WBCompRosaCollision();
	virtual ~WBCompRosaCollision();

	DEFINE_WBCOMP( RosaCollision, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }
	virtual bool	BelongsInComponentArray() { return true; }

	bool	Collide( const Vector& StartLocation, Vector& InOutMovement );

	void	OnLanded( const float LandedMagnitude, WBEntity* const pCollidedEntity, const HashedString& CollisionSurface );
	void	OnCollided( const Vector& CollisionNormal, WBEntity* const pCollidedEntity, const HashedString& CollisionSurface, const Vector& CollisionVelocity );
	void	OnSteppedUp( const float StepHeight );

	void	Jump();
	void	Fall();
	bool	IsLanded() const { return m_Landed; }
	bool	IsRecentlyLanded( const float TimeThreshold ) const;	// Returns landed status within a threshold; never returns true if jumping.

	const AABB&	GetBounds() const { /*ASSERT( m_Bounds.Equals( GetCurrentBounds() ) );*/ return m_Bounds; }
	Vector		GetExtents() const { return m_HalfExtents; }
	void		SetExtents( const Vector& HalfExtents );

	bool		GetUseMeshExtents() { return m_UseMeshExtents; }

	const AABB& GetNavBounds() const { return m_NavBounds; }
	bool		FindNavNodesUnder( Array<uint>& OutNavNodeIndices ) const;	// This iterates all nav nodes, use with care

	float		GetFrictionCoefficient() const;
	float		GetAirFrictionCoefficient() const;
	float		GetElasticity() { return m_Elasticity; }

	bool		IsDynamicBlocker() const { return MatchesAllCollisionFlags( EECF_BlocksBlockers | EECF_IsDynamic ); }

	// Uses EECF flags defined in rosaworld.h
	inline uint	GetCollisionFlags() const							{ return m_CollisionFlags; }
	inline uint	GetDefaultCollisionFlags() const					{ return m_DefaultCollisionFlags; }
	inline bool	HasDefaultCollision() const							{ return m_CollisionFlags == m_DefaultCollisionFlags; }
	void		SetCollisionFlags( const uint Flags, const uint Mask = 0xffffffff, const bool SendEvents = false, const bool UpdateCollisionMap = true );
	inline void	ResetCollisionFlags()								{ SetCollisionFlags( m_DefaultCollisionFlags ); }
	inline bool	MatchesAllCollisionFlags( const uint Flags ) const	{ return ( m_CollisionFlags & Flags ) == Flags; }
	inline bool	MatchesAnyCollisionFlags( const uint Flags ) const	{ return ( m_CollisionFlags & Flags ) > 0; }

	void		DisableCollision();
	void		EnableCollision();

	void		GetTouchingEntities( Array<WBEntity*>& OutTouchingEntities ) const;

	HashedString	GetSurface() const { return m_Surface; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	typedef Array<WBCompRosaCollision*>	TCollisionArray;
	typedef Map<uint, TCollisionArray>	TCollisionMap;

	static const TCollisionArray*	GetCollisionArray( const uint Flags );
	static const TCollisionArray&	GetTouchingArray() { return sm_TouchingArray; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			GatherTouching( Array<WBEntityRef>& OutTouching ) const;
	void			UpdateTouching();	// This also sends touch events
	void			SendTouchEvent( const WBEntityRef& TouchingEntity );
	void			SendUntouchEvent( const WBEntityRef& TouchingEntity );

	void			AddTouching( const WBEntityRef& Entity )	{ m_Touching.PushBack( Entity ); }
	void			RemoveTouching( const WBEntityRef& Entity )	{ m_Touching.FastRemoveItem( Entity ); }

	void			UpdateBounds() { m_Bounds = GetCurrentBounds(); m_NavBounds = GetCurrentNavBounds(); }
	AABB			GetCurrentBounds() const;
	AABB			GetCurrentNavBounds() const;

	void			AddToCollisionMap();
	void			AddToCollisionArray( const uint Flags );
	void			AddToTouchingArray();

	void			RemoveFromCollisionMap();
	void			RemoveFromCollisionArray( const uint Flags );
	void			RemoveFromTouchingArray();

	void			ConditionalSetNavBlocking( const bool NavBlocking );
	void			ConditionalSendStaticCollisionChangedEvent();

	Vector				m_HalfExtents;						// Config/serialized
	AABB				m_Bounds;							// Transient, constructed from location and half extents
	bool				m_UseMeshExtents;					// Config
	float				m_ExtentsFatten;					// Config
	float				m_NavExtentsFatten;					// Config

	Vector				m_NavExtents;						// Config/serialized; optional separate extents for rasterization into nav grid
	AABB				m_NavBounds;						// Transient

	float				m_Elasticity;						// Config
	float				m_FrictionTargetTime;				// Config
	float				m_FrictionTargetPercentVelocity;	// Config
	bool				m_UseAirFriction;					// Config
	float				m_AirFrictionTargetTime;			// Config
	float				m_AirFrictionTargetPercentVelocity;	// Config

	bool				m_StandOnSlopes;					// Config
	float				m_MaxStepHeight;					// Config
	bool				m_CanStepUp;						// Config, derived from MaxStepHeight > 0

	HashedString		m_Surface;							// Config

	bool				m_IsNavBlocking;					// Transient

	bool				m_Landed;							// Transient
	float				m_UnlandedTime;						// Transient

	// m_Touching used to be a set for quicker Removes; I'm trying an Array for quicker UpdateTouching.
	Array<WBEntityRef>	m_Touching;							// Serialized
	bool				m_CanTouch;							// Config/serialized

	uint				m_CollisionFlags;					// Config/serialized, see RosaWorld::ECollisionFlags
	uint				m_DefaultCollisionFlags;			// Config

	// Map from collision flags to collidables that match those flags
	static TCollisionMap	sm_CollisionMap;
	static TCollisionArray	sm_TouchingArray;
};

#endif // WBCOMPROSACOLLISION_H
