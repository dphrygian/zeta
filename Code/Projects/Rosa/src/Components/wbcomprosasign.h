#ifndef WBCOMPROSASIGN_H
#define WBCOMPROSASIGN_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "simplestring.h"

class WBEvent;

class WBCompRosaSign : public WBRosaComponent
{
public:
	WBCompRosaSign();
	virtual ~WBCompRosaSign();

	DEFINE_WBCOMP( RosaSign, WBRosaComponent );

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

	bool	IsReadable() const { return m_IsReadable; }

	void	SetBoundOffset( const Vector& Offset ) { m_BoundOffset = Offset; }
	Vector	GetBoundOffset() const { return m_BoundOffset; }

	void	SetBoundExtents( const Vector& Extents ) { m_BoundExtents = Extents; }
	Vector	GetBoundExtents() const { return m_BoundExtents; }

	float	GetReadDistance() const { return m_ReadDistance; }
	AABB	GetBound() const;

	void	SetIsSignTarget( const bool IsSignTarget );
	bool	GetIsSignTarget() const { return m_IsSignTarget; }

	bool	GetUseMeshExtents() { return m_UseMeshExtents; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	PublishToHUD() const;
	void	SetHUDHidden( const bool Hidden ) const;

	bool	m_IsReadable;			// Serialized

	bool	m_IsSignTarget;			// Transient; are we the focus of the sign reader?

	float	m_ReadDistance;			// Config

	bool	m_UseCollisionExtents;	// Config: if true, uses collisions's extents (else, uses configured extents)
	bool	m_UseMeshExtents;		// Config: if true, uses mesh's AABB
	float	m_ExtentsFatten;		// If using collision or mesh extents, optionally fatten up the bounds

	Vector	m_BoundOffset;			// Config/serialized
	Vector	m_BoundExtents;			// Config/serialized

	SimpleString	m_SignText;		// Config/serialized
};

#endif // WBCOMPROSASIGN_H
