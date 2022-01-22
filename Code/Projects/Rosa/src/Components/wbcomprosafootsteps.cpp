#include "core.h"
#include "wbcomprosafootsteps.h"
#include "rosasurfaces.h"
#include "configmanager.h"
#include "mathcore.h"
#include "wbeventmanager.h"
#include "rosaworld.h"
#include "wbcomprosacollision.h"
#include "idatastream.h"
#include "rosaframework.h"
#include "Achievements/iachievementmanager.h"
#include "Components/wbcompstatmod.h"

WBCompRosaFootsteps::WBCompRosaFootsteps()
:	m_FootstepStride( 0.0f )
,	m_LastFootstepLocation()
,	m_HasTakenFirstStep( false )
,	m_StepPhase( ESP_RightFoot )
,	m_StepPhaseAlpha( 0.0f )
,	m_StepPhaseSignedAlpha( 0.0f )
,	m_FootstepsDisabled( false )
{
	RosaSurfaces::AddRef();
}

WBCompRosaFootsteps::~WBCompRosaFootsteps()
{
	RosaSurfaces::Release();
}

/*virtual*/ void WBCompRosaFootsteps::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( FootstepStride );
	m_FootstepStride = ConfigManager::GetInheritedFloat( sFootstepStride, 0.0f, sDefinitionName );
}

void WBCompRosaFootsteps::DoFootstep( const Vector& CurrentLocation, const float AdditionalSpeed )
{
	m_LastFootstepLocation = CurrentLocation;

	// HACKHACK to stop playing footstep on spawn
	if( !m_HasTakenFirstStep )
	{
		m_HasTakenFirstStep = true;
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaCollision* const	pCollision		= WB_GETCOMP( pEntity, RosaCollision );
	const HashedString			FootstepSurface	= GetWorld()->GetSurfaceBelow( CurrentLocation, pCollision->GetExtents(), pEntity );

	if( !m_FootstepsDisabled )
	{
		WB_MAKE_EVENT( OnFootstep, pEntity );
		WB_SET_AUTO( OnFootstep, Bool, LeftFoot, m_StepPhase == ESP_LeftFoot );
		WB_SET_AUTO( OnFootstep, Bool, RightFoot, m_StepPhase == ESP_RightFoot );
		WB_SET_AUTO( OnFootstep, Float, AdditionalSpeed, AdditionalSpeed );
		WB_SET_AUTO( OnFootstep, Hash, FootstepSurface, FootstepSurface );
		WB_DISPATCH_EVENT( GetEventManager(), OnFootstep, pEntity );
	}

	m_StepPhase			= ( m_StepPhase == ESP_RightFoot ) ? ESP_LeftFoot : ESP_RightFoot;
	m_StepPhaseAlpha	= 0.0f;
	UpdateStepPhaseSignedAlpha();

	// ROSATODO: Re-enable if desired
	//INCREMENT_STAT( "NumFootsteps", 1 );
}

/*virtual*/ void WBCompRosaFootsteps::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( ForceFootstep );
	STATIC_HASHED_STRING( OnMoved );
	STATIC_HASHED_STRING( ToggleFootsteps );
	STATIC_HASHED_STRING( EnableFootsteps );
	STATIC_HASHED_STRING( DisableFootsteps );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sForceFootstep )
	{
		STATIC_HASHED_STRING( Location );
		const Vector Location = Event.GetVector( sLocation );

		STATIC_HASHED_STRING( AdditionalSpeed );
		const float AdditionalSpeed = Event.GetFloat( sAdditionalSpeed );

		DoFootstep( Location, AdditionalSpeed );
	}
	else if( EventName == sOnMoved )
	{
		STATIC_HASHED_STRING( IsLanded );
		const bool IsLanded = Event.GetBool( sIsLanded );

		STATIC_HASHED_STRING( Location );
		const Vector Location = Event.GetVector( sLocation );

		const float DistanceSq = ( Location - m_LastFootstepLocation ).LengthSquared2D();

		WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
		WB_MODIFY_FLOAT( FootstepStride, m_FootstepStride, pStatMod );
		const float FootstepStrideSq = Square( WB_MODDED( FootstepStride ) );

		if( IsLanded && DistanceSq >= FootstepStrideSq )
		{
			DoFootstep( Location, 0.0f );
		}
		else
		{
			m_StepPhaseAlpha	= Saturate( SqRt( DistanceSq / FootstepStrideSq ) );
			UpdateStepPhaseSignedAlpha();
		}
	}
	else if( EventName == sToggleFootsteps )
	{
		SetFootstepsDisabled( !m_FootstepsDisabled );
	}
	else if( EventName == sEnableFootsteps )
	{
		SetFootstepsDisabled( false );
	}
	else if( EventName == sDisableFootsteps )
	{
		SetFootstepsDisabled( true );
	}
}

void WBCompRosaFootsteps::UpdateStepPhaseSignedAlpha()
{
	m_StepPhaseSignedAlpha = ( m_StepPhase == ESP_RightFoot ) ? ( ( 2.0f * m_StepPhaseAlpha ) - 1.0f ) : ( ( -2.0f * m_StepPhaseAlpha ) + 1.0f );
}

#define VERSION_EMPTY				0
#define VERSION_FOOTSTEPSDISABLED	1
#define VERSION_STEPPHASE			2
#define VERSION_HASTAKENFIRSTSTEP	3
#define VERSION_CURRENT				3

uint WBCompRosaFootsteps::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 1;	// m_FootstepsDisabled
	Size += 1;	// m_HasTakenFirstStep

	Size += sizeof( Vector );	// m_LastFootstepLocation
	Size += 4;	// m_StepPhase
	Size += 4;	// m_StepPhaseAlpha
	Size += 4;	// m_StepPhaseSignedAlpha

	return Size;
}

void WBCompRosaFootsteps::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_FootstepsDisabled );
	Stream.WriteBool( m_HasTakenFirstStep );

	Stream.Write( sizeof( Vector ), &m_LastFootstepLocation );
	Stream.WriteUInt32( m_StepPhase );
	Stream.WriteFloat( m_StepPhaseAlpha );
	Stream.WriteFloat( m_StepPhaseSignedAlpha );
}

void WBCompRosaFootsteps::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_FOOTSTEPSDISABLED )
	{
		m_FootstepsDisabled = Stream.ReadBool();
	}

	if( Version >= VERSION_HASTAKENFIRSTSTEP )
	{
		m_HasTakenFirstStep = Stream.ReadBool();
	}

	if( Version >= VERSION_STEPPHASE )
	{
		Stream.Read( sizeof( Vector ), &m_LastFootstepLocation );
		m_StepPhase = static_cast<EStepPhase>( Stream.ReadUInt32() );
		m_StepPhaseAlpha = Stream.ReadFloat();
		m_StepPhaseSignedAlpha = Stream.ReadFloat();
	}
}
