#include "core.h"
#include "wbcomprosaspin.h"
#include "wbcomprosatransform.h"
#include "configmanager.h"
#include "wbentity.h"
#include "wbevent.h"
#include "idatastream.h"
#include "matrix.h"
#include "mathcore.h"

WBCompRosaSpin::WBCompRosaSpin()
:	m_Axis()
,	m_Velocity( 0.0f )
,	m_DampingScalar( 0.0f )
{
}

WBCompRosaSpin::~WBCompRosaSpin()
{
}

/*virtual*/ void WBCompRosaSpin::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AxisX );
	m_Axis.x = ConfigManager::GetInheritedFloat( sAxisX, 0.0f, sDefinitionName );

	STATICHASH( AxisY );
	m_Axis.y = ConfigManager::GetInheritedFloat( sAxisY, 0.0f, sDefinitionName );

	STATICHASH( AxisZ );
	m_Axis.z = ConfigManager::GetInheritedFloat( sAxisZ, 0.0f, sDefinitionName );

	m_Axis.Normalize();

	STATICHASH( Velocity );
	m_Velocity = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sVelocity, 0.0f, sDefinitionName ) );

	STATICHASH( DampingScalar );
	m_DampingScalar = ConfigManager::GetInheritedFloat( sDampingScalar, 0.0f, sDefinitionName );
}

void WBCompRosaSpin::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();

	const Matrix	OrientationMatrix	= pTransform->GetOrientation().ToMatrix();
	const Matrix	SpinMatrixOS		= Matrix::CreateRotation( m_Axis, m_Velocity * DeltaTime );
	const Matrix	SpinMatrixWS		= SpinMatrixOS * OrientationMatrix;
	const Angles	NewOrientation		= SpinMatrixWS.ToAngles();

	pTransform->SetOrientation( NewOrientation );
}

/*virtual*/ void WBCompRosaSpin::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( DampSpin );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sDampSpin )
	{
		m_Velocity *= m_DampingScalar;
	}
}

#define VERSION_EMPTY		0
#define VERSION_VELOCITY	1
#define VERSION_CURRENT		1

uint WBCompRosaSpin::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_Velocity

	return Size;
}

void WBCompRosaSpin::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteFloat( m_Velocity );
}

void WBCompRosaSpin::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_VELOCITY )
	{
		m_Velocity = Stream.ReadFloat();
	}
}
