#include "core.h"
#include "wbcomprosatransform.h"
#include "configmanager.h"
#include "mathcore.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "rosaworld.h"
#include "wbcomprosaplayer.h"
#include "wbcomprosacollision.h"
#include "wbcomprosacamera.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcompowner.h"
#include "Components/wbcomprosafootsteps.h"
#include "Components/wbcomprosacharacterconfig.h"
#include "Components/wbcomprosamesh.h"
#include "quat.h"
#include "irenderer.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "matrix.h"
#include "collisioninfo.h"
#include "fontmanager.h"

WBCompRosaTransform::WBCompRosaTransform()
:	m_Location()
,	m_Velocity()
,	m_Acceleration()
,	m_Gravity( 0.0f )
,	m_UseSpeedLimit( false )
,	m_SpeedLimit( 0.0f )
,	m_AllowImpulses( false )
,	m_Orientation()
,	m_RotationalVelocity()
,	m_CanMove( false )
,	m_IsAttachedToOwner( false )
,	m_OwnerOffset()
,	m_OwnerOffsetMatrix()
,	m_IsSettled( false )
,	m_Scale( 0.0f )
,	m_InterpMoveToLocation()
,	m_InterpMoveToOrientation()
{
}

WBCompRosaTransform::~WBCompRosaTransform()
{
}

/*virtual*/ void WBCompRosaTransform::Initialize()
{
	Super::Initialize();

	WBEntity* const pEntity = GetEntity();
	DEVASSERT( pEntity );
	pEntity->SetTransformComponent( this );
}

/*virtual*/ void WBCompRosaTransform::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	WBCompRosaCharacterConfig* const	pCharacterConfig	= WB_GETCOMP( GetEntity(), RosaCharacterConfig );
	const float							DefaultScale		= ( pCharacterConfig && pCharacterConfig->HasCostume() ) ? pCharacterConfig->GetCostume().m_Scale : 1.0f;

	MAKEHASH( DefinitionName );

	STATICHASH( RosaTransform );

	STATICHASH( CanMove );
	m_CanMove = ConfigManager::GetInheritedBool( sCanMove, true, sDefinitionName );

	STATICHASH( UseGravity );
	const bool UseGravity = ConfigManager::GetInheritedBool( sUseGravity, false, sDefinitionName );

	STATICHASH( Gravity );
	const float DefaultGravity = ConfigManager::GetFloat( sGravity, 0.0f, sRosaTransform );
	m_Gravity = -ConfigManager::GetInheritedFloat( sGravity, UseGravity ? DefaultGravity : 0.0f, sDefinitionName );

	STATICHASH( SpeedLimit );
	m_SpeedLimit = ConfigManager::GetInheritedFloat( sSpeedLimit, 0.0f, sDefinitionName );
	m_UseSpeedLimit = ( m_SpeedLimit > 0.0f );

	STATICHASH( AllowImpulses );
	m_AllowImpulses = ConfigManager::GetInheritedBool( sAllowImpulses, true, sDefinitionName );

	STATICHASH( Yaw );
	m_Orientation.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sYaw, 0.0f, sDefinitionName ) );

	STATICHASH( IsAttachedToOwner );
	m_IsAttachedToOwner = ConfigManager::GetInheritedBool( sIsAttachedToOwner, false, sDefinitionName );

	STATICHASH( OwnerOffsetX );
	m_OwnerOffset.x = ConfigManager::GetInheritedFloat( sOwnerOffsetX, 0.0f, sDefinitionName );

	STATICHASH( OwnerOffsetY );
	m_OwnerOffset.y = ConfigManager::GetInheritedFloat( sOwnerOffsetY, 0.0f, sDefinitionName );

	STATICHASH( OwnerOffsetZ );
	m_OwnerOffset.z = ConfigManager::GetInheritedFloat( sOwnerOffsetZ, 0.0f, sDefinitionName );

	Angles OwnerOffsetAngles;

	// HACKHACK for lowered weapon in hub
	const bool LowerWeapon = RosaFramework::GetInstance()->GetGame()->ShouldLowerWeapon();
	STATICHASH( HubOwnerOffsetPitch );
	const float HubOwnerOffsetPitch = ConfigManager::GetInheritedFloat( sHubOwnerOffsetPitch, 0.0f, sDefinitionName );
	const float DefaultOwnerOffsetPitch = LowerWeapon ? HubOwnerOffsetPitch : 0.0f;

	STATICHASH( OwnerOffsetPitch );
	OwnerOffsetAngles.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOwnerOffsetPitch, DefaultOwnerOffsetPitch, sDefinitionName ) );

	STATICHASH( OwnerOffsetRoll );
	OwnerOffsetAngles.Roll = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOwnerOffsetRoll, 0.0f, sDefinitionName ) );

	STATICHASH( OwnerOffsetYaw );
	OwnerOffsetAngles.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sOwnerOffsetYaw, 0.0f, sDefinitionName ) );

	m_OwnerOffsetMatrix = OwnerOffsetAngles.ToMatrix();

	STATICHASH( Scale );
	m_Scale = ConfigManager::GetInheritedFloat( sScale, DefaultScale, sDefinitionName );
}

void WBCompRosaTransform::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	if( m_IsAttachedToOwner )
	{
		MoveWithOwner();
	}
	else if( m_CanMove )
	{
		TickMotion( DeltaTime );
	}
}

float WBCompRosaTransform::GetStatModdedSpeedLimit() const
{
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT_SAFE( SpeedLimit, m_SpeedLimit, pStatMod );
	return WB_MODDED( SpeedLimit );
}

void WBCompRosaTransform::TickAcceleration( const float DeltaTime )
{
	XTRACE_FUNCTION;

	DEVASSERT( m_CanMove );

	if( m_UseSpeedLimit && m_Acceleration.LengthSquared() > EPSILON )
	{
		const float		StatModdedSpeedLimit	= GetStatModdedSpeedLimit();

		Vector			ComponentDirection;
		float			AccelerationLength;
		float			AccelerationLengthOverOne;
		m_Acceleration.GetNormalized( ComponentDirection, AccelerationLength, AccelerationLengthOverOne );

		const Vector	ComponentVelocity		= m_Velocity.ProjectionOnto( ComponentDirection );
		const Vector	ComplementVelocity		= m_Velocity - ComponentVelocity;
		const float		ComponentSpeed			= ComponentVelocity.Length();
		const float		SignedSpeed				= ( ComponentVelocity.Dot( ComponentDirection ) > 0.0f ) ? ComponentSpeed : -ComponentSpeed;

		// Allow component velocity to exceed the limit, but *not* by adding to it with acceleration.
		if( SignedSpeed < StatModdedSpeedLimit )
		{
			const float NewComponentSpeed = Min( SignedSpeed + AccelerationLength * DeltaTime, StatModdedSpeedLimit );
			m_Velocity = ComplementVelocity + ComponentDirection * NewComponentSpeed;
		}
	}
	else
	{
		m_Velocity += m_Acceleration * DeltaTime;
	}

	m_Velocity.z += m_Gravity * DeltaTime;
}

void WBCompRosaTransform::TickMotion( const float DeltaTime )
{
	XTRACE_FUNCTION;

	DEVASSERT( m_CanMove );

	TickAcceleration( DeltaTime );

	if( m_Velocity.LengthSquared() > EPSILON )	// Optimization, don't want to pay MoveBy costs for static entities
	{
		MoveBy( m_Velocity * DeltaTime );
	}

	if( !m_RotationalVelocity.IsZero() )
	{
		const Angles NewOrientation =  m_Orientation + m_RotationalVelocity * DeltaTime;
		SetOrientation( ClampPitch( NewOrientation ) );
	}

	if( !m_InterpMoveToLocation.IsFinished() )
	{
		m_InterpMoveToLocation.Tick( DeltaTime );
		SetLocation( m_InterpMoveToLocation.GetValue() );
	}

	if( !m_InterpMoveToOrientation.IsFinished() )
	{
		m_InterpMoveToOrientation.Tick( DeltaTime );
		SetOrientation( m_InterpMoveToOrientation.GetValue() );
	}
}

/*static*/ Angles WBCompRosaTransform::ClampPitch( const Angles& InOrientation )
{
	static const float skMaxPitch = PI * 0.49f;
	Angles RetVal = InOrientation;
	RetVal.Pitch = Clamp( InOrientation.Pitch, -skMaxPitch, skMaxPitch );
	return RetVal;
}

void WBCompRosaTransform::OnTeleport() const
{
	// NOTE: Use this when the entity has been teleported outside of its own tick,
	// to be sure the mesh is updated with the new transform and stuff.

	WB_MAKE_EVENT( OnTeleported, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnTeleported, GetEntity() );
}

void WBCompRosaTransform::SetInitialTransform( const Vector& Location, const Angles& Orientation )
{
	SetLocation( Location );
	SetOrientation( Orientation );

	WB_MAKE_EVENT( OnInitialTransformSet, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnInitialTransformSet, GetEntity() );
}

void WBCompRosaTransform::SetLocation( const Vector& NewLocation )
{
	XTRACE_FUNCTION;

	if( NewLocation == m_Location )
	{
		// Save the event call if location hasn't changed
		return;
	}

	m_Location = NewLocation;

	WB_MAKE_EVENT( OnMoved, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnMoved, GetEntity() );
}

void WBCompRosaTransform::SetOrientation( const Angles& NewOrientation )
{
	XTRACE_FUNCTION;

	if( NewOrientation == m_Orientation )
	{
		// Save the event call if orientation hasn't changed
		return;
	}

	m_Orientation = NewOrientation;

	WB_MAKE_EVENT( OnTurned, GetEntity() );
	WB_DISPATCH_EVENT( GetEventManager(), OnTurned, GetEntity() );
}

void WBCompRosaTransform::SetDefaultGravity()
{
	STATICHASH( RosaTransform );
	STATICHASH( Gravity );
	const float DefaultGravity = ConfigManager::GetFloat( sGravity, 0.0f, sRosaTransform );

	m_Gravity = -DefaultGravity;
}

void WBCompRosaTransform::MoveBy( const Vector& Offset )
{
	XTRACE_FUNCTION;

	DEVASSERT( m_CanMove );

	WBCompRosaCollision* const pCollision = WB_GETCOMP( GetEntity(), RosaCollision );

	if( pCollision )
	{
		Vector ModifiedOffset = Offset;
		const bool Collided = pCollision->Collide( m_Location, ModifiedOffset );
		SetLocation( m_Location + ModifiedOffset );

		if( Collided &&
			ModifiedOffset.LengthSquared() < EPSILON &&
			m_Velocity.LengthSquared() < 0.36f )	// ROSAHACK: Magic number, works fine for current cases
		{
			if( !m_IsSettled )
			{
				m_IsSettled = true;
				WB_MAKE_EVENT( OnSettled, GetEntity() );
				WB_DISPATCH_EVENT( GetEventManager(), OnSettled, GetEntity() );
			}
		}
		else
		{
			m_IsSettled = false;
		}
	}
	else
	{
		SetLocation( m_Location + Offset );
	}
}

void WBCompRosaTransform::MoveWithOwner()
{
	XTRACE_FUNCTION;

	DEVASSERT( m_IsAttachedToOwner );

	WBEntity* const pEntity = GetEntity();
	DEVASSERT( pEntity );

	WBCompOwner* const pOwner = pEntity->GetOwnerComponent<WBCompOwner>();
	DEVASSERT( pOwner );

	WBEntity* const pOwnerEntity = pOwner->GetOwner();
	if( pOwnerEntity )
	{
		WBCompRosaTransform* const	pOwnerTransform	= pOwnerEntity->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pOwnerTransform );

		WBCompRosaCamera* const		pOwnerCamera	= WB_GETCOMP( pOwnerEntity, RosaCamera );
		DEVASSERT( pOwnerCamera );

		WBCompRosaMesh* const		pMeshComp		= WB_GETCOMP( pEntity, RosaMesh );
		Mesh* const					pMesh			= pMeshComp ? pMeshComp->GetMesh() : NULL;
		uint						MaterialFlags	= pMesh ? pMesh->GetMaterialFlags() : MAT_NONE;
		const bool					IsForeground	= ( 0 != ( MaterialFlags & MAT_FOREGROUND ) );

		WBEvent ContextEvent;
		pOwnerEntity->AddContextToEvent( ContextEvent );

		STATIC_HASHED_STRING( IsAiming );
		const bool								IsAiming	= ContextEvent.GetBool( sIsAiming );

		STATICHASH( LeftyMode );
		const bool								LeftyMode	= ConfigManager::GetBool( sLeftyMode );

		const WBCompRosaCamera::EViewModifiers	ViewMods	= IsAiming ? WBCompRosaCamera::EVM_All : WBCompRosaCamera::EVM_All_Hands;

		const Matrix	OrientationMatrix	= pOwnerCamera->GetModifiedOrientation( ViewMods, pOwnerTransform->GetOrientation() ).ToMatrix();
		const Vector	TransformedOffset	= m_OwnerOffset * OrientationMatrix;
		const Vector	Location			= pOwnerCamera->GetModifiedTranslation( ViewMods, pOwnerTransform->GetLocation() ) + TransformedOffset;
		const Angles	Orientation			= ( m_OwnerOffsetMatrix * OrientationMatrix ).ToAngles();

		if( LeftyMode && IsForeground )
		{
			// Mirror modifications across view
			const Angles	ViewOrientationAngles	= pOwnerCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, pOwnerTransform->GetOrientation() );
			const Matrix	ViewOrientationMatrix	= ViewOrientationAngles.ToMatrix();
			const Vector	ViewLocation			= pOwnerCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pOwnerTransform->GetLocation() );
			const Matrix	ViewLocationMatrix		= Matrix::CreateTranslation( ViewLocation );
			const Matrix	ViewMatrix				= ViewOrientationMatrix * ViewLocationMatrix;

			const Matrix	ReflectionMatrixX		= Matrix::CreateReflection( Vector( 1.0f, 0.0f, 0.0f ) );
			const Matrix	LeftyMatrix				= ViewMatrix.GetInverse() * ReflectionMatrixX * ViewMatrix;
			const Vector	LeftyLocation			= Location * LeftyMatrix;

			// For orientation, we can just use the angles difference, ignoring pitch.
			// I tried to do this with the reflection matrix and could not get it to work, but this is fine.
			Angles			LeftyOrientation		= ViewOrientationAngles + ( ViewOrientationAngles - Orientation );
			LeftyOrientation.Pitch					= Orientation.Pitch;

			SetLocation( LeftyLocation );
			SetOrientation( LeftyOrientation );
		}
		else
		{
			SetLocation( Location );
			SetOrientation( Orientation );
		}
	}
}

void WBCompRosaTransform::TeleportTo( WBEntity* const pDestination, const bool ShouldSetOrientation )
{
	DEVASSERT( pDestination );

	if( !pDestination )
	{
		return;
	}

	WBCompRosaTransform* const	pDestinationTransform	= pDestination->GetTransformComponent<WBCompRosaTransform>();
	if( !pDestinationTransform )
	{
		return;
	}

	Vector			Destination							= pDestinationTransform->GetLocation();
	const Angles	Orientation							= pDestinationTransform->GetOrientation();

	WBEntity* const	pEntity								= GetEntity();
	WBCompRosaCollision* const	pCollision				= WB_GETCOMP( pEntity, RosaCollision );
	if( pCollision )
	{
		CollisionInfo Info;
		Info.m_In_CollideWorld		= true;
		Info.m_In_CollideEntities	= true;
		Info.m_In_CollidingEntity	= pEntity;
		Info.m_In_UserFlags			= EECF_BlockerCollision;

		RosaWorld* const pWorld = RosaFramework::GetInstance()->GetWorld();
		if( !pWorld->FindSpot( Destination, pCollision->GetExtents(), Info ) )
		{
			return;
		}
	}

	SetLocation(		Destination );
	SetVelocity(		Vector() );
	SetAcceleration(	Vector() );

	if( ShouldSetOrientation )
	{
		SetOrientation(	Orientation );
	}
}

void WBCompRosaTransform::CopyOwnerOffset( const WBEntity* const pSourceEntity )
{
	if( !pSourceEntity )
	{
		return;
	}

	const WBCompRosaTransform* const pSourceTransform = pSourceEntity->GetTransformComponent<WBCompRosaTransform>();
	if( !pSourceTransform )
	{
		return;
	}

	m_OwnerOffset		= pSourceTransform->m_OwnerOffset;
	m_OwnerOffsetMatrix	= pSourceTransform->m_OwnerOffsetMatrix;
}

/*virtual*/ void WBCompRosaTransform::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( ApplyImpulse );
	STATIC_HASHED_STRING( DisableGravity );
	STATIC_HASHED_STRING( EnableGravity );
	STATIC_HASHED_STRING( SetDefaultGravity );
	STATIC_HASHED_STRING( SetAcceleration );
	STATIC_HASHED_STRING( SetCanMove );
	STATIC_HASHED_STRING( MoveBy );
	STATIC_HASHED_STRING( SetLocation );
	STATIC_HASHED_STRING( SetOrientation );
	STATIC_HASHED_STRING( TeleportTo );
	STATIC_HASHED_STRING( TeleportBy );
	STATIC_HASHED_STRING( KillMotion );
	STATIC_HASHED_STRING( CopyOwnerOffset );
	STATIC_HASHED_STRING( InterpMoveTo );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sApplyImpulse )
	{
		if( m_AllowImpulses && m_CanMove )
		{
			STATIC_HASHED_STRING( Impulse );
			const Vector Impulse = Event.GetVector( sImpulse );
			ApplyImpulse( Impulse );

			m_IsSettled = false;
		}
	}
	else if( EventName == sDisableGravity )
	{
		m_Gravity = 0.0f;
	}
	else if( EventName == sEnableGravity || EventName == sSetDefaultGravity )
	{
		SetDefaultGravity();
	}
	else if( EventName == sSetAcceleration )
	{
		STATIC_HASHED_STRING( Acceleration );
		const Vector Acceleration = Event.GetVector( sAcceleration );

		SetAcceleration( Acceleration );
	}
	else if( EventName == sSetCanMove )
	{
		STATICHASH( CanMove );
		const bool CanMove = Event.GetBool( sCanMove );

		SetCanMove( CanMove );
	}
	else if( EventName == sMoveBy )
	{
		STATIC_HASHED_STRING( Offset );
		const Vector Offset = Event.GetVector( sOffset );

		MoveBy( Offset );
	}
	else if( EventName == sSetLocation )
	{
		STATIC_HASHED_STRING( NewLocation );
		const Vector NewLocation = Event.GetVector( sNewLocation );

		SetLocation( NewLocation );
	}
	else if( EventName == sSetOrientation )
	{
		STATIC_HASHED_STRING( NewOrientation );
		const Angles NewOrientation = Event.GetAngles( sNewOrientation );

		SetOrientation( NewOrientation );
	}
	else if( EventName == sTeleportTo )
	{
		STATIC_HASHED_STRING( Destination );
		WBEntity* const pDestination = Event.GetEntity( sDestination );

		STATIC_HASHED_STRING( SetOrientation );
		const bool SetOrientation = Event.GetBool( sSetOrientation, true );

		TeleportTo( pDestination, SetOrientation );
	}
	else if( EventName == sTeleportBy )
	{
		STATIC_HASHED_STRING( Offset );
		const Vector Offset = Event.GetVector( sOffset );

		// HACKHACK: Update footstep location before move
		WBCompRosaFootsteps* const pFootsteps = WB_GETCOMP( GetEntity(), RosaFootsteps );
		if( pFootsteps )
		{
			pFootsteps->TeleportBy( Offset );
		}

		SetLocation( m_Location + Offset );
	}
	else if( EventName == sKillMotion )
	{
		m_Velocity.Zero();
		m_Acceleration.Zero();
		m_RotationalVelocity.Zero();
	}
	else if( EventName == sCopyOwnerOffset )
	{
		STATIC_HASHED_STRING( Source );
		const WBEntity* const pSourceEntity = Event.GetEntity( sSource );

		CopyOwnerOffset( pSourceEntity );
	}
	else if( EventName == sInterpMoveTo )
	{
		STATIC_HASHED_STRING( Location );
		const Vector Location = Event.GetVector( sLocation );

		STATIC_HASHED_STRING( Orientation );
		const Angles Orientation = Event.GetAngles( sOrientation );

		STATIC_HASHED_STRING( Duration );
		const float Duration = Event.GetFloat( sDuration );

		STATIC_HASHED_STRING( EaseIn );
		const bool EaseIn = Event.GetBool( sEaseIn );

		STATIC_HASHED_STRING( EaseOut );
		const bool EaseOut = Event.GetBool( sEaseOut );

		m_InterpMoveToLocation.Reset( m_InterpMoveToLocation.GetInterpolationType( EaseIn, EaseOut ), m_Location, Location, Duration );
		m_InterpMoveToOrientation.Reset( m_InterpMoveToOrientation.GetInterpolationType( EaseIn, EaseOut ), m_Orientation, Orientation, Duration );
	}
}

/*virtual*/ void WBCompRosaTransform::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Vector, Location, m_Location );
	WB_SET_CONTEXT( Event, Angles, Orientation, m_Orientation );
	WB_SET_CONTEXT( Event, Vector, Velocity, m_Velocity );
	WB_SET_CONTEXT( Event, Float, SpeedSq, m_Velocity.LengthSquared() );
	WB_SET_CONTEXT( Event, Float, Scale, m_Scale );
}

#if BUILD_DEV
void WBCompRosaTransform::Report() const
{
	Super::Report();

	PRINTF( WBPROPERTY_REPORT_PREFIX "Location: %s\n", m_Location.GetString().CStr() );
	PRINTF( WBPROPERTY_REPORT_PREFIX "Orientation: %s\n", m_Orientation.GetString().CStr() );
	if( !m_Velocity.IsZero() ) { PRINTF( WBPROPERTY_REPORT_PREFIX "Velocity: %s\n", m_Velocity.GetString().CStr() ); }
	if( !m_Acceleration.IsZero() ) { PRINTF( WBPROPERTY_REPORT_PREFIX "Acceleration: %s\n", m_Acceleration.GetString().CStr() ); }
}

/*virtual*/ void WBCompRosaTransform::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const		pFramework	= RosaFramework::GetInstance();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();

	pRenderer->DEBUGDrawCross( m_Location, 0.25f, ARGB_TO_COLOR( 255, 255, 255, 255 ) );

	pRenderer->DEBUGPrint(
		SimpleString::PrintF(
			"%sLocation: %s",
			DebugRenderLineFeed().CStr(),
			m_Location.GetString().CStr()
		),
		m_Location,
		pView,
		pDisplay,
		DEFAULT_FONT_TAG,
		ARGB_TO_COLOR( 255, 255, 255, 255 ),
		ARGB_TO_COLOR( 255, 0, 0, 0 ) );
}
#endif

#define VERSION_EMPTY			0
#define VERSION_LOCATION		1
#define VERSION_VELOCITY		2
#define VERSION_ORIENTATION		3
#define VERSION_ACCELERATION	4
#define VERSION_ISSETTLED		5
#define VERSION_GRAVITY			6
#define VERSION_CANMOVE			7
#define VERSION_ROTVELOCITY		8
#define VERSION_SCALE			9
#define VERSION_CURRENT			9

uint WBCompRosaTransform::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += sizeof( Vector );	// m_Location
	Size += sizeof( Vector );	// m_Velocity
	Size += sizeof( Vector );	// m_Acceleration
	Size += sizeof( Angles );	// m_Orientation
	Size += sizeof( Angles );	// m_RotationalVelocity
	Size += 1;					// m_IsSettled
	Size += 4;					// m_Gravity
	Size += 1;					// m_CanMove
	Size += 4;					// m_Scale

	return Size;
}

void WBCompRosaTransform::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.Write( sizeof( Vector ), &m_Location );
	Stream.Write( sizeof( Vector ), &m_Velocity );
	Stream.Write( sizeof( Vector ), &m_Acceleration );
	Stream.Write( sizeof( Angles ), &m_Orientation );
	Stream.Write( sizeof( Angles ), &m_RotationalVelocity );
	Stream.WriteBool( m_IsSettled );
	Stream.WriteFloat( m_Gravity );
	Stream.WriteBool( m_CanMove );
	Stream.WriteFloat( m_Scale );
}

void WBCompRosaTransform::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_LOCATION )
	{
		Stream.Read( sizeof( Vector ), &m_Location );
	}

	if( Version >= VERSION_VELOCITY )
	{
		Stream.Read( sizeof( Vector ), &m_Velocity );
	}

	if( Version >= VERSION_ACCELERATION )
	{
		Stream.Read( sizeof( Vector ), &m_Acceleration );
	}

	if( Version >= VERSION_ORIENTATION )
	{
		Stream.Read( sizeof( Angles ), &m_Orientation );
	}

	if( Version >= VERSION_ROTVELOCITY )
	{
		Stream.Read( sizeof( Angles ), &m_RotationalVelocity );
	}

	if( Version >= VERSION_ISSETTLED )
	{
		m_IsSettled = Stream.ReadBool();
	}

	if( Version >= VERSION_GRAVITY )
	{
		m_Gravity = Stream.ReadFloat();
	}

	if( Version >= VERSION_CANMOVE )
	{
		m_CanMove = Stream.ReadBool();
	}

	if( Version >= VERSION_SCALE )
	{
		m_Scale = Stream.ReadFloat();
	}
}
