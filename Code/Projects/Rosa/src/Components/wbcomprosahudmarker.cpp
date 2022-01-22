#include "core.h"
#include "wbcomprosahudmarker.h"
#include "wbcomprosatransform.h"
#include "wbentity.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "view.h"
#include "wbeventmanager.h"
#include "vector2.h"
#include "mathcore.h"
#include "rosaworld.h"
#include "collisioninfo.h"
#include "mesh.h"
#include "irenderer.h"
#include "texturemanager.h"
#include "fontmanager.h"
#include "meshfactory.h"
#include "idatastream.h"
#include "hsv.h"
#include "font.h"

WBCompRosaHUDMarker::WBCompRosaHUDMarker()
:	m_Mesh( NULL )
,	m_Material()
,	m_OccludedImage()
,	m_UnoccludedImage()
,	m_Size( 0.0f )
,	m_FalloffRadius( 0.0f )
,	m_OffsetZ( 0.0f )
,	m_DistanceMesh( NULL )
,	m_DistanceFont( NULL )
,	m_DistanceColor( 0 )
,	m_DistanceHeight( 0.0f )
,	m_Hidden( false )
,	m_ConfigEnabled( false )
{
	STATIC_HASHED_STRING( OnCameraTicked );
	GetEventManager()->AddObserver( sOnCameraTicked, this );

	STATIC_HASHED_STRING( OnViewChanged );
	GetEventManager()->AddObserver( sOnViewChanged, this );

	STATIC_HASHED_STRING( OnSetRes );
	GetEventManager()->AddObserver( sOnSetRes, this );

	STATIC_HASHED_STRING( OnShowHUDChanged );
	GetEventManager()->AddObserver( sOnShowHUDChanged, this );

	STATIC_HASHED_STRING( OnToggledHUDMarkers );
	GetEventManager()->AddObserver( sOnToggledHUDMarkers, this );
}

WBCompRosaHUDMarker::~WBCompRosaHUDMarker()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnCameraTicked );
		GetEventManager()->RemoveObserver( sOnCameraTicked, this );

		STATIC_HASHED_STRING( OnViewChanged );
		GetEventManager()->RemoveObserver( sOnViewChanged, this );

		STATIC_HASHED_STRING( OnSetRes );
		GetEventManager()->RemoveObserver( sOnSetRes, this );

		STATIC_HASHED_STRING( OnShowHUDChanged );
		GetEventManager()->RemoveObserver( sOnShowHUDChanged, this );

		STATIC_HASHED_STRING( OnToggledHUDMarkers );
		GetEventManager()->RemoveObserver( sOnToggledHUDMarkers, this );
	}

	SafeDelete( m_Mesh );
	SafeDelete( m_DistanceMesh );
}

/*virtual*/ void WBCompRosaHUDMarker::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	RosaFramework* const	pFramework		= GetFramework();
	IRenderer* const		pRenderer		= pFramework->GetRenderer();
	TextureManager* const	pTextureManager	= pRenderer->GetTextureManager();
	FontManager* const		pFontManager	= pRenderer->GetFontManager();

	MAKEHASH( DefinitionName );

	STATICHASH( Material );
	m_Material = ConfigManager::GetInheritedString( sMaterial, "", sDefinitionName );

	STATICHASH( UnoccludedImage );
	const char* const UnoccludedImage = ConfigManager::GetInheritedString( sUnoccludedImage, NULL, sDefinitionName );
	m_UnoccludedImage = pTextureManager->GetTexture( UnoccludedImage );

	STATICHASH( OccludedImage );
	const char* const OccludedImage = ConfigManager::GetInheritedString( sOccludedImage, UnoccludedImage, sDefinitionName );
	m_OccludedImage = pTextureManager->GetTexture( OccludedImage );

	STATICHASH( Size );
	m_Size = ConfigManager::GetInheritedFloat( sSize, 0.0f, sDefinitionName );

	STATICHASH( FalloffRadius );
	m_FalloffRadius = ConfigManager::GetInheritedFloat( sFalloffRadius, 0.0f, sDefinitionName );

	STATICHASH( OffsetZ );
	m_OffsetZ = ConfigManager::GetInheritedFloat( sOffsetZ, 0.0f, sDefinitionName );

	// Equivalent to ScreenHH in UI system
	STATICHASH( FontHeight );
	m_DistanceHeight = ConfigManager::GetInheritedFloat( sFontHeight, 0.0f, sDefinitionName );

	if( m_DistanceHeight > 0.0f )
	{
		STATICHASH( DisplayHeight );
		const float DisplayHeight = ConfigManager::GetFloat( sDisplayHeight );

		STATICHASH( FontTag );
		const char* const pFontTag = ConfigManager::GetInheritedString( sFontTag, DEFAULT_FONT_TAG, sDefinitionName );
		m_DistanceFont = pFontManager->GetFont( pFontTag, m_DistanceHeight * DisplayHeight );

		const Vector FontColorHSV	= HSV::GetConfigHSV( "FontColor", sDefinitionName, Vector( 0.0f, 0.0f, 1.0f ) );
		const Vector FontColor		= HSV::GetConfigRGB( "FontColor", sDefinitionName, HSV::HSVToRGB( FontColorHSV ) );
		m_DistanceColor				= FontColor.ToColor();
	}

	STATICHASH( Hidden );
	m_Hidden = ConfigManager::GetInheritedBool( sHidden, false, sDefinitionName );

	// Mirror the config option
	STATICHASH( HUDMarkers );
	STATICHASH( ShowHUD );
	m_ConfigEnabled = ConfigManager::GetBool( sHUDMarkers ) && ConfigManager::GetBool( sShowHUD );

	CreateMesh();
}

/*virtual*/ void WBCompRosaHUDMarker::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnCameraTicked );
	STATIC_HASHED_STRING( OnViewChanged );
	STATIC_HASHED_STRING( OnSetRes );
	STATIC_HASHED_STRING( OnShowHUDChanged );
	STATIC_HASHED_STRING( OnToggledHUDMarkers );
	STATIC_HASHED_STRING( Hide );
	STATIC_HASHED_STRING( Show );
	STATIC_HASHED_STRING( HideHUDMarker );
	STATIC_HASHED_STRING( ShowHUDMarker );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnCameraTicked ||
		EventName == sOnViewChanged ||
		EventName == sOnSetRes )
	{
		UpdateMesh();
	}
	else if( EventName == sOnShowHUDChanged ||
			 EventName == sOnToggledHUDMarkers )
	{
		STATICHASH( HUDMarkers );
		STATICHASH( ShowHUD );
		m_ConfigEnabled = ConfigManager::GetBool( sHUDMarkers ) && ConfigManager::GetBool( sShowHUD );
	}
	else if( EventName == sHide || EventName == sHideHUDMarker )
	{
		m_Hidden = true;
	}
	else if( EventName == sShow || EventName == sShowHUDMarker )
	{
		m_Hidden = false;
	}
}

void WBCompRosaHUDMarker::CreateMesh()
{
	XTRACE_FUNCTION;

	IRenderer* const	pRenderer	= GetFramework()->GetRenderer();

	m_Mesh							= pRenderer->GetMeshFactory()->CreateSprite();
	m_Mesh->SetMaterialDefinition( m_Material, pRenderer );
	m_Mesh->SetTexture( 0, m_OccludedImage );
	m_Mesh->SetMaterialFlags( MAT_HUD );
	m_Mesh->m_ConstantColor			= Vector4( 1.0f, 1.0f, 1.0f, 1.0f );
#if BUILD_DEBUG
	m_Mesh->m_DEBUG_Name			= "HUDMarker";
#endif
}

void WBCompRosaHUDMarker::UpdateMesh()
{
	RosaFramework* const		pFramework		= GetFramework();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();
	Display* const				pDisplay		= pFramework->GetDisplay();
	const float					DisplayWidth	= static_cast<float>( pDisplay->m_Width );
	const float					DisplayHeight	= static_cast<float>( pDisplay->m_Height );
	const float					ActualSize		= DisplayHeight * m_Size;
	const float					HalfActualSize	= 0.5f * ActualSize;

	WBEntity* const				pEntity			= GetEntity();
	WBCompRosaTransform* const	pTransform		= pEntity->GetTransformComponent<WBCompRosaTransform>();
	RosaWorld* const			pWorld			= GetWorld();
	const View* const			pView			= GetFramework()->GetMainView();
	Vector						Location		= pTransform->GetLocation();
	Location.z									+= m_OffsetZ;
	const Vector&				ViewLocation	= pView->GetLocation();
	Vector2						ScreenLocation	= pView->ProjectAndClipToScreen( Location );

	CollisionInfo Info;
	Info.m_In_CollideWorld						= true;
	Info.m_In_CollideEntities					= true;
	Info.m_In_CollidingEntity					= pEntity;
	Info.m_In_UserFlags							= EECF_Occlusion;
	Info.m_In_StopAtAnyCollision				= true;

	const bool					Occluded		= pWorld->LineCheck( ViewLocation, Location, Info );
	const float					Distance		= ( Location - ViewLocation ).Length();
	const float					Alpha			= Occluded ? AttenuateQuad( Distance, m_FalloffRadius ) : 1.0f;

	m_Mesh->SetTexture( 0, Occluded ? m_OccludedImage : m_UnoccludedImage );
	m_Mesh->m_Location			= Vector( DisplayWidth * ScreenLocation.x, 0.0f, DisplayHeight * ScreenLocation.y );
	// We've already clipped to the edge of screen, but this prevents the image halfway overlapping the edge.
	m_Mesh->m_Location.x		= Min( Max( m_Mesh->m_Location.x, HalfActualSize ), DisplayWidth - HalfActualSize );
	m_Mesh->m_Location.z		= Min( Max( m_Mesh->m_Location.z, HalfActualSize ), DisplayHeight - HalfActualSize );
	m_Mesh->m_Scale				= Vector( ActualSize, 1.0f, ActualSize );
	m_Mesh->m_ConstantColor.a	= Alpha;

	SafeDelete( m_DistanceMesh );
	if( !m_Hidden && NULL != m_DistanceFont )
	{
		m_DistanceMesh					= m_DistanceFont->Print( SimpleString::PrintF( "%dm", static_cast<uint>( Floor( Distance ) ) ), SRect( 0.0f, 0.0f, 0.0f, 0.0f ), FONT_PRINT_CENTER, m_DistanceColor );
		m_DistanceMesh->SetMaterialDefinition( m_Material, pRenderer );
		m_DistanceMesh->SetMaterialFlags( MAT_HUD );
		m_DistanceMesh->m_ConstantColor	= Vector4( 1.0f, 1.0f, 1.0f, Alpha );
#if BUILD_DEBUG
		m_DistanceMesh->m_DEBUG_Name	= "HUDMarkerDistance";
#endif

		// DLP 20 Sep 2020: I'm not sure this math is correct but it works fine for now.
		const float	DistanceScale		= ( m_DistanceHeight * DisplayHeight ) / m_DistanceFont->GetCapHeight();
		float		DistanceX			= m_Mesh->m_Location.x;
		DistanceX						-= m_DistanceMesh->GetAABB().GetExtents().x * DistanceScale;
		float		DistanceY			= m_Mesh->m_Location.z;
		DistanceY						-= m_DistanceFont->GetInternalLeading() * DistanceScale;
		DistanceY						-= ( m_DistanceFont->GetVerticalA() * DistanceScale );
		DistanceY						+= ActualSize;	// TODO: Make this a configurable offset from icon?

		m_DistanceMesh->m_Location.x	= DistanceX;
		m_DistanceMesh->m_Location.z	= DistanceY;
		m_DistanceMesh->m_Scale			= Vector( DistanceScale, 1.0f, DistanceScale );
		m_DistanceMesh->m_AABB.m_Min.x	= m_DistanceMesh->m_AABB.m_Min.x * DistanceScale + DistanceX;
		m_DistanceMesh->m_AABB.m_Min.z	= m_DistanceMesh->m_AABB.m_Min.z * DistanceScale + DistanceY;
		m_DistanceMesh->m_AABB.m_Max.x	= m_DistanceMesh->m_AABB.m_Max.x * DistanceScale + DistanceX;
		m_DistanceMesh->m_AABB.m_Max.z	= m_DistanceMesh->m_AABB.m_Max.z * DistanceScale + DistanceY;
	}
}

/*virtual*/ void WBCompRosaHUDMarker::Render()
{
	if( m_Hidden )
	{
		return;
	}

	if( !m_ConfigEnabled )
	{
		return;
	}

	IRenderer* const pRenderer = GetFramework()->GetRenderer();
	pRenderer->AddMesh( m_Mesh );

	if( m_DistanceMesh )
	{
		pRenderer->AddMesh( m_DistanceMesh );
	}
}

#define VERSION_EMPTY	0
#define VERSION_HIDDEN	1
#define VERSION_CURRENT	1

uint WBCompRosaHUDMarker::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version
	Size += 1;	// m_Hidden

	return Size;
}

void WBCompRosaHUDMarker::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Hidden );
}

void WBCompRosaHUDMarker::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_HIDDEN )
	{
		m_Hidden = Stream.ReadBool();
	}
}
