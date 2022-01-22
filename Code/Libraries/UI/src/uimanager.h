#ifndef IUIMANAGER_H
#define IUIMANAGER_H

// Manages UIScreens for the lifetime of the game

#include "ui.h"
#include "array.h"
#include "simplestring.h"
#include "map.h"
#include "hashedstring.h"
#include "iwbeventobserver.h"

class UIScreen;
class UIStack;
class Vector4;

class UIManager : public IWBEventObserver
{
public:
	UIManager();
	virtual ~UIManager();

	virtual void		Initialize();
	void				Reinitialize();

	void				PostEvent( const HashedString& EventName );
	void				ProcessEvents();
	void				FlushEvents();

	UIScreen*				AddScreen( const HashedString& Name, UIScreen* pScreen );	// NOTE: The UIManager takes ownership of the memory of this UIScreen.
	UIScreen*				GetScreen( const HashedString& Name ) const;
	template<class C> C*	GetScreen( const HashedString& Name ) const { return static_cast<C*>( GetScreen( Name ) ); }
	UIStack*				GetUIStack() const { return m_UIStack; }

	// IWBEventObserver
	virtual void			HandleEvent( const WBEvent& Event );

	virtual void			RegisterForEvents();

	virtual class Window*		GetWindow() const = 0;
	virtual class InputSystem*	GetInputSystem() const = 0;
	virtual class Keyboard*		GetKeyboard() const = 0;
	virtual class Mouse*		GetMouse() const = 0;
	virtual class IUIInputMap*	GetUIInputMap() const = 0;
	virtual class Clock*		GetClock() const = 0;
	virtual class Display*		GetDisplay() const = 0;
	virtual class IRenderer*	GetRenderer() const = 0;
	virtual class IAudioSystem*	GetAudioSystem() const = 0;
	virtual class LineBatcher*	GetLineBatcher() const = 0;

	// Helper function
	void			SetSliderValue( const HashedString& Screen, const HashedString& Widget, const float Value ) const;
	void			SetSliderNotch( const HashedString& Screen, const HashedString& Widget, const float NotchValue ) const;

	// HACKHACK to update things like the reticle with the offset
	void			OnVanishingPointYChanged();

#if BUILD_DEBUG
	void			DEBUGSetIsSafeToUpdateRender( bool IsSafeToUpdateRender, debug_int64_t Time = 0 );
	bool			DEBUGIsSafeToUpdateRender( debug_int64_t LastRenderedTime ) const;
#endif

protected:
	UIStack*						m_UIStack;
	Map<HashedString, UIScreen*>	m_Screens;
	Array<HashedString>				m_Events;	// Ancient UI event system, only used for popping screens anymore

#if BUILD_DEBUG
	bool			m_DEBUGIsSafeToUpdateRender;
	debug_int64_t	m_DEBUGLastSafeToUpdateRenderTime;
#endif

	virtual bool	ProcessEventInternal( const HashedString& EventName );

private:
	void			ProcessEvent( const HashedString& EventName );
};

#endif // IUIMANAGER_H
