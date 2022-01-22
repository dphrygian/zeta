#ifndef WBCOMPROSAHEALTH_H
#define WBCOMPROSAHEALTH_H

#include "wbrosacomponent.h"
#include "wbeventmanager.h"

class WBCompRosaHealth : public WBRosaComponent
{
public:
	WBCompRosaHealth();
	virtual ~WBCompRosaHealth();

	DEFINE_WBCOMP( RosaHealth, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	int				GetHealth() const { return m_Health; }
	float			GetHealthAlpha() const;
	bool			IsAlive() const { return m_Health > 0; }
	bool			IsDead() const { return m_Health <= 0; }
	void			Kill( const HashedString& DamageSetName, WBEntity* const pDamager, const Vector& DamageLocation );
	void			TakeDamage( const HashedString& DamageSetName, const float DamageScalar, const float StaggerScalar, WBEntity* const pDamager, const Vector& DamageLocation, const Angles& DamageOrientation, const Vector& DamageDirection, const HashedString& DamageBone );
	void			Stagger( const float StaggerDuration );
	void			GiveHealth( const int HealthAmount );
	void			GiveMaxHealth( const int MaxHealthAmount, const int HealthAmount );
	void			RestoreHealth( const int TargetHealth );	// Top off health at this value.
	bool			HasMaxHealth() const { return m_Health >= m_MaxHealth; }

	const HashedString&	GetResistanceSetName() const { return m_ResistanceSetName; }

	void			AddDebuff( const HashedString& DebuffName, const float DebuffScalar, WBEntity* const pDamager );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			SetHealth( const int Health );

	void			PublishToHUD() const;
	void			PushPersistence() const;
	void			PullPersistence();

	int				m_Health;			// Serialized
	int				m_MaxHealth;		// Serialized
	int				m_InitialHealth;	// Config
	bool			m_PublishToHUD;		// Config

	float			m_DamageTimeout;	// Config
	float			m_NextDamageTime;	// Serialized (as time remaining)

	int				m_SaveThreshold;	// Config; last chance save, if damage would reduce health from above this threshold to below it, it gets clamped here

	bool			m_Invulnerable;			// Config/serialized
	bool			m_OneHPInvulnerable;	// Config/serialized; if true, you take damage like normal but your health doesn't drop below 1HP

	// Pickup UI management
	float			m_HidePickupScreenDelay;
	TEventUID		m_HidePickupScreenUID;

	HashedString	m_ResistanceSetName;	// Config/serialized
	bool			m_StrictResistance;		// Config; if true, resisted damage effects happen iff all damage vectors are resisted
	bool			m_LenientVulnerability;	// Config; if true, vulnerable damage effects happen as long as some damage vector is unresisted

	// HACKHACK: debuffs system; allows stacking different kinds of debuffs in case I have them
	bool						m_CanBeDebuffed;	// Config; used for e.g. hostages that have health but shouldn't be debuffed
	bool						m_CanBeFactioned;	// Config; same as above
	Map<HashedString, float>	m_Debuffs;			// Serialized; map from debuff name to damage scalar
};

#endif // WBCOMPROSAHEALTH_H
