#include "core.h"
#include "3d.h"
#include "uiscreen.h"
#include "uiwidget.h"
#include "uimanager.h"
#include "iuiinputmap.h"
#include "inputsystem.h"
#include "mouse.h"
#include "keyboard.h"
#include "display.h"
#include "windowwrapper.h"
#include "configmanager.h"
#include "stringmanager.h"
#include "iaudiosystem.h"
#include "linebatcher.h"
#include "uifactory.h"
#include "wbaction.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "wbevent.h"
#include "mathcore.h"
#include "clock.h"

// For old non-factory based code.
#include "Widgets/uiwidget-image.h"
#include "Widgets/uiwidget-text.h"
#include "Widgets/uiwidget-slider.h"

UIManager*	UIScreen::m_UIManager = NULL;
bool		UIScreen::m_MouseButtonsSwapped = false;

UIScreen::UIScreen()
:   m_Widgets()
,	m_RenderWidgets()
,	m_FocusWidgets()
,	m_FocusedWidget( NULL )
,   m_Name()
,	m_TakesFocus( false )
,	m_PausesGame( false )
,	m_SuppressesInput( false )
,	m_AllowClose( false )
,	m_SeizesMouse( false )
,	m_HasSeizedMouse( false )
,   m_GameTick( false )
,	m_FocusXShift( 0 )
,	m_FocusYShift( 0 )
,	m_LastMousePosX( 0 )
,	m_LastMousePosY( 0 )
,	m_ClickedWidget( NULL )
,	m_PushedSound()
,	m_PoppedSound()
,	m_OnStack( false )
,	m_Hidden( false )
,	m_PushedTime( 0.0f )
,	m_PopTimeout( 0.0f )
,	m_PushedActions()
,	m_PoppedActions()
,	m_ClosedActions()
{
}

UIScreen::UIScreen( const SimpleString& DefinitionName )
:   m_Widgets()
,	m_RenderWidgets()
,	m_FocusWidgets()
,	m_FocusedWidget( NULL )
,   m_Name()
,	m_TakesFocus( false )
,	m_PausesGame( false )
,	m_SuppressesInput( false )
,	m_AllowClose( false )
,	m_SeizesMouse( false )
,	m_HasSeizedMouse( false )
,   m_GameTick( false )
,	m_FocusXShift( 0 )
,	m_FocusYShift( 0 )
,	m_LastMousePosX( 0 )
,	m_LastMousePosY( 0 )
,	m_ClickedWidget( NULL )
,	m_PushedSound()
,	m_PoppedSound()
,	m_OnStack( false )
,	m_Hidden( false )
,	m_PushedTime( 0.0f )
,	m_PopTimeout( 0.0f )
,	m_PushedActions()
,	m_PoppedActions()
,	m_ClosedActions()
{
	InitializeFromDefinition( DefinitionName );
}

UIScreen::~UIScreen()
{
	Flush();
}

void UIScreen::Flush()
{
	FlushWidgets();
	FlushActions();
}

void UIScreen::FlushWidgets()
{
	FOR_EACH_MAP( WidgetIter, m_Widgets, HashedString, UIWidget* )
	{
		UIWidget* pWidget = WidgetIter.GetValue();
		SafeDelete( pWidget );
	}

	m_Widgets.Clear();
	m_RenderWidgets.Clear();
	m_FocusWidgets.Clear();
	m_FocusedWidget = NULL;
	m_ClickedWidget = NULL;
}

void UIScreen::FlushActions()
{
	FOR_EACH_ARRAY( ActionIter, m_PushedActions, WBAction* )
	{
		WBAction* pAction = ActionIter.GetValue();
		SafeDelete( pAction );
	}
	m_PushedActions.Clear();

	FOR_EACH_ARRAY( ActionIter, m_PoppedActions, WBAction* )
	{
		WBAction* pAction = ActionIter.GetValue();
		SafeDelete( pAction );
	}
	m_PoppedActions.Clear();

	FOR_EACH_ARRAY( ActionIter, m_ClosedActions, WBAction* )
	{
		WBAction* pAction = ActionIter.GetValue();
		SafeDelete( pAction );
	}
	m_ClosedActions.Clear();
}

void UIScreen::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	m_Name = DefinitionName;

	STATICHASH( TakesFocus );
	m_TakesFocus		= ConfigManager::GetInheritedBool( sTakesFocus, false, sDefinitionName );

	STATICHASH( PausesGame );
	m_PausesGame		= ConfigManager::GetInheritedBool( sPausesGame, false, sDefinitionName );

	STATICHASH( SuppressesInput );
	m_SuppressesInput	= ConfigManager::GetInheritedBool( sSuppressesInput, false, sDefinitionName );

	STATICHASH( AllowClose );
	m_AllowClose		= ConfigManager::GetInheritedBool( sAllowClose, true, sDefinitionName );

	STATICHASH( SeizesMouse );
	m_SeizesMouse		= ConfigManager::GetInheritedBool( sSeizesMouse, false, sDefinitionName );

	STATICHASH( GameTick );
	m_GameTick			= ConfigManager::GetInheritedBool( sGameTick, false, sDefinitionName );

	STATICHASH( FocusXShift );
	m_FocusXShift		= ConfigManager::GetInheritedInt( sFocusXShift, 1, sDefinitionName );

	STATICHASH( FocusYShift );
	m_FocusYShift		= ConfigManager::GetInheritedInt( sFocusYShift, 1, sDefinitionName );

	STATICHASH( PopTimeout );
	m_PopTimeout		= ConfigManager::GetInheritedFloat( sPopTimeout, 0.0f, sDefinitionName );

	STATICHASH( NumPushedActions );
	const uint NumPushedActions = ConfigManager::GetInheritedInt( sNumPushedActions, 0, sDefinitionName );
	for( uint PushedActionIndex = 0; PushedActionIndex < NumPushedActions; ++PushedActionIndex )
	{
		const SimpleString Action = ConfigManager::GetInheritedSequenceString( "PushedAction%d", PushedActionIndex, "", sDefinitionName );
		WBAction* const pAction = WBActionFactory::Create( Action );
		if( pAction )
		{
			m_PushedActions.PushBack( pAction );
		}
	}

	STATICHASH( NumPoppedActions );
	const uint NumPoppedActions = ConfigManager::GetInheritedInt( sNumPoppedActions, 0, sDefinitionName );
	for( uint PoppedActionIndex = 0; PoppedActionIndex < NumPoppedActions; ++PoppedActionIndex )
	{
		const SimpleString Action = ConfigManager::GetInheritedSequenceString( "PoppedAction%d", PoppedActionIndex, "", sDefinitionName );
		WBAction* const pAction = WBActionFactory::Create( Action );
		if( pAction )
		{
			m_PoppedActions.PushBack( pAction );
		}
	}

	STATICHASH( NumClosedActions );
	const uint NumClosedActions = ConfigManager::GetInheritedInt( sNumClosedActions, 0, sDefinitionName );
	for( uint ClosedActionIndex = 0; ClosedActionIndex < NumClosedActions; ++ClosedActionIndex )
	{
		const SimpleString Action = ConfigManager::GetInheritedSequenceString( "ClosedAction%d", ClosedActionIndex, "", sDefinitionName );
		WBAction* const pAction = WBActionFactory::Create( Action );
		if( pAction )
		{
			m_ClosedActions.PushBack( pAction );
		}
	}

	STATICHASH( PushedSound );
	m_PushedSound = ConfigManager::GetInheritedString( sPushedSound, "", sDefinitionName );

	STATICHASH( PoppedSound );
	m_PoppedSound = ConfigManager::GetInheritedString( sPoppedSound, "", sDefinitionName );

	STATICHASH( NumWidgets );
	const uint NumWidgets = ConfigManager::GetInheritedInt( sNumWidgets, 0, sDefinitionName );
	Array<UIWidget*> ListWidgets;
	uint ListSteps = 0;
	for( uint WidgetIndex = 0; WidgetIndex < NumWidgets; ++WidgetIndex )
	{
		// Dev widgets are supported with a DevOnly bool in the config, see UIFactory::CreateWidget.
		const SimpleString WidgetName = ConfigManager::GetInheritedSequenceString( "Widget%d", WidgetIndex, "", sDefinitionName );
		UIWidget* const pUIWidget = UIFactory::CreateWidget( WidgetName, this, NULL );
		if( pUIWidget )
		{
			AddWidget( pUIWidget );

			if( pUIWidget->m_ListStep )
			{
				ListSteps++;
			}
			if( pUIWidget->m_ListStep || pUIWidget->m_UseListY )
			{
				ListWidgets.PushBack( pUIWidget );
			}
		}
	}

	// Fix up list widgets, if any
	if( ListWidgets.Size() > 0 )
	{
		STATICHASH( ListMinParentHY );
		const float ListMinParentHY = ConfigManager::GetInheritedFloat( sListMinParentHY, 0.0f, sDefinitionName );

		STATICHASH( ListMaxParentHY );
		const float ListMaxParentHY = ConfigManager::GetInheritedFloat( sListMaxParentHY, 0.0f, sDefinitionName );

		ASSERT( ListMaxParentHY > ListMinParentHY );
		const float ListStepParentHY = ( ListSteps > 1 ) ? ( ( ListMaxParentHY - ListMinParentHY ) / static_cast<float>( ListSteps - 1 ) ) : 0.0f;

		float ListY = ListMinParentHY;
		FOR_EACH_ARRAY( ListWidgetIter, ListWidgets, UIWidget* )
		{
			UIWidget* const pListWidget = ListWidgetIter.GetValue();
			pListWidget->SetListY( ListY );
			ListY += ListStepParentHY;
		}
	}

	UpdateRender();
	ResetFocus();
}

void UIScreen::Reinitialize( const bool WithState )
{
	FOR_EACH_ARRAY( WidgetIter, m_RenderWidgets, UIWidget* )
	{
		WidgetIter.GetValue()->Reinitialize( WithState );
	}

	UpdateRender();
}

/*virtual*/ void UIScreen::RegisterForEvents()
{
	// By default, UI screens are not event listeners and do not need to implement this
}

/*virtual*/ bool UIScreen::TakesFocus()
{
	return m_TakesFocus;
}

/*virtual*/ bool UIScreen::PausesGame()
{
	return m_PausesGame;
}

/*virtual*/ bool UIScreen::SeizesMouse()
{
	return m_SeizesMouse;
}

/*virtual*/ bool UIScreen::SuppressesInput()
{
	return m_SuppressesInput;
}

void UIScreen::TickMouse()
{
	int MousePosX = 0;
	int MousePosY = 0;
	m_UIManager->GetMouse()->GetPosition( MousePosX, MousePosY, m_UIManager->GetWindow() );

	const int MouseOffsetX = MousePosX - m_LastMousePosX;
	const int MouseOffsetY = MousePosY - m_LastMousePosY;
	m_LastMousePosX = MousePosX;
	m_LastMousePosY = MousePosY;

	// HACKHACK: Transform the mouse position to where it appears to be, for upscaled display
	Display* const	pDisplay	= m_UIManager->GetDisplay();
	const float		OffsetX		= static_cast<float>( ( pDisplay->m_FrameWidth - pDisplay->m_PropWidth ) / 2 );
	const float		OffsetY		= static_cast<float>( ( pDisplay->m_FrameHeight - pDisplay->m_PropHeight ) / 2 );
	const float		ScalarX		= static_cast<float>( pDisplay->m_Width ) / static_cast<float>( pDisplay->m_PropWidth );
	const float		ScalarY		= static_cast<float>( pDisplay->m_Height ) / static_cast<float>( pDisplay->m_PropHeight );
	const float		MouseX		= ( static_cast<float>( MousePosX ) - OffsetX ) * ScalarX;
	const float		MouseY		= ( static_cast<float>( MousePosY ) - OffsetY ) * ScalarY;

	// Only activate the focused widget on a mouse click if
	// the widget is still under the cursor when it is clicked.
	// Use keyboard to query mouse buttons because mouse is
	// deactivated (from DirectInput) for UI control.
	// Don't do mouse UI stuff if we're unfocused or binding keys
	if( m_FocusedWidget && !m_FocusedWidget->IsUnfocusable() )
	{
		if( m_UIManager->GetKeyboard()->OnRise( LEFT_MOUSE_KEY ) ||
			m_UIManager->GetMouse()->OnRise( LEFT_MOUSE_BUTTON ) )
		{
			SRect WidgetBounds;
			m_FocusedWidget->GetBounds( WidgetBounds );
			if(
				MouseX >= WidgetBounds.m_Left &&
				MouseX <= WidgetBounds.m_Right &&
				MouseY >= WidgetBounds.m_Top &&
				MouseY <= WidgetBounds.m_Bottom )
			{
				m_ClickedWidget = m_FocusedWidget;
			}
			else
			{
				m_ClickedWidget = NULL;
			}
		}
		else if(
			m_UIManager->GetKeyboard()->OnFall( LEFT_MOUSE_KEY ) ||
			m_UIManager->GetMouse()->OnFall( LEFT_MOUSE_BUTTON ) )
		{
			if( m_ClickedWidget == m_FocusedWidget )
			{
				SRect WidgetBounds;
				m_FocusedWidget->GetBounds( WidgetBounds );
				if(
					MouseX >= WidgetBounds.m_Left &&
					MouseX <= WidgetBounds.m_Right &&
					MouseY >= WidgetBounds.m_Top &&
					MouseY <= WidgetBounds.m_Bottom )
				{
					m_ClickedWidget->OnTrigger();
					m_ClickedWidget = NULL;
				}
				else
				{
					m_ClickedWidget->Released();
					m_ClickedWidget = NULL;
				}
			}
		}
		else if(
			m_UIManager->GetKeyboard()->IsHigh( LEFT_MOUSE_KEY ) ||
			m_UIManager->GetMouse()->IsHigh( LEFT_MOUSE_BUTTON ) )
		{
			if( m_ClickedWidget )
			{
				m_ClickedWidget->Drag( MouseX, MouseY );
			}
		}
	}

	if( !m_UIManager->GetKeyboard()->IsHigh( LEFT_MOUSE_KEY ) &&
		!m_UIManager->GetMouse()->IsHigh( LEFT_MOUSE_BUTTON ) )
	{
		// Only refocus under the cursor when the mouse moves and the button is not held down
		if( MouseOffsetX || MouseOffsetY ||
			m_UIManager->GetKeyboard()->OnFall( LEFT_MOUSE_KEY ) ||
			m_UIManager->GetMouse()->OnFall( LEFT_MOUSE_BUTTON ) )
		{
			for( uint i = 0; i < m_FocusWidgets.Size(); ++i )
			{
				UIWidget* const pWidget = m_FocusWidgets[i];

				if( pWidget->IsUnfocusable() )
				{
					continue;
				}

				SRect WidgetBounds;
				pWidget->GetBounds( WidgetBounds );
				if(	// TODO: Make this test a function of SRect, and maybe make SRect a full-fledged class
					MouseX >= WidgetBounds.m_Left &&
					MouseX <= WidgetBounds.m_Right &&
					MouseY >= WidgetBounds.m_Top &&
					MouseY <= WidgetBounds.m_Bottom )
				{
					SetFocus( pWidget );
					break;
				}
			}
		}
	}
}

void UIScreen::TickWidgets( const float DeltaTime )
{
	for( uint i = 0; i < m_RenderWidgets.Size(); ++i )
	{
		UIWidget* pWidget = m_RenderWidgets[i];
		pWidget->Tick( DeltaTime );
	}
}

void UIScreen::RefreshWidgets()
{
	for( uint i = 0; i < m_RenderWidgets.Size(); ++i )
	{
		UIWidget* pWidget = m_RenderWidgets[i];
		pWidget->Refresh();
	}
}

UIScreen::ETickReturn UIScreen::Tick( const float DeltaTime, bool HasFocus )
{
	XTRACE_FUNCTION;

	Unused( DeltaTime );

	if( !HasFocus )
	{
		m_ClickedWidget = NULL;
	}

	RefreshWidgets();
	TickWidgets( DeltaTime );

	if( HasFocus && m_UIManager->GetWindow()->HasFocus() )
	{
		// Mouse input has to be handled different than keyboard
		// and controller input, because we have a cursor.
		TickMouse();

		if( m_FocusedWidget && !m_FocusedWidget->IsUnfocusable() && m_FocusedWidget->HandleInput() )
		{
			// Widget consumed input, don't do anything here.
		}
		else
		{
			return TickInput();
		}
	}

	return ETR_None;
}

/*virtual*/ UIScreen::ETickReturn UIScreen::TickInput()
{
	IUIInputMap* const pInputMap = m_UIManager->GetUIInputMap();
	if( pInputMap )
	{
		if( pInputMap->OnNext() )
		{
			FocusNext();
		}
		else if( pInputMap->OnPrevious() )
		{
			FocusPrevious();
		}
		else if( pInputMap->OnUp() )
		{
			FocusUp();
		}
		else if( pInputMap->OnDown() )
		{
			FocusDown();
		}
		else if( pInputMap->OnLeft() )
		{
			FocusLeft();
		}
		else if( pInputMap->OnRight() )
		{
			FocusRight();
		}
		else if( pInputMap->OnPrevPage() )
		{
			PrevPage();
		}
		else if( pInputMap->OnNextPage() )
		{
			NextPage();
		}
		else if( pInputMap->OnCancel() )
		{
			return ETR_Close;
		}
	}

	return ETR_None;
}

void UIScreen::Pushed()
{
	m_OnStack = true;
	if( m_UIManager->GetAudioSystem() && m_PushedSound != "" )
	{
		m_UIManager->GetAudioSystem()->Play( m_PushedSound, Vector() );
	}

	if( m_SeizesMouse )
	{
		Mouse* pMouse = m_UIManager->GetMouse();
		if( pMouse->IsActive() )
		{
			pMouse->SetActive( false );
			m_HasSeizedMouse = true;
		}
		else
		{
			m_HasSeizedMouse = false;
		}
	}

	m_PushedTime = m_UIManager->GetClock()->GetMachineCurrentTime();

	RefreshWidgets();

	// If this screen takes focus but does not pause the game, push a
	// null input context to suppress game input behind the screen
	if( SuppressesInput() )
	{
		InputSystem* const pInputSystem = m_UIManager->GetInputSystem();
		if( pInputSystem )
		{
			STATIC_HASHED_STRING( Context_Null );
			pInputSystem->PushContext( sContext_Null );
		}
	}

	WBActionFactory::ExecuteActionArray( m_PushedActions, WBEvent(), NULL );
}

void UIScreen::Popped()
{
	m_OnStack = false;
	m_ClickedWidget = NULL;
	if( m_UIManager->GetAudioSystem() && m_PoppedSound != "" )
	{
		m_UIManager->GetAudioSystem()->Play( m_PoppedSound, Vector() );
	}

	if( m_SeizesMouse && m_HasSeizedMouse )
	{
		m_UIManager->GetMouse()->SetActive( true );
		m_HasSeizedMouse = false;
	}

	if( SuppressesInput() )
	{
		InputSystem* const pInputSystem = m_UIManager->GetInputSystem();
		if( pInputSystem )
		{
			STATIC_HASHED_STRING( Context_Null );
			pInputSystem->PopContext( sContext_Null );
			pInputSystem->Tick();	// Debounce inputs
		}
	}

	WBActionFactory::ExecuteActionArray( m_PoppedActions, WBEvent(), NULL );
}

void UIScreen::Closed()
{
	WBActionFactory::ExecuteActionArray( m_ClosedActions, WBEvent(), NULL );
}

bool UIScreen::CanClose() const
{
	return m_AllowClose && CanPop();
}

bool UIScreen::CanPop() const
{
	const float CurrentTime	= m_UIManager->GetClock()->GetMachineCurrentTime();
	const float PopTime		= m_PushedTime + m_PopTimeout;
	return CurrentTime >= PopTime;
}

void UIScreen::AddWidget( UIWidget* pNewWidget )
{
	ASSERT( pNewWidget );

	m_RenderWidgets.PushBack( pNewWidget );
	if( pNewWidget->m_CanBeFocused )
	{
		m_FocusWidgets.PushBack( pNewWidget );
	}

	m_Widgets[ pNewWidget->m_Name ] = pNewWidget;
}

/*virtual*/ void UIScreen::Render( bool HasFocus )
{
	XTRACE_FUNCTION;

	Unused( HasFocus );

	const uint NumRenderWidgets = m_RenderWidgets.Size();
	for( uint RenderWidgetIndex = 0; RenderWidgetIndex < NumRenderWidgets; ++RenderWidgetIndex )
	{
		UIWidget* const pWidget = m_RenderWidgets[ RenderWidgetIndex ];
		if( pWidget->IsHidden() )
		{
			// Skip
		}
		else
		{
			pWidget->Render( ( m_FocusedWidget == pWidget ) );
		}
	}

#if BUILD_DEBUG
	STATICHASH( DebugRenderUIBounds );
	if( HasFocus && ConfigManager::GetBool( sDebugRenderUIBounds, false ) )
	{
		if( m_UIManager->GetLineBatcher() )
		{
			for( uint i = 0; i < m_FocusWidgets.Size(); ++i )
			{
				UIWidget* pWidget = m_FocusWidgets[i];
				SRect WidgetBounds;
				pWidget->GetBounds( WidgetBounds );
				m_UIManager->GetLineBatcher()->DrawBox(
					Vector( WidgetBounds.m_Left, 0.0f, WidgetBounds.m_Top ),
					Vector( WidgetBounds.m_Right, 0.0f, WidgetBounds.m_Bottom ),
					ARGB_TO_COLOR( 255, 255, 255, 0 ) );
			}
		}
		else
		{
			WARNDESC( "UI manager does not have a line batcher and cannot render debug bounds." );
		}
	}
#endif
}

void UIScreen::ResetFocus( const bool DoSetFocus )
{
	// Initialize focus to the first enabled element (so that we're never
	// unfocused, because that doesn't make sense for controllers).
	FOR_EACH_ARRAY( FocusWidgetIter, m_FocusWidgets, UIWidget* )
	{
		UIWidget* const pFocusWidget = FocusWidgetIter.GetValue();
		if( pFocusWidget->IsUnfocusable() )
		{
			continue;
		}

		if( DoSetFocus )
		{
			// Force focus here for the same reason we're setting focus at all.
			// This is all part of a hack for the bracelet screen in Rosa, where
			// a single widget changes its purpose when the page flips.
			ForceSetFocus( pFocusWidget );
		}
		else
		{
			// In many cases, we just need an initial focus for controllers.
			// We don't want side effects because it's not really *getting* focus.
			m_FocusedWidget = pFocusWidget;
		}

		return;
	}
}

void UIScreen::UpdateRender()
{
	SortWidgets();
}

void UIScreen::SortWidgets()
{
	// Simple insertion sort. Stable, doesn't reorder widgets with same priority.

	// Sort render widgets by render priority
	for( uint i = 0; i < m_RenderWidgets.Size(); ++i )
	{
		UIWidget* pWidget = m_RenderWidgets[i];
		int j = i - 1;
		while( j >= 0 && m_RenderWidgets[j]->m_RenderPriority > pWidget->m_RenderPriority )
		{
			m_RenderWidgets[ j + 1 ] = m_RenderWidgets[j];
			--j;
		}
		m_RenderWidgets[ j + 1 ] = pWidget;
	}

	// Sort focus widgets by focus order
	for( uint i = 0; i < m_FocusWidgets.Size(); ++i )
	{
		UIWidget* pWidget = m_FocusWidgets[i];
		int j = i - 1;
		while( j >= 0 && m_FocusWidgets[j]->m_FocusOrder > pWidget->m_FocusOrder )
		{
			m_FocusWidgets[ j + 1 ] = m_FocusWidgets[j];
			--j;
		}
		m_FocusWidgets[ j + 1 ] = pWidget;
	}
}

UIWidget* UIScreen::GetWidget( const HashedString& Name )
{
	Map<HashedString, UIWidget*>::Iterator WidgetIter = m_Widgets.Search( Name );
	if( WidgetIter.IsValid() )
	{
		return *WidgetIter;
	}
	else
	{
		return NULL;
	}
}

void UIScreen::SetFocus( UIWidget* const pWidget )
{
	DEVASSERT( pWidget );
	DEVASSERT( !pWidget->IsUnfocusable() );

	if( m_FocusedWidget != pWidget )
	{
		m_FocusedWidget = pWidget;
		m_FocusedWidget->GetFocus();
	}
}

void UIScreen::ForceSetFocus( UIWidget* const pWidget )
{
	DEVASSERT( pWidget );
	DEVASSERT( !pWidget->IsUnfocusable() );

	m_FocusedWidget = pWidget;
	m_FocusedWidget->GetFocus();
}

void UIScreen::FocusNext()
{
	ShiftFocus( 1 );
}

void UIScreen::FocusPrevious()
{
	ShiftFocus( -1 );
}

void UIScreen::FocusUp()
{
	const int FocusShift = m_FocusedWidget ? m_FocusedWidget->OverrideFocusUp( -m_FocusYShift ) : -m_FocusYShift;
	ShiftFocus( FocusShift );
}

void UIScreen::FocusDown()
{
	const int FocusShift = m_FocusedWidget ? m_FocusedWidget->OverrideFocusDown( m_FocusYShift ) : m_FocusYShift;
	ShiftFocus( FocusShift );
}

void UIScreen::FocusLeft()
{
	const int FocusShift = m_FocusedWidget ? m_FocusedWidget->OverrideFocusLeft( -m_FocusXShift ) : -m_FocusXShift;
	ShiftFocus( FocusShift );
}

void UIScreen::FocusRight()
{
	const int FocusShift = m_FocusedWidget ? m_FocusedWidget->OverrideFocusRight( m_FocusXShift ) : m_FocusXShift;
	ShiftFocus( FocusShift );
}

void UIScreen::ShiftFocus( int Amount )
{
	const uint NumFocusWidgets = m_FocusWidgets.Size();
	if( NumFocusWidgets > 0 )
	{
		uint		OriginalFocusIndex	= GetFocusIndex();
		uint		FocusIndex			= OriginalFocusIndex;
		UIWidget*	pFocusWidget		= NULL;

		do
		{
			// Add size to focus index to prevent subtracting from zero
			FocusIndex = ( FocusIndex + NumFocusWidgets + Amount ) % NumFocusWidgets;
			pFocusWidget	= m_FocusWidgets[ FocusIndex ];

			// HACK: Go to the next widget in order instead of continuing to skip by the shift amount.
			// This is specifically to deal with a special case in Eldritch (gear icons on the pause
			// screen being disabled in any combination), and shouldn't break anything anywhere else.
			// Generally, I don't know enough about the layout of a page to infer what should happen.
			Amount = Sign( Amount );
		}
		while( ( pFocusWidget->IsUnfocusable() ) && FocusIndex != OriginalFocusIndex );

		if( FocusIndex == OriginalFocusIndex )
		{
			// There wasn't a better option for focus!
		}
		else
		{
			SetFocus( pFocusWidget );
		}
	}
}

uint UIScreen::GetFocusIndex()
{
	const uint NumFocusWidgets = m_FocusWidgets.Size();
	for( uint i = 0; i < NumFocusWidgets; ++i )
	{
		if( m_FocusWidgets[i] == m_FocusedWidget )
		{
			return i;
		}
	}
	return 0;
}

/*virtual*/ void UIScreen::PrevPage()
{
	// Do nothing, most screens aren't paginated
}

/*virtual*/ void UIScreen::NextPage()
{
	// Do nothing, most screens aren't paginated
}

/*static*/ void UIScreen::UpdateMouseButtonsSwapped()
{
#if BUILD_WINDOWS_NO_SDL
	m_MouseButtonsSwapped = ( GetSystemMetrics( SM_SWAPBUTTON ) != 0 );
#endif
	// NOTE: SDL does this automatically, no need to support it.
}

void UIScreen::OnVanishingPointYChanged()
{
	FOR_EACH_ARRAY( WidgetIter, m_RenderWidgets, UIWidget* )
	{
		UIWidget* const pWidget = WidgetIter.GetValue();
		if( pWidget->GetUseVanishingPointY( true /*CheckParents*/ ) )
		{
			pWidget->Reinitialize();
		}
	}
}
