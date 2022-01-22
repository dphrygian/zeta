#include "core.h"
#include "wbcomprosadamageeffects.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "mesh.h"
#include "wbcomprosatransform.h"
#include "Components/wbcompowner.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "irenderer.h"
#include "meshfactory.h"
#include "texturemanager.h"
#include "ivertexdeclaration.h"
#include "idatastream.h"
#include "display.h"
#include "mathcore.h"

WBCompRosaDamageEffects::WBCompRosaDamageEffects()
:	m_DamageRecords()
,	m_Duration( 0.0f )
,	m_InvFadeDuration( 0.0f )
,	m_Material()
,	m_Texture( NULL )
,	m_Radius( 0.0f )
,	m_Size( 0.0f )
,	m_OverlayScreenName()
,	m_OverlayWidgetName()
,	m_OverlayDuration( 0.0f )
,	m_InvOverlayDuration( 0.0f )
,	m_OverlayExpireTime( 0.0f )
,	m_LastOverlayAlpha( 0.0f )
,	m_ConfigEnabled( false )
{
	STATIC_HASHED_STRING( OnSetRes );
	GetEventManager()->AddObserver( sOnSetRes, this );

	STATIC_HASHED_STRING( OnShowHUDChanged );
	GetEventManager()->AddObserver( sOnShowHUDChanged, this );
}

WBCompRosaDamageEffects::~WBCompRosaDamageEffects()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnSetRes );
		pEventManager->RemoveObserver( sOnSetRes, this );

		STATIC_HASHED_STRING( OnShowHUDChanged );
		pEventManager->RemoveObserver( sOnShowHUDChanged, this );
	}

	FOR_EACH_MAP( DamageRecordIter, m_DamageRecords, WBEntityRef, SDamageRecord )
	{
		SDamageRecord& DamageRecord = DamageRecordIter.GetValue();
		SafeDelete( DamageRecord.m_Mesh );
	}
}

/*virtual*/ void WBCompRosaDamageEffects::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Duration );
	m_Duration = ConfigManager::GetInheritedFloat( sDuration, 0.0f, sDefinitionName );

	STATICHASH( FadeDuration );
	m_InvFadeDuration = 1.0f / ConfigManager::GetInheritedFloat( sFadeDuration, 0.0f, sDefinitionName );

	STATICHASH( Material );
	m_Material = ConfigManager::GetInheritedString( sMaterial, "", sDefinitionName );

	STATICHASH( Texture );
	const SimpleString Texture = ConfigManager::GetInheritedString( sTexture, "", sDefinitionName );
	m_Texture = GetFramework()->GetRenderer()->GetTextureManager()->GetTexture( Texture.CStr() );

	STATICHASH( Radius );
	m_Radius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName );

	STATICHASH( Size );
	m_Size = ConfigManager::GetInheritedFloat( sSize, 0.0f, sDefinitionName );

	STATICHASH( OverlayScreenName );
	m_OverlayScreenName = ConfigManager::GetInheritedHash( sOverlayScreenName, HashedString::NullString, sDefinitionName );

	STATICHASH( OverlayWidgetName );
	m_OverlayWidgetName = ConfigManager::GetInheritedHash( sOverlayWidgetName, HashedString::NullString, sDefinitionName );

	STATICHASH( OverlayDuration );
	m_OverlayDuration = ConfigManager::GetInheritedFloat( sOverlayDuration, 0.0f, sDefinitionName );
	m_InvOverlayDuration = 1.0f / m_OverlayDuration;

	// Mirror the config option
	STATICHASH( ShowHUD );
	m_ConfigEnabled = ConfigManager::GetBool( sShowHUD );
}

/*virtual*/ void WBCompRosaDamageEffects::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnShowHUDChanged );
	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( OnDamaged );
	STATIC_HASHED_STRING( OnSetRes );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnShowHUDChanged )
	{
		STATICHASH( ShowHUD );
		m_ConfigEnabled = ConfigManager::GetBool( sShowHUD );
	}
	else if( EventName == sOnInitialized )
	{
		const bool ForceUpdate = true;
		UpdateDamageOverlay( ForceUpdate );
	}
	else if( EventName == sOnDamaged )
	{
		STATIC_HASHED_STRING( Damager );
		WBEntity* const pDamager = Event.GetEntity( sDamager );

		OnDamaged( pDamager );
	}
	else if( EventName == sOnSetRes )
	{
		// Update mesh transforms for new display
		FOR_EACH_MAP( DamageRecordIter, m_DamageRecords, WBEntityRef, SDamageRecord )
		{
			SDamageRecord& DamageRecord = DamageRecordIter.GetValue();
			UpdateDamageTransform( DamageRecord );
		}
	}
}

void WBCompRosaDamageEffects::OnDamaged( WBEntity* const pDamager )
{
	const float Time = GetTime();

	// Update overlay regardless of damager
	m_OverlayExpireTime = Time + m_OverlayDuration;

	const bool ForceUpdate = true;
	UpdateDamageOverlay( ForceUpdate );

	WBEntity* const pDamagerOwner = WBCompOwner::GetTopmostOwner( pDamager );

	if( pDamagerOwner == NULL ||
		pDamagerOwner == GetEntity() )
	{
		// Ignore damage that we can't point at
		return;
	}

	WBCompRosaTransform* const pTransform = pDamagerOwner->GetTransformComponent<WBCompRosaTransform>();
	if( !pTransform )
	{
		// Ignore damage that we can't point at
		return;
	}

	SDamageRecord& DamageRecord	= m_DamageRecords[ pDamagerOwner ];

	if( !DamageRecord.m_Mesh )
	{
		DamageRecord.m_Mesh = CreateMesh();
	}

	DamageRecord.m_Location					= pTransform->GetLocation();
	DamageRecord.m_ExpireTime				= Time + m_Duration;
	DamageRecord.m_Mesh->m_ConstantColor	= Vector4( 1.0f, 1.0f, 1.0f, 1.0f );

	UpdateDamageTransform( DamageRecord );
}

void WBCompRosaDamageEffects::UpdateDamageTransform( SDamageRecord& DamageRecord )
{
	RosaFramework* const		pFramework		= GetFramework();
	Display* const				pDisplay		= pFramework->GetDisplay();
	const float					DisplayWidth	= static_cast<float>( pDisplay->m_Width );
	const float					DisplayHeight	= static_cast<float>( pDisplay->m_Height );
	const float					ActualRadius	= DisplayHeight * m_Radius;
	const float					ActualSize		= DisplayHeight * m_Size;

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();

	const Vector				Location		= pTransform->GetLocation();
	const Vector				Facing			= pTransform->GetOrientation().ToVector2D();
	const Vector				Right			= Facing.Cross( Vector::Up ).GetFastNormalized(); // DLP 8 Nov 2019: This didn't used to be normalized, not sure if it could've been a problem
	const Vector				ToDamager		= ( DamageRecord.m_Location - Location ).Get2D().GetFastNormalized();
	const float					CosAngle		= Facing.Dot( ToDamager );
	const float					CosRight		= Right.Dot( ToDamager );
	const float					SignedRoll		= ACos( CosAngle ) * ( ( CosRight < 0.0f ) ? 1.0f : -1.0f );

	const float					CenterX			= 0.5f * DisplayWidth;
	const float					CenterY			= 0.5f * DisplayHeight;
	const float					X				= ( CenterX + ( ActualRadius * CosRight ) );
	const float					Y				= ( CenterY - ( ActualRadius * CosAngle ) );

	DEVASSERT( DamageRecord.m_Mesh );
	DamageRecord.m_Mesh->m_Location	= Vector( X, 0.0f, Y );
	DamageRecord.m_Mesh->m_Rotation	= Angles( 0.0f, SignedRoll, 0.0f );
	DamageRecord.m_Mesh->m_Scale	= Vector( ActualSize, 1.0f, ActualSize );
}

void WBCompRosaDamageEffects::UpdateDamageOverlay( const bool ForceUpdate )
{
	const float	TimeRemaining	= m_OverlayExpireTime - GetTime();
	const float	Alpha			= Saturate( TimeRemaining * m_InvOverlayDuration );

	if( !ForceUpdate && Alpha == m_LastOverlayAlpha )
	{
		return;
	}

	m_LastOverlayAlpha = Alpha;

	WB_MAKE_EVENT( SetWidgetAlpha, NULL );
	WB_SET_AUTO( SetWidgetAlpha, Hash, Screen, m_OverlayScreenName );
	WB_SET_AUTO( SetWidgetAlpha, Hash, Widget, m_OverlayWidgetName );
	WB_SET_AUTO( SetWidgetAlpha, Float, Alpha, m_LastOverlayAlpha );
	WB_DISPATCH_EVENT( GetEventManager(), SetWidgetAlpha, NULL );
}

Mesh* WBCompRosaDamageEffects::CreateMesh() const
{
	XTRACE_FUNCTION;

	IRenderer* const pRenderer = GetFramework()->GetRenderer();

	Mesh* const pMesh = pRenderer->GetMeshFactory()->CreateSprite();
	pMesh->SetMaterialDefinition( m_Material, pRenderer );
	pMesh->SetTexture( 0, m_Texture );
	pMesh->SetMaterialFlags( MAT_HUD );
#if BUILD_DEBUG
	pMesh->m_DEBUG_Name = "DamageEffects";
#endif

	return pMesh;
}

/*virtual*/ void WBCompRosaDamageEffects::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	const float Time = GetTime();

	FOR_EACH_MAP_NOINCR( DamageRecordIter, m_DamageRecords, WBEntityRef, SDamageRecord )
	{
		WBEntityRef&	DamagerRef		= DamageRecordIter.GetKey();
		SDamageRecord&	DamageRecord	= DamageRecordIter.GetValue();

		const float TimeRemaining = DamageRecord.m_ExpireTime - Time;
		if( TimeRemaining <= 0.0f )
		{
			SafeDelete( DamageRecord.m_Mesh );
			m_DamageRecords.Remove( DamageRecordIter );
			continue;
		}

		// Fade out at end
		DamageRecord.m_Mesh->m_ConstantColor.a = Saturate( TimeRemaining * m_InvFadeDuration );

		WBEntity* const pDamager = DamagerRef.Get();
		if( pDamager )
		{
			// Update location from entity if it's still around
			DamageRecord.m_Location = pDamager->GetTransformComponent<WBCompRosaTransform>()->GetLocation();
		}

		// Update mesh transforms
		UpdateDamageTransform( DamageRecord );

		++DamageRecordIter;
	}

	const bool ForceUpdate = false;
	UpdateDamageOverlay( ForceUpdate );
}

/*virtual*/ void WBCompRosaDamageEffects::Render()
{
	if( !m_ConfigEnabled )
	{
		return;
	}

	IRenderer* const pRenderer = GetFramework()->GetRenderer();

	FOR_EACH_MAP( DamageRecordIter, m_DamageRecords, WBEntityRef, SDamageRecord )
	{
		const SDamageRecord& DamageRecord = DamageRecordIter.GetValue();

		DEVASSERT( DamageRecord.m_Mesh );
		pRenderer->AddMesh( DamageRecord.m_Mesh );
	}
}
