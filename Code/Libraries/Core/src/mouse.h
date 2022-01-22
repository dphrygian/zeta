#ifndef MOUSE_H
#define MOUSE_H

#include "icontroller.h"

#if BUILD_WINDOWS_NO_SDL
#ifndef DIRECTINPUT_VERSION
	#define DIRECTINPUT_VERSION 0x0800
#endif

#include <Windows.h>
#include <dinput.h>
#endif

class Window;

class Mouse : public IController
{
public:
	enum EAxes
	{
		EA_None,
		EA_X,
		EA_Y,
		EA_Max,
	};

	enum EButtons
	{
		EB_None,
		EB_Left,
		EB_Middle,
		EB_Right,
		EB_X1,
		EB_X2,
		EB_WheelUp,
		EB_WheelDown,
		EB_Max,
	};

	struct SMouseState
	{
		int		m_Axes[ EA_Max ];
		bool	m_Buttons[ EB_Max ];
	};

#if BUILD_WINDOWS_NO_SDL
	Mouse( HINSTANCE hInstance, HWND hWnd );
#endif
#if BUILD_SDL
	Mouse( Window* const pWindow );
#endif
	virtual ~Mouse();

	virtual void	Tick( const float DeltaTime );

	virtual bool	IsHigh( uint Signal );
	virtual bool	IsLow( uint Signal );
	virtual bool	OnRise( uint Signal );
	virtual bool	OnFall( uint Signal );

	// Not normalized to (-1,1) right now... maybe divide by screen size?
	virtual float	GetPosition( uint Axis );
	virtual float	GetVelocity( uint Axis );

	void	SetPosition( int X, int Y, Window* const pWindow );
	void	GetPosition( int& X, int& Y, Window* const pWindow );

	void	SetActive( bool Active );
	bool	IsActive() const;

#if BUILD_WINDOWS_NO_SDL
	// HACKHACK: Now that I'm supporting DI controllers, reuse the mouse for that.
	static LPDIRECTINPUT8	GetDirectInput() { return sm_pDirectInput; }
#endif

#if BUILD_SDL
	void	SetRelativeMode( const bool Relative );
#endif

	void	ShowCursor( const bool Show );

	// Intended for handling wheel from Windows messages
	void	Buffer( uint Signal );

	void	NotifyMouseMoved( const uint X, const uint Y );
	bool	ReceivedInputThisTick() const { return m_ReceivedInputThisTick; }
	void	SetAllowCursor( const bool AllowCursor );

	SMouseState	m_CurrentState;
	SMouseState	m_LastState;
	SMouseState	m_BufferedState;

#if BUILD_WINDOWS_NO_SDL
	static LPDIRECTINPUT8	sm_pDirectInput;
	LPDIRECTINPUTDEVICE8	m_pMouse;
#endif
#if BUILD_SDL
	Window*					m_Window;
	// HACKHACK: To compensate for moving cursor in non-relative mode
	int						m_AdjustmentX;
	int						m_AdjustmentY;
	bool					m_RelativeModeActive;
	bool					m_RelativeModeSupported;
	bool					m_IgnoreNextDeltas;
#endif
	bool					m_Active;

	// Used in conjunction with controller (if applicable) to determine if cursor should be visible.
	bool					m_NotifiedMouseMoved;
	bool					m_ReceivedInputThisTick;
	bool					m_AllowCursor;
	bool					m_CursorShown;	// In in exclusive mode, reflects whether the cursor *would* be shown.

	// Cache these to prevent incorrect mouse move notifications from messing with input display
	uint					m_LastMouseMoveX;
	uint					m_LastMouseMoveY;
};

#endif // MOUSE_H
