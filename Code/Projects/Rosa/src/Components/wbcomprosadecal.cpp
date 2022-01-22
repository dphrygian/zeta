#include "core.h"
#include "wbcomprosadecal.h"
#include "wbcomprosatransform.h"
#include "idatastream.h"
#include "configmanager.h"
#include "wbeventmanager.h"

WBCompRosaDecal::WBCompRosaDecal()
:	m_Lifetime( 0.0f )
,	m_FadeOutTime( 0.0f )
,	m_InvFadeOutTime( 0.0f )
,	m_ExpireTime( 0.0f )
,	m_PrescribedNormalBasis( false )
,	m_NormalBasis()
{
}

WBCompRosaDecal::~WBCompRosaDecal()
{
}

/*virtual*/ void WBCompRosaDecal::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Lifetime );
	m_Lifetime = ConfigManager::GetInheritedFloat( sLifetime, 0.0f, sDefinitionName );

	STATICHASH( FadeOutTime );
	m_FadeOutTime = ConfigManager::GetInheritedFloat( sFadeOutTime, 0.0f, sDefinitionName );
	m_InvFadeOutTime = ( m_FadeOutTime > 0.0f ) ? ( 1.0f / m_FadeOutTime ) : 0.0f;
}

/*virtual*/ void WBCompRosaDecal::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( OnInitialTransformSet );
	STATIC_HASHED_STRING( OnTurned );
	STATIC_HASHED_STRING( SetPrescribedNormalBasis );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawned )
	{
		if( IsFinite() )
		{
			m_ExpireTime = GetTime() + m_Lifetime;

			WB_MAKE_EVENT( Destroy, GetEntity() );
			WB_QUEUE_EVENT_DELAY( GetEventManager(), Destroy, GetEntity(), m_Lifetime );
		}
	}
	else if(
		EventName == sOnInitialTransformSet ||
		EventName == sOnTurned )
	{
		if( !m_PrescribedNormalBasis )
		{
			WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
			DEVASSERT( pTransform );

			// ROSATODO: Only do this if we don't have a prescribed normal basis
			m_NormalBasis = pTransform->GetOrientation().ToMatrix();
		}
	}
	else if( EventName == sSetPrescribedNormalBasis )
	{
		STATIC_HASHED_STRING( NormalBasisOrientation );
		const Angles NormalBasisOrientation = Event.GetAngles( sNormalBasisOrientation );

		m_NormalBasis			= NormalBasisOrientation.ToMatrix();
		m_PrescribedNormalBasis	= true;
	}
}

float WBCompRosaDecal::GetAlpha() const
{
	if( IsFinite() )
	{
		const float	TimeRemaining	= m_ExpireTime - GetTime();
		const float Alpha			= ( TimeRemaining < m_FadeOutTime ) ? ( TimeRemaining * m_InvFadeOutTime ) : 1.0f;
		return Alpha;
	}
	else
	{
		return 1.0f;
	}
}

#define VERSION_EMPTY					0
#define VERSION_EXPIRETIME				1
#define VERSION_NORMALBASIS				1
#define VERSION_PRESCRIBEDNORMALBASIS	2
#define VERSION_CURRENT					2

uint WBCompRosaDecal::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += 4;					// m_ExpireTime
	Size += sizeof( Matrix );	// m_NormalBasis
	Size += 1;					// m_PrescribedNormalBasis

	return Size;
}

void WBCompRosaDecal::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteFloat( m_ExpireTime );
	Stream.Write<Matrix>( m_NormalBasis );
	Stream.WriteBool( m_PrescribedNormalBasis );
}

void WBCompRosaDecal::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_EXPIRETIME )
	{
		m_ExpireTime = Stream.ReadFloat();
	}

	if( Version >= VERSION_NORMALBASIS )
	{
		m_NormalBasis = Stream.Read<Matrix>();
	}

	if( Version >= VERSION_PRESCRIBEDNORMALBASIS )
	{
		m_PrescribedNormalBasis = Stream.ReadBool();
	}
}
