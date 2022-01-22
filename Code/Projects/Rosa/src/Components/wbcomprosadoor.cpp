#include "core.h"
#include "wbcomprosadoor.h"
#include "wbcomprosamesh.h"
#include "wbcomprosacollision.h"
#include "wbcomprosatransform.h"
#include "wbcomprosafrobbable.h"
#include "wbcomprosakeyring.h"
#include "wbcomprosaplayer.h"
#include "wbeventmanager.h"
#include "rosaworld.h"
#include "collisioninfo.h"
#include "configmanager.h"
#include "idatastream.h"
#include "aabb.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "rosanav.h"

WBCompRosaDoor::WBCompRosaDoor()
:	m_Open( false )
,	m_Locked( false )
,	m_HasKeycode( false )
,	m_Keycode( 0 )
,	m_Keycards()
,	m_InterpTime( 0.0f )
,	m_ClosedOffset()
,	m_ClosedOrientation()
,	m_ClosedFrobOffset()
,	m_ClosedFrobExtents()
,	m_OpenOffset()
,	m_OpenOrientation()
,	m_OpenFrobOffset()
,	m_OpenFrobExtents()
,	m_ClosedFrobPriority( 0 )
,	m_OpenFrobPriority( 0 )
,	m_OffsetInterpolator()
,	m_OrientationInterpolator()
,	m_WaitingToRetoggle( false )
,	m_RetoggleTime( 0.0f )
,	m_RetoggleEntity()
,	m_UnlockedMesh()
,	m_LockedMesh()
,	m_UnlockedTexture()
,	m_LockedTexture()
,	m_UnlockedFriendlyName()
,	m_LockedFriendlyName()
,	m_ClosedTexture()
,	m_OpenTexture()
,	m_NextAITryHandleTime( 0.0f )
,	m_AITryHandleTimeMin( 0.0f )
,	m_AITryHandleTimeMax( 0.0f )
,	m_AITryHandleDistSq( 0.0f )
{
}

WBCompRosaDoor::~WBCompRosaDoor()
{
}

/*virtual*/ void WBCompRosaDoor::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( StartOpen );
	m_Open = ConfigManager::GetInheritedBool( sStartOpen, false, sDefinitionName );

	STATICHASH( InterpTime );
	m_InterpTime = ConfigManager::GetInheritedFloat( sInterpTime, 0.0f, sDefinitionName );

	STATICHASH( ClosedOffsetX );
	m_ClosedOffset.x = ConfigManager::GetInheritedFloat( sClosedOffsetX, 0.0f, sDefinitionName );

	STATICHASH( ClosedOffsetY );
	m_ClosedOffset.y = ConfigManager::GetInheritedFloat( sClosedOffsetY, 0.0f, sDefinitionName );

	STATICHASH( ClosedOffsetZ );
	m_ClosedOffset.z = ConfigManager::GetInheritedFloat( sClosedOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( ClosedAngle );
	m_ClosedOrientation.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sClosedAngle, 0.0f, sDefinitionName ) );

	STATICHASH( ClosedFrobOffsetX );
	m_ClosedFrobOffset.x = ConfigManager::GetInheritedFloat( sClosedFrobOffsetX, 0.0f, sDefinitionName );

	STATICHASH( ClosedFrobOffsetY );
	m_ClosedFrobOffset.y = ConfigManager::GetInheritedFloat( sClosedFrobOffsetY, 0.0f, sDefinitionName );

	STATICHASH( ClosedFrobOffsetZ );
	m_ClosedFrobOffset.z = ConfigManager::GetInheritedFloat( sClosedFrobOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( ClosedFrobBoxX );
	m_ClosedFrobExtents.x = ConfigManager::GetInheritedFloat( sClosedFrobBoxX, 0.0f, sDefinitionName );

	STATICHASH( ClosedFrobBoxY );
	m_ClosedFrobExtents.y = ConfigManager::GetInheritedFloat( sClosedFrobBoxY, 0.0f, sDefinitionName );

	STATICHASH( ClosedFrobBoxZ );
	m_ClosedFrobExtents.z = ConfigManager::GetInheritedFloat( sClosedFrobBoxZ, 0.0f, sDefinitionName );

	STATICHASH( OpenOffsetX );
	m_OpenOffset.x = ConfigManager::GetInheritedFloat( sOpenOffsetX, 0.0f, sDefinitionName );

	STATICHASH( OpenOffsetY );
	m_OpenOffset.y = ConfigManager::GetInheritedFloat( sOpenOffsetY, 0.0f, sDefinitionName );

	STATICHASH( OpenOffsetZ );
	m_OpenOffset.z = ConfigManager::GetInheritedFloat( sOpenOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( OpenPitch );
	m_OpenOrientation.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOpenPitch, 0.0f, sDefinitionName ) );

	STATICHASH( OpenRoll );
	m_OpenOrientation.Roll = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOpenRoll, 0.0f, sDefinitionName ) );

	STATICHASH( OpenYaw );
	m_OpenOrientation.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOpenYaw, 0.0f, sDefinitionName ) );

	STATICHASH( OpenFrobOffsetX );
	m_OpenFrobOffset.x = ConfigManager::GetInheritedFloat( sOpenFrobOffsetX, 0.0f, sDefinitionName );

	STATICHASH( OpenFrobOffsetY );
	m_OpenFrobOffset.y = ConfigManager::GetInheritedFloat( sOpenFrobOffsetY, 0.0f, sDefinitionName );

	STATICHASH( OpenFrobOffsetZ );
	m_OpenFrobOffset.z = ConfigManager::GetInheritedFloat( sOpenFrobOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( OpenFrobBoxX );
	m_OpenFrobExtents.x = ConfigManager::GetInheritedFloat( sOpenFrobBoxX, 0.0f, sDefinitionName );

	STATICHASH( OpenFrobBoxY );
	m_OpenFrobExtents.y = ConfigManager::GetInheritedFloat( sOpenFrobBoxY, 0.0f, sDefinitionName );

	STATICHASH( OpenFrobBoxZ );
	m_OpenFrobExtents.z = ConfigManager::GetInheritedFloat( sOpenFrobBoxZ, 0.0f, sDefinitionName );

	STATICHASH( ClosedFrobPriority );
	m_ClosedFrobPriority = ConfigManager::GetInheritedInt( sClosedFrobPriority, 0, sDefinitionName );

	STATICHASH( OpenFrobPriority );
	m_OpenFrobPriority = ConfigManager::GetInheritedInt( sOpenFrobPriority, 0, sDefinitionName );

	STATICHASH( Locked );
	m_Locked = ConfigManager::GetInheritedBool( sLocked, false, sDefinitionName );

	STATICHASH( Keycode );
	m_Keycode = ConfigManager::GetInheritedInt( sKeycode, 0, sDefinitionName );

	// We implicitly have a keycode if m_Keycode is non-zero. Config var is also used to allow 0000 as a valid code.
	STATICHASH( HasKeycode );
	m_HasKeycode = ( m_Keycode > 0 ) || ConfigManager::GetInheritedBool( sHasKeycode, false, sDefinitionName );

	STATICHASH( Keycard );
	const HashedString Keycard = ConfigManager::GetInheritedHash( sKeycard, HashedString::NullString, sDefinitionName );
	if( Keycard != HashedString::NullString )
	{
		m_Keycards.PushBack( Keycard );
	}

	STATICHASH( NumKeycards );
	const uint NumKeycards = ConfigManager::GetInheritedInt( sNumKeycards, 0, sDefinitionName );
	for( uint KeycardIndex = 0; KeycardIndex < NumKeycards; ++KeycardIndex )
	{
		const HashedString IndexedKeycard = ConfigManager::GetInheritedSequenceHash( "Keycard%d", KeycardIndex, HashedString::NullString, sDefinitionName );
		m_Keycards.PushBack( IndexedKeycard );
	}

	STATICHASH( UnlockedMesh );
	m_UnlockedMesh = ConfigManager::GetInheritedString( sUnlockedMesh, "", sDefinitionName );

	STATICHASH( LockedMesh );
	m_LockedMesh = ConfigManager::GetInheritedString( sLockedMesh, "", sDefinitionName );

	STATICHASH( UnlockedTexture );
	m_UnlockedTexture = ConfigManager::GetInheritedString( sUnlockedTexture, "", sDefinitionName );

	STATICHASH( LockedTexture );
	m_LockedTexture = ConfigManager::GetInheritedString( sLockedTexture, "", sDefinitionName );

	STATICHASH( UnlockedFriendlyName );
	m_UnlockedFriendlyName = ConfigManager::GetInheritedString( sUnlockedFriendlyName, "", sDefinitionName );

	STATICHASH( LockedFriendlyName );
	m_LockedFriendlyName = ConfigManager::GetInheritedString( sLockedFriendlyName, "", sDefinitionName );

	STATICHASH( ClosedTexture );
	m_ClosedTexture = ConfigManager::GetInheritedString( sClosedTexture, "", sDefinitionName );

	STATICHASH( OpenTexture );
	m_OpenTexture = ConfigManager::GetInheritedString( sOpenTexture, "", sDefinitionName );

	STATICHASH( AITryHandleTimeMin );
	m_AITryHandleTimeMin = ConfigManager::GetInheritedFloat( sAITryHandleTimeMin, 0.0f, sDefinitionName );

	STATICHASH( AITryHandleTimeMax );
	m_AITryHandleTimeMax = ConfigManager::GetInheritedFloat( sAITryHandleTimeMax, m_AITryHandleTimeMin, sDefinitionName );

	DEVASSERT( m_AITryHandleTimeMin <= m_AITryHandleTimeMax );

	STATICHASH( AITryHandleDistance );
	m_AITryHandleDistSq = Square( ConfigManager::GetInheritedFloat( sAITryHandleDistance, 0.0f, sDefinitionName ) );
}

WBCompRosaDoor::EDoorRotation WBCompRosaDoor::GetRotation() const
{
	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	const float					Yaw			= Mod( TWOPI + pTransform->GetOrientation().Yaw, TWOPI );

	if( Equal( Yaw, DEGREES_TO_RADIANS( 0.0f ) ) )		{ return EDR_0; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 90.0f ) ) )		{ return EDR_90; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 180.0f ) ) )	{ return EDR_180; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 270.0f ) ) )	{ return EDR_270; }
	if( Equal( Yaw, DEGREES_TO_RADIANS( 360.0f ) ) )	{ return EDR_0; }

	WARN;
	return EDR_0;
}

void WBCompRosaDoor::AdjustForFacing()
{
	EDoorRotation Rotation = GetRotation();

	if( Rotation == EDR_0 )
	{
		return;
	}

	m_ClosedOffset				= RotateOffset(		m_ClosedOffset,				Rotation );
	m_ClosedOrientation.Yaw		= RotateYaw(		m_ClosedOrientation.Yaw,	Rotation );
	m_ClosedFrobOffset			= RotateOffset(		m_ClosedFrobOffset,			Rotation );
	m_ClosedFrobExtents			= RotateExtents(	m_ClosedFrobExtents,		Rotation );
	m_OpenOffset				= RotateOffset(		m_OpenOffset,				Rotation );
	m_OpenOrientation.Yaw		= RotateYaw(		m_OpenOrientation.Yaw,		Rotation );
	m_OpenFrobOffset			= RotateOffset(		m_OpenFrobOffset,			Rotation );
	m_OpenFrobExtents			= RotateExtents(	m_OpenFrobExtents,			Rotation );

	// NOTE: Collision gets updated from facing in RosaCollision component and doesn't change when opening/closing the door.
}

Vector WBCompRosaDoor::RotateExtents( const Vector& Extents, const EDoorRotation Rotation ) const
{
	ASSERT( Rotation != EDR_0 );

	return ( Rotation == EDR_180 ) ? Extents : Vector( Extents.y, Extents.x, Extents.z );
}

Vector WBCompRosaDoor::RotateOffset( const Vector& Offset, const EDoorRotation Rotation ) const
{
	ASSERT( Rotation != EDR_0 );

	if( Rotation == EDR_90 )	{ return Vector(	-Offset.y,	Offset.x,	Offset.z ); }
	if( Rotation == EDR_180 )	{ return Vector(	-Offset.x,	-Offset.y,	Offset.z ); }
	if( Rotation == EDR_270 )	{ return Vector(	Offset.y,	-Offset.x,	Offset.z ); }

	WARN;
	return Offset;
}

float WBCompRosaDoor::RotateYaw( const float Yaw, const EDoorRotation Rotation ) const
{
	ASSERT( Rotation != EDR_0 );

	if( Rotation == EDR_90 )	{ return Yaw + PI * 0.5f; }
	if( Rotation == EDR_180 )	{ return Yaw + PI; }
	if( Rotation == EDR_270 )	{ return Yaw + PI * 1.5f; }

	WARN;
	return Yaw;
}

/*virtual*/ void WBCompRosaDoor::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( Toggle );
	STATIC_HASHED_STRING( Open );
	STATIC_HASHED_STRING( Close );
	STATIC_HASHED_STRING( TryToggle );
	STATIC_HASHED_STRING( OnInitialTransformSet );
	STATIC_HASHED_STRING( OnLoaded );
	STATIC_HASHED_STRING( Lock );
	STATIC_HASHED_STRING( Unlock );

	const HashedString EventName = Event.GetEventName();

	if( EventName == sToggle )
	{
		STATIC_HASHED_STRING( Frobber );
		WBEntity* const	pFrobber		= Event.GetEntity( sFrobber );

		const bool		ForceCanOpen	= true;

		TryToggle( pFrobber, 0.0f, ForceCanOpen );
	}
	else if( EventName == sOpen )
	{
		if( m_Open )
		{
			// If we try to open a door while it's already open, just cancel the retoggle if any
			m_WaitingToRetoggle = false;
		}
		else
		{
			STATIC_HASHED_STRING( Frobber );
			WBEntity* const	pFrobber		= Event.GetEntity( sFrobber );

			const bool		ForceCanOpen	= true;

			TryToggle( pFrobber, 0.0f, ForceCanOpen );
		}
	}
	else if( EventName == sClose )
	{
		if( m_Open )
		{
			STATIC_HASHED_STRING( Frobber );
			WBEntity* const	pFrobber		= Event.GetEntity( sFrobber );

			const bool		ForceCanOpen	= true;

			TryToggle( pFrobber, 0.0f, ForceCanOpen );
		}
		else
		{
			// If we try to close a door while it's already closed, just cancel the retoggle if any
			m_WaitingToRetoggle = false;
		}
	}
	else if( EventName == sTryToggle )
	{
		STATIC_HASHED_STRING( Frobber );
		WBEntity* const	pFrobber		= Event.GetEntity( sFrobber );

		STATIC_HASHED_STRING( RetoggleTime );
		const float		RetoggleTime	= Event.GetFloat( sRetoggleTime );

		const bool		ForceCanOpen	= false;

		TryToggle( pFrobber, RetoggleTime, ForceCanOpen );
	}
	else if( EventName == sOnInitialTransformSet )
	{
		AdjustForFacing();
		UpdateFromOpenState( true, false );
	}
	else if( EventName == sOnLoaded )
	{
		UpdateFromOpenState( false, true );
	}
	else if( EventName == sLock )
	{
		Lock();
	}
	else if( EventName == sUnlock )
	{
		Unlock();
	}
}

/*virtual*/ void WBCompRosaDoor::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsDoor,	true );
	WB_SET_CONTEXT( Event, Bool, Open,		m_Open );
	WB_SET_CONTEXT( Event, Bool, Closed,	!m_Open );
	WB_SET_CONTEXT( Event, Bool, Locked,	m_Locked );
	WB_SET_CONTEXT( Event, Bool, Unlocked,	!m_Locked );
}

/*virtual*/ void WBCompRosaDoor::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	if( m_WaitingToRetoggle && GetTime() >= m_RetoggleTime )
	{
		m_WaitingToRetoggle = false;

		const bool ForceCanOpen = true;
		TryToggle( m_RetoggleEntity.Get(), 0.0f, ForceCanOpen );
	}

	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaMesh* const		pMesh		= WB_GETCOMP( pEntity, RosaMesh );

	DEVASSERT( pTransform );
	DEVASSERT( pMesh );

	m_OffsetInterpolator.Tick( DeltaTime );
	m_OrientationInterpolator.Tick( DeltaTime );

	pMesh->SetMeshOffset( m_OffsetInterpolator.GetValue() );
	pTransform->SetOrientation( m_OrientationInterpolator.GetValue() );
}

bool WBCompRosaDoor::CanOpenDoor( WBEntity* const pFrobber )
{
	if( !pFrobber )
	{
		return true;
	}

	// Well, this is an abuse of the system. But it makes it easy to deal
	// with AI motion, and I can hack things further for the player.
	WBEvent ContextEvent;
	pFrobber->AddContextToEvent( ContextEvent );

	STATIC_HASHED_STRING( CanOpenDoors );
	const bool FrobberCanOpenDoors = ContextEvent.GetBool( sCanOpenDoors );

	if( !FrobberCanOpenDoors )
	{
		return false;
	}

	if( !m_Locked )
	{
		return true;
	}

	if( IsInsideDoor( pFrobber ) )
	{
		return true;
	}

	STATIC_HASHED_STRING( CanUnlockDoors );
	const bool FrobberCanUnlockDoors = ContextEvent.GetBool( sCanUnlockDoors );

	if( FrobberCanUnlockDoors )
	{
		Unlock();
		return true;
	}

	return false;
}

bool WBCompRosaDoor::IsInsideDoor( WBEntity* const pEntity ) const
{
	WBCompRosaTransform* const pDoorTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaTransform* const pEntityTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();

	const Vector	EntityLocation	= pEntityTransform->GetLocation();
	const Vector	DoorLocation	= pDoorTransform->GetLocation();
	const Vector	DoorToEntity	= EntityLocation - DoorLocation;
	const Vector	DoorFacing		= m_ClosedOrientation.ToVector();
	const float		Dot				= DoorFacing.Dot( DoorToEntity );
	const bool		IsInsideDoor	= ( Dot > 0.0f );

	return IsInsideDoor;
}

void WBCompRosaDoor::TryToggle( WBEntity* const pFrobber, const float RetoggleTime, const bool ForceCanOpen )
{
	// ROSANOTE: RetoggleTime is a timer after which the door will be automatically toggled again.
	// If it is 0.0, we will not retoggle (default behavior).

	if( m_Open )
	{
		RosaWorld* const pWorld = GetWorld();
		DEVASSERT( pWorld );

		WBEntity* const pEntity = GetEntity();
		DEVASSERT( pEntity );

		WBCompRosaTransform* const pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
		WBCompRosaCollision* const pCollision	= WB_GETCOMP( pEntity, RosaCollision );
		DEVASSERT( pTransform );
		DEVASSERT( pCollision );

		CollisionInfo Info;
		Info.m_In_CollideEntities	= true;
		Info.m_In_CollidingEntity	= pEntity;
		Info.m_In_UserFlags			= EECF_CollideAsWorld | EECF_CollideStaticEntities | EECF_CollideDynamicEntities;

		if( pWorld->CheckClearance( pTransform->GetLocation(), pCollision->GetExtents(), Info ) )
		{
			// Door is blocked from closing
		}
		else
		{
			Toggle( pFrobber );

			m_WaitingToRetoggle	= ( RetoggleTime > 0.0f );
			m_RetoggleTime		= GetTime() + RetoggleTime;
			m_RetoggleEntity	= pFrobber;
		}
	}
	else
	{
		WBEventManager* const pEventManager = GetEventManager();

		if( ForceCanOpen || CanOpenDoor( pFrobber ) )
		{
			Toggle( pFrobber );

			WB_MAKE_EVENT( TryToggleSucceeded, GetEntity() );
			WB_SET_AUTO( TryToggleSucceeded, Entity, Frobber, pFrobber );
			WB_DISPATCH_EVENT( pEventManager, TryToggleSucceeded, GetEntity() );

			m_WaitingToRetoggle	= ( RetoggleTime > 0.0f );
			m_RetoggleTime		= GetTime() + RetoggleTime;
			m_RetoggleEntity	= pFrobber;
		}
		else
		{
			WBCompRosaPlayer* const pPlayer = WB_GETCOMP_SAFE( pFrobber, RosaPlayer );
			if( m_HasKeycode && pPlayer )
			{
				// Tell the frobber that they are opening this door.
				STATIC_HASHED_STRING( KeypadDoor );
				WB_MAKE_EVENT( SetVariable, NULL );
				WB_SET_AUTO( SetVariable, Hash, Name, sKeypadDoor );
				WB_SET_AUTO( SetVariable, Entity, Value, GetEntity() );
				WB_DISPATCH_EVENT( pEventManager, SetVariable, pFrobber );

				// Tell the keypad screen what the code is
				WB_MAKE_EVENT( SetKeycode, NULL );
				WB_SET_AUTO( SetKeycode, Int, Keycode, m_Keycode );
				WB_DISPATCH_EVENT( pEventManager, SetKeycode, NULL );

				// Open the keypad screen
				STATIC_HASHED_STRING( KeypadScreen );
				WB_MAKE_EVENT( PushUIScreen, NULL );
				WB_SET_AUTO( PushUIScreen, Hash, Screen, sKeypadScreen );
				WB_DISPATCH_EVENT( pEventManager, PushUIScreen, NULL );
			}
			else
			{
				WB_MAKE_EVENT( TryToggleFailed, GetEntity() );
				WB_SET_AUTO( TryToggleFailed, Entity, Frobber, pFrobber );
				WB_DISPATCH_EVENT( pEventManager, TryToggleFailed, GetEntity() );

				// Tell the frobber it failed so it can repath if it is an AI
				WB_MAKE_EVENT( DoorOpenFailed, NULL );
				WB_DISPATCH_EVENT( pEventManager, DoorOpenFailed, pFrobber );
			}
		}
	}
}

void WBCompRosaDoor::Toggle( WBEntity* const pFrobber )
{
	m_Open = !m_Open;
	UpdateFromOpenState( false, false );

	if( m_Open )
	{
		WB_MAKE_EVENT( OnOpened, GetEntity() );
		WB_SET_AUTO( OnOpened, Entity, Frobber, pFrobber );
		WB_DISPATCH_EVENT( GetEventManager(), OnOpened, GetEntity() );
	}
	else
	{
		WB_MAKE_EVENT( OnClosed, GetEntity() );
		WB_SET_AUTO( OnClosed, Entity, Frobber, pFrobber );
		WB_DISPATCH_EVENT( GetEventManager(), OnClosed, GetEntity() );
	}
}

void WBCompRosaDoor::Lock()
{
	WBEntity* const				pEntity		= GetEntity();
	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	// If the door is closed, clear current nav state since we'll be adding the locked flag
	if( !m_Open )
	{
		RosaNav::GetInstance()->UpdateWorldFromEntity( pCollision, false );
	}

	m_Locked = true;

	if( m_LockedMesh != "" )
	{
		WB_MAKE_EVENT( SetMesh, GetEntity() );
		WB_SET_AUTO( SetMesh, Hash, Mesh, m_LockedMesh );
		WB_DISPATCH_EVENT( GetEventManager(), SetMesh, GetEntity() );
	}

	if( m_LockedTexture != "" )
	{
		WB_MAKE_EVENT( SetTexture, GetEntity() );
		WB_SET_AUTO( SetTexture, Hash, Texture, m_LockedTexture );
		WB_DISPATCH_EVENT( GetEventManager(), SetTexture, GetEntity() );
	}

	if( m_LockedFriendlyName != "" )
	{
		WB_MAKE_EVENT( SetFriendlyName, GetEntity() );
		WB_SET_AUTO( SetFriendlyName, Hash, FriendlyName, m_LockedFriendlyName );
		WB_DISPATCH_EVENT( GetEventManager(), SetFriendlyName, GetEntity() );
	}

	// If the door is closed, update nav state to add the locked flag
	if( !m_Open )
	{
		RosaNav::GetInstance()->UpdateWorldFromEntity( pCollision, true );
	}

	WB_MAKE_EVENT( OnLocked, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnLocked, GetEntity() );
}

void WBCompRosaDoor::Unlock()
{
	WBEntity* const				pEntity		= GetEntity();
	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	// If the door is closed, clear current nav state since we'll be removing the locked flag
	if( !m_Open )
	{
		RosaNav::GetInstance()->UpdateWorldFromEntity( pCollision, false );
	}

	m_Locked = false;

	if( m_UnlockedMesh != "" )
	{
		WB_MAKE_EVENT( SetMesh, GetEntity() );
		WB_SET_AUTO( SetMesh, Hash, Mesh, m_UnlockedMesh );
		WB_DISPATCH_EVENT( GetEventManager(), SetMesh, GetEntity() );
	}

	if( m_UnlockedTexture != "" )
	{
		WB_MAKE_EVENT( SetTexture, GetEntity() );
		WB_SET_AUTO( SetTexture, Hash, Texture, m_UnlockedTexture );
		WB_DISPATCH_EVENT( GetEventManager(), SetTexture, GetEntity() );
	}

	if( m_UnlockedFriendlyName != "" )
	{
		WB_MAKE_EVENT( SetFriendlyName, GetEntity() );
		WB_SET_AUTO( SetFriendlyName, Hash, FriendlyName, m_UnlockedFriendlyName );
		WB_DISPATCH_EVENT( GetEventManager(), SetFriendlyName, GetEntity() );
	}

	// If the door is closed, update nav state to remove the locked flag
	if( !m_Open )
	{
		RosaNav::GetInstance()->UpdateWorldFromEntity( pCollision, true );
	}

	WB_MAKE_EVENT( OnUnlocked, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnUnlocked, GetEntity() );
}

void WBCompRosaDoor::UpdateFromOpenState( const bool InitialSetup, const bool LoadingSetup )
{
	WBEntity* const				pEntity		= GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	DEVASSERT( pCollision );

	WBCompRosaFrobbable* const	pFrobbable	= WB_GETCOMP( pEntity, RosaFrobbable );

	const float CurrentInterpT		= m_OffsetInterpolator.GetT();
	const float InterpTime			= ( InitialSetup || LoadingSetup ) ? 0.0f : ( m_InterpTime * CurrentInterpT );

	const uint	DoorCollisionMask	= pCollision->GetDefaultCollisionFlags() & ( EECF_BlocksOcclusion | EECF_BlocksEntities | EECF_BlocksBlockers | EECF_BlocksTrace | EECF_BlocksAudio | EECF_BlocksNav | EECF_BlocksRagdolls );

	if( m_Open )
	{
		m_OffsetInterpolator.Reset( Interpolator<Vector>::EIT_Linear, m_OffsetInterpolator.GetValue(), m_OpenOffset, InterpTime );
		m_OrientationInterpolator.Reset( Interpolator<Angles>::EIT_Linear, m_OrientationInterpolator.GetValue(), m_OpenOrientation, InterpTime );

		pCollision->SetCollisionFlags( 0, DoorCollisionMask, true );

		if( pFrobbable )
		{
			pFrobbable->SetBoundOffset( m_OpenFrobOffset );
			pFrobbable->SetBoundExtents( m_OpenFrobExtents );
			pFrobbable->SetFrobPriority( m_OpenFrobPriority );
		}

		if( m_OpenTexture != "" )
		{
			WB_MAKE_EVENT( SetTexture, GetEntity() );
			WB_SET_AUTO( SetTexture, Hash, Texture, m_OpenTexture );
			WB_DISPATCH_EVENT( GetEventManager(), SetTexture, GetEntity() );
		}

		if( LoadingSetup )
		{
			// Do nothing, a serialized open door should already be removed from the nav world.
			// For StartOpen doors, we *do* want to update this (during InitialSetup).
		}
		else
		{
			RosaNav::GetInstance()->UpdateWorldFromEntity( pCollision, false );
		}
	}
	else
	{
		m_OffsetInterpolator.Reset( Interpolator<Vector>::EIT_Linear, m_OffsetInterpolator.GetValue(), m_ClosedOffset, InterpTime );
		m_OrientationInterpolator.Reset( Interpolator<Angles>::EIT_Linear, m_OrientationInterpolator.GetValue(), m_ClosedOrientation, InterpTime );

		pCollision->SetCollisionFlags( DoorCollisionMask, DoorCollisionMask, true );

		if( pFrobbable )
		{
			pFrobbable->SetBoundOffset( m_ClosedFrobOffset );
			pFrobbable->SetBoundExtents( m_ClosedFrobExtents );
			pFrobbable->SetFrobPriority( m_ClosedFrobPriority );
		}

		if( m_ClosedTexture != "" )
		{
			WB_MAKE_EVENT( SetTexture, GetEntity() );
			WB_SET_AUTO( SetTexture, Hash, Texture, m_ClosedTexture );
			WB_DISPATCH_EVENT( GetEventManager(), SetTexture, GetEntity() );
		}

		if( InitialSetup || LoadingSetup )
		{
			// Don't double up where RosaCollision already updates the nav world from this entity.
		}
		else
		{
			RosaNav::GetInstance()->UpdateWorldFromEntity( pCollision, true );
		}
	}
}

bool WBCompRosaDoor::ShouldAITryHandle( WBEntity* const pEntity )
{
	DEVASSERT( pEntity );

	if( GetTime() < m_NextAITryHandleTime )
	{
		return false;
	}

	WBCompRosaTransform*	pDoorTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	WBCompRosaTransform*	pEntityTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
	const float				DistSq				= ( pDoorTransform->GetLocation() - pEntityTransform->GetLocation() ).LengthSquared();

	if( DistSq > m_AITryHandleDistSq )
	{
		return false;
	}

	m_NextAITryHandleTime = GetTime() + Math::Random( m_AITryHandleTimeMin, m_AITryHandleTimeMax );
	return true;
}

#define VERSION_EMPTY			0
#define VERSION_OPEN			1
#define VERSION_FACINGCONFIG	2
#define VERSION_LOCKED			3
#define VERSION_RETOGGLE		4
#define VERSION_RETOGGLEENTITY	5
#define VERSION_CURRENT			5

uint WBCompRosaDoor::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_Open

	Size += sizeof( Vector );		// m_ClosedOffset
	Size += 4;						// m_ClosedOrientation.Yaw
	Size += sizeof( Vector );		// m_ClosedFrobOffset
	Size += sizeof( Vector );		// m_ClosedFrobExtents
	Size += sizeof( Vector );		// m_OpenOffset
	Size += 4;						// m_OpenOrientation.Yaw
	Size += sizeof( Vector );		// m_OpenFrobOffset
	Size += sizeof( Vector );		// m_OpenFrobExtents

	Size += 1;						// m_Locked

	Size += 1;						// m_WaitingToRetoggle
	Size += 4;						// m_RetoggleTime

	Size += sizeof( WBEntityRef );	// m_RetoggleEntity

	return Size;
}

void WBCompRosaDoor::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Open );

	Stream.Write( sizeof( Vector ), &m_ClosedOffset );
	Stream.WriteFloat( m_ClosedOrientation.Yaw );
	Stream.Write( sizeof( Vector ), &m_ClosedFrobOffset );
	Stream.Write( sizeof( Vector ), &m_ClosedFrobExtents );
	Stream.Write( sizeof( Vector ), &m_OpenOffset );
	Stream.WriteFloat( m_OpenOrientation.Yaw );
	Stream.Write( sizeof( Vector ), &m_OpenFrobOffset );
	Stream.Write( sizeof( Vector ), &m_OpenFrobExtents );

	Stream.WriteBool( m_Locked );

	Stream.WriteBool( m_WaitingToRetoggle );
	Stream.WriteFloat( m_RetoggleTime - GetTime() );

	Stream.Write( sizeof( WBEntityRef ), &m_RetoggleEntity );
}

void WBCompRosaDoor::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_OPEN )
	{
		m_Open = Stream.ReadBool();
	}

	if( Version >= VERSION_FACINGCONFIG )
	{
		Stream.Read( sizeof( Vector ), &m_ClosedOffset );
		m_ClosedOrientation.Yaw = Stream.ReadFloat();
		Stream.Read( sizeof( Vector ), &m_ClosedFrobOffset );
		Stream.Read( sizeof( Vector ), &m_ClosedFrobExtents );
		Stream.Read( sizeof( Vector ), &m_OpenOffset );
		m_OpenOrientation.Yaw = Stream.ReadFloat();
		Stream.Read( sizeof( Vector ), &m_OpenFrobOffset );
		Stream.Read( sizeof( Vector ), &m_OpenFrobExtents );
	}

	if( Version >= VERSION_LOCKED )
	{
		m_Locked = Stream.ReadBool();
	}

	if( Version >= VERSION_RETOGGLE )
	{
		m_WaitingToRetoggle	= Stream.ReadBool();
		m_RetoggleTime		= Stream.ReadFloat() + GetTime();
	}

	if( Version >= VERSION_RETOGGLEENTITY )
	{
		Stream.Read( sizeof( WBEntityRef ), &m_RetoggleEntity );
	}
}
