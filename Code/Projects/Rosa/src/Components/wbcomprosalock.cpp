#include "core.h"
#include "wbcomprosalock.h"
#include "rosagame.h"
#include "wbeventmanager.h"
#include "configmanager.h"
#include "mathcore.h"
#include "wbcomprosatransform.h"

WBCompRosaLock::WBCompRosaLock()
:	m_LockDef()
,	m_UseCameraOverride( false )
,	m_CameraTranslation()
,	m_CameraOrientation()
{
}

WBCompRosaLock::~WBCompRosaLock()
{
}

/*virtual*/ void WBCompRosaLock::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( LockDef );
	m_LockDef = ConfigManager::GetInheritedString( sLockDef, "", sDefinitionName );

	STATICHASH( UseCameraOverride );
	m_UseCameraOverride = ConfigManager::GetInheritedBool( sUseCameraOverride, false, sDefinitionName );

	STATICHASH( CameraTranslationX );
	m_CameraTranslation.x = ConfigManager::GetInheritedFloat( sCameraTranslationX, 0.0f, sDefinitionName );

	STATICHASH( CameraTranslationY );
	m_CameraTranslation.y = ConfigManager::GetInheritedFloat( sCameraTranslationY, 0.0f, sDefinitionName );

	STATICHASH( CameraTranslationZ );
	m_CameraTranslation.z = ConfigManager::GetInheritedFloat( sCameraTranslationZ, 0.0f, sDefinitionName );

	STATICHASH( CameraOrientationPitch );
	m_CameraOrientation.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sCameraOrientationPitch, 0.0f, sDefinitionName ) );

	STATICHASH( CameraOrientationYaw );
	m_CameraOrientation.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sCameraOrientationYaw, 0.0f, sDefinitionName ) );

	STATICHASH( CameraOrientationRoll );
	m_CameraOrientation.Roll = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sCameraOrientationRoll, 0.0f, sDefinitionName ) );
}

/*virtual*/ void WBCompRosaLock::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnBecameFrobTarget );
	STATIC_HASHED_STRING( StartLockpick );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnBecameFrobTarget )
	{
		// Publish the lock name for the dynamic frob verb string
		STATICHASH( WBCompRosaLock );
		STATICHASH( LockDef );
		ConfigManager::SetString( sLockDef, m_LockDef.CStr(), sWBCompRosaLock );
	}
	else if( EventName == sStartLockpick )
	{
		// Tell the player that they are lockpicking this entity.
		{
			STATIC_HASHED_STRING( LockpickTarget );
			WB_MAKE_EVENT( SetVariable, NULL );
			WB_SET_AUTO( SetVariable, Hash, Name, sLockpickTarget );
			WB_SET_AUTO( SetVariable, Entity, Value, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), SetVariable, RosaGame::GetPlayer() );
		}

		// Tell the lockpicking minigame to start
		{
			WBCompRosaTransform* const	pTransform			= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
			DEVASSERT( pTransform );
			const Matrix				OrientationMatrix	= pTransform->GetOrientation().ToMatrix();
			const Vector				CameraTranslation	= pTransform->GetLocation() + ( m_CameraTranslation * OrientationMatrix );
			const Angles				CameraOrientation	= pTransform->GetOrientation() + m_CameraOrientation;

			WB_MAKE_EVENT(	BeginLockpicking, GetEntity() );
			WB_SET_AUTO(	BeginLockpicking, Hash,		LockDef,			m_LockDef );
			WB_SET_AUTO(	BeginLockpicking, Bool,		UseCameraOverride,	m_UseCameraOverride );
			WB_SET_AUTO(	BeginLockpicking, Vector,	CameraTranslation,	CameraTranslation );
			WB_SET_AUTO(	BeginLockpicking, Angles,	CameraOrientation,	CameraOrientation );
			WB_DISPATCH_EVENT( GetEventManager(), BeginLockpicking, NULL );
		}
	}
}
