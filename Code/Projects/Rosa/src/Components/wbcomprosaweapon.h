#ifndef WBCOMPROSAWEAPON_H
#define WBCOMPROSAWEAPON_H

#include "wbrosacomponent.h"
#include "wbeventmanager.h"

class WBCompRosaAmmoBag;
class WBCompStatMod;
class ITexture;

struct SMagazine
{
	SMagazine()
	:	m_AmmoType()
	,	m_DamageSet()
	,	m_AmmoCount( 0 )
	,	m_AmmoMax( 0 )
	{
	}

	HashedString	m_AmmoType;		// Config; what ammo type this magazine uses
	HashedString	m_DamageSet;	// Config; what damage set this ammo type in this weapon does
	uint			m_AmmoCount;	// Serialized; how much ammo is in this magazine
	uint			m_AmmoMax;		// Config; how much ammo can be in this magazine at once
};

class WBCompRosaWeapon : public WBRosaComponent
{
public:
	WBCompRosaWeapon();
	virtual ~WBCompRosaWeapon();

	DEFINE_WBCOMP( RosaWeapon, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

#if BUILD_DEV
	virtual void	Report() const;
#endif

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	const SMagazine&	GetMagazine() const	{ DEVASSERT( UsesAmmo() ); return m_Magazines[ m_CurrentMagazine ]; }
	SMagazine&			GetMagazine()		{ DEVASSERT( UsesAmmo() ); return m_Magazines[ m_CurrentMagazine ]; }

	uint			GetCurrentMagazine() const						{ return m_CurrentMagazine; }
	void			SetCurrentMagazine( const uint MagazineIndex )	{ m_CurrentMagazine = MagazineIndex; }

	bool			UsesAmmo() const						{ return m_Magazines.Size() > 0; }
	bool			UsesMultipleMagazines() const			{ return m_Magazines.Size() > 1; }
	uint			GetNumMagazines() const					{ return m_Magazines.Size(); }

	HashedString	GetDamageSet() const					{ return UsesAmmo() ? GetMagazine().m_DamageSet	: m_DamageSet; }
	HashedString	GetAmmoType() const						{ return UsesAmmo() ? GetMagazine().m_AmmoType	: HashedString::NullString; }
	uint			GetAmmoCount() const					{ return UsesAmmo() ? GetMagazine().m_AmmoCount	: 0; }
	uint&			GetAmmoCountRef()						{ return GetMagazine().m_AmmoCount; }
	uint			GetAmmoMax() const						{ return UsesAmmo() ? GetMagazine().m_AmmoMax	: 0; }
	uint			GetAmmoSpace() const					{ return GetAmmoMax() - GetAmmoCount(); }
	void			SetAmmoCount( const uint AmmoCount )	{ GetAmmoCountRef() = AmmoCount; }
	bool			HasAmmo() const							{ return GetAmmoCount() > 0; }
	bool			HasAmmo( const uint Count ) const		{ return GetAmmoCount() >= Count; }

	uint			GetAmmoCount( const uint MagazineIndex ) const					{ return m_Magazines[ MagazineIndex ].m_AmmoCount; }
	void			SetAmmoCount( const uint MagazineIndex, const uint AmmoCount )	{ m_Magazines[ MagazineIndex ].m_AmmoCount = AmmoCount; }

	const Array<SMagazine>&	GetMagazines() const			{ return m_Magazines; }

	bool			CanRaise() const;
	bool			CanPutDown() const;
	bool			CanTrigger() const;
	bool			CanUnTrigger() const;
	bool			CanShove() const;
	bool			CanReload() const;
	bool			CanCycleMagazine() const;
	bool			ShouldCycleMagazine() const;
	bool			CanAim() const;
	bool			CanUnAim() const;

	bool			IsAutomatic() const { return m_RefireRate > 0.0f; }
	bool			IsAiming() const;
	bool			IsAimChanging() const;
	float			GetAimTime() const { return m_AimTime; }
	float			GetAimZoom() const;
	float			GetAimZoomFG() const;

	ITexture*			GetIcon() const { return m_Icon; }
	const SimpleString&	GetIconName() const { return m_IconName; }

	float			GetKickSpringK() const { return m_KickSpringK; }
	float			GetKickDamperC() const { return m_KickDamperC; }

	bool			CanAutoAim() const { return m_CanAutoAim; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	enum EWeaponState
	{
		EWS_None,
		EWS_Down,
		EWS_DownToIdle,
		EWS_IdleToDown,
		EWS_Idle,
		EWS_Firing,
		EWS_Reloading,
		EWS_CyclingMagazine,
		EWS_IdleToAimed,
		EWS_AimedToIdle,
		EWS_Aimed,
		EWS_AimedFiring,
		EWS_Shoving,
	};

	WBCompStatMod*		GetOwnerStatMod() const;

	void				ConditionalShow();
	void				RequestHandAnim( const HashedString& HandAnim, const bool Loop, const float PlayRate, const float BlendTime ) const;

	void				TryRaise();
	void				RaiseWeapon();
	void				SlamToIdle();

	void				MarshalTriggerEvent( const uint Input );
	void				TryTrigger();
	void				TryTriggerHeld();
	void				TryUnTrigger();
	void				OnTriggered();
	void				OnTriggeredHeld();
	void				OnUnTriggered();

	void				StartFiring();
	void				FireWeapon();
	void				EndFiring();

	void				TryShove();
	void				StartShoving();
	void				EndShoving();

	WBCompRosaAmmoBag*	GetAmmoBag() const;
	void				SpendAmmo( const uint AmmoCount );
	uint				GetMagazineIndex( const HashedString& AmmoType ) const;

	// Available here means unlocked and has ammo
	uint				GetNumAvailableMagazines() const;
	bool				IsAvailableMagazine( const SMagazine& Magazine ) const;

	uint				GetActualReload() const;
	void				TryReload();
	void				StartReload();
	void				EndReload();
	void				PlayReloadAnim();
	void				ReloadMagazine();

	void				TryCycleMagazine();
	void				StartCycleMagazine();
	void				EndCycleMagazine();

	void				TryPutDown();
	void				TryPutDown( const HashedString& CycleSlot );
	void				StartPutDown();
	void				EndPutDown();

	void				TryAim();
	void				StartAim();
	void				TryUnAim();
	void				StartUnAim();

	void				PublishToHUD() const;

	void				StartTransitionTo( const EWeaponState EndState );
	void				ContinueTransition();
	void				OnStateTransition( const EWeaponState WeaponState );
	void				QueueStateTransition( const EWeaponState WeaponState, const float Delay );
	void				CancelStateTransition();
#if BUILD_DEV
	static SimpleString	GetStateFromEnum( const EWeaponState WeaponState );
#endif

	EWeaponState		m_WeaponState;			// Serialized
	EWeaponState		m_TransitionEndState;	// Serialized
	TEventUID			m_TransitionEventUID;	// Serialized

	HashedString		m_CycleSlot;			// Serialized; which slot do we cycle to after putting weapon down

	bool				m_CanAim;				// Config; can this weapon be aimed (iron sights)
	float				m_AimTime;				// Config; duration of idle-to-aimed and aimed-to-idle transitions
	float				m_AimZoom;				// Config; scalar applied to FOV in iron sights (e.g., 0.5 for a 2x zoom)
	float				m_AimZoomFG;			// Config; same but for foreground
	float				m_RaiseTime;			// Config; duration of down-to-idle transition
	float				m_LowerTime;			// Config; duration of idle-to-down transition
	bool				m_HoldReleaseMode;		// Config; supports two triggered reactions, one for held input and one for released

	float				m_FireBlendTime;		// Config; for blending into fire anim for automatic weapons
	float				m_RefireRate;			// Config; for automatic weapons
	TEventUID			m_RefireUID;			// Serialized; request for next refire on automatic weapons

	HashedString		m_DamageSet;			// Config; what damage set we use if this weapon has no magazines
	uint				m_CurrentMagazine;		// Serialized; which magazine we're using, if any
	Array<SMagazine>	m_Magazines;			// Config/serialized; what ammo this weapon uses, if any

	SimpleString		m_IconName;				// Config
	ITexture*			m_Icon;					// Config; icon for radial and maybe other places

	float				m_KickSpringK;			// Config
	float				m_KickDamperC;			// Config
	Angles				m_KickImpulse;			// Config; yaw is random in this range, pitch is fixed

	bool				m_CanAutoAim;			// Config
};

#endif // WBCOMPROSAWEAPON_H
