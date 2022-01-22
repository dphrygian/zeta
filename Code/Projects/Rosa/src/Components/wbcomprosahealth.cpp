#include "core.h"
#include "wbcomprosahealth.h"
#include "wbeventmanager.h"
#include "configmanager.h"
#include "idatastream.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "rosagame.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcomprosaheadshot.h"
#include "Components/wbcomprosacharacterconfig.h"
#include "rosadamage.h"
#include "rosacampaign.h"
#include "rosadifficulty.h"

WBCompRosaHealth::WBCompRosaHealth()
:	m_Health( 0 )
,	m_MaxHealth( 0 )
,	m_InitialHealth( 0 )
,	m_PublishToHUD( false )
,	m_DamageTimeout( 0.0f )
,	m_NextDamageTime( 0.0f )
,	m_SaveThreshold( 0 )
,	m_Invulnerable( false )
,	m_OneHPInvulnerable( false )
,	m_HidePickupScreenDelay( 0.0f )
,	m_HidePickupScreenUID( 0 )
,	m_ResistanceSetName()
,	m_StrictResistance( false )
,	m_LenientVulnerability( false )
,	m_CanBeDebuffed( false )
,	m_CanBeFactioned( false )
,	m_Debuffs()
{
	RosaDamage::AddRef();
}

WBCompRosaHealth::~WBCompRosaHealth()
{
	RosaDamage::Release();
}

/*virtual*/ void WBCompRosaHealth::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	// OLDVAMP: Replace with a campaign modify, possibly?
	//WBCompRosaCharacterConfig* const	pCharacterConfig		= WB_GETCOMP( GetEntity(), RosaCharacterConfig );
	//const bool							IsActor					= pCharacterConfig && pCharacterConfig->IsActor();
	//const HashedString					DefaultResistanceSet	= IsActor ? GetCampaign()->GetResistanceSet( pCharacterConfig->GetActorID() ) : HashedString::NullString;
	const HashedString					DefaultResistanceSet	= HashedString::NullString;

	// OLDVAMP: Replace with a campaign modify, possibly?
	//STATICHASH( UseChallengeScalar );
	//const bool UseChallengeScalar = ConfigManager::GetInheritedBool( sUseChallengeScalar, false, sDefinitionName );
	//const float ChallengeScalar = UseChallengeScalar ? RosaCampaign::GetCampaign()->GetChallengeScalar() : 1.0f;
	const float ChallengeScalar = 1.0f;

	STATICHASH( Health );
	const float Health = ChallengeScalar * ConfigManager::GetInheritedFloat( sHealth, 0.0f, sDefinitionName );

	m_InitialHealth = m_Health = static_cast<int>( Round( Health ) );

	STATICHASH( MaxHealth );
	m_MaxHealth = ConfigManager::GetInheritedInt( sMaxHealth, m_Health, sDefinitionName );

	STATICHASH( Invulnerable );
	m_Invulnerable = ConfigManager::GetInheritedBool( sInvulnerable, false, sDefinitionName );

	STATICHASH( OneHPInvulnerable );
	m_OneHPInvulnerable = ConfigManager::GetInheritedBool( sOneHPInvulnerable, false, sDefinitionName );

	STATICHASH( CanBeDebuffed );
	m_CanBeDebuffed = ConfigManager::GetInheritedBool( sCanBeDebuffed, false, sDefinitionName );

	STATICHASH( CanBeFactioned );
	m_CanBeFactioned = ConfigManager::GetInheritedBool( sCanBeFactioned, false, sDefinitionName );

	STATICHASH( PublishToHUD );
	m_PublishToHUD = ConfigManager::GetInheritedBool( sPublishToHUD, false, sDefinitionName );

	STATICHASH( DamageTimeout );
	m_DamageTimeout = ConfigManager::GetInheritedFloat( sDamageTimeout, 0.0f, sDefinitionName );

	STATICHASH( SaveThreshold );
	m_SaveThreshold = ConfigManager::GetInheritedInt( sSaveThreshold, 0, sDefinitionName );

	STATICHASH( HidePickupScreenDelay );
	m_HidePickupScreenDelay = ConfigManager::GetInheritedFloat( sHidePickupScreenDelay, 0.0f, sDefinitionName );

	STATICHASH( ResistanceSet );
	m_ResistanceSetName = ConfigManager::GetInheritedHash( sResistanceSet, DefaultResistanceSet, sDefinitionName );

	STATICHASH( NumResistanceSets );
	const uint NumResistanceSets		= ConfigManager::GetInheritedInt( sNumResistanceSets, 0, sDefinitionName );
	if( NumResistanceSets > 0 )
	{
		const uint RandomResistanceSet	= Math::Random( NumResistanceSets );
		m_ResistanceSetName				= ConfigManager::GetInheritedSequenceHash( "ResistanceSet%d", RandomResistanceSet, m_ResistanceSetName, sDefinitionName );
	}

	STATICHASH( StrictResistance );
	m_StrictResistance = ConfigManager::GetInheritedBool( sStrictResistance, false, sDefinitionName );

	STATICHASH( LenientVulnerability );
	m_LenientVulnerability = ConfigManager::GetInheritedBool( sLenientVulnerability, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaHealth::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( Kill );
	STATIC_HASHED_STRING( TakeDamage );
	STATIC_HASHED_STRING( GiveHealth );
	STATIC_HASHED_STRING( RestoreHealth );
	STATIC_HASHED_STRING( GiveMaxHealth );
	STATIC_HASHED_STRING( SetInvulnerable );
	STATIC_HASHED_STRING( SetVulnerable );
	STATIC_HASHED_STRING( ToggleInvulnerable );
	STATIC_HASHED_STRING( SetOneHPInvulnerable );
	STATIC_HASHED_STRING( SetOneHPVulnerable );
	STATIC_HASHED_STRING( ToggleOneHPInvulnerable );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( AddDebuff );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		if( m_PublishToHUD )
		{
			PublishToHUD();
		}
	}
	else if( EventName == sKill )
	{
		STATIC_HASHED_STRING( DamageSet );
		const HashedString DamageSetName = Event.GetHash( sDamageSet );

		STATIC_HASHED_STRING( Damager );
		WBEntity* const pDamager = Event.GetEntity( sDamager );

		STATIC_HASHED_STRING( DamageLocation );
		const Vector DamageLocation = Event.GetVector( sDamageLocation );

		Kill( DamageSetName, pDamager, DamageLocation );
	}
	else if( EventName == sTakeDamage )
	{
		STATIC_HASHED_STRING( DamageSet );
		const HashedString DamageSetName = Event.GetHash( sDamageSet );

		STATIC_HASHED_STRING( Scalar );
		const float CommonScalar = Event.GetFloat( sScalar, 1.0f );

		STATIC_HASHED_STRING( DamageScalar );
		const float DamageScalar = CommonScalar * Event.GetFloat( sDamageScalar, 1.0f );

		STATIC_HASHED_STRING( StaggerScalar );
		const float StaggerScalar = CommonScalar * Event.GetFloat( sStaggerScalar, 1.0f );

		STATIC_HASHED_STRING( Damager );
		WBEntity* const pDamager = Event.GetEntity( sDamager );

		// Damage location is used for all kinds of VFX;
		// it's typically the place on this entity where it was hit.
		// If unspecified, we use an invalid zero location.
		STATIC_HASHED_STRING( DamageLocation );
		const Vector DamageLocation = Event.GetVector( sDamageLocation );

		// Damage orientation is used for things like blood VFX;
		// it's typically the normal/tangent orientation at the hit location.
		// If unspecified, we use a valid zero orientation.
		STATIC_HASHED_STRING( DamageOrientation );
		const Angles DamageOrientation = Event.GetAngles( sDamageOrientation );

		// Damage direction is used for things like blood splatter decal;
		// it's typically the direction of the fired weapon trace.
		// If unspecified, we use the damage orientation's Y+.
		STATIC_HASHED_STRING( DamageDirection );
		const Vector DamageDirection = Event.GetVector( sDamageDirection, DamageOrientation.ToVector() );

		// Damage bone is used for headshot and other locational multipliers.
		// If unspecified, we do not modify the damage.
		STATIC_HASHED_STRING( DamageBone );
		const HashedString& DamageBone = Event.GetHash( sDamageBone );

		TakeDamage( DamageSetName, DamageScalar, StaggerScalar, pDamager, DamageLocation, DamageOrientation, DamageDirection, DamageBone );
	}
	else if( EventName == sGiveHealth )
	{
		STATIC_HASHED_STRING( HealthAmount );
		const int HealthAmount = Event.GetInt( sHealthAmount );

		GiveHealth( HealthAmount );
	}
	else if( EventName == sGiveMaxHealth )
	{
		STATIC_HASHED_STRING( MaxHealthAmount );
		const int MaxHealthAmount = Event.GetInt( sMaxHealthAmount );

		STATIC_HASHED_STRING( HealthAmount );
		const int HealthAmount = Event.GetInt( sHealthAmount );

		GiveMaxHealth( MaxHealthAmount, HealthAmount );
	}
	else if( EventName == sRestoreHealth )
	{
		STATIC_HASHED_STRING( TargetHealth );
		const int TargetHealth = Event.GetInt( sTargetHealth );

		RestoreHealth( TargetHealth );
	}
	else if( EventName == sSetInvulnerable )
	{
		m_Invulnerable = true;
	}
	else if( EventName == sSetVulnerable )
	{
		m_Invulnerable = false;
	}
	else if( EventName == sToggleInvulnerable )
	{
		m_Invulnerable = !m_Invulnerable;
	}
	else if( EventName == sSetOneHPInvulnerable )
	{
		m_OneHPInvulnerable = true;
	}
	else if( EventName == sSetOneHPVulnerable )
	{
		m_OneHPInvulnerable = false;
	}
	else if( EventName == sToggleOneHPInvulnerable )
	{
		m_OneHPInvulnerable = !m_OneHPInvulnerable;
	}
	else if( EventName == sAddDebuff )
	{
		STATICHASH( DebuffName );
		const HashedString DebuffName = Event.GetHash( sDebuffName );

		STATICHASH( DebuffScalar );
		const float DebuffScalar = Event.GetFloat( sDebuffScalar, 1.0f );

		STATIC_HASHED_STRING( Damager );
		WBEntity* const pDamager = Event.GetEntity( sDamager );

		AddDebuff( DebuffName, DebuffScalar, pDamager );
	}
	else if( EventName == sPushPersistence )
	{
		PushPersistence();
	}
	else if( EventName == sPullPersistence )
	{
		PullPersistence();
	}
}

/*virtual*/ void WBCompRosaHealth::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsAlive, IsAlive() );
	WB_SET_CONTEXT( Event, Bool, IsDead, IsDead() );
}

void WBCompRosaHealth::SetHealth( const int Health )
{
	m_Health = m_OneHPInvulnerable ? Max( 1, Health ) : Health;

	WB_MAKE_EVENT( OnHealthChanged, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnHealthChanged, GetEntity() );

	if( m_PublishToHUD )
	{
		PublishToHUD();
	}
}

void WBCompRosaHealth::Kill( const HashedString& DamageSetName, WBEntity* const pDamager, const Vector& DamageLocation )
{
	XTRACE_FUNCTION;

	if( m_Invulnerable || m_OneHPInvulnerable || m_Health <= 0 )
	{
		return;
	}

	WB_MAKE_EVENT( OnDamaged, GetEntity() );
	WB_LOG_EVENT( OnDamaged );
	WB_SET_AUTO( OnDamaged, Int,	DamageAmount,	m_Health );
	WB_SET_AUTO( OnDamaged, Hash,	DamageSet,		DamageSetName );
	WB_SET_AUTO( OnDamaged, Entity,	Damager,		pDamager );
	WB_SET_AUTO( OnDamaged, Vector,	DamageLocation,	DamageLocation );
	WB_DISPATCH_EVENT( GetEventManager(), OnDamaged, GetEntity() );

	SetHealth( 0 );
	m_NextDamageTime = GetTime() + m_DamageTimeout;

	WB_MAKE_EVENT( OnDied, GetEntity() );
	WB_LOG_EVENT( OnDied );
	WB_SET_AUTO( OnDied, Entity, Killer, pDamager );
	WB_DISPATCH_EVENT( GetEventManager(), OnDied, GetEntity() );
}

void WBCompRosaHealth::AddDebuff( const HashedString& DebuffName, const float DebuffScalar, WBEntity* const pDamager )
{
	if( !m_CanBeDebuffed )
	{
		return;
	}

	Map<HashedString, float>::Iterator DebuffIter = m_Debuffs.Search( DebuffName );
	if( DebuffIter.IsValid() )
	{
		// We already have a debuff of this type; update if the new scalar is higher
		DebuffIter.GetValue() = Max( DebuffIter.GetValue(), DebuffScalar );
	}
	else
	{
		// Insert the debuff and provide a script hook
		m_Debuffs.Insert( DebuffName, DebuffScalar );

		WB_MAKE_EVENT( OnDebuffed, GetEntity() );
		WB_SET_AUTO( OnDebuffed, Hash,		DebuffName, DebuffName );
		WB_SET_AUTO( OnDebuffed, Entity,	Damager,	pDamager );
		WB_DISPATCH_EVENT( GetEventManager(), OnDebuffed, GetEntity() );
	}
}

void WBCompRosaHealth::TakeDamage( const HashedString& DamageSetName, const float DamageScalar, const float StaggerScalar, WBEntity* const pDamager, const Vector& DamageLocation, const Angles& DamageOrientation, const Vector& DamageDirection, const HashedString& DamageBone )
{
	XTRACE_FUNCTION;

	// First, apply debuffs; this happens even if we can't be damaged
	const Array<RosaDamage::SDebuff>&	Debuffs	= RosaDamage::GetDebuffs( DamageSetName );
	FOR_EACH_ARRAY( DebuffIter, Debuffs, RosaDamage::SDebuff )
	{
		const RosaDamage::SDebuff&	Debuff	= DebuffIter.GetValue();
		AddDebuff( Debuff.m_DebuffName, Debuff.m_DamageScalar, pDamager );
	}

	// Also apply faction; this happens even if we can't be damaged
	if( m_CanBeFactioned )
	{
		const HashedString Faction = RosaDamage::GetFaction( DamageSetName );
		if( Faction )
		{
			WB_MAKE_EVENT( SetFaction, GetEntity() );
			WB_SET_AUTO( SetFaction, Hash, Faction, Faction );
			WB_DISPATCH_EVENT( GetEventManager(), SetFaction, GetEntity() );

			// HACKHACK: Notify damage sensor so target is aware of us now
			WB_MAKE_EVENT( NotifyDamageSensor, GetEntity() );
			WB_SET_AUTO( NotifyDamageSensor, Entity,	Damager,	pDamager );
			WB_DISPATCH_EVENT( GetEventManager(), NotifyDamageSensor, GetEntity() );
		}
	}

	if( m_Invulnerable )
	{
		return;
	}

	const bool	ExpertMode	= RosaDifficulty::GetGameDifficulty() == 4;
	const float	CurrentTime	= GetTime();

	if( ExpertMode )
	{
		// ROSAHACK: In expert mode, ignore damage timeout (which is currently only used for player!)
	}
	else
	{
		if( CurrentTime < m_NextDamageTime )
		{
			return;
		}
	}

	RosaDamage::SResistedDamage DefaultDamage;
	static const HashedString skNullResistanceSet = HashedString::NullString;
	const uint DefaultDamageFlags = RosaDamage::GetResistedDamageAmount( DamageSetName, skNullResistanceSet, DefaultDamage );
	DEVASSERT( DefaultDamageFlags == RosaDamage::ERDF_None || DefaultDamageFlags == RosaDamage::ERDF_Unmodified );
	Unused( DefaultDamageFlags );

	RosaDamage::SResistedDamage ModifiedDamage;
	const uint ResistedDamageFlags = RosaDamage::GetResistedDamageAmount( DamageSetName, m_ResistanceSetName, ModifiedDamage );

	// ROSATODO: Do some VFX/SFX if resisted damage amount is greater than or less than the unresisted sum.
	// (This also means making VFX/SFX be a side effect here instead of only in scripting!)
	const bool Vulnerable	= m_LenientVulnerability	? ( ResistedDamageFlags != RosaDamage::ERDF_Resisted ) : ( ModifiedDamage.m_DamageAmount > DefaultDamage.m_DamageAmount );
	const bool Resisted		= m_StrictResistance		? ( ResistedDamageFlags == RosaDamage::ERDF_Resisted ) : ( ModifiedDamage.m_DamageAmount < DefaultDamage.m_DamageAmount );
	DEVASSERT( !( Vulnerable && Resisted ) );

	// Multiply by constant scalar (for cases where damage amount can't be precisely defined by a damage set)
	ModifiedDamage.m_DamageAmount		*= DamageScalar;
	ModifiedDamage.m_StaggerDuration	*= StaggerScalar;

	// ROSATODO: Also modify stagger duration if desired
	// Multiply damage amount by locational damage scalar
	WBCompRosaHeadshot* const pHeadshot = WB_GETCOMP( GetEntity(), RosaHeadshot );
	if( pHeadshot )
	{
		ModifiedDamage.m_DamageAmount *= pHeadshot->GetHeadshotMod( DamageBone );
	}

	// ROSATODO: Also modify stagger duration if desired
	// Statmod the total damage amount
	WBCompStatMod* const pStatMod			= WB_GETCOMP( GetEntity(), StatMod );
	WBCompStatMod* const pDamagerStatMod	= WB_GETCOMP_SAFE( pDamager, StatMod );

	WB_MODIFY_FLOAT_SAFE( DamageTaken, ModifiedDamage.m_DamageAmount, pStatMod );
	ModifiedDamage.m_DamageAmount = WB_MODDED( DamageTaken );

	WB_MODIFY_FLOAT_SAFE( DamageDealt, ModifiedDamage.m_DamageAmount, pDamagerStatMod );
	ModifiedDamage.m_DamageAmount = WB_MODDED( DamageDealt );

	// HACKHACK: Special support for sneak attack since it requires both entities, hard to script
	WB_MODIFY_FLOAT_SAFE( SneakAttackable, 0.0f, pStatMod );
	const bool SneakAttackable = ( WB_MODDED( SneakAttackable ) != 0.0f );

	if( SneakAttackable )
	{
		WB_MODIFY_FLOAT_SAFE( SneakAttackDamage, ModifiedDamage.m_DamageAmount, pDamagerStatMod );
		ModifiedDamage.m_DamageAmount = WB_MODDED( SneakAttackDamage );
	}

	// Modify damage by debuffs
	FOR_EACH_MAP( DebuffIter, m_Debuffs, HashedString, float )
	{
		const float DebuffScalar = DebuffIter.GetValue();
		ModifiedDamage.m_DamageAmount *= DebuffScalar;
	}

	const int	UseSaveThreshold		= ExpertMode ? 0 : m_SaveThreshold;
	const int	ModifiedDamageAmount	= static_cast<int>( Round( ModifiedDamage.m_DamageAmount ) );
	const int	FinalDamageAmount		= ( m_Health > UseSaveThreshold ) ? Min( ModifiedDamageAmount, m_Health - UseSaveThreshold ) : ModifiedDamageAmount;
	const bool	Saved					= ( m_Health > UseSaveThreshold ) && ( FinalDamageAmount < ModifiedDamageAmount );

	if( FinalDamageAmount <= 0 )
	{
		// Don't apply damage, but do try to stagger, so that staggering doesn't have to deal damage
		Stagger( ModifiedDamage.m_StaggerDuration );
		return;
	}

	// ROSATODO: Figure out what to do with damage types. Might be useful to
	// respond to specific types with different effects, where an entity
	// has a vulnerability. But just sending the damage set along doesn't
	// specify which damages were resisted or vulnerable, and I don't think
	// I want to send multiple OnDamaged events.

	// Send the event before subtracting health. This way, the event arrives
	// while the entity is still alive, if this is the killing blow.
	WB_MAKE_EVENT( OnDamaged, GetEntity() );
	WB_LOG_EVENT( OnDamaged );
	WB_SET_AUTO( OnDamaged, Int,	DamageAmount,		FinalDamageAmount );
	WB_SET_AUTO( OnDamaged, Hash,	DamageSet,			DamageSetName );
	WB_SET_AUTO( OnDamaged, Entity,	Damager,			pDamager );
	WB_SET_AUTO( OnDamaged, Vector,	DamageLocation,		DamageLocation );
	WB_SET_AUTO( OnDamaged, Angles,	DamageOrientation,	DamageOrientation );
	WB_SET_AUTO( OnDamaged, Vector,	DamageDirection,	DamageDirection );
	WB_SET_AUTO( OnDamaged, Bool,	Vulnerable,			Vulnerable );
	WB_SET_AUTO( OnDamaged, Bool,	Resisted,			Resisted );
	WB_SET_AUTO( OnDamaged, Bool,	Saved,				Saved );
	WB_DISPATCH_EVENT( GetEventManager(), OnDamaged, GetEntity() );

	const int PreviousHealth = m_Health;

	SetHealth( m_Health - FinalDamageAmount );
	m_NextDamageTime = CurrentTime + m_DamageTimeout;

	Stagger( ModifiedDamage.m_StaggerDuration );

	if( m_Health <= 0 && PreviousHealth > 0 )
	{
		WB_MAKE_EVENT( OnDied, GetEntity() );
		WB_LOG_EVENT( OnDied );
		WB_SET_AUTO( OnDied, Entity,	Killer,				pDamager );
		WB_SET_AUTO( OnDied, Vector,	DamageDirection,	DamageDirection );	// DLP 7 Jun 2021: Added so I can always apply a ragdoll impulse on death
		WB_DISPATCH_EVENT( GetEventManager(), OnDied, GetEntity() );
	}
}

void WBCompRosaHealth::Stagger( const float StaggerDuration )
{
	if( StaggerDuration <= 0.0f )
	{
		return;
	}

	WB_MAKE_EVENT( Stagger, GetEntity() );
	WB_LOG_EVENT( Stagger );
	WB_SET_AUTO( Stagger, Float,	StaggerDuration,	StaggerDuration );
	WB_DISPATCH_EVENT( GetEventManager(), Stagger, GetEntity() );
}

void WBCompRosaHealth::GiveHealth( const int HealthAmount )
{
	XTRACE_FUNCTION;

	if( HealthAmount <= 0 )
	{
		return;
	}

	SetHealth( Min( m_Health + HealthAmount, m_MaxHealth ) );

	// Show the health pickup screen and hide it after some time

	STATICHASH( HealthPickup );
	STATICHASH( Health );
	ConfigManager::SetInt( sHealth, HealthAmount, sHealthPickup );

	STATIC_HASHED_STRING( HealthPickupScreen );

	{
		WB_MAKE_EVENT( PushUIScreen, NULL );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sHealthPickupScreen );
		WB_DISPATCH_EVENT( GetEventManager(), PushUIScreen, NULL );
	}

	{
		// Remove previously queued hide event if any
		GetEventManager()->UnqueueEvent( m_HidePickupScreenUID );

		WB_MAKE_EVENT( RemoveUIScreen, NULL );
		WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sHealthPickupScreen );
		m_HidePickupScreenUID = WB_QUEUE_EVENT_DELAY( GetEventManager(), RemoveUIScreen, NULL, m_HidePickupScreenDelay );
	}
}

void WBCompRosaHealth::GiveMaxHealth( const int MaxHealthAmount, const int HealthAmount )
{
	m_MaxHealth += MaxHealthAmount;

	GiveHealth( HealthAmount );
}

void WBCompRosaHealth::RestoreHealth( const int TargetHealth )
{
	// ROSANOTE: In Eldritch, this restored to initial health (i.e., topping up to 3 hearts in the library).
	// Now, I want it to always refill to max health.
	const int ActualTargetHealth = ( TargetHealth > 0 ) ? TargetHealth : m_MaxHealth;
	SetHealth( Max( ActualTargetHealth, m_Health ) );
}

float WBCompRosaHealth::GetHealthAlpha() const
{
	return Saturate( static_cast<float>( m_Health ) / static_cast<float>( m_MaxHealth ) );
}

void WBCompRosaHealth::PublishToHUD() const
{
	ASSERT( m_PublishToHUD );

	STATICHASH( HUD );

	STATICHASH( Health );
	ConfigManager::SetInt( sHealth, Max( 0, m_Health ), sHUD );

	STATICHASH( MaxHealth );
	ConfigManager::SetInt( sMaxHealth, m_MaxHealth, sHUD );

	// Shrink health bar
	{
		STATIC_HASHED_STRING( HUD );		// HACKHACK: Hard-coded name
		STATIC_HASHED_STRING( HealthBar );	// HACKHACK: Hard-coded name
		WB_MAKE_EVENT( SetWidgetExtentsScalar, NULL );
		WB_SET_AUTO( SetWidgetExtentsScalar, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetExtentsScalar, Hash, Widget, sHealthBar );
		WB_SET_AUTO( SetWidgetExtentsScalar, Float, W, GetHealthAlpha() );
		WB_SET_AUTO( SetWidgetExtentsScalar, Float, H, 1.0f );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetExtentsScalar, NULL );
	}
}

void WBCompRosaHealth::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( Health );
	Persistence.SetInt( sHealth, m_Health );

	STATIC_HASHED_STRING( MaxHealth );
	Persistence.SetInt( sMaxHealth, m_MaxHealth );
}

void WBCompRosaHealth::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( MaxHealth );
	m_MaxHealth = Persistence.GetInt( sMaxHealth );

	STATIC_HASHED_STRING( Health );
	SetHealth( Persistence.GetInt( sHealth ) );
}

#define VERSION_EMPTY				0
#define VERSION_HEALTH				1
#define VERSION_NEXTDAMAGETIME		2
#define VERSION_MAXHEALTH			3
#define VERSION_INVULNERABLE		4
#define VERSION_RESISTANCESET		5
#define VERSION_ONEHPINVULNERABLE	6
#define VERSION_DEBUFFS				7
#define VERSION_CURRENT				7

uint WBCompRosaHealth::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version

	Size += 4;						// m_Health
	Size += 4;						// m_NextDamageTime (as time remaining)
	Size += 4;						// m_MaxHealth
	Size += 1;						// m_Invulnerable
	Size += 1;						// m_OneHPInvulnerable
	Size += sizeof( HashedString );	// m_ResistanceSetName

	// m_Debuffs
	Size += 4;
	Size += ( sizeof( HashedString ) + sizeof( float ) ) * m_Debuffs.Size();

	return Size;
}

void WBCompRosaHealth::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteInt32( m_Health );
	Stream.WriteFloat( m_NextDamageTime - GetTime() );
	Stream.WriteInt32( m_MaxHealth );
	Stream.WriteBool( m_Invulnerable );
	Stream.WriteBool( m_OneHPInvulnerable );
	Stream.WriteHashedString( m_ResistanceSetName );

	Stream.WriteUInt32( m_Debuffs.Size() );
	FOR_EACH_MAP( DebuffIter, m_Debuffs, HashedString, float )
	{
		Stream.WriteHashedString(	DebuffIter.GetKey() );
		Stream.WriteFloat(			DebuffIter.GetValue() );
	}
}

void WBCompRosaHealth::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_HEALTH )
	{
		m_Health = Stream.ReadInt32();
	}

	if( Version >= VERSION_NEXTDAMAGETIME )
	{
		m_NextDamageTime = GetTime() + Stream.ReadFloat();
	}

	if( Version >= VERSION_MAXHEALTH )
	{
		m_MaxHealth = Stream.ReadInt32();
	}

	if( Version >= VERSION_INVULNERABLE )
	{
		m_Invulnerable = Stream.ReadBool();
	}

	if( Version >= VERSION_ONEHPINVULNERABLE )
	{
		m_OneHPInvulnerable = Stream.ReadBool();
	}

	if( Version >= VERSION_RESISTANCESET )
	{
		m_ResistanceSetName = Stream.ReadHashedString();
	}

	if( Version >= VERSION_DEBUFFS )
	{
		DEVASSERT( m_Debuffs.Empty() );
		const uint NumDebuffs = Stream.ReadUInt32();
		FOR_EACH_INDEX( DebuffIndex, NumDebuffs )
		{
			const HashedString	DebuffName		= Stream.ReadHashedString();
			const float			DebuffScalar	= Stream.ReadFloat();
			m_Debuffs[ DebuffName ] = DebuffScalar;
		}
	}
}
