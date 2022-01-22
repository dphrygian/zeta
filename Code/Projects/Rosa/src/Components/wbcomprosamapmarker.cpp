#include "core.h"
#include "wbcomprosamapmarker.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "mesh.h"
#include "wbcomprosatransform.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "irenderer.h"
#include "meshfactory.h"
#include "texturemanager.h"
#include "ivertexdeclaration.h"
#include "idatastream.h"
#include "view.h"
#include "mathcore.h"

WBCompRosaMapMarker::WBCompRosaMapMarker()
:	m_Orient( false )
,	m_Hidden( false )
,	m_AlwaysShow( false )
,	m_ConfigEnabled( false )
,	m_Size( 0.0f )
,	m_Mesh()
{
	STATIC_HASHED_STRING( OnToggledMinimapMarkers );
	GetEventManager()->AddObserver( sOnToggledMinimapMarkers, this );
}

WBCompRosaMapMarker::~WBCompRosaMapMarker()
{
	SafeDelete( m_Mesh );

	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnToggledMinimapMarkers );
		pEventManager->RemoveObserver( sOnToggledMinimapMarkers, this );
	}
}

/*virtual*/ void WBCompRosaMapMarker::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Orient );
	m_Orient = ConfigManager::GetInheritedBool( sOrient, true, sDefinitionName );

	STATICHASH( Hidden );
	m_Hidden = ConfigManager::GetInheritedBool( sHidden, false, sDefinitionName );

	STATICHASH( AlwaysShow );
	m_AlwaysShow = ConfigManager::GetInheritedBool( sAlwaysShow, false, sDefinitionName );

	STATICHASH( Size );
	m_Size = ConfigManager::GetInheritedFloat( sSize, 0.0f, sDefinitionName );

	STATICHASH( Texture );
	const SimpleString Texture = ConfigManager::GetInheritedString( sTexture, "", sDefinitionName );

	CreateMesh( Texture );

	// Mirror the config option
	STATICHASH( MinimapMarkers );
	m_ConfigEnabled = ConfigManager::GetBool( sMinimapMarkers );
}

/*virtual*/ void WBCompRosaMapMarker::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnToggledMinimapMarkers );
	STATIC_HASHED_STRING( Hide );
	STATIC_HASHED_STRING( Show );
	STATIC_HASHED_STRING( HideMapMarker );
	STATIC_HASHED_STRING( ShowMapMarker );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnToggledMinimapMarkers )
	{
		STATICHASH( MinimapMarkers );
		m_ConfigEnabled = ConfigManager::GetBool( sMinimapMarkers );
	}
	else if( EventName == sHide || EventName == sHideMapMarker )
	{
		m_Hidden = true;
	}
	else if( EventName == sShow || EventName == sShowMapMarker )
	{
		m_Hidden = false;
	}
}

/*virtual*/ void WBCompRosaMapMarker::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	DEVASSERT( m_Mesh );

	WBCompRosaTransform* pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	RosaFramework* const	pFramework			= GetFramework();
	View* const				pMinimapView		= pFramework->GetMinimapView();
	const float				MinimapExtent		= pFramework->GetMinimapViewExtent();
	const float				MinimapHalfExtent	= 0.5f * MinimapExtent;
	const Angles			MinimapRotation		= Angles( 0.0f, 0.0f, -pMinimapView->GetRotation().Yaw );
	const Matrix			MinimapViewMat		= Matrix::CreateTranslation( -pMinimapView->GetLocation() ) * MinimapRotation.ToMatrix();
	const Vector			MinimapLocation		= pTransform->GetLocation() * MinimapViewMat;	// Location of this entity relative to view frame

	const Vector2			UnitCircleLocation	= Vector2( MinimapLocation ).GetNormalized();
	const float				PrimaryAxisDivisor	= ( Abs( UnitCircleLocation.x ) > Abs( UnitCircleLocation.y ) ) ? Abs( UnitCircleLocation.x ) : Abs( UnitCircleLocation.y );
	const Vector2			UnitSquareLocation	= ( PrimaryAxisDivisor > EPSILON ) ? ( UnitCircleLocation / PrimaryAxisDivisor ) : UnitCircleLocation;
	const Vector			EdgeLocation		= Vector( UnitSquareLocation.x * MinimapHalfExtent, UnitSquareLocation.y * MinimapHalfExtent, MinimapLocation.z );
	const float				OrigDistanceSq		= MinimapLocation.LengthSquared2D();
	const float				EdgeDistanceSq		= EdgeLocation.LengthSquared2D();
	const Vector			FinalLocation		= ( OrigDistanceSq < EdgeDistanceSq ) ? MinimapLocation : EdgeLocation;

	const float				Yaw					= m_Orient ? pTransform->GetOrientation().Yaw : RosaGame::GetPlayerViewOrientation().Yaw;

	m_Mesh->m_Location	= FinalLocation * MinimapViewMat.GetInverse();
	m_Mesh->m_Rotation	= Angles( 0.0f, 0.0f, Yaw );
	m_Mesh->m_Scale		= ( m_Size * MinimapExtent ) * Vector( 1.0f, 1.0f, 1.0f );	// Counteract the view extents so markers remain a constant size
	// TODO: If maximap is a shipping feature, this scale hack only works if the maximap scales by the same amount.
}

/*virtual*/ void WBCompRosaMapMarker::Render()
{
	if( m_Hidden )
	{
		return;
	}

	if( !m_ConfigEnabled && !m_AlwaysShow )
	{
		return;
	}

	DEVASSERT( m_Mesh );
	GetFramework()->GetRenderer()->AddMesh( m_Mesh );
}

void WBCompRosaMapMarker::CreateMesh( const SimpleString& Texture )
{
	XTRACE_FUNCTION;

	IRenderer* const pRenderer = RosaFramework::GetInstance()->GetRenderer();

	m_Mesh = pRenderer->GetMeshFactory()->CreateQuad( 1.0f, XY_PLANE, false );
	m_Mesh->SetVertexDeclaration( pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS ) );	// Override what CreatePlane gives us
	m_Mesh->SetMaterialFlags( MAT_MINIMAPMARKER );
	m_Mesh->SetMaterialDefinition( "Material_MinimapC", pRenderer );
	m_Mesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( Texture.CStr() ) );
#if BUILD_DEBUG
	m_Mesh->m_DEBUG_Name = "MapMarker";
#endif
}

#define VERSION_EMPTY	0
#define VERSION_HIDDEN	1
#define VERSION_CURRENT	1

uint WBCompRosaMapMarker::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_Hidden

	return Size;
}

void WBCompRosaMapMarker::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Hidden );
}

void WBCompRosaMapMarker::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_HIDDEN )
	{
		m_Hidden = Stream.ReadBool();
	}
}
