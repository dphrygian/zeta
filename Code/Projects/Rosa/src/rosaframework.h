#ifndef ROSAFRAMEWORK_H
#define ROSAFRAMEWORK_H

#include "framework3d.h"
#include "rosagame.h"

#if BUILD_STEAM
#include "steam/steam_api.h"
#endif

class RosaGame;
class RosaWorld;
class RosaIntro;
class WBEventManager;
class View;
class InputSystem;
class Vector;
class Angles;
class RosaTools;
class IDataStream;
class RosaSound3DListener;
class Mesh;
class RosaTargetManager;
class RosaCloudManager;
class XInputController;
class IAchievementManager;

class RosaFramework : public Framework3D
{
public:
	RosaFramework();
	virtual ~RosaFramework();

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

	RosaWorld*				GetWorld() const				{ return m_World; }
	RosaGame*				GetGame() const					{ return m_Game; }
	RosaTargetManager*		GetTargetManager() const		{ return m_TargetManager; }
	RosaCloudManager*		GetCloudManager() const			{ return m_CloudManager; }
	XInputController*		GetController() const			{ return m_Controller; }
	IAchievementManager*	GetAchievementManager() const	{ return m_AchievementManager; }

#if BUILD_DEV
	RosaTools*				GetTools() const				{ return m_Tools; }
#endif

	virtual InputSystem*	GetInputSystem() const			{ return m_InputSystem; }

	virtual SimpleString	GetUserDataPath();
	virtual SimpleString	GetSaveLoadPath();

	View*			GetMainView() const				{ return m_MainView; }
	View*			GetFGView() const				{ return m_FGView; }
	void			SetMainViewTransform( const Vector& Location, const Angles& Orientation );

	void			SetMinimapViewExtent( const float MinimapViewExtent );
	float			GetMinimapViewExtent() const	{ return m_MinimapViewExtent; }
	View*			GetMinimapView() const			{ return m_MinimapAView; }

#if ROSA_USE_MAXIMAP
	void			SetMaximapViewExtent( const float MaximapViewExtent );
	float			GetMaximapViewExtent() const	{ return m_MaximapViewExtent; }
	View*			GetMaximapView() const			{ return m_MaximapAView; }
#endif

	// This strictly sets the FOV for the views; it does not set the config var or publish the FOV.
	void			SetFOV( const float FOV );
	void			SetFGFOV( const float FGFOV );
	void			SetVanishingPointY( const float VanishingPointY );

	void			RegenerateWorld();
	void			GoToLevel( const SimpleString& WorldDef );

	void			PrepareForLoad( const bool ResetToGameScreens );
	void			InitializeTools();

	// Singleton accessor
	static RosaFramework*	GetInstance();
	static void				SetInstance( RosaFramework* const pFramework );

	void			RequestRenderTick() { m_SimTickHasRequestedRenderTick = true; }

protected:
	virtual void	Initialize();
	virtual void	ShutDown();

	void			InitializePackagesAndConfig();	// Can be called again to reinit and hotload
	void			InitializePackages();
	void			InitializeMods();

	void			InitializeWorld( const SimpleString& WorldDef, const bool CreateWorld );
	void			ShutDownWorld();

	void			LoadPrefsConfig();
	void			WritePrefsConfig();

	void			PushDefaultOptions();

	void			RegisterForEvents();

	void			HandleUISliderEvent( const HashedString& SliderName, const float SliderValue );

	bool			CanPause() const;
	void			TryPause();

	void			TryFinishInputBinding();

	float			GetMouseSpeedFromSliderValue( const float SliderValue );
	float			GetSliderValueFromMouseSpeed( const float MouseSpeed );
	float			GetControllerSpeedFromSliderValue( const float SliderValue );
	float			GetSliderValueFromControllerSpeed( const float ControllerSpeed );
	float			GetBrightnessFromSliderValue( const float SliderValue );
	float			GetSliderValueFromBrightness( const float MouseSpeed );
	float			GetFOVFromSliderValue( const float SliderValue );
	float			GetSliderValueFromFOV( const float FOV );
	float			GetVanishingPointYFromSliderValue( const float SliderValue );
	float			GetSliderValueFromVanishingPointY( const float VanishingPointY );
	float			GetLightDistanceFromSliderValue( const float SliderValue );
	float			GetSliderValueFromLightDistance( const float LightDistance );

	virtual bool	TickSim( const float DeltaTime );
	virtual bool	TickGame( const float DeltaTime );
	virtual bool	TickPaused( const float DeltaTime );
	virtual void	OnUnpaused();
	virtual void	TickDevices();
	virtual bool	TickInput( const float DeltaTime, const bool UIHasFocus );
	virtual void	TickPausedInput( const float DeltaTime );
	virtual void	TickRender();

	virtual bool	SimTickHasRequestedRenderTick() const { return m_SimTickHasRequestedRenderTick; }

	virtual void	GetInitialWindowTitle( SimpleString& WindowTitle );
	virtual void	GetInitialWindowIcon( uint& WindowIcon );
	virtual void	GetUIManagerDefinitionName( SimpleString& DefinitionName );
	virtual void	InitializeUIInputMap();
	virtual bool	ShowWindowASAP() { return false; }
	virtual void	InitializeAudioSystem();

	void			CreateBuckets();
	void			UpdateViews();
	void			CreateHUDView();
	void			CreateMinimapViews();
	virtual void	ToggleFullscreen();
	virtual void	ToggleVSync();
	void			ToggleFXAA();
	void			ToggleSSAO();
	void			ToggleBloom();
	virtual void	SetResolution( const uint DisplayWidth, const uint DisplayHeight );
	virtual void	RefreshDisplay( const bool Fullscreen, const bool VSync, const uint DisplayWidth, const uint DisplayHeight );

	void			PublishDisplayedBrightness() const;
	void			PublishDisplayedFOV() const;
	void			PublishDisplayedVanishingPointY() const;

	void			OnShowHUDChanged();
	void			OnShowHUDMarkersChanged();
	void			OnShowMinimapChanged();
	void			OnShowMinimapMarkersChanged();
#if ROSA_USE_MAXIMAP
	void			OnShowMaximapChanged();
#endif

	void			OnInvertYChanged();
	void			OnAutoAimChanged();
	void			OnControllerTypeChanged();
	void			OnViewBobChanged();
	void			OnViewSwayChanged();
	void			OnSlideRollChanged();
	void			OnStrafeRollChanged();
	void			OnSprintFOVChanged();
	void			OnHandsVelocityChanged();
	void			OnLeftyModeChanged();
	void			OnStylizedAnimChanged();

	static void		OnSetRes( void* pUIElement, void* pVoid );

#if BUILD_STEAM
	bool			GetPublishedFileFolder( const PublishedFileId_t FileId, SimpleString& OutFolder );
#endif

private:
	static RosaFramework*	m_Instance;

	RosaGame*				m_Game;
	RosaWorld*				m_World;
	RosaIntro*				m_Intro;
#if BUILD_DEV
	RosaTools*				m_Tools;
#endif
	XInputController*		m_Controller;
	InputSystem*			m_InputSystem;

	uint					m_DisplayWidth;
	uint					m_DisplayHeight;

	RosaTargetManager*		m_TargetManager;
	RosaCloudManager*		m_CloudManager;

	View*					m_MainView;
	View*					m_FGView;
	View*					m_HUDView;
	View*					m_UpscaleView;
	View*					m_BloomViewA;
	View*					m_BloomViewB;
	View*					m_BloomViewC;
	float					m_MinimapViewExtent;
	float					m_MinimapViewOffset;
	float					m_MinimapViewHeight;	// Shared with maximap, no reason for them to differ
	View*					m_MinimapAView;			// The view of the minimap world
	View*					m_MinimapBView;			// The view of the minimap RT
#if ROSA_USE_MAXIMAP
	float					m_MaximapViewExtent;
	float					m_MaximapViewOffset;
	View*					m_MaximapAView;			// The view of the maximap world
	View*					m_MaximapBView;			// The view of the maximap RT
#endif
	View*					m_SkyView;
	View*					m_SkylineView;
	View*					m_LightView;

	RosaSound3DListener*	m_Audio3DListener;

	IAchievementManager*	m_AchievementManager;

	// HACKHACK to force loading screen to display
	bool					m_SimTickHasRequestedRenderTick;

	bool					m_PauseOnLostFocus;
	bool					m_MuteWhenUnfocused;

public:
#if BUILD_STEAM
	STEAM_CALLBACK( RosaFramework, OnItemInstalled, ItemInstalled_t, m_CallbackItemInstalled );
	STEAM_CALLBACK( RosaFramework, OnOverlayActivated, GameOverlayActivated_t, m_CallbackOverlayActivated );
#endif
};

#endif // ROSAFRAMEWORK_H
