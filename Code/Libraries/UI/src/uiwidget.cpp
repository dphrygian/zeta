#include "core.h"
#include "3d.h"
#include "uiwidget.h"
#include "uimanager.h"
#include "uiscreen.h"
#include "configmanager.h"
#include "iaudiosystem.h"
#include "vector4.h"
#include "clock.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "wbevent.h"
#include "iuiinputmap.h"
#include "mathcore.h"
#include "hsv.h"
#include "irenderer.h"

UIManager* UIWidget::m_UIManager = NULL;

UIWidget::UIWidget()
:	m_Name()
,	m_RenderPriority( 0 )
,	m_CanBeFocused( false )
,	m_RenderInWorld( false )
,	m_IsDisabled( false )
,	m_IsDisabled_SavedState( false )
,	m_FocusOrder( 0 )
,	m_OwnerScreen( NULL )
,	m_FocusShiftUp( 0 )
,	m_FocusShiftDown( 0 )
,	m_FocusShiftLeft( 0 )
,	m_FocusShiftRight( 0 )
,	m_EventName()
,	m_Callback()
,	m_Actions()
,	m_FocusActions()
,	m_OwnsActions( false )
,	m_Hidden( false )
,	m_Hidden_SavedState( false )
,	m_Color( 1.0f, 1.0f, 1.0f, 1.0f )
,	m_ScreenColor()
,	m_Highlight( 1.0f, 1.0f, 1.0f, 1.0f )
,	m_DisabledColor( 1.0f, 1.0f, 1.0f, 1.0f )
,	m_Location()
,	m_Extents()
,	m_Velocity()
,	m_AllowNegativeOrigin( false )
,	m_Origin( EWO_TopLeft )
,	m_ParentWidget( NULL )
,	m_ClampToPixelGrid( false )
,	m_UseVanishingPointY( false )
,	m_UsePulsingHighlight( false )
,	m_PulsingHighlightMul( 0.0f )
,	m_PulsingHighlightAdd( 0.0f )
,	m_PulsingHighlightTimeScalar( 0.0f )
,	m_ListStep( false )
,	m_UseListY( false )
#if BUILD_DEBUG
,	m_LastRenderedTime( 0 )
#endif
{
}

UIWidget::~UIWidget()
{
	ClearActions();
}

void UIWidget::Render( bool HasFocus )
{
	Unused( HasFocus );

#if BUILD_DEBUG
	m_LastRenderedTime = m_UIManager->GetClock()->GetPhysicalCurrentTime();
#endif
}

void UIWidget::UpdateRender()
{
}

void UIWidget::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( UI );

	m_Name = DefinitionName;

	STATICHASH( RenderPriority );
	m_RenderPriority	= ConfigManager::GetInheritedInt( sRenderPriority, 0, sDefinitionName );

	STATICHASH( Focus );
	m_CanBeFocused		= ConfigManager::GetInheritedBool( sFocus, false, sDefinitionName );

	STATICHASH( RenderInWorld );
	m_RenderInWorld		= ConfigManager::GetInheritedBool( sRenderInWorld, false, sDefinitionName );

	STATICHASH( FocusOrder );
	m_FocusOrder		= ConfigManager::GetInheritedInt( sFocusOrder, 0, sDefinitionName );

	STATICHASH( Event );
	m_EventName			= ConfigManager::GetInheritedString( sEvent, "", sDefinitionName );

	STATICHASH( AllowNegativeOrigin );
	m_AllowNegativeOrigin = ConfigManager::GetInheritedBool( sAllowNegativeOrigin, false, sDefinitionName );

	STATICHASH( FocusShiftUp );
	m_FocusShiftUp = ConfigManager::GetInheritedInt( sFocusShiftUp, 0, sDefinitionName );

	STATICHASH( FocusShiftDown );
	m_FocusShiftDown = ConfigManager::GetInheritedInt( sFocusShiftDown, 0, sDefinitionName );

	STATICHASH( FocusShiftLeft );
	m_FocusShiftLeft = ConfigManager::GetInheritedInt( sFocusShiftLeft, 0, sDefinitionName );

	STATICHASH( FocusShiftRight );
	m_FocusShiftRight = ConfigManager::GetInheritedInt( sFocusShiftRight, 0, sDefinitionName );

	const Vector ColorHSV		= HSV::GetConfigHSV( "Color",		sDefinitionName, Vector( 0.0f, 0.0f, 1.0f ) );
	const Vector ScreenColorHSV	= HSV::GetConfigHSV( "ScreenColor",	sDefinitionName, Vector() );
	m_Color			= HSV::GetConfigRGBA( "Color",			sDefinitionName, Vector4( HSV::HSVToRGB( ColorHSV ), 1.0f ) );
	m_ScreenColor	= HSV::GetConfigRGBA( "ScreenColor",	sDefinitionName, Vector4( HSV::HSVToRGB( ScreenColorHSV ), 0.0f ) );
	m_Highlight		= HSV::GetConfigRGBA( "Highlight",		sDefinitionName, Vector4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	m_DisabledColor	= HSV::GetConfigRGBA( "DisabledColor",	sDefinitionName, Vector4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	// Pulsing highlights are currently global
	{
		STATICHASH( UsePulsingHighlight );
		m_UsePulsingHighlight = ConfigManager::GetBool( sUsePulsingHighlight, false, sUI );

		STATICHASH( PulsingHighlightMin );
		const float PulsingHighlightMin = ConfigManager::GetFloat( sPulsingHighlightMin, 0.0f, sUI );

		STATICHASH( PulsingHighlightMax );
		const float PulsingHighlightMax = ConfigManager::GetFloat( sPulsingHighlightMax, 0.0f, sUI );

		m_PulsingHighlightMul = ( PulsingHighlightMax - PulsingHighlightMin ) / 2.0f;
		m_PulsingHighlightAdd = PulsingHighlightMin - ( -1.0f * m_PulsingHighlightMul );

		STATICHASH( PulsingHighlightCycleSeconds );
		const float PulsingHighlightCycleSeconds = ConfigManager::GetFloat( sPulsingHighlightCycleSeconds, 0.0f, sUI );
		m_PulsingHighlightTimeScalar = ( 1.0f / PulsingHighlightCycleSeconds ) * 2.0f * PI;
	}

	STATICHASH( ListStep );
	m_ListStep = ConfigManager::GetInheritedBool( sListStep, false, sDefinitionName );

	STATICHASH( UseListY );
	m_UseListY = ConfigManager::GetInheritedBool( sUseListY, false, sDefinitionName );

	STATICHASH( Disabled );
	m_IsDisabled = ConfigManager::GetInheritedBool( sDisabled, false, sDefinitionName );

	STATICHASH( Hidden );
	m_Hidden = ConfigManager::GetInheritedBool( sHidden, false, sDefinitionName );

	STATICHASH( Parent );
	const HashedString ParentWidget = ConfigManager::GetInheritedHash( sParent, HashedString::NullString, sDefinitionName );
	if( ParentWidget != HashedString::NullString )
	{
		ASSERT( m_OwnerScreen );
		m_ParentWidget = m_OwnerScreen->GetWidget( ParentWidget );

		if( !m_ParentWidget )
		{
			PRINTF( "Could not find specified parent for widget %s.\n", DefinitionName.CStr() );
			WARNDESC( "Could not find specified parent for widget." );
		}
	}

	// This is important not only for rendering but because reinitialization is based on
	// render order, and we need parents to be in the correct position to update children.
	// DLP 18 Oct 2021: This used to only happen if render priority was <= parent's priority,
	// but it makes more sense to just use the config render priority as relative to any
	// sibling and bump everything up to the priority above the parent. This way, anything
	// can be set to RenderPriority=1 to float above its siblings without caring about the
	// absolutely priority of its parent chain.
	if( m_ParentWidget )
	{
		m_RenderPriority += ( m_ParentWidget->m_RenderPriority + 1 );

		STATIC_HASHED_STRING( UI );
		CATPRINTF( sUI, 2, "Promoting %s's render priority to %d to float above parent.\n", m_Name.CStr(), m_RenderPriority );
	}

	STATICHASH( DefaultClampToPixelGrid );
	const bool DefaultClampToPixelGrid = ConfigManager::GetBool( sDefaultClampToPixelGrid, true, sUI );

	STATICHASH( ClampToPixelGrid );
	m_ClampToPixelGrid = ConfigManager::GetInheritedBool( sClampToPixelGrid, DefaultClampToPixelGrid, sDefinitionName );

	STATICHASH( UseVanishingPointY );
	m_UseVanishingPointY = ConfigManager::GetInheritedBool( sUseVanishingPointY, false, sDefinitionName );

	STATICHASH( DisplayWidth );
	const float DisplayWidth	= ConfigManager::GetFloat( sDisplayWidth );
	const float ParentWidth		= m_ParentWidget ? m_ParentWidget->GetWidth() : DisplayWidth;

	STATICHASH( DisplayHeight );
	const float DisplayHeight	= ConfigManager::GetFloat( sDisplayHeight );
	const float ParentHeight	= m_ParentWidget ? m_ParentWidget->GetHeight() : DisplayHeight;

	STATICHASH( PixelVelocityX );
	STATICHASH( ParentWVelocityX );
	STATICHASH( ScreenWVelocityX );
	STATICHASH( ParentHVelocityX );
	STATICHASH( ScreenHVelocityX );
	m_Velocity.x =
		Pick(
			ConfigManager::GetInheritedFloat( sPixelVelocityX, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWVelocityX, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWVelocityX, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHVelocityX, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHVelocityX, 0.0f, sDefinitionName ) );

	STATICHASH( PixelVelocityY );
	STATICHASH( ParentHVelocityY );
	STATICHASH( ScreenHVelocityY );
	STATICHASH( ParentWVelocityY );
	STATICHASH( ScreenWVelocityY );
	m_Velocity.y =
		Pick(
			ConfigManager::GetInheritedFloat( sPixelVelocityY, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHVelocityY, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHVelocityY, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWVelocityY, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWVelocityY, 0.0f, sDefinitionName ) );

	STATICHASH( Origin );
	GetOrigin( ConfigManager::GetInheritedHash( sOrigin, HashedString::NullString, sDefinitionName ) );

	ClearActions();
	WBActionFactory::InitializeActionArray( sDefinitionName,			m_Actions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "Focus",	m_FocusActions );
	m_OwnsActions = true;

	SetConfigTransform( DefinitionName );
}

void UIWidget::ClearActions()
{
	if( m_OwnsActions )
	{
		WBActionFactory::ClearActionArray( m_Actions );
		WBActionFactory::ClearActionArray( m_FocusActions );
	}
	else
	{
		// Just clear the array but don't free the actions
		m_Actions.Clear();
		m_FocusActions.Clear();
	}
}

void UIWidget::GetBounds( SRect& OutBounds )
{
	OutBounds = SRect( m_Location.x, m_Location.y, m_Location.x + m_Extents.x, m_Location.y + m_Extents.y );
}

void UIWidget::OnTrigger()
{
	if( m_EventName != HashedString::NullString )
	{
		m_UIManager->PostEvent( m_EventName );
	}

	if( m_Callback.m_Callback )
	{
		m_Callback.m_Callback( this, m_Callback.m_Void );
	}

	WBActionFactory::ExecuteActionArray( m_Actions, WBEvent(), NULL );
}

/*virtual*/ void UIWidget::Released()
{
}

/*virtual*/ void UIWidget::Drag( float X, float Y )
{
	Unused( X );
	Unused( Y );
}

/*virtual*/ bool UIWidget::HandleInput()
{
	IUIInputMap* const pInputMap = m_UIManager->GetUIInputMap();

	if( pInputMap )
	{
		if( pInputMap->OnAccept() )
		{
			OnTrigger();
			return true;
		}
	}

	return false;
}

void UIWidget::GetFocus()
{
	WBActionFactory::ExecuteActionArray( m_FocusActions, WBEvent(), NULL );
}

bool UIWidget::HasFocus() const
{
	ASSERT( GetOwnerScreen() );

	return GetOwnerScreen()->GetFocusedWidget() == this;
}

int UIWidget::OverrideFocusUp( int FocusShift )
{
	return Pick( m_FocusShiftUp, FocusShift );
}

int UIWidget::OverrideFocusDown( int FocusShift )
{
	return Pick( m_FocusShiftDown, FocusShift );
}

int UIWidget::OverrideFocusLeft( int FocusShift )
{
	return Pick( m_FocusShiftLeft, FocusShift );
}

int UIWidget::OverrideFocusRight( int FocusShift )
{
	return Pick( m_FocusShiftRight, FocusShift );
}

void UIWidget::Reinitialize( const bool WithState )
{
	if( WithState )
	{
		PushState();
	}

	InitializeFromDefinition( m_Name );

	if( WithState )
	{
		PullState();
	}
}

void UIWidget::Refresh()
{
}

/*virtual*/ void UIWidget::Tick( const float DeltaTime )
{
	Unused( DeltaTime );
}

void UIWidget::SetHidden( const bool Hidden )
{
	m_Hidden = Hidden;
	Refresh();

	if( IsHidden() && HasFocus() )
	{
		GetOwnerScreen()->FocusNext();
	}
}

void UIWidget::Show()
{
	m_Hidden = false;
	Refresh();
}

void UIWidget::SetDisabled( const bool Disabled )
{
	m_IsDisabled = Disabled;

	if( IsDisabled() && HasFocus() )
	{
		GetOwnerScreen()->FocusNext();
	}
}

// This is really ugly. I should refactor UI system to support dynamic
// positions uniformly for all widgets, and without rebuilding meshes.
void UIWidget::SetScreenLocation( const float X, const float Y )
{
	MAKEHASH( m_Name );

	STATICHASH( ScreenWX );
	ConfigManager::SetFloat( sScreenWX, X, sm_Name );

	STATICHASH( ScreenHY );
	ConfigManager::SetFloat( sScreenHY, Y, sm_Name );

	Reinitialize();
}

void UIWidget::SetParentLocation( const float X, const float Y )
{
	MAKEHASH( m_Name );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, X, sm_Name );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, Y, sm_Name );

	Reinitialize();
}

void UIWidget::SetListY( const float ListY )
{
	MAKEHASH( m_Name );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, ListY, sm_Name );

	// HACKHACK: Reinitialize() (with state) will trigger slider events before the world exists
	Reinitialize_Stateless();
}

void UIWidget::SetExtentsScalar( const float W, const float H )
{
	MAKEHASH( m_Name );

	STATICHASH( ExtentsScalarW );
	ConfigManager::SetFloat( sExtentsScalarW, W, sm_Name );

	STATICHASH( ExtentsScalarH );
	ConfigManager::SetFloat( sExtentsScalarH, H, sm_Name );

	Reinitialize();
}

/*virtual*/ void UIWidget::PushState()
{
	m_IsDisabled_SavedState	= m_IsDisabled;
	m_Hidden_SavedState		= m_Hidden;
}

/*virtual*/ void UIWidget::PullState()
{
	m_IsDisabled	= m_IsDisabled_SavedState;
	m_Hidden		= m_Hidden_SavedState;
}

void UIWidget::SetUIManager( UIManager* pManager )
{
	m_UIManager = pManager;
}

void UIWidget::GetOrigin( const HashedString& Origin )
{
	STATIC_HASHED_STRING( TopLeft );
	STATIC_HASHED_STRING( TopCenter );
	STATIC_HASHED_STRING( Top );
	STATIC_HASHED_STRING( TopRight );
	STATIC_HASHED_STRING( CenterLeft );
	STATIC_HASHED_STRING( Left );
	STATIC_HASHED_STRING( Center );
	STATIC_HASHED_STRING( CenterRight );
	STATIC_HASHED_STRING( Right );
	STATIC_HASHED_STRING( BottomLeft );
	STATIC_HASHED_STRING( BottomCenter );
	STATIC_HASHED_STRING( Bottom );
	STATIC_HASHED_STRING( BottomRight );

	if( Origin == sTopLeft )
	{
		m_Origin = EWO_TopLeft;
	}
	else if( Origin == sTopCenter || Origin == sTop )
	{
		m_Origin = EWO_TopCenter;
	}
	else if( Origin == sTopRight )
	{
		m_Origin = EWO_TopRight;
	}
	else if( Origin == sCenterLeft || Origin == sLeft )
	{
		m_Origin = EWO_CenterLeft;
	}
	else if( Origin == sCenter )
	{
		m_Origin = EWO_Center;
	}
	else if( Origin == sCenterRight || Origin == sRight )
	{
		m_Origin = EWO_CenterRight;
	}
	else if( Origin == sBottomLeft )
	{
		m_Origin = EWO_BottomLeft;
	}
	else if( Origin == sBottomCenter || Origin == sBottom )
	{
		m_Origin = EWO_BottomCenter;
	}
	else if( Origin == sBottomRight )
	{
		m_Origin = EWO_BottomRight;
	}
	else
	{
		// Unspecified origin defaults to top-left. That's fine. Some widgets don't need origins.
	}
}

void UIWidget::SetTransform( const Vector2& Location, const Vector2& Extents )
{
	SetPositionFromOrigin( Location, Extents );
	ClampAlignForPixelGrid();

	m_Extents = Extents;
	if( m_ClampToPixelGrid )
	{
		m_Extents.x = Round( m_Extents.x );
		m_Extents.y = Round( m_Extents.y );
	}
}

void UIWidget::SetConfigTransform( const SimpleString& DefinitionName )
{
	Vector2 Location;
	Vector2 Extents;

	GetConfigTransform( DefinitionName, Location, Extents );
	AdjustDimensionsToParent( DefinitionName, Location, Extents );

	SetTransform( Location, Extents );
}

void UIWidget::GetConfigTransform( const SimpleString& DefinitionName, Vector2& OutLocation, Vector2& OutExtents ) const
{
	MAKEHASH( DefinitionName );

	// HACKHACK for ultrawidescreen support, requires user to set HUDDisplayWidth/Height in their prefs.cfg manually to override screen dimensions
	STATICHASH( UseHUDScreen );
	const bool	UseHUDScreen		= ConfigManager::GetBool( sUseHUDScreen, false, sDefinitionName );

	STATICHASH( HUDDisplayWidth );
	const float HUDDisplayWidth		= ConfigManager::GetFloat( sHUDDisplayWidth );

	STATICHASH( HUDDisplayHeight );
	const float HUDDisplayHeight	= ConfigManager::GetFloat( sHUDDisplayHeight );

	STATICHASH( DisplayWidth );
	const float TrueDisplayWidth	= ConfigManager::GetFloat( sDisplayWidth );

	STATICHASH( DisplayHeight );
	const float TrueDisplayHeight	= ConfigManager::GetFloat( sDisplayHeight );

	const float DisplayWidth		= ( UseHUDScreen && HUDDisplayWidth > 0 ) ? HUDDisplayWidth : TrueDisplayWidth;
	const float ParentWidth			= m_ParentWidget ? m_ParentWidget->GetWidth() : DisplayWidth;

	const float DisplayHeight		= ( UseHUDScreen && HUDDisplayHeight > 0 ) ? HUDDisplayHeight : TrueDisplayHeight;
	const float ParentHeight		= m_ParentWidget ? m_ParentWidget->GetHeight() : DisplayHeight;

	STATICHASH( PixelOffsetX );
	STATICHASH( PixelX );
	STATICHASH( ParentWX );
	STATICHASH( ScreenWX );
	STATICHASH( ParentHX );
	STATICHASH( ScreenHX );
	OutLocation.x =
		ConfigManager::GetInheritedFloat( sPixelOffsetX, 0.0f, sDefinitionName ) +
		Pick(
			ConfigManager::GetInheritedFloat( sPixelX, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWX, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWX, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHX, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHX, 0.0f, sDefinitionName ) );

	STATICHASH( PixelOffsetY );
	STATICHASH( PixelY );
	STATICHASH( ParentHY );
	STATICHASH( ScreenHY );
	STATICHASH( ParentWY );
	STATICHASH( ScreenWY );
	OutLocation.y =
		ConfigManager::GetInheritedFloat( sPixelOffsetY, 0.0f, sDefinitionName ) +
		Pick(
			ConfigManager::GetInheritedFloat( sPixelY, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHY, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHY, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWY, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWY, 0.0f, sDefinitionName ) );

	if( m_UseVanishingPointY )
	{
		STATICHASH( VanishingPointY );
		const float VanishingPointY = ConfigManager::GetFloat( sVanishingPointY );
		OutLocation.y += DisplayHeight * VanishingPointY;
	}

	STATICHASH( PixelW );
	STATICHASH( ParentWW );
	STATICHASH( ScreenWW );
	STATICHASH( ParentHW );
	STATICHASH( ScreenHW );
	OutExtents.x =
		Pick(
			ConfigManager::GetInheritedFloat( sPixelW, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWW, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWW, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHW, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHW, 0.0f, sDefinitionName ) );

	STATICHASH( PixelH );
	STATICHASH( ParentHH );
	STATICHASH( ScreenHH );
	STATICHASH( ParentWH );
	STATICHASH( ScreenWH );
	OutExtents.y =
		Pick(
			ConfigManager::GetInheritedFloat( sPixelH, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHH, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHH, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWH, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWH, 0.0f, sDefinitionName ) );

	// Some things like spacers are allowed to user zero extents, everything else should adjust for aspect
	STATICHASH( AllowZeroExtents );
	const bool AllowZeroExtents = ConfigManager::GetInheritedBool( sAllowZeroExtents, false, sDefinitionName );
	if( !AllowZeroExtents )
	{
		// Adjust for desired aspect ratio if one dimension is not given
		// (This is used to size images using ScreenWidth or ScreenHeight
		// properly regardless of screen aspect ratio.
		STATICHASH( AspectRatio );
		const float AspectRatio = ConfigManager::GetInheritedFloat( sAspectRatio, 1.0f, sDefinitionName );
		if( OutExtents.x == 0.0f )
		{
			OutExtents.x = OutExtents.y * AspectRatio;
		}
		else if( OutExtents.y == 0.0f )
		{
			OutExtents.y = OutExtents.x / AspectRatio;
		}
	}

	STATICHASH( ExtentsScalarW );
	OutExtents.x *= ConfigManager::GetInheritedFloat( sExtentsScalarW, 1.0f, sDefinitionName );

	STATICHASH( ExtentsScalarH );
	OutExtents.y *= ConfigManager::GetInheritedFloat( sExtentsScalarH, 1.0f, sDefinitionName );
}

void UIWidget::AdjustDimensionsToParent( const SimpleString& DefinitionName, Vector2& Location, Vector2& Extents )
{
	MAKEHASH( DefinitionName );

	// HACKHACK for ultrawidescreen support, requires user to set HUDDisplayWidth/Height in their prefs.cfg manually to override screen dimensions
	STATICHASH( UseHUDScreen );
	const bool	UseHUDScreen		= ConfigManager::GetBool( sUseHUDScreen, false, sDefinitionName );

	STATICHASH( HUDDisplayWidth );
	const float HUDDisplayWidth		= ConfigManager::GetFloat( sHUDDisplayWidth );

	STATICHASH( HUDDisplayHeight );
	const float HUDDisplayHeight	= ConfigManager::GetFloat( sHUDDisplayHeight );

	STATICHASH( DisplayWidth );
	const float TrueDisplayWidth	= ConfigManager::GetFloat( sDisplayWidth );

	STATICHASH( DisplayHeight );
	const float TrueDisplayHeight	= ConfigManager::GetFloat( sDisplayHeight );

	const float DisplayWidth		= ( UseHUDScreen && HUDDisplayWidth > 0 ) ? HUDDisplayWidth : TrueDisplayWidth;
	const float	OffsetX				= ( TrueDisplayWidth - DisplayWidth ) * 0.5f;
	const float ParentWidth			= m_ParentWidget ? m_ParentWidget->GetWidth() : DisplayWidth;
	const float ParentX				= m_ParentWidget ? m_ParentWidget->GetX() : 0.0f;	// This used to be Ceiling( m_ParentWidget->GetX() ), maybe for clamping to pixel grid? But it messes things up.

	const float DisplayHeight		= ( UseHUDScreen && HUDDisplayHeight > 0 ) ? HUDDisplayHeight : TrueDisplayHeight;
	const float	OffsetY				= ( TrueDisplayHeight - DisplayHeight ) * 0.5f;
	const float ParentHeight		= m_ParentWidget ? m_ParentWidget->GetHeight() : DisplayHeight;
	const float ParentY				= m_ParentWidget ? m_ParentWidget->GetY() : 0.0f;	// This used to be Ceiling( m_ParentWidget->GetY() ), maybe for clamping to pixel grid? But it messes things up.

	// Use negative numbers as offsets from the right/bottom of the screen unless negative origin is allowed
	Location.x	= ( !m_AllowNegativeOrigin && Location.x < 0.0f )	? ( Location.x	+ ( ParentX + ParentWidth ) )	: ParentX + Location.x;
	Location.y	= ( !m_AllowNegativeOrigin && Location.y < 0.0f )	? ( Location.y	+ ( ParentY + ParentHeight ) )	: ParentY + Location.y;
	Extents.x	= ( Extents.x < 0.0f )								? ( Extents.x	+ ParentWidth )					: Extents.x;
	Extents.y	= ( Extents.y < 0.0f )								? ( Extents.y	+ ParentHeight )				: Extents.y;

	Location.x	+= OffsetX;
	Location.y	+= OffsetY;
}

void UIWidget::SetPositionFromOrigin( const Vector2& Location, const Vector2& Extents )
{
	switch( m_Origin )
	{
	case EWO_TopLeft:
		m_Location = Location;
		break;
	case EWO_TopCenter:
		m_Location = Vector2( Location.x - Extents.x * 0.5f,	Location.y );
		break;
	case EWO_TopRight:
		m_Location = Vector2( Location.x - Extents.x,			Location.y );
		break;
	case EWO_CenterLeft:
		m_Location = Vector2( Location.x,						Location.y - Extents.y * 0.5f );
		break;
	case EWO_Center:
		m_Location = Vector2( Location.x - Extents.x * 0.5f,	Location.y - Extents.y * 0.5f );
		break;
	case EWO_CenterRight:
		m_Location = Vector2( Location.x - Extents.x,			Location.y - Extents.y * 0.5f );
		break;
	case EWO_BottomLeft:
		m_Location = Vector2( Location.x,						Location.y - Extents.y );
		break;
	case EWO_BottomCenter:
		m_Location = Vector2( Location.x - Extents.x * 0.5f,	Location.y - Extents.y );
		break;
	case EWO_BottomRight:
		m_Location = Vector2( Location.x - Extents.x,			Location.y - Extents.y );
		break;
	default:
		WARNDESC( "Bad origin for unknown reason" );
		break;
	}
}

Vector4 UIWidget::GetHighlightColor() const
{
	if( m_UsePulsingHighlight )
	{
		const float		CurrentTime		= m_UIManager->GetClock()->GetMachineCurrentTime();
		const float		TimePulse		= Cos( CurrentTime * m_PulsingHighlightTimeScalar );
		const float		LerpT			= ( TimePulse * m_PulsingHighlightMul ) + m_PulsingHighlightAdd;
		const Vector4	PulseHighlight	= m_Color.LERP( LerpT, m_Highlight );

		return PulseHighlight;
	}
	else
	{
		return m_Highlight;
	}
}

void UIWidget::ClampAlignForPixelGrid()
{
	if( m_ClampToPixelGrid )
	{
		m_Location.x = Round( m_Location.x );
		m_Location.y = Round( m_Location.y );
	}
}

bool UIWidget::GetUseVanishingPointY( const bool CheckParents ) const
{
	if( m_UseVanishingPointY )
	{
		return true;
	}

	if( CheckParents && NULL != m_ParentWidget )
	{
		return m_ParentWidget->GetUseVanishingPointY( true );
	}

	return false;
}
