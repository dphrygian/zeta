#ifndef UISCREEN_H
#define UISCREEN_H

#include "ui.h"
#include "array.h"
#include "simplestring.h"
#include "map.h"

class Game;
class UIManager;
class UIWidget;
class WBAction;

#define DEFINE_UISCREEN_FACTORY( type ) static class UIScreen* Factory() { return new UIScreen##type; }
typedef class UIScreen* ( *UIScreenFactoryFunc )( void );

// TODO: Generalize this beyond UI. This will fix it for a lot of cases, though.
#define LEFT_MOUSE_KEY		( UIScreen::m_MouseButtonsSwapped ? Keyboard::EB_Mouse_Right	: Keyboard::EB_Mouse_Left )
#define LEFT_MOUSE_BUTTON	( UIScreen::m_MouseButtonsSwapped ? Mouse::EB_Right				: Mouse::EB_Left )
#define RIGHT_MOUSE_KEY		( UIScreen::m_MouseButtonsSwapped ? Keyboard::EB_Mouse_Left		: Keyboard::EB_Mouse_Right )
#define RIGHT_MOUSE_BUTTON	( UIScreen::m_MouseButtonsSwapped ? Mouse::EB_Left				: Mouse::EB_Right )

class UIScreen
{
public:
	UIScreen();
	UIScreen( const SimpleString& DefinitionName );
	virtual ~UIScreen();

	enum ETickReturn
	{
		ETR_None,
		ETR_Close,
	};

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	void				Reinitialize( const bool WithState );
	void				Reinitialize()				{ Reinitialize( true ); }
	void				Reinitialize_Stateless()	{ Reinitialize( false ); }

	virtual void		RegisterForEvents();

	virtual bool		TakesFocus();
	virtual bool		PausesGame();
	virtual bool		SeizesMouse();
	virtual bool		SuppressesInput();
	virtual ETickReturn	Tick( const float DeltaTime, bool HasFocus );
	virtual ETickReturn	TickInput();
	void				TickMouse();

	virtual void		Pushed();
	virtual void		Popped();	// Invoked whenever the screen is removed from the stack
	virtual void		Closed();	// Only invoked when the screen is manually closed, not any other time it is popped

	void				SetAllowClose( const bool AllowClose ) { m_AllowClose = AllowClose; }
	bool				CanClose() const;
	bool				CanPop() const;

	void				AddWidget( UIWidget* pNewWidget );
	virtual void		Render( bool HasFocus );
	void				UpdateRender();
	void				ResetFocus( const bool DoSetFocus );
	void				ResetFocus()				{ ResetFocus( false ); }
	void				ResetFocus_WithEffects()	{ ResetFocus( true ); }
	void				SortWidgets();
	void				RefreshWidgets();
	void				TickWidgets( const float DeltaTime );

	void				Flush();
	void				FlushWidgets();
	void				FlushActions();

	UIWidget*				GetWidget( const HashedString& Name );
	template<class C> C*	GetWidget( const HashedString& Name ) { return static_cast<C*>( GetWidget( Name ) ); }

	UIWidget*			GetFocusedWidget() const { return m_FocusedWidget; }

	void				SetFocus( UIWidget* const pWidget );
	void				ForceSetFocus( UIWidget* const pWidget );
	void				FocusNext();
	void				FocusPrevious();
	void				FocusUp();
	void				FocusDown();
	void				FocusLeft();
	void				FocusRight();
	void				ShiftFocus( int Amount );
	uint				GetFocusIndex();

	virtual void		PrevPage();
	virtual void		NextPage();

	// HACKHACK to update things like the reticle with the offset
	void				OnVanishingPointYChanged();

	Map<HashedString, UIWidget*>	m_Widgets;			// All the widgets, accessible by name
	Array<UIWidget*>				m_RenderWidgets;	// All the widgets, sorted in render order
	Array<UIWidget*>				m_FocusWidgets;		// Just the usable widgets, sorted in cycling order
	UIWidget*						m_FocusedWidget;

	SimpleString		m_Name;

	bool				m_TakesFocus;		// Config; Receive input on this screen when it is active
	bool				m_PausesGame;		// Config; Pause the game when this screen is active
	bool				m_SuppressesInput;	// Config; Push/pop a null input context when this screen is active
	bool				m_AllowClose;		// Config; Allow the user to close this screen with the usual input
	bool				m_SeizesMouse;		// Config; Show the mouse cursor when this screen is active
	bool				m_HasSeizedMouse;	// Transient
	bool				m_GameTick;			// Use the game sim delta time (e.g. 0.0 when the game is paused)

	int					m_FocusXShift;
	int					m_FocusYShift;

	// Cache the mouse position so we only consider it when it moves
	int					m_LastMousePosX;
	int					m_LastMousePosY;
	UIWidget*			m_ClickedWidget;	// The widget that was clicked on, to check that the release is on the same widget

	SimpleString		m_PushedSound;		// Sound definition that is played when this screen is pushed
	SimpleString		m_PoppedSound;		// Sound definition that is played when this screen is popped

	bool				m_OnStack;
	bool				m_Hidden;			// Programmatically hide screens on the stack; hidden screens are neither drawn nor ticked

	float				m_PushedTime;		// When this screen was pushed onto stack; used to prevent closing screens for a short time
	float				m_PopTimeout;		// Prevent closing the screen for this duration

	Array<WBAction*>	m_PushedActions;	// Actions that are executed when this widget is pushed
	Array<WBAction*>	m_PoppedActions;	// Actions that are executed whenever this widget is popped (compare to m_ClosedActions)

	Array<WBAction*>	m_ClosedActions;	// Actions that are executed only when this widget is manually closed (compare to m_PoppedActions)

	static void			SetUIManager( UIManager* pManager )	{ m_UIManager = pManager; }
	static UIManager*	GetUIManager()						{ return m_UIManager; }
	static UIManager*	m_UIManager;

	static void			UpdateMouseButtonsSwapped();
	static bool			m_MouseButtonsSwapped;
};

#endif // UISCREEN_H
