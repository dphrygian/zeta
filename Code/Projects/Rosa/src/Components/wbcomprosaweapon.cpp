#include "core.h"
#include "wbcomprosaweapon.h"
#include "configmanager.h"
#include "Components/wbcompowner.h"
#include "Components/wbcompstatmod.h"
#include "wbcomprosahands.h"
#include "wbcomprosaammobag.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "idatastream.h"
#include "rosahudlog.h"
#include "Common/uimanagercommon.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosacampaign.h"
#include "inputsystem.h"
#include "irenderer.h"
#include "texturemanager.h"

WBCompRosaWeapon::WBCompRosaWeapon()
:	m_WeaponState( EWS_None )
,	m_TransitionEndState( EWS_None )
,	m_TransitionEventUID( 0 )
,	m_CycleSlot()
,	m_CanAim( false )
,	m_AimTime( 0.0f )
,	m_AimZoom( 0.0f )
,	m_AimZoomFG( 0.0f )
,	m_RaiseTime( 0.0f )
,	m_LowerTime( 0.0f )
,	m_HoldReleaseMode( false )
,	m_FireBlendTime( 0.0f )
,	m_RefireRate( 0.0f )
,	m_RefireUID( 0 )
,	m_CurrentMagazine( 0 )
,	m_Magazines()
,	m_IconName()
,	m_Icon( NULL )
,	m_KickSpringK( 0.0f )
,	m_KickDamperC( 0.0f )
,	m_KickImpulse()
,	m_CanAutoAim( false )
{
}

WBCompRosaWeapon::~WBCompRosaWeapon()
{
}

/*virtual*/ void WBCompRosaWeapon::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( CanAim );
	m_CanAim = ConfigManager::GetInheritedBool( sCanAim, false, sDefinitionName );

	STATICHASH( AimTime );
	m_AimTime = ConfigManager::GetInheritedFloat( sAimTime, 0.0f, sDefinitionName );

	STATICHASH( AimZoom );
	const float AimZoom = ConfigManager::GetInheritedFloat( sAimZoom, 1.0f, sDefinitionName );
	m_AimZoom = 1.0f / AimZoom;

	STATICHASH( AimZoomFG );
	const float AimZoomFG = ConfigManager::GetInheritedFloat( sAimZoomFG, AimZoom, sDefinitionName );
	m_AimZoomFG = 1.0f / AimZoomFG;

	STATICHASH( RaiseTime );
	m_RaiseTime = ConfigManager::GetInheritedFloat( sRaiseTime, 0.0f, sDefinitionName );

	STATICHASH( LowerTime );
	m_LowerTime = ConfigManager::GetInheritedFloat( sLowerTime, 0.0f, sDefinitionName );

	STATICHASH( HoldReleaseMode );
	m_HoldReleaseMode = ConfigManager::GetInheritedBool( sHoldReleaseMode, false, sDefinitionName );

	STATICHASH( FireBlendTime );
	m_FireBlendTime = ConfigManager::GetInheritedFloat( sFireBlendTime, 0.0f, sDefinitionName );

	STATICHASH( RefireRate );
	m_RefireRate = ConfigManager::GetInheritedFloat( sRefireRate, 0.0f, sDefinitionName );

	STATICHASH( DamageSet );
	m_DamageSet = ConfigManager::GetInheritedHash( sDamageSet, HashedString::NullString, sDefinitionName );

	STATICHASH( NumMagazines );
	const uint NumMagazines = ConfigManager::GetInheritedInt( sNumMagazines, 0, sDefinitionName );
	FOR_EACH_INDEX( MagazineIndex, NumMagazines )
	{
		SMagazine&	Magazine	= m_Magazines.PushBack();
		Magazine.m_AmmoType		= ConfigManager::GetInheritedSequenceHash(	"Magazine%dAmmoType",	MagazineIndex, HashedString::NullString,	sDefinitionName );
		Magazine.m_DamageSet	= ConfigManager::GetInheritedSequenceHash(	"Magazine%dDamageSet",	MagazineIndex, HashedString::NullString,	sDefinitionName );
		Magazine.m_AmmoMax		= ConfigManager::GetInheritedSequenceInt(	"Magazine%dAmmoMax",	MagazineIndex, 0,							sDefinitionName );
	}

	STATICHASH( Icon );
	m_IconName = ConfigManager::GetInheritedString( sIcon, "", sDefinitionName );
	if( m_IconName != "" )
	{
		m_Icon = RosaFramework::GetInstance()->GetRenderer()->GetTextureManager()->GetTexture( m_IconName.CStr(), TextureManager::ETL_Permanent );
	}

	STATICHASH( KickSpringK );
	m_KickSpringK = ConfigManager::GetInheritedFloat( sKickSpringK, 0.0f, sDefinitionName );

	STATICHASH( KickDamperC );
	m_KickDamperC = ConfigManager::GetInheritedFloat( sKickDamperC, 0.0f, sDefinitionName );

	STATICHASH( KickImpulseYaw );
	m_KickImpulse.Yaw = ConfigManager::GetInheritedFloat( sKickImpulseYaw, 0.0f, sDefinitionName );

	STATICHASH( KickImpulsePitch );
	m_KickImpulse.Pitch = ConfigManager::GetInheritedFloat( sKickImpulsePitch, 0.0f, sDefinitionName );

	STATICHASH( CanAutoAim );
	m_CanAutoAim = ConfigManager::GetInheritedBool( sCanAutoAim, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaWeapon::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( UseWeapon );
	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( TryPutDown );
	STATIC_HASHED_STRING( TryRaise );
	STATIC_HASHED_STRING( ForceRaise );
	STATIC_HASHED_STRING( Redraw );
	STATIC_HASHED_STRING( TryCycleToSlot );
	STATIC_HASHED_STRING( OnCycled );
	STATIC_HASHED_STRING( OnUncycled );
	STATIC_HASHED_STRING( TryAim );
	STATIC_HASHED_STRING( TryUnAim );
	STATIC_HASHED_STRING( SpendAmmo );
	STATIC_HASHED_STRING( FireWeapon );
	STATIC_HASHED_STRING( EndFiring );
	STATIC_HASHED_STRING( TryShove );
	STATIC_HASHED_STRING( EndShove );
	STATIC_HASHED_STRING( TryReload );
	STATIC_HASHED_STRING( EndReload );
	STATIC_HASHED_STRING( TryCycleMagazine );
	STATIC_HASHED_STRING( OnAmmoBagUpdated );
	STATIC_HASHED_STRING( OnWeaponEquipped );
	STATIC_HASHED_STRING( OnUnequipped );
	STATIC_HASHED_STRING( WeaponStateTransition );
	STATIC_HASHED_STRING( FlushMagazines );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sUseWeapon )
	{
		STATIC_HASHED_STRING( Input );
		const uint Input = Event.GetInt( sInput );

		MarshalTriggerEvent( Input );
	}
	else if( EventName == sOnSpawned )
	{
		WB_MAKE_EVENT( Hide, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), Hide, GetEntity() );

		m_WeaponState = EWS_Down;
	}
	else if( EventName == sTryPutDown )
	{
		TryPutDown();
	}
	else if( EventName == sTryRaise )
	{
		TryRaise();
	}
	else if( EventName == sForceRaise || EventName == sRedraw )
	{
		// Slam state to what we need and then raise weapon again (used when changing hand meshes or finishing a lockpick)
		m_WeaponState = EWS_Down;

		DEVASSERT( CanRaise() );
		RaiseWeapon();
	}
	else if( EventName == sTryCycleToSlot )
	{
		STATIC_HASHED_STRING( Slot );
		const HashedString Slot = Event.GetHash( sSlot );

		TryPutDown( Slot );
	}
	else if( EventName == sOnCycled )
	{
		ConditionalShow();
		TryRaise();
	}
	else if( EventName == sOnUncycled )
	{
		// Make sure we've put weapon down before cycling away
		// If not, we're being forcibly uncycled and need to slam state and cancel any ongoing transition
		if( m_WeaponState != EWS_Down )
		{
			m_WeaponState = EWS_Down;
			CancelStateTransition();
		}

		WB_MAKE_EVENT( Hide, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), Hide, GetEntity() );
	}
	else if( EventName == sTryAim )
	{
		TryAim();
	}
	else if( EventName == sTryUnAim )
	{
		TryUnAim();
	}
	else if( EventName == sFireWeapon )
	{
		FireWeapon();
	}
	else if( EventName == sSpendAmmo )
	{
		// ROSANOTE: Currently unused (29 June 2015) since I've moved stuff from script to code.
		STATIC_HASHED_STRING( AmmoCount );
		const uint AmmoCount = Event.GetInt( sAmmoCount );

		SpendAmmo( AmmoCount );
	}
	else if( EventName == sTryShove )
	{
		TryShove();
	}
	else if( EventName == sEndShove )
	{
		EndShoving();
	}
	else if( EventName == sTryReload )
	{
		TryReload();
	}
	else if( EventName == sEndReload )
	{
		// This event comes from animation system and is reused for cycling magazines
		if( m_WeaponState == EWS_CyclingMagazine )
		{
			EndCycleMagazine();
		}
		else
		{
			EndReload();
		}
	}
	else if( EventName == sTryCycleMagazine )
	{
		TryCycleMagazine();
	}
	else if( EventName == sEndFiring )
	{
		EndFiring();
	}
	else if( EventName == sOnAmmoBagUpdated )
	{
		// Update HUD regardless of what changed in the ammo bag (i.e., even if it's not for this weapon)
		PublishToHUD();

		// HACKHACK for Zeta: immediately reload when getting ammo
		if( EWS_Reloading != m_WeaponState )
		{
			TryReload();
		}
	}
	else if( EventName == sOnWeaponEquipped )
	{
		PublishToHUD();
	}
	else if( EventName == sOnUnequipped )
	{
		// This weapon is being removed from the inventory; return the ammo in its magazines.
		WBCompRosaAmmoBag* const pAmmoBag = GetAmmoBag();
		FOR_EACH_ARRAY( MagazineIter, m_Magazines, SMagazine )
		{
			const SMagazine& Magazine = MagazineIter.GetValue();
			if( Magazine.m_AmmoCount > 0 )
			{
				pAmmoBag->AddAmmo( Magazine.m_AmmoType, Magazine.m_AmmoCount, false );
			}
		}
	}
	else if( EventName == sWeaponStateTransition )
	{
		STATIC_HASHED_STRING( WeaponState );
		EWeaponState WeaponState = static_cast<EWeaponState>( Event.GetInt( sWeaponState ) );

		OnStateTransition( WeaponState );
	}
	else if( EventName == sFlushMagazines )
	{
		// Return ammo in magazines to ammo bag same as OnUnequipped but also zero it on the magazine
		WBCompRosaAmmoBag* const pAmmoBag = GetAmmoBag();
		FOR_EACH_ARRAY( MagazineIter, m_Magazines, SMagazine )
		{
			SMagazine& Magazine = MagazineIter.GetValue();
			if( Magazine.m_AmmoCount > 0 )
			{
				pAmmoBag->AddAmmo( Magazine.m_AmmoType, Magazine.m_AmmoCount, false );
				Magazine.m_AmmoCount = 0;
			}
		}
	}
}

/*virtual*/ void WBCompRosaWeapon::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Hash,	DamageSet,	GetDamageSet() );
	WB_SET_CONTEXT( Event, Hash,	AmmoType,	GetAmmoType() );
	WB_SET_CONTEXT( Event, Int,		AmmoCount,	GetAmmoCount() );
	WB_SET_CONTEXT( Event, Bool,	HasAmmo,	HasAmmo() );
	WB_SET_CONTEXT( Event, Bool,	UsesAmmo,	UsesAmmo() );
}

void WBCompRosaWeapon::ConditionalShow()
{
	WBEntity* const			pPlayer	= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaHands* const	pHands	= WB_GETCOMP( pPlayer, RosaHands );
	DEVASSERT( pHands );

	if( pHands->IsHidden() || pHands->IsModal() )
	{
		// Hands are hidden or weapon should be hidden; we'll show the weapon later when hands decide it's okay.
		return;
	}

	WB_MAKE_EVENT( Show, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), Show, GetEntity() );
}

void WBCompRosaWeapon::RequestHandAnim( const HashedString& HandAnim, const bool Loop, const float PlayRate, const float BlendTime ) const
{
	WB_MAKE_EVENT( PlayHandAnim, GetEntity() );
	WB_SET_AUTO( PlayHandAnim, Hash,	AnimationName,		HandAnim );
	WB_SET_AUTO( PlayHandAnim, Entity,	AnimatingEntity,	GetEntity() );
	WB_SET_AUTO( PlayHandAnim, Bool,	Loop,				Loop );
	WB_SET_AUTO( PlayHandAnim, Float,	PlayRate,			PlayRate );
	WB_SET_AUTO( PlayHandAnim, Float,	BlendTime,			BlendTime );
	WB_DISPATCH_EVENT( GetEventManager(), PlayHandAnim, RosaGame::GetPlayer() );
}

void WBCompRosaWeapon::TryRaise()
{
	if( CanRaise() )
	{
		RaiseWeapon();
	}
}

void WBCompRosaWeapon::RaiseWeapon()
{
	DEVASSERT( m_WeaponState == EWS_Down );
	m_WeaponState = EWS_DownToIdle;
	QueueStateTransition( EWS_Idle, m_RaiseTime );

	//// Slam hands and weapon into the down pose...
	//STATIC_HASHED_STRING( Down );
	//static const bool	sLoop		= false;
	//static const float	sBlendTime	= 0.0f;
	//static const float	sPlayRate	= 1.0f;
	//RequestHandAnim( sDown, sLoop, sPlayRate, sBlendTime );

	//// ...then blend up into idle.
	//STATIC_HASHED_STRING( Idle );
	//RequestHandAnim( sIdle, sLoop, sPlayRate, m_RaiseTime );

	// HACKHACK for Zeta: slam to idle
	STATIC_HASHED_STRING( Idle );
	static const bool	sLoop		= false;
	static const float	sBlendTime	= 0.0f;
	static const float	sPlayRate	= 1.0f;
	RequestHandAnim( sIdle, sLoop, sPlayRate, sBlendTime );

	WB_MAKE_EVENT( OnStartRaise, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnStartRaise, GetEntity() );
}

// This is used to interrupt e.g. reloading into idle so we
// can put weapon down and cycle away immediately.
void WBCompRosaWeapon::SlamToIdle()
{
	static const float sSlamTime = 0.0f;

	// Ignore what state we're coming from in this case.
	QueueStateTransition( EWS_Idle, sSlamTime );

	// Slam hands and weapon into the idle pose.
	STATIC_HASHED_STRING( Idle );
	static const bool	sLoop		= false;
	static const float	sPlayRate	= 1.0f;
	RequestHandAnim( sIdle, sLoop, sPlayRate, sSlamTime );

	// Cancel pending refire, if any (HACKHACK)
	GetEventManager()->UnqueueEvent( m_RefireUID );
}

void WBCompRosaWeapon::MarshalTriggerEvent( const uint Input )
{
	if( m_HoldReleaseMode )
	{
		if( INPUT_TEST( Input, INPUT_ONHOLD ) )
		{
			TryTriggerHeld();
		}
		else if( INPUT_TEST( Input, INPUT_ONFALL ) )
		{
			TryTrigger();
		}
	}
	else
	{
		if( INPUT_TEST( Input, INPUT_ONRISE ) ||
			( IsAutomatic() && INPUT_TEST( Input, INPUT_ISHIGH ) ) )
		{
			TryTrigger();
		}
		else if( INPUT_TEST( Input, INPUT_ISLOW ) )
		{
			TryUnTrigger();
		}
	}
}

bool WBCompRosaWeapon::CanRaise() const
{
	// We can only raise from the down state
	if( m_WeaponState != EWS_Down )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanPutDown() const
{
	if( m_WeaponState != EWS_Idle &&
		m_WeaponState != EWS_Aimed &&
		m_WeaponState != EWS_Reloading &&
		m_WeaponState != EWS_CyclingMagazine &&
		m_WeaponState != EWS_Firing &&
		m_WeaponState != EWS_AimedFiring )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanTrigger() const
{
	// We can't fire weapons in hub
	if( RosaFramework::GetInstance()->GetGame()->ShouldLowerWeapon() )
	{
		return false;
	}

	// We can only fire from the idle or aimed states
	if( m_WeaponState != EWS_Idle &&
		m_WeaponState != EWS_Aimed )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanUnTrigger() const
{
	// We can only untrigger from the firing states
	if( m_WeaponState != EWS_Firing &&
		m_WeaponState != EWS_AimedFiring )
	{
		return false;
	}

	// We can only untrigger automatic weapons; semi-auto weapons untrigger automatically
	if( !IsAutomatic() )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanShove() const
{
	// We can't shove in hub
	if( RosaFramework::GetInstance()->GetGame()->ShouldLowerWeapon() )
	{
		return false;
	}

	// We can only fire from the idle or aimed states (should be the same as CanTrigger)
	if( m_WeaponState != EWS_Idle &&
		m_WeaponState != EWS_Aimed )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanReload() const
{
	// We can only reload from the idle or aimed states
	if( m_WeaponState != EWS_Idle &&
		m_WeaponState != EWS_Aimed )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanCycleMagazine() const
{
	// We can only cycle from the idle or aimed states
	if( m_WeaponState != EWS_Idle &&
		m_WeaponState != EWS_Aimed )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::ShouldCycleMagazine() const
{
	// Do we have multiple magazines? (Redundant quick check)
	if( !UsesMultipleMagazines() )
	{
		return false;
	}

	// Do we have multiple *available* magazines? (Slower check)
	if( GetNumAvailableMagazines() < 2 )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanAim() const
{
	// We can't aim weapons in hub
	if( RosaFramework::GetInstance()->GetGame()->ShouldLowerWeapon() )
	{
		return false;
	}

	// Some weapons can never be aimed
	if( !m_CanAim )
	{
		return false;
	}

	// We can only aim from the idle state
	if( m_WeaponState != EWS_Idle )
	{
		return false;
	}

	return true;
}

bool WBCompRosaWeapon::CanUnAim() const
{
	// We can only unaim from the aimed state
	if( m_WeaponState != EWS_Aimed )
	{
		return false;
	}

	return true;
}

void WBCompRosaWeapon::TryTrigger()
{
	if( CanTrigger() )
	{
		OnTriggered();
	}
}

void WBCompRosaWeapon::TryTriggerHeld()
{
	if( CanTrigger() )	// TEMPHACK, I don't actually have a use case for this at the moment
	{
		OnTriggeredHeld();
	}
}

void WBCompRosaWeapon::TryUnTrigger()
{
	if( CanUnTrigger() )
	{
		OnUnTriggered();
	}
}

void WBCompRosaWeapon::OnTriggered()
{
	if( UsesAmmo() )
	{
		if( HasAmmo() )
		{
			StartFiring();
		}
		else if( GetActualReload() > 0 )
		{
			TryReload();
		}
		else
		{
			WB_MAKE_EVENT( OnDryFired, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), OnDryFired, GetEntity() );
		}
	}
	else
	{
		StartFiring();
	}
}

void WBCompRosaWeapon::OnTriggeredHeld()
{
	// ROSATODO: Find a use case for this
}

void WBCompRosaWeapon::OnUnTriggered()
{
	EndFiring();
}

void WBCompRosaWeapon::StartFiring()
{
	DEVASSERT( m_WeaponState == EWS_Idle || m_WeaponState == EWS_Aimed );
	const bool IsAimed	= ( m_WeaponState == EWS_Aimed );
	m_WeaponState		= IsAimed ? EWS_AimedFiring : EWS_Firing;

	STATIC_HASHED_STRING( Fire );
	STATIC_HASHED_STRING( ZoomFire );
	const HashedString	HandAnim	= IsAimed ? sZoomFire : sFire;
	const bool			Loop		= IsAutomatic();
	static const float	sPlayRate	= 1.0f;
	RequestHandAnim( HandAnim, Loop, sPlayRate, m_FireBlendTime );

	WB_MAKE_EVENT( OnStartFiring, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnStartFiring, GetEntity() );

	if( IsAutomatic() )
	{
		// Fire first shot immediately, don't wait for animation
		FireWeapon();
	}
}

void WBCompRosaWeapon::FireWeapon()
{
	DEVASSERT( m_WeaponState == EWS_Firing || m_WeaponState == EWS_AimedFiring );

	if( UsesAmmo() )
	{
		DEVASSERT( HasAmmo() );
		SpendAmmo( 1 );
	}

	WB_MAKE_EVENT( OnFired, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnFired, GetEntity() );

	{
		// Weapon kick: pitch is fixed, yaw is random in range, roll is zero
		const Angles KickImpulse = Angles( m_KickImpulse.Pitch, 0.0f, Math::Random( -m_KickImpulse.Yaw, m_KickImpulse.Yaw ) );
		WB_MAKE_EVENT( OnKickImpulse, GetEntity() );
		WB_SET_AUTO( OnKickImpulse, Angles, KickImpulse, KickImpulse );
		WB_DISPATCH_EVENT( GetEventManager(), OnKickImpulse, WBCompOwner::GetTopmostOwner( GetEntity() ) );
	}

	if( IsAutomatic() )
	{
		if( HasAmmo() )
		{
			WB_MAKE_EVENT( FireWeapon, GetEntity() );
			m_RefireUID = WB_QUEUE_EVENT_DELAY( GetEventManager(), FireWeapon, GetEntity(), m_RefireRate );
		}
		else
		{
			EndFiring();
		}
	}
}

void WBCompRosaWeapon::EndFiring()
{
	DEVASSERT( m_WeaponState == EWS_Firing || m_WeaponState == EWS_AimedFiring );
	const bool IsAimed	= ( m_WeaponState == EWS_AimedFiring );
	m_WeaponState		= IsAimed ? EWS_Aimed : EWS_Idle;

	// If this is an automatic weapon, the animation won't stop on its own.
	if( IsAutomatic() )
	{
		// Cancel pending refire
		GetEventManager()->UnqueueEvent( m_RefireUID );

		STATIC_HASHED_STRING( Idle );
		STATIC_HASHED_STRING( ZoomIdle );
		const HashedString	HandAnim	= IsAimed ? sZoomIdle : sIdle;
		static const bool	sLoop		= false;
		static const float	sPlayRate	= 1.0f;
		RequestHandAnim( HandAnim, sLoop, sPlayRate, m_FireBlendTime );
	}

	WB_MAKE_EVENT( OnEndFiring, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnEndFiring, GetEntity() );

	if( !HasAmmo() )
	{
		// Queue this; if we're using a semi-auto weapon, EndFiring is called via
		// anim event, and reloading will request a new animation, causing problems.
		WB_MAKE_EVENT( TryReload, GetEntity() );
		WB_QUEUE_EVENT( GetEventManager(), TryReload, GetEntity() );
	}
}

void WBCompRosaWeapon::TryShove()
{
	if( CanShove() )
	{
		StartTransitionTo( EWS_Shoving );
	}
}

void WBCompRosaWeapon::StartShoving()
{
	DEVASSERT( m_WeaponState == EWS_Idle );
	m_WeaponState = EWS_Shoving;

	STATIC_HASHED_STRING( Shove );
	static const bool	sLoop		= false;
	static const float	sPlayRate	= 1.0f;
	static const float	sBlendTime	= 0.0f;
	RequestHandAnim( sShove, sLoop, sPlayRate, sBlendTime );

	WB_MAKE_EVENT( OnStartShoving, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnStartShoving, GetEntity() );
}

void WBCompRosaWeapon::EndShoving()
{
	DEVASSERT( m_WeaponState == EWS_Shoving );
	m_WeaponState = EWS_Idle;

	// HACKHACK: Because we don't actually finish in the shoving state, this doesn't automatically happen.
	// ROSATODO: Make EndFiring/EndReload/EndShoving/etc. go through the state management the same as blended transitions.
	if( m_TransitionEndState == EWS_Shoving )
	{
		m_TransitionEndState = EWS_None;
	}
}

WBCompRosaAmmoBag* WBCompRosaWeapon::GetAmmoBag() const
{
	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBEntity* const				pOwner		= WBCompOwner::GetTopmostOwner( pEntity );
	DEVASSERT( pOwner );

	WBCompRosaAmmoBag* const	pAmmoBag	= WB_GETCOMP( pOwner, RosaAmmoBag );
	return pAmmoBag;
}

void WBCompRosaWeapon::SpendAmmo( const uint AmmoCount )
{
	ASSERT( HasAmmo( AmmoCount ) );
	GetAmmoCountRef() -= AmmoCount;

	PublishToHUD();
}

uint WBCompRosaWeapon::GetMagazineIndex( const HashedString& AmmoType ) const
{
	FOR_EACH_ARRAY( MagazineIter, m_Magazines, SMagazine )
	{
		const SMagazine& Magazine = MagazineIter.GetValue();
		if( Magazine.m_AmmoType == AmmoType )
		{
			return MagazineIter.GetIndex();
		}
	}

	return INVALID_ARRAY_INDEX;
}

uint WBCompRosaWeapon::GetNumAvailableMagazines() const
{
	uint NumAvailableMagazines = 0;
	FOR_EACH_ARRAY( MagazineIter, m_Magazines, SMagazine )
	{
		const SMagazine& Magazine = MagazineIter.GetValue();

		if( MagazineIter.GetIndex() == m_CurrentMagazine ||	// HACKHACK: The current magazine is always considered available, since we're already using it
			IsAvailableMagazine( Magazine ) )
		{
			++NumAvailableMagazines;
		}
	}

	return NumAvailableMagazines;
}

bool WBCompRosaWeapon::IsAvailableMagazine( const SMagazine& Magazine ) const
{
	// OLDVAMP
	//RosaCampaign* const			pCampaign	= RosaCampaign::GetCampaign();
	//DEVASSERT( pCampaign );

	WBCompRosaAmmoBag* const	pAmmoBag	= GetAmmoBag();
	DEVASSERT( pAmmoBag );

	// OLDVAMP
	//if( pCampaign->IsLockedAmmoType( Magazine.m_AmmoType ) )
	//{
	//	// This ammo type is locked
	//	return false;
	//}

	if( 0 == Magazine.m_AmmoCount &&
		0 == pAmmoBag->GetAmmoCount( Magazine.m_AmmoType ) )
	{
		// No ammo available for this magazine
		return false;
	}

	// All checks passed
	return true;
}

uint WBCompRosaWeapon::GetActualReload() const
{
	if( !UsesAmmo() )
	{
		return 0;
	}

	const uint DesiredReload = GetAmmoSpace();
	if( DesiredReload == 0 )
	{
		return 0;
	}

	WBCompRosaAmmoBag* const pAmmoBag = GetAmmoBag();
	DEVASSERT( pAmmoBag );

	const uint AvailableReload	= pAmmoBag->GetAmmoCount( GetAmmoType() );
	const uint ActualReload		= Min( AvailableReload, DesiredReload );

	return ActualReload;
}

void WBCompRosaWeapon::TryReload()
{
	// Make sure player's input context allows reloading (since reload can come from a Frob/Use input now)
	InputSystem* const pInputSystem = GetFramework()->GetInputSystem();
	STATIC_HASHED_STRING( Reload );
	if( pInputSystem->IsSuppressed( sReload ) )
	{
		// Ignore this request, or maybe queue it up
		DEVPRINTF( "Ignored reload input because of suppressing input context.\n" );
		return;
	}

	if( !CanReload() )
	{
		return;
	}

	// Don't start reloading unless we'd actually be able to do something.
	if( GetActualReload() == 0 )
	{
		return;
	}

	StartTransitionTo( EWS_Reloading );
}

void WBCompRosaWeapon::StartReload()
{
	DEVASSERT( m_WeaponState == EWS_Idle );
	m_WeaponState = EWS_Reloading;

	// HACKHACK for Zeta: skip reload animation
	//PlayReloadAnim();
	EndReload();
}

void WBCompRosaWeapon::PlayReloadAnim()
{
	STATIC_HASHED_STRING( Reload );
	static const bool	sLoop		= false;
	WB_MODIFY_FLOAT( ReloadRate, 1.0f, GetOwnerStatMod() );
	static const float	sBlendTime	= 0.0f;
	RequestHandAnim( sReload, sLoop, WB_MODDED( ReloadRate ), sBlendTime );

	WB_MAKE_EVENT( OnStartReload, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnStartReload, GetEntity() );
}

void WBCompRosaWeapon::EndReload()
{
	ReloadMagazine();

	DEVASSERT( m_WeaponState == EWS_Reloading );
	m_WeaponState = EWS_Idle;

	// HACKHACK: Because we don't actually finish in the reload state, this doesn't automatically happen.
	// ROSATODO: Make EndFiring/EndReload/etc. go through the state management the same as blended transitions.
	if( m_TransitionEndState == EWS_Reloading )
	{
		m_TransitionEndState = EWS_None;
	}
}

void WBCompRosaWeapon::ReloadMagazine()
{
	const uint					ActualReload	= GetActualReload();
	WBCompRosaAmmoBag* const	pAmmoBag		= GetAmmoBag();
	DEVASSERT( pAmmoBag );

	GetAmmoCountRef() += ActualReload;

	static const bool sShowLogMessage = false;
	pAmmoBag->RemoveAmmo( GetAmmoType(), ActualReload, sShowLogMessage );

	PublishToHUD();
}

void WBCompRosaWeapon::TryCycleMagazine()
{
	if( !CanCycleMagazine() )
	{
		return;
	}

	// Don't cycle unless we'd actually be able to do something.
	if( !ShouldCycleMagazine() )
	{
		return;
	}

	StartTransitionTo( EWS_CyclingMagazine );
}

void WBCompRosaWeapon::StartCycleMagazine()
{
	DEVASSERT( m_WeaponState == EWS_Idle );
	m_WeaponState = EWS_CyclingMagazine;

	// Just use reload anim for this
	PlayReloadAnim();
}

void WBCompRosaWeapon::EndCycleMagazine()
{
	DEVASSERT( m_WeaponState == EWS_CyclingMagazine );
	m_WeaponState = EWS_Idle;

	// HACKHACK: Because we don't actually finish in the reload state, this doesn't automatically happen.
	// ROSATODO: Make EndFiring/EndReload/EndCycleMagazine etc. go through the state management the same as blended transitions.
	if( m_TransitionEndState == EWS_CyclingMagazine )
	{
		m_TransitionEndState = EWS_None;
	}

	// Cycle to the next *unlocked* magazine
	do
	{
		m_CurrentMagazine = ( m_CurrentMagazine + 1 ) % m_Magazines.Size();
	}
	while( !IsAvailableMagazine( GetMagazine() ) );

	ReloadMagazine();
}

void WBCompRosaWeapon::TryPutDown()
{
	TryPutDown( HashedString::NullString );
}

void WBCompRosaWeapon::TryPutDown( const HashedString& CycleSlot )
{
	if( !CanPutDown() )
	{
		return;
	}

	m_CycleSlot = CycleSlot;
	StartTransitionTo( EWS_Down );
}

void WBCompRosaWeapon::StartPutDown()
{
	DEVASSERT( m_WeaponState == EWS_Idle );
	m_WeaponState = EWS_IdleToDown;
	QueueStateTransition( EWS_Down, m_LowerTime );

	STATIC_HASHED_STRING( Down );
	static const bool	sLoop		= false;
	static const float	sPlayRate	= 1.0f;
	RequestHandAnim( sDown, sLoop, sPlayRate, m_LowerTime );

	WB_MAKE_EVENT( OnStartPutDown, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnStartPutDown, GetEntity() );
}

void WBCompRosaWeapon::EndPutDown()
{
	if( m_CycleSlot )
	{
		WB_MAKE_EVENT( CycleToSlot, GetEntity() );
		WB_SET_AUTO( CycleToSlot, Hash, Slot, m_CycleSlot );
		WB_DISPATCH_EVENT( GetEventManager(), CycleToSlot, RosaGame::GetPlayer() );
	}
}

void WBCompRosaWeapon::TryAim()
{
	if( !CanAim() )
	{
		return;
	}

	StartAim();
}

void WBCompRosaWeapon::StartAim()
{
	DEVASSERT( m_WeaponState == EWS_Idle );
	m_WeaponState = EWS_IdleToAimed;
	QueueStateTransition( EWS_Aimed, m_AimTime );

	STATIC_HASHED_STRING( ZoomIdle );
	static const bool	sLoop		= false;
	static const float	sPlayRate	= 1.0f;
	RequestHandAnim( sZoomIdle, sLoop, sPlayRate, m_AimTime );

	WB_MAKE_EVENT( OnZoomed, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnZoomed, GetEntity() );

	// ROSATODO: Find best target and start turning to aim at them
}

void WBCompRosaWeapon::TryUnAim()
{
	if( !CanUnAim() )
	{
		return;
	}

	StartUnAim();
}

void WBCompRosaWeapon::StartUnAim()
{
	DEVASSERT( m_WeaponState == EWS_Aimed );
	m_WeaponState = EWS_AimedToIdle;
	QueueStateTransition( EWS_Idle, m_AimTime );

	STATIC_HASHED_STRING( Idle );
	static const bool	sLoop		= false;
	static const float	sPlayRate	= 1.0f;
	RequestHandAnim( sIdle, sLoop, sPlayRate, m_AimTime );

	WB_MAKE_EVENT( OnUnZoomed, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnUnZoomed, GetEntity() );
}

bool WBCompRosaWeapon::IsAiming() const
{
	return
		m_WeaponState == EWS_Aimed ||
		m_WeaponState == EWS_AimedFiring ||
		m_WeaponState == EWS_IdleToAimed;
}

bool WBCompRosaWeapon::IsAimChanging() const
{
	return
		m_WeaponState == EWS_IdleToAimed ||
		m_WeaponState == EWS_AimedToIdle;
}

float WBCompRosaWeapon::GetAimZoom() const
{
	if( IsAiming() )
	{
		// We should be using (or lerping to) the aimed FOV
		return m_AimZoom;
	}

	// We should be using (or lerping to) the normal FOV
	return 1.0f;
}

float WBCompRosaWeapon::GetAimZoomFG() const
{
	if( IsAiming() )
	{
		// We should be using (or lerping to) the aimed FOV
		return m_AimZoomFG;
	}

	// We should be using (or lerping to) the normal FOV
	return 1.0f;
}

void WBCompRosaWeapon::PublishToHUD() const
{
	WBCompRosaAmmoBag* const pAmmoBag = GetAmmoBag();
	DEVASSERT( pAmmoBag );

	UIManager* const pUIManager = GetFramework()->GetUIManager();
	DEVASSERT( pUIManager );

	STATICHASH( HUD );
	STATICHASH( Ammo );
	const uint	MagazineAmmoCount	= GetAmmoCount();
	const uint	AmmoBagAmmoCount	= pAmmoBag->GetAmmoCount( GetAmmoType() );
	const uint	TotalAmmoCount		= MagazineAmmoCount + AmmoBagAmmoCount;
	ConfigManager::SetInt(		sAmmo,		TotalAmmoCount,	sHUD );

	//STATICHASH( AmmoBag );
	//ConfigManager::SetInt(		sAmmoBag,	pAmmoBag->GetAmmoCount( GetAmmoType() ),				sHUD );

	//STATICHASH( AmmoType );
	//ConfigManager::SetString(	sAmmoType,	pAmmoBag->GetAmmoName( GetAmmoType() ).CStr(),				sHUD );

	const bool AmmoHidden		= !UsesAmmo();
	const bool AmmoHelpHidden	= ( GetNumAvailableMagazines() < 2 );

	{
		STATIC_HASHED_STRING( AmmoIcon );
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAmmoIcon );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, AmmoHidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		STATIC_HASHED_STRING( AmmoCounter );
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAmmoCounter );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, AmmoHidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		STATIC_HASHED_STRING( AmmoHelp );
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAmmoHelp );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, AmmoHelpHidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}
}

void WBCompRosaWeapon::OnStateTransition( const EWeaponState WeaponState )
{
	m_WeaponState = WeaponState;

	// Continue to next stage as needed
	if( m_TransitionEndState != EWS_None )
	{
		ContinueTransition();
	}

	if( m_TransitionEndState != EWS_None )
	{
		// Transition is continuing
		return;
	}

	// Else, we've reached the end of transition, do any side effects that aren't handled by anim events

	if( m_WeaponState == EWS_Idle )
	{
		if( !HasAmmo() )
		{
			TryReload();
		}
	}

	if( m_WeaponState == EWS_Down )
	{
		EndPutDown();
	}
}

void WBCompRosaWeapon::QueueStateTransition( const EWeaponState WeaponState, const float Delay )
{
	WB_MAKE_EVENT( WeaponStateTransition, GetEntity() );
	WB_SET_AUTO( WeaponStateTransition, Int, WeaponState, WeaponState );
	m_TransitionEventUID = WB_QUEUE_EVENT_DELAY( GetEventManager(), WeaponStateTransition, GetEntity(), Delay );
}

void WBCompRosaWeapon::CancelStateTransition()
{
	GetEventManager()->UnqueueEvent( m_TransitionEventUID );
}

void WBCompRosaWeapon::StartTransitionTo( const EWeaponState EndState )
{
	if( m_WeaponState == EndState )
	{
		return;
	}

	m_TransitionEndState = EndState;

	ContinueTransition();
}

void WBCompRosaWeapon::ContinueTransition()
{
	if( m_WeaponState == m_TransitionEndState )
	{
		// Base case: we've reached our target state
		m_TransitionEndState = EWS_None;
		return;
	}

	if( m_WeaponState == EWS_Down )
	{
		if( m_TransitionEndState == EWS_Idle )
		{
			RaiseWeapon();
		}
		else
		{
			WARN;
		}
	}
	else if( m_WeaponState == EWS_Idle )
	{
		if( m_TransitionEndState == EWS_Down )
		{
			StartPutDown();
		}
		else if( m_TransitionEndState == EWS_Aimed )
		{
			StartAim();
		}
		else if( m_TransitionEndState == EWS_Shoving )
		{
			StartShoving();
		}
		else if( m_TransitionEndState == EWS_Reloading )
		{
			StartReload();
		}
		else if( m_TransitionEndState == EWS_CyclingMagazine )
		{
			StartCycleMagazine();
		}
		else
		{
			WARN;
		}
	}
	else if( m_WeaponState == EWS_Aimed )
	{
		if( m_TransitionEndState == EWS_Idle			||
			m_TransitionEndState == EWS_Shoving			||
			m_TransitionEndState == EWS_Reloading		||
			m_TransitionEndState == EWS_CyclingMagazine	||
			m_TransitionEndState == EWS_Down			)
		{
			StartUnAim();
		}
		else
		{
			WARN;
		}
	}
	else if(
		m_WeaponState == EWS_Reloading			||
		m_WeaponState == EWS_CyclingMagazine	||
		m_WeaponState == EWS_Firing				||
		m_WeaponState == EWS_AimedFiring		)
	{
		if( m_TransitionEndState == EWS_Down )
		{
			SlamToIdle();
		}
		else
		{
			WARN;
		}
	}
	else
	{
		WARN;
	}
}

WBCompStatMod* WBCompRosaWeapon::GetOwnerStatMod() const
{
	WBEntity* const			pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBEntity* const			pOwnerEntity	= WBCompOwner::GetTopmostOwner( pEntity );
	DEVASSERT( pOwnerEntity );

	WBCompStatMod* const	pStatMod		= WB_GETCOMP( pOwnerEntity, StatMod );
	DEVASSERT( pStatMod	);

	return pStatMod;
}

#if BUILD_DEV
/*static*/ SimpleString WBCompRosaWeapon::GetStateFromEnum( const EWeaponState WeaponState )
{
	switch( WeaponState )
	{
#define TRY_CASE( c ) case c: return #c
	TRY_CASE( EWS_None );
	TRY_CASE( EWS_Down );
	TRY_CASE( EWS_DownToIdle );
	TRY_CASE( EWS_IdleToDown );
	TRY_CASE( EWS_Idle );
	TRY_CASE( EWS_Firing );
	TRY_CASE( EWS_Reloading );
	TRY_CASE( EWS_CyclingMagazine );
	TRY_CASE( EWS_IdleToAimed );
	TRY_CASE( EWS_AimedToIdle );
	TRY_CASE( EWS_Aimed );
	TRY_CASE( EWS_AimedFiring );
	TRY_CASE( EWS_Shoving );
#undef TRY_CASE
	default:				return "Unknown";
	}
}

/*virtual*/ void WBCompRosaWeapon::Report() const
{
	Super::Report();

	PRINTF( WBPROPERTY_REPORT_PREFIX "Weapon State: %s\n", GetStateFromEnum( m_WeaponState ).CStr() );
}
#endif // BUILD_DEV

#define VERSION_EMPTY				0
#define VERSION_AMMO				1
#define VERSION_WEAPONSTATE			3
#define VERSION_CYCLENEXT			5
#define VERSION_TRANSITIONENDSTATE	6
#define VERSION_REFIREUID			9
#define VERSION_CYCLESLOT			10
#define VERSION_AMMOCOUNT_DEPR		11
#define VERSION_MAGAZINES			11
#define VERSION_TRANSITIONEVENTUID	12
#define VERSION_CURRENT				12

uint WBCompRosaWeapon::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version

	Size += 4;												// m_CurrentMagazine
	Size += 4;												// m_Magazines.Size()
	Size += sizeof( HashedString ) * m_Magazines.Size();	// SMagazine::m_AmmoType
	Size += 4 * m_Magazines.Size();							// SMagazine::m_AmmoCount

	Size += 4;						// m_WeaponState
	Size += sizeof( HashedString );	// m_CycleSlot
	Size += 4;						// m_TransitionEndState
	Size += sizeof( TEventUID );	// m_RefireUID
	Size += sizeof( TEventUID );	// m_TransitionEventUID

	return Size;
}

void WBCompRosaWeapon::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_CurrentMagazine );
	Stream.WriteUInt32( m_Magazines.Size() );
	FOR_EACH_ARRAY( MagazineIter, m_Magazines, SMagazine )
	{
		const SMagazine& Magazine = MagazineIter.GetValue();
		Stream.WriteHashedString(	Magazine.m_AmmoType );
		Stream.WriteUInt32(			Magazine.m_AmmoCount );
	}

	Stream.WriteUInt32( m_WeaponState );
	Stream.WriteHashedString( m_CycleSlot );
	Stream.WriteUInt32( m_TransitionEndState );
	Stream.Write<TEventUID>( m_RefireUID );
	Stream.Write<TEventUID>( m_TransitionEventUID );
}

void WBCompRosaWeapon::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_AMMO && Version < VERSION_AMMOCOUNT_DEPR )
	{
		Stream.ReadUInt32();
	}

	if( Version >= VERSION_MAGAZINES )
	{
		m_CurrentMagazine = Stream.ReadUInt32();
		const uint NumMagazines = Stream.ReadUInt32();
		FOR_EACH_INDEX( MagazineIterIndex, NumMagazines )
		{
			const HashedString	AmmoType		= Stream.ReadHashedString();
			const uint			AmmoCount		= Stream.ReadUInt32();
			const uint			MagazineIndex	= GetMagazineIndex( AmmoType );
			if( MagazineIndex < m_Magazines.Size() )
			{
				SMagazine&		Magazine		= m_Magazines[ MagazineIndex ];
				Magazine.m_AmmoCount			= Min( AmmoCount, Magazine.m_AmmoMax );
			}
		}
	}

	if( Version >= VERSION_WEAPONSTATE )
	{
		m_WeaponState = static_cast<EWeaponState>( Stream.ReadUInt32() );
	}

	if( Version >= VERSION_CYCLESLOT )
	{
		m_CycleSlot = Stream.ReadHashedString();
	}

	if( Version >= VERSION_TRANSITIONENDSTATE )
	{
		m_TransitionEndState = static_cast<EWeaponState>( Stream.ReadUInt32() );
	}

	if( Version >= VERSION_REFIREUID )
	{
		m_RefireUID = Stream.Read<TEventUID>();
	}

	if( Version >= VERSION_TRANSITIONEVENTUID )
	{
		m_TransitionEventUID = Stream.Read<TEventUID>();
	}
}
