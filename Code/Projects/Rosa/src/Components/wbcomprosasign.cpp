#include "core.h"
#include "wbcomprosasign.h"
#include "aabb.h"
#include "wbcomprosatransform.h"
#include "wbcomprosacollision.h"
#include "wbcomprosamesh.h"
#include "wbentity.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "rosamesh.h"
#include "idatastream.h"
#include "Common/uimanagercommon.h"
#include "mathcore.h"

WBCompRosaSign::WBCompRosaSign()
:	m_IsReadable( false )
,	m_IsSignTarget( false )
,	m_ReadDistance( 0.0f )
,	m_UseCollisionExtents( false )
,	m_UseMeshExtents( false )
,	m_ExtentsFatten( 0.0f )
,	m_BoundOffset()
,	m_BoundExtents()
,	m_SignText()
{
}

WBCompRosaSign::~WBCompRosaSign()
{
}

/*virtual*/ void WBCompRosaSign::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	STATICHASH( RosaSign );

	MAKEHASH( DefinitionName );

	STATICHASH( IsReadable );
	m_IsReadable = ConfigManager::GetInheritedBool( sIsReadable, true, sDefinitionName );

	STATICHASH( ReadDistance );
	const float DefaultReadDistance = ConfigManager::GetFloat( sReadDistance, 0.0f, sRosaSign );
	m_ReadDistance = ConfigManager::GetInheritedFloat( sReadDistance, DefaultReadDistance, sDefinitionName );

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

	STATICHASH( SignText );
	m_SignText = ConfigManager::GetInheritedString( sSignText, GetEntity()->GetName().CStr(), sDefinitionName );
}

/*virtual*/ void WBCompRosaSign::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( OnInitialTransformSet );
	STATIC_HASHED_STRING( OnDestroyed );
	STATIC_HASHED_STRING( OnMeshUpdated );
	STATIC_HASHED_STRING( SetIsReadable );
	STATIC_HASHED_STRING( BecomeReadable );
	STATIC_HASHED_STRING( BecomeNonReadable );
	STATIC_HASHED_STRING( SetSignText );
	STATIC_HASHED_STRING( SetBoundExtents );
	STATIC_HASHED_STRING( SetBoundOffsetZ );

	const HashedString EventName = Event.GetEventName();

	if( EventName == sOnInitialized )
	{
		if( m_UseCollisionExtents )
		{
			WBCompRosaCollision* const pCollision = WB_GETCOMP( GetEntity(), RosaCollision );
			if( pCollision )
			{
				m_BoundExtents = pCollision->GetExtents() + Vector( m_ExtentsFatten, m_ExtentsFatten, m_ExtentsFatten );
			}
		}
	}
	else if( EventName == sOnInitialTransformSet )
	{
		if( !m_UseCollisionExtents && !m_UseMeshExtents )
		{
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
	}
	else if( EventName == sOnDestroyed )
	{
		if( GetIsSignTarget() )
		{
			SetHUDHidden( true );
		}
	}
	else if( EventName == sOnMeshUpdated )
	{
		ASSERT( m_UseMeshExtents );

		WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		WBCompRosaMesh* const pMeshComponent = WB_GETCOMP( GetEntity(), RosaMesh );
		ASSERT( pMeshComponent );

		RosaMesh* const pMesh = pMeshComponent->GetMesh();
		ASSERT( pMesh );

		m_BoundExtents	= pMesh->m_AABB.GetExtents() + Vector( m_ExtentsFatten, m_ExtentsFatten, m_ExtentsFatten );
		m_BoundOffset	= pMesh->m_AABB.GetCenter() - pTransform->GetLocation();
	}
	else if( EventName == sSetIsReadable )
	{
		STATIC_HASHED_STRING( IsReadable );
		m_IsReadable = Event.GetBool( sIsReadable );
	}
	else if( EventName == sBecomeReadable )
	{
		m_IsReadable = true;
	}
	else if( EventName == sBecomeNonReadable )
	{
		m_IsReadable = false;
	}
	else if( EventName == sSetSignText )
	{
		STATIC_HASHED_STRING( SignText );
		m_SignText = Event.GetString( sSignText );

		if( GetIsSignTarget() )
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
}

/*virtual*/ void WBCompRosaSign::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Bool, IsReadable, m_IsReadable );
}

AABB WBCompRosaSign::GetBound() const
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	return AABB::CreateFromCenterAndExtents( pTransform->GetLocation() + m_BoundOffset, m_BoundExtents );
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaSign::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector Location = pTransform->GetLocation() + m_BoundOffset;
	GetFramework()->GetRenderer()->DEBUGDrawBox( Location - m_BoundExtents, Location + m_BoundExtents, ARGB_TO_COLOR( 255, 192, 128, 64 ) );
}
#endif

void WBCompRosaSign::SetIsSignTarget( const bool IsSignTarget )
{
	m_IsSignTarget = IsSignTarget;

	if( IsSignTarget )
	{
		PublishToHUD();
	}
	else
	{
		SetHUDHidden( true );
	}
}

void WBCompRosaSign::PublishToHUD() const
{
	STATICHASH( HUD );

	STATICHASH( SignText );
	ConfigManager::SetString( sSignText, m_SignText.CStr(), sHUD );

	SetHUDHidden( false );
}

void WBCompRosaSign::SetHUDHidden( const bool Hidden ) const
{
	UIManager* const pUIManager = GetFramework()->GetUIManager();
	ASSERT( pUIManager );

	STATIC_HASHED_STRING( HUD );
	STATIC_HASHED_STRING( SignText );

	{
		WB_MAKE_EVENT( SetWidgetHidden, GetEntity() );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sSignText );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, Hidden );
		WB_DISPATCH_EVENT( GetEventManager(), SetWidgetHidden, pUIManager );
	}
}

#define VERSION_EMPTY		0
#define VERSION_ISREADABLE	1
#define VERSION_SIGNTEXT	2
#define VERSION_BOUNDS		3
#define VERSION_CURRENT		3

uint WBCompRosaSign::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 1;	// m_IsReadable

	Size += IDataStream::SizeForWriteString( m_SignText );

	Size += sizeof( Vector );	// m_BoundOffset
	Size += sizeof( Vector );	// m_BoundExtents

	return Size;
}

void WBCompRosaSign::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_IsReadable );

	Stream.WriteString( m_SignText );

	Stream.Write( sizeof( Vector ), &m_BoundOffset );
	Stream.Write( sizeof( Vector ), &m_BoundExtents );
}

void WBCompRosaSign::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ISREADABLE )
	{
		m_IsReadable = Stream.ReadBool();
	}

	if( Version >= VERSION_SIGNTEXT )
	{
		m_SignText = Stream.ReadString();
	}

	if( Version >= VERSION_BOUNDS )
	{
		Stream.Read( sizeof( Vector ), &m_BoundOffset );
		Stream.Read( sizeof( Vector ), &m_BoundExtents );
	}
}
