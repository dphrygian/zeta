#ifndef WBCOMPROSAFROBBER_H
#define WBCOMPROSAFROBBER_H

// This also doubles as an "aimer" because otherwise there's a lot of duplicated code and data

#include "wbrosacomponent.h"
#include "wbentityref.h"
#include "vector4.h"

class WBCompRosaFrobber : public WBRosaComponent
{
public:
	WBCompRosaFrobber();
	virtual ~WBCompRosaFrobber();

	DEFINE_WBCOMP( RosaFrobber, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Needs to tick after transform.

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	WBEntityRef		GetFrobTarget() const { return m_FrobTarget; }
	WBEntityRef		GetAimTarget() const { return m_AimTarget; }
	void			GetAimTarget( WBEntity*& pOutAimTarget, HashedString& OutAimTargetBone );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void		TryFrob( const uint Input );

	WBEntity*	FindTargetFrobbable() const;
	void		OnSetFrobTarget( WBEntity* const pFrobTarget );
	void		OnUnsetFrobTarget( WBEntity* const pFrobTarget );

	void		FindAimTarget( WBEntity*& pOutAimTarget, HashedString& OutAimTargetBone ) const;
	void		OnSetAimTarget( WBEntity* const pAimTarget );
	void		OnUnsetAimTarget( WBEntity* const pAimTarget );

	float			m_FrobDistance;		// Config
	WBEntityRef		m_FrobTarget;		// Transient
	uint			m_DisableRefCount;	// Transient (because it is currently used for death and hacking, and shouldn't need to be serialized)
	WBEntityRef		m_ForcedTarget;		// Serialized; override frob line check with a specific target, e.g. for carrying a body in Neon

	float			m_AimDistance;		// Config
	WBEntityRef		m_AimTarget;		// Transient
	HashedString	m_AimTargetBone;	// Transient; the bone (if any) that we're aiming at on aim target
};

#endif // WBCOMPROSAFROBBER_H
