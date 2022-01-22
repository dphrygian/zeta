#include "core.h"
#include "wbcomprosavisible.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacamera.h"
#include "wbcomprosaheadtracker.h"
#include "Components/wbcompstatmod.h"
#include "wbentity.h"
#include "configmanager.h"
#include "wbevent.h"
#include "idatastream.h"

WBCompRosaVisible::WBCompRosaVisible()
:	m_Visible( false )
,	m_VisionPriority( 0 )
{
}

WBCompRosaVisible::~WBCompRosaVisible()
{
}

/*virtual*/ void WBCompRosaVisible::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Visible );
	m_Visible = ConfigManager::GetInheritedBool( sVisible, true, sDefinitionName );

	STATICHASH( VisionPriority );
	m_VisionPriority = ConfigManager::GetInheritedInt( sVisionPriority, 0, sDefinitionName );
}

/*virtual*/ void WBCompRosaVisible::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetVisible );
	STATIC_HASHED_STRING( SetInvisible );
	STATIC_HASHED_STRING( Show );
	STATIC_HASHED_STRING( Hide );
	STATIC_HASHED_STRING( ToggleVisible );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetVisible || EventName == sShow )
	{
		m_Visible = true;
	}
	else if( EventName == sSetInvisible || EventName == sHide )
	{
		m_Visible = false;
	}
	else if( EventName == sToggleVisible )
	{
		m_Visible = !m_Visible;
	}
}

/*virtual*/ void WBCompRosaVisible::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, Visible, m_Visible );
}

// This is a bit hackity. Maybe consider a better way, but I'm deep in other AI stuff now.
Vector WBCompRosaVisible::GetVisibleLocation( const bool IsAlreadyVisible ) const
{
	WBEntity* const pEntity = GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	Vector VisibleLocation = pTransform->GetLocation();

	WBCompRosaCamera* const pCamera = WB_GETCOMP( pEntity, RosaCamera );
	if( pCamera )
	{
		// ROSANOTE: For vision sensor, only use Z offset; this means that leaning won't reveal player.
		// For head-tracking, use everything so AIs follow player camera as expected; except don't use the override
		// because then enemies can't always hit the player while lockpicking (because they attack in their
		// headtracker direction).
		const WBCompRosaCamera::EViewModifiers ViewModifiers = IsAlreadyVisible ? WBCompRosaCamera::EVM_All_NoOverride : WBCompRosaCamera::EVM_OffsetZ;
		pCamera->ModifyTranslation( ViewModifiers, VisibleLocation );
	}

	WBCompRosaHeadTracker* const pHeadTracker = WB_GETCOMP( pEntity, RosaHeadTracker );
	if( pHeadTracker )
	{
		VisibleLocation = pHeadTracker->GetEyesLocation();
	}

	return VisibleLocation;
}

// Compared to GetVisibleLocation, I'm just always assuming this thing is already visible.
Angles WBCompRosaVisible::GetVisibleOrientation() const
{
	WBEntity* const pEntity = GetEntity();
	DEVASSERT( pEntity );

	WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	Angles VisibleOrientation = pTransform->GetOrientation();

	WBCompRosaCamera* const pCamera = WB_GETCOMP( pEntity, RosaCamera );
	if( pCamera )
	{
		const WBCompRosaCamera::EViewModifiers ViewModifiers = WBCompRosaCamera::EVM_All_NoOverride;
		pCamera->ModifyOrientation( ViewModifiers, VisibleOrientation );
	}

	return VisibleOrientation;
}

float WBCompRosaVisible::GetVisibleCertainty() const
{
	const float				VisibleCertainty	= 1.0f;
	WBCompStatMod* const	pStatMod			= WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT_SAFE( VisibleCertainty, VisibleCertainty, pStatMod );
	return WB_MODDED( VisibleCertainty );
}

#define VERSION_EMPTY	0
#define VERSION_VISIBLE	1
#define VERSION_CURRENT	1

uint WBCompRosaVisible::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_Visible

	return Size;
}

void WBCompRosaVisible::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Visible );
}

void WBCompRosaVisible::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_VISIBLE )
	{
		m_Visible = Stream.ReadBool();
	}
}
