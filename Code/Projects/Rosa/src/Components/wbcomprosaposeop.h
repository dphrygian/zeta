#ifndef WBCOMPROSAPOSEOP_H
#define WBCOMPROSAPOSEOP_H

#include "wbrosacomponent.h"
#include "wbentityref.h"

class WBCompRosaPoseOp : public WBRosaComponent
{
public:
	WBCompRosaPoseOp();
	virtual ~WBCompRosaPoseOp();

	DEFINE_WBCOMP( RosaPoseOp, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual int		GetTickOrder() { return ETO_NoTick; }

	HashedString	GetIdleAnimation() const	{ return m_IdleAnimation; }
	float			GetRefHeight() const		{ return m_RefHeight; }

	bool			IsEnabled() const			{ return m_Enabled; }

	bool			IsAssigned() const						{ return NULL != GetAssignedEntity(); }
	WBEntity*		GetAssignedEntity() const				{ return m_AssignedEntity.Get(); }
	void			AssignEntity( WBEntity* const pEntity )	{ DEVASSERT( !IsAssigned() ); m_AssignedEntity = pEntity; }

	bool			IsReserved() const			{ return m_RequiredActorProps.Size() > 0 || m_ForbiddenActorProps.Size() > 0; }
	bool			IsPaired() const			{ return m_RequiredActorRels.Size() > 0 || m_ForbiddenActorRels.Size() > 0; }
	HashedString	GetPairTag() const			{ return m_PairTag; }

	const Array<HashedString>&	GetRequiredActorProps() const	{ return m_RequiredActorProps; }
	const Array<HashedString>&	GetForbiddenActorProps() const	{ return m_ForbiddenActorProps; }
	const Array<HashedString>&	GetRequiredActorRels() const	{ return m_RequiredActorRels; }
	const Array<HashedString>&	GetForbiddenActorRels() const	{ return m_ForbiddenActorRels; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	WBEntityRef			m_AssignedEntity;		// Transient
	HashedString		m_IdleAnimation;		// Config
	float				m_RefHeight;			// Config; defines the height from ground that should be maintained for the pose when character is scaled
	HashedString		m_PairTag;				// Config

	bool				m_Enabled;				// Config; default for filtering, overridden by campaign to enable/disable as needed

	Array<HashedString>	m_RequiredActorProps;	// Config
	Array<HashedString>	m_ForbiddenActorProps;	// Config
	Array<HashedString>	m_RequiredActorRels;	// Config
	Array<HashedString>	m_ForbiddenActorRels;	// Config
};

#endif // WBCOMPROSAALARMTRIPPER_H
