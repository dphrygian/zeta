#include "core.h"
#include "wbcomprosafrobbable.h"
#include "aabb.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosahealth.h"
#include "wbcomprosafaction.h"
#include "wbcomprosamesh.h"
#include "wbcomprosacharacterconfig.h"
#include "wbentity.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosamesh.h"
#include "idatastream.h"
#include "inputsystem.h"
#include "Common/uimanagercommon.h"
#include "mathcore.h"
#include "rosagame.h"
#include "hsv.h"

WBCompRosaFrobbable::WBCompRosaFrobbable()
:	m_IsFrobbable( false )
,	m_CanBeAimedAt( false )
,	m_CanBeAutoAimedAt( false )
,	m_IsProbableFrobbable( false )
,	m_IsAimTarget( false )
,	m_MainFrobDisabled( false )
,	m_MainFrobHidden( false )
,	m_HoldReleaseMode( false )
,	m_HandleHoldRelease( false )
,	m_UseCollisionExtents( false )
,	m_UseMeshExtents( false )
,	m_ExtentsFatten( 0.0f )
,	m_BoundOffset()
,	m_BoundExtents()
,	m_OverrideBounds()
,	m_CosFrobAngleLow( 0.0f )
,	m_CosFrobAngleHigh( 0.0f )
,	m_FrobPriority( 0 )
,	m_Highlight()
,	m_FriendlyName()
,	m_LiteralName()
,	m_FrobVerb()
,	m_HoldVerb()
{
}

WBCompRosaFrobbable::~WBCompRosaFrobbable()
{
}

/*virtual*/ void WBCompRosaFrobbable::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( RosaFrobbable );

	STATICHASH( IsFrobbable );
	m_IsFrobbable = ConfigManager::GetInheritedBool( sIsFrobbable, true, sDefinitionName );

	STATICHASH( CanBeAimedAt );
	m_CanBeAimedAt = ConfigManager::GetInheritedBool( sCanBeAimedAt, false, sDefinitionName );

	STATICHASH( CanBeAutoAimedAt );
	m_CanBeAutoAimedAt = ConfigManager::GetInheritedBool( sCanBeAutoAimedAt, m_CanBeAimedAt, sDefinitionName );

	STATICHASH( MainFrobDisabled );
	m_MainFrobDisabled = ConfigManager::GetInheritedBool( sMainFrobDisabled, false, sDefinitionName );

	STATICHASH( MainFrobHidden );
	m_MainFrobHidden = ConfigManager::GetInheritedBool( sMainFrobHidden, false, sDefinitionName );

	STATICHASH( HoldReleaseMode );
	m_HoldReleaseMode = ConfigManager::GetInheritedBool( sHoldReleaseMode, false, sDefinitionName );

	STATICHASH( UseCollisionExtents );
	m_UseCollisionExtents = ConfigManager::GetInheritedBool( sUseCollisionExtents, false, sDefinitionName );

	STATICHASH( UseMeshExtents );
	m_UseMeshExtents = ConfigManager::GetInheritedBool( sUseMeshExtents, false, sDefinitionName );

	STATICHASH( ExtentsFatten );
	m_ExtentsFatten = ConfigManager::GetInheritedFloat( sExtentsFatten, 0.0f, sDefinitionName );

	STATICHASH( ExtentsXY );
	const float ExtentsXY = ConfigManager::GetInheritedFloat( sExtentsXY, 0.0f, sDefinitionName );

	STATICHASH( ExtentsX );
	m_BoundExtents.x = ConfigManager::GetInheritedFloat( sExtentsX, ExtentsXY, sDefinitionName );

	STATICHASH( ExtentsY );
	m_BoundExtents.y = ConfigManager::GetInheritedFloat( sExtentsY, ExtentsXY, sDefinitionName );

	STATICHASH( ExtentsZ );
	m_BoundExtents.z = ConfigManager::GetInheritedFloat( sExtentsZ, 0.0f, sDefinitionName );

	STATICHASH( OffsetZ );
	m_BoundOffset.z = ConfigManager::GetInheritedFloat( sOffsetZ, 0.0f, sDefinitionName );

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	m_BoundExtents	*= pTransform->GetScale();
	m_BoundOffset	*= pTransform->GetScale();

	STATICHASH( FrobAngleLow );
	m_CosFrobAngleLow = Cos( DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sFrobAngleLow, 0.0f, sDefinitionName ) ) );

	STATICHASH( FrobAngleHigh );
	m_CosFrobAngleHigh = Cos( DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sFrobAngleHigh, 180.0f, sDefinitionName ) ) );

	ASSERT( m_CosFrobAngleHigh <= m_CosFrobAngleLow );

	STATICHASH( FrobPriority );
	m_FrobPriority = ConfigManager::GetInheritedInt( sFrobPriority, 0, sDefinitionName );

	// NOTE: The default can only be configured with HSV, otherwise it's not clear
	// whether the local RGB would inherit from the local HSV or the default RGB.
	const Vector4	DefaultHighlightHSVA	= HSV::GetConfigHSVA( "Highlight", sRosaFrobbable, Vector4() );
	const Vector4	HighlightHSVA			= HSV::GetConfigHSVA( "Highlight", sDefinitionName, DefaultHighlightHSVA );
	m_Highlight								= HSV::GetConfigRGBA( "Highlight", sDefinitionName, HSV::HSVToRGB_AlphaPass( HighlightHSVA ) );

	STATICHASH( FriendlyName );
	m_FriendlyName = ConfigManager::GetInheritedString( sFriendlyName, GetEntity()->GetName().CStr(), sDefinitionName );

	STATICHASH( FrobVerb );
	m_FrobVerb = ConfigManager::GetInheritedString( sFrobVerb, "", sDefinitionName );

	STATICHASH( HoldVerb );
	m_HoldVerb = ConfigManager::GetInheritedString( sHoldVerb, "", sDefinitionName );
}

/*virtual*/ void WBCompRosaFrobbable::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( MarshalFrob );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( OnInitialTransformSet );
	STATIC_HASHED_STRING( OnDestroyed );
	STATIC_HASHED_STRING( OnMeshUpdated );
	STATIC_HASHED_STRING( OnHealthChanged );
	STATIC_HASHED_STRING( SetIsFrobbable );
	STATIC_HASHED_STRING( BecomeFrobbable );
	STATIC_HASHED_STRING( BecomeNonFrobbable );
	STATIC_HASHED_STRING( SetCanBeAimedAt );
	STATIC_HASHED_STRING( EnableCanBeAimedAt );
	STATIC_HASHED_STRING( DisableCanBeAimedAt );
	STATIC_HASHED_STRING( SetHoldReleaseMode );
	STATIC_HASHED_STRING( EnableHoldReleaseMode );
	STATIC_HASHED_STRING( DisableHoldReleaseMode );
	STATIC_HASHED_STRING( SetMainFrobEnabled );
	STATIC_HASHED_STRING( SetMainFrobDisabled );
	STATIC_HASHED_STRING( EnableMainFrob );
	STATIC_HASHED_STRING( DisableMainFrob );
	STATIC_HASHED_STRING( ShowMainFrob );
	STATIC_HASHED_STRING( HideMainFrob );
	STATIC_HASHED_STRING( SetMainFrobHidden );
	STATIC_HASHED_STRING( SetFriendlyName );
	STATIC_HASHED_STRING( SetFrobVerb );
	STATIC_HASHED_STRING( SetFrobHoldVerb );
	STATIC_HASHED_STRING( SetBoundExtents );
	STATIC_HASHED_STRING( SetBoundOffsetZ );
	STATIC_HASHED_STRING( SetFrobAngles );
	STATIC_HASHED_STRING( SetFrobPriority );
	STATIC_HASHED_STRING( OnRagdollTicked );
	STATIC_HASHED_STRING( OnRagdollStopped );

	const HashedString EventName = Event.GetEventName();

	if( EventName == sMarshalFrob )
	{
		STATIC_HASHED_STRING( Frobber );
		WBEntity* const pFrobber = Event.GetEntity( sFrobber );

		STATIC_HASHED_STRING( Input );
		const uint Input = Event.GetInt( sInput );

		MarshalFrob( pFrobber, Input );
	}
	else if( EventName == sOnInitialized )
	{
		if( m_UseCollisionExtents )
		{
			WBCompRosaCollision* const pCollision = WB_GETCOMP( GetEntity(), RosaCollision );
			if( pCollision )
			{
				// This should already have transform scale in it
				m_BoundExtents = pCollision->GetExtents() + Vector( m_ExtentsFatten, m_ExtentsFatten, m_ExtentsFatten );
			}
		}
	}
	else if( EventName == sOnInitialTransformSet )
	{
		// For initialization of certain classes of static entities, fix up extents based on orientation.
		// ROSANOTE: This should play nice with doors now that I guarantee order of component initialization and event handling
		if( m_BoundExtents.x != m_BoundExtents.y )
		{
			STATIC_HASHED_STRING( Orientation );
			const Angles	Orientation	= Event.GetAngles( sOrientation );
			const float		Yaw			= Mod( TWOPI + Orientation.Yaw, TWOPI );

			if( Equal( Yaw, DEGREES_TO_RADIANS( 90.0f ) ) ||
				Equal( Yaw, DEGREES_TO_RADIANS( 270.0f ) ) )
			{
				Swap( m_BoundExtents.x, m_BoundExtents.y );
			}
		}
	}
	else if( EventName == sOnDestroyed )
	{
		if( GetIsFrobTarget() )
		{
			SetHUDHidden( true );
		}

		if( GetIsAimTarget() )
		{
			ResetAimHUD();
		}
	}
	else if( EventName == sOnMeshUpdated )
	{
		ASSERT( m_UseMeshExtents );

		WBCompRosaTransform* const	pTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		WBCompRosaMesh* const		pMeshComponent	= WB_GETCOMP( GetEntity(), RosaMesh );
		DEVASSERT( pMeshComponent );

		RosaMesh* const			pMesh			= pMeshComponent->GetMesh();
		DEVASSERT( pMesh );

		// This should already have transform scale in it
		m_BoundExtents	= pMesh->m_AABB.GetExtents() + Vector( m_ExtentsFatten, m_ExtentsFatten, m_ExtentsFatten );
		m_BoundOffset	= pMesh->m_AABB.GetCenter() - pTransform->GetLocation();
	}
	else if( EventName == sOnHealthChanged )
	{
		// HACKHACK: Update HUD when our health changes, since health component doesn't know about being targeted.
		if( GetIsAimTarget() )
		{
			SetAimHealthBar();
		}
	}
	else if( EventName == sSetIsFrobbable )
	{
		STATIC_HASHED_STRING( IsFrobbable );
		m_IsFrobbable = Event.GetBool( sIsFrobbable );
	}
	else if( EventName == sBecomeFrobbable )
	{
		m_IsFrobbable = true;
	}
	else if( EventName == sBecomeNonFrobbable )
	{
		m_IsFrobbable = false;
	}
	else if( EventName == sSetCanBeAimedAt )
	{
		STATIC_HASHED_STRING( CanBeAimedAt );
		m_CanBeAimedAt = Event.GetBool( sCanBeAimedAt );
	}
	else if( EventName == sEnableCanBeAimedAt )
	{
		m_CanBeAimedAt = true;
	}
	else if( EventName == sDisableCanBeAimedAt )
	{
		m_CanBeAimedAt = false;
	}
	else if( EventName == sSetHoldReleaseMode )
	{
		STATIC_HASHED_STRING( HoldReleaseMode );
		const bool HoldReleaseMode = Event.GetBool( sHoldReleaseMode );

		SetHoldReleaseMode( HoldReleaseMode );
	}
	else if( EventName == sEnableHoldReleaseMode )
	{
		SetHoldReleaseMode( true );
	}
	else if( EventName == sDisableHoldReleaseMode )
	{
		SetHoldReleaseMode( false );
	}
	else if( EventName == sSetMainFrobEnabled )
	{
		STATIC_HASHED_STRING( Enabled );
		m_MainFrobDisabled = !Event.GetBool( sEnabled );
	}
	else if( EventName == sSetMainFrobDisabled )
	{
		STATIC_HASHED_STRING( Disabled );
		m_MainFrobDisabled = Event.GetBool( sDisabled );
	}
	else if( EventName == sEnableMainFrob )
	{
		m_MainFrobDisabled = false;
	}
	else if( EventName == sDisableMainFrob )
	{
		m_MainFrobDisabled = true;
	}
	else if( EventName == sShowMainFrob )
	{
		m_MainFrobHidden = false;
	}
	else if( EventName == sHideMainFrob )
	{
		m_MainFrobHidden = true;
	}
	else if( EventName == sSetMainFrobHidden )
	{
		STATIC_HASHED_STRING( Hidden );
		m_MainFrobHidden = Event.GetBool( sHidden );
	}
	else if( EventName == sSetFriendlyName )
	{
		STATIC_HASHED_STRING( FriendlyName );
		m_FriendlyName = Event.GetString( sFriendlyName );

		if( GetIsFrobTarget() )
		{
			PublishToHUD();
		}

		if( GetIsAimTarget() )
		{
			SetAimHUD();
		}
	}
	else if( EventName == sSetFrobVerb )
	{
		STATIC_HASHED_STRING( FrobVerb );
		m_FrobVerb = Event.GetString( sFrobVerb );

		if( GetIsFrobTarget() )
		{
			PublishToHUD();
		}
	}
	else if( EventName == sSetFrobHoldVerb )
	{
		STATIC_HASHED_STRING( FrobHoldVerb );
		m_HoldVerb = Event.GetString( sFrobHoldVerb );

		if( GetIsFrobTarget() )
		{
			PublishToHUD();
		}
	}
	else if( EventName == sSetBoundExtents )
	{
		STATIC_HASHED_STRING( BoundExtents );
		m_BoundExtents = Event.GetVector( sBoundExtents );
	}
	else if( EventName == sSetBoundOffsetZ )
	{
		STATIC_HASHED_STRING( BoundOffsetZ );
		m_BoundOffset.z = Event.GetFloat( sBoundOffsetZ );
	}
	else if( EventName == sSetFrobAngles )
	{
		STATIC_HASHED_STRING( FrobAngleLow );
		m_CosFrobAngleLow = Cos( DEGREES_TO_RADIANS( Event.GetFloat( sFrobAngleLow ) ) );

		STATIC_HASHED_STRING( FrobAngleHigh );
		m_CosFrobAngleHigh = Cos( DEGREES_TO_RADIANS( Event.GetFloat( sFrobAngleHigh ) ) );

		ASSERT( m_CosFrobAngleHigh <= m_CosFrobAngleLow );
	}
	else if( EventName == sSetFrobPriority )
	{
		STATIC_HASHED_STRING( FrobPriority );
		m_FrobPriority = Event.GetInt( sFrobPriority );
	}
	else if( EventName == sOnRagdollTicked )
	{
		STATIC_HASHED_STRING( WSMassBoundsMin );
		m_OverrideBounds.m_Min = Event.GetVector( sWSMassBoundsMin );

		STATIC_HASHED_STRING( WSMassBoundsMax );
		m_OverrideBounds.m_Max = Event.GetVector( sWSMassBoundsMax );
	}
	else if( EventName == sOnRagdollStopped )
	{
		m_OverrideBounds = AABB();
	}
}

/*virtual*/ void WBCompRosaFrobbable::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsFrobbable, m_IsFrobbable );
	WB_SET_CONTEXT( Event, Bool, CanBeAimedAt, m_CanBeAimedAt );
}

void WBCompRosaFrobbable::SetHoldReleaseMode( const bool HoldReleaseMode )
{
	const bool WasHoldReleaseMode	= m_HoldReleaseMode;
	m_HoldReleaseMode				= HoldReleaseMode;

	if( HoldReleaseMode && !WasHoldReleaseMode )
	{
		m_HandleHoldRelease	= false;
	}

	if( GetIsFrobTarget() )
	{
		PublishToHUD();
	}
}

AABB WBCompRosaFrobbable::GetBound() const
{
	if( m_OverrideBounds.IsZero() )
	{
		WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		return AABB::CreateFromCenterAndExtents( pTransform->GetLocation() + m_BoundOffset, m_BoundExtents );
	}
	else
	{
		return m_OverrideBounds;
	}
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaFrobbable::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	const AABB Bounds = GetBound();
	GetFramework()->GetRenderer()->DEBUGDrawBox( Bounds.m_Min, Bounds.m_Max, ARGB_TO_COLOR( 255, 255, 128, 0 ) );
}
#endif

void WBCompRosaFrobbable::SetIsFrobTarget( const bool IsFrobTarget, WBEntity* const pFrobber )
{
	m_IsProbableFrobbable = IsFrobTarget;

	if( IsFrobTarget )
	{
		// Default highlight is 1,1,1,0 because RGB highlight is multiplicative. (See also WBCompRosaMesh)
		static const Vector4	skDefaultHighlight	= Vector4( 1.0f, 1.0f, 1.0f, 0.0f );
		const bool				HasFrobAction		= ( !m_MainFrobDisabled || m_HoldReleaseMode );
		const Vector4			Highlight			= HasFrobAction ? m_Highlight : skDefaultHighlight;
		WB_MAKE_EVENT( OnBecameFrobTarget, GetEntity() );
		WB_SET_AUTO( OnBecameFrobTarget, Entity, Frobber, pFrobber );
		WB_SET_AUTO( OnBecameFrobTarget, Vector, HighlightRGB, Vector( Highlight ) );
		WB_SET_AUTO( OnBecameFrobTarget, Float, HighlightA, Highlight.a );
		WB_DISPATCH_EVENT( GetEventManager(), OnBecameFrobTarget, GetEntity() );

		PublishToHUD();
	}
	else
	{
		WB_MAKE_EVENT( OnUnbecameFrobTarget, GetEntity() );
		WB_SET_AUTO( OnUnbecameFrobTarget, Entity, Frobber, pFrobber );
		WB_DISPATCH_EVENT( GetEventManager(), OnUnbecameFrobTarget, GetEntity() );

		SetHUDHidden( true );
	}
}

void WBCompRosaFrobbable::SetIsAimTarget( const bool IsAimTarget )
{
	m_IsAimTarget = IsAimTarget;

	if( IsAimTarget )
	{
		SetAimHUD();
	}
	else
	{
		ResetAimHUD();
	}
}

void WBCompRosaFrobbable::SetAimHUD() const
{
	STATICHASH( HUD );
	STATICHASH( AimName );
	ConfigManager::SetString( sAimName, m_FriendlyName.CStr(), sHUD );

	STATICHASH( AimLiteralName );
	ConfigManager::SetString( sAimLiteralName, m_LiteralName.CStr(), sHUD );

	RosaFactions::EFactionCon FactionCon = WBCompRosaFaction::GetCon( RosaGame::GetPlayer(), GetEntity() );
	SetCrosshairsHidden(
		FactionCon != RosaFactions::EFR_Neutral,
		FactionCon != RosaFactions::EFR_Friendly,
		FactionCon != RosaFactions::EFR_Hostile );

	if( FactionCon == RosaFactions::EFR_Hostile )
	{
		SetAimHUDHidden( false );
		SetAimHealthBar();
	}
	else
	{
		SetAimHUDHidden( true );
	}
}

void WBCompRosaFrobbable::ResetAimHUD() const
{
	SetCrosshairsHidden( false, true, true );
	SetAimHUDHidden( true );
}

void WBCompRosaFrobbable::SetAimHUDHidden( const bool Hidden ) const
{
	UIManager* const pUIManager = GetFramework()->GetUIManager();
	ASSERT( pUIManager );

	STATIC_HASHED_STRING( HUD );
	STATIC_HASHED_STRING( AimName );
	STATIC_HASHED_STRING( AimLiteralName );
	STATIC_HASHED_STRING( AimHealthBarBack );
	STATIC_HASHED_STRING( AimHealthBar );

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAimName );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAimLiteralName );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAimHealthBarBack );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sAimHealthBar );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}
}

void WBCompRosaFrobbable::SetCrosshairsHidden( const bool Default, const bool Friendly, const bool Hostile ) const
{
	UIManager* const pUIManager = GetFramework()->GetUIManager();
	ASSERT( pUIManager );

	STATIC_HASHED_STRING( HUD );
	STATIC_HASHED_STRING( Crosshair );
	STATIC_HASHED_STRING( Crosshair_Friendly );
	STATIC_HASHED_STRING( Crosshair_Hostile );

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Default );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair_Friendly );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Friendly );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair_Hostile );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hostile );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}
}

void WBCompRosaFrobbable::SetAimHealthBar() const
{
	// Shrink health bar
	WBCompRosaHealth* const pHealth = WB_GETCOMP( GetEntity(), RosaHealth );
	if( pHealth )
	{
		STATIC_HASHED_STRING( HUD );			// HACKHACK: Hard-coded name
		STATIC_HASHED_STRING( AimHealthBar );	// HACKHACK: Hard-coded name
		WB_MAKE_EVENT( SetWidgetExtentsScalar, NULL );
		WB_SET_AUTO( SetWidgetExtentsScalar, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetExtentsScalar, Hash, Widget, sAimHealthBar );
		WB_SET_AUTO( SetWidgetExtentsScalar, Float, W, pHealth->GetHealthAlpha() );
		WB_SET_AUTO( SetWidgetExtentsScalar, Float, H, 1.0f );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetExtentsScalar, NULL );
	}
}

void WBCompRosaFrobbable::PublishToHUD() const
{
	STATICHASH( HUD );

	STATICHASH( FrobName );
	ConfigManager::SetString( sFrobName, m_FriendlyName.CStr(), sHUD );

	STATICHASH( FrobLiteralName );
	ConfigManager::SetString( sFrobLiteralName, m_LiteralName.CStr(), sHUD );

	STATICHASH( FrobVerb );
	ConfigManager::SetString( sFrobVerb, m_FrobVerb.CStr(), sHUD );

	STATICHASH( HoldVerb );
	ConfigManager::SetString( sHoldVerb, m_HoldVerb.CStr(), sHUD );

	SetHUDHidden( false );
}

void WBCompRosaFrobbable::SetHUDHidden( const bool Hidden ) const
{
	UIManager* const pUIManager = GetFramework()->GetUIManager();
	ASSERT( pUIManager );

	STATIC_HASHED_STRING( HUD );
	STATIC_HASHED_STRING( FrobName );
	STATIC_HASHED_STRING( FrobLiteralName );
	STATIC_HASHED_STRING( FrobVerb );
	STATIC_HASHED_STRING( FrobHold );

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sFrobName );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sFrobLiteralName );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sFrobVerb );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden || m_MainFrobDisabled || m_MainFrobHidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sFrobHold );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden || !m_HoldReleaseMode );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}
}

void WBCompRosaFrobbable::MarshalFrob( WBEntity* const pFrobber, const uint Input )
{
	if( m_HoldReleaseMode )
	{
		if( INPUT_TEST( Input, INPUT_ONRISE ) )
		{
			m_HandleHoldRelease = true;
		}
		else if( INPUT_TEST( Input, INPUT_ONHOLD ) && m_HandleHoldRelease )
		{
			SendOnFrobbedHeldEvent( pFrobber );
			m_HandleHoldRelease = false;	// NOTE: This means we won't get the OnRelease event for this input! That's what I want currently, but maybe not always.
		}
		else if( INPUT_TEST( Input, INPUT_ONFALL ) && m_HandleHoldRelease )
		{
			if( !m_MainFrobDisabled )
			{
				SendOnFrobbedEvent( pFrobber );
			}
			m_HandleHoldRelease = false;
		}
	}
	else
	{
		if( INPUT_TEST( Input, INPUT_ONRISE ) )
		{
			if( !m_MainFrobDisabled )
			{
				SendOnFrobbedEvent( pFrobber );
			}
		}
	}
}

void WBCompRosaFrobbable::SendOnFrobbedEvent( WBEntity* const pFrobber ) const
{
	WB_MAKE_EVENT( OnFrobbed, GetEntity() );
	WB_SET_AUTO( OnFrobbed, Entity, Frobber, pFrobber );
	WB_DISPATCH_EVENT( GetEventManager(), OnFrobbed, GetEntity() );
}

void WBCompRosaFrobbable::SendOnFrobbedHeldEvent( WBEntity* const pFrobber ) const
{
	WB_MAKE_EVENT( OnFrobbedHeld, GetEntity() );
	WB_SET_AUTO( OnFrobbedHeld, Entity, Frobber, pFrobber );
	WB_DISPATCH_EVENT( GetEventManager(), OnFrobbedHeld, GetEntity() );
}

#define VERSION_EMPTY				0
#define VERSION_ISFROBBABLE			1
#define VERSION_HOLDRELEASEMODE		2
#define VERSION_FRIENDLYNAME		3
#define VERSION_FROBVERB			4
#define VERSION_BOUNDS				5
#define VERSION_FROBANGLES			6
#define VERSION_MAINFROBDISABLED	7
#define VERSION_HOLDVERB			8
#define VERSION_FROBPRIORITY		9
#define VERSION_CANBEAIMEDAT		10
#define VERSION_LITERALNAME			11
#define VERSION_MAINFROBHIDDEN		12
#define VERSION_CURRENT				12

uint WBCompRosaFrobbable::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_IsFrobbable
	Size += 1;	// m_CanBeAimedAt
	Size += 1;	// m_HoldReleaseMode
	Size += IDataStream::SizeForWriteString( m_FriendlyName );
	Size += IDataStream::SizeForWriteString( m_LiteralName );
	Size += IDataStream::SizeForWriteString( m_FrobVerb );

	Size += sizeof( Vector );	// m_BoundOffset
	Size += sizeof( Vector );	// m_BoundExtents

	Size += 8;	// m_CosFrobAngleLow/High

	Size += 1;	// m_MainFrobDisabled
	Size += 1;	// m_MainFrobHidden

	Size += IDataStream::SizeForWriteString( m_HoldVerb );

	Size += 4;	// m_FrobPriority

	return Size;
}

void WBCompRosaFrobbable::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_IsFrobbable );
	Stream.WriteBool( m_CanBeAimedAt );

	Stream.WriteBool( m_HoldReleaseMode );

	Stream.WriteString( m_FriendlyName );
	Stream.WriteString( m_LiteralName );

	Stream.WriteString( m_FrobVerb );

	Stream.Write( sizeof( Vector ), &m_BoundOffset );
	Stream.Write( sizeof( Vector ), &m_BoundExtents );

	Stream.WriteFloat( m_CosFrobAngleLow );
	Stream.WriteFloat( m_CosFrobAngleHigh );

	Stream.WriteBool( m_MainFrobDisabled );
	Stream.WriteBool( m_MainFrobHidden );

	Stream.WriteString( m_HoldVerb );

	Stream.WriteInt32( m_FrobPriority );
}

void WBCompRosaFrobbable::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ISFROBBABLE )
	{
		m_IsFrobbable = Stream.ReadBool();
	}

	if( Version >= VERSION_CANBEAIMEDAT )
	{
		m_CanBeAimedAt = Stream.ReadBool();
	}

	if( Version >= VERSION_HOLDRELEASEMODE )
	{
		m_HoldReleaseMode = Stream.ReadBool();
	}

	if( Version >= VERSION_FRIENDLYNAME )
	{
		m_FriendlyName = Stream.ReadString();
	}

	if( Version >= VERSION_LITERALNAME )
	{
		m_LiteralName = Stream.ReadString();
	}

	if( Version >= VERSION_FROBVERB )
	{
		m_FrobVerb = Stream.ReadString();
	}

	if( Version >= VERSION_BOUNDS )
	{
		Stream.Read( sizeof( Vector ), &m_BoundOffset );
		Stream.Read( sizeof( Vector ), &m_BoundExtents );
	}

	if( Version >= VERSION_FROBANGLES )
	{
		m_CosFrobAngleLow = Stream.ReadFloat();
		m_CosFrobAngleHigh = Stream.ReadFloat();
	}

	if( Version >= VERSION_MAINFROBDISABLED )
	{
		m_MainFrobDisabled = Stream.ReadBool();
	}

	if( Version >= VERSION_MAINFROBHIDDEN )
	{
		m_MainFrobHidden = Stream.ReadBool();
	}

	if( Version >= VERSION_HOLDVERB )
	{
		m_HoldVerb = Stream.ReadString();
	}

	if( Version >= VERSION_FROBPRIORITY )
	{
		m_FrobPriority = Stream.ReadInt32();
	}
}
