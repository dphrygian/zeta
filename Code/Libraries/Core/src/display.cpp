#include "core.h"
#include "display.h"
#include "configmanager.h"

#if BUILD_WINDOWS_NO_SDL
#include <Windows.h>
#endif

#if BUILD_SDL
#include "SDL2/SDL.h"
#endif

Display::Display()
:	m_Width( 0 )
,	m_Height( 0 )
,	m_FrameWidth( 0 )
,	m_FrameHeight( 0 )
,	m_PropWidth( 0 )
,	m_PropHeight( 0 )
,	m_ScreenWidth( 0 )
,	m_ScreenHeight( 0 )
,	m_AspectRatio( 0.0f )
,	m_Fullscreen( false )
,	m_UpscaleFullscreen( false )
,	m_VSync( false )
,	m_DisplayModes()
{
	// Get the stored resolution
#if BUILD_WINDOWS_NO_SDL
	DEVMODE DevMode;
	XTRACE_BEGIN( EnumDisplaySettings );
		EnumDisplaySettings( NULL, ENUM_REGISTRY_SETTINGS, &DevMode );
	XTRACE_END;
	m_ScreenWidth	= DevMode.dmPelsWidth;
	m_ScreenHeight	= DevMode.dmPelsHeight;
#endif
#if BUILD_SDL
	SDL_DisplayMode DisplayMode;
	SDL_GetDesktopDisplayMode( 0, &DisplayMode );
	m_ScreenWidth	= DisplayMode.w;
	m_ScreenHeight	= DisplayMode.h;
#endif

	ASSERT( m_ScreenWidth > 0 );
	ASSERT( m_ScreenHeight > 0 );

	// DLP 2 Jan 2020: Default to the screen dimensions if nothing is specified
	STATICHASH( DisplayWidth );
	m_Width = ConfigManager::GetInt( sDisplayWidth, m_ScreenWidth );
	ASSERT( m_Width > 0 );

	STATICHASH( DisplayHeight );
	m_Height = ConfigManager::GetInt( sDisplayHeight, m_ScreenHeight );
	ASSERT( m_Height > 0 );

	m_AspectRatio = static_cast<float>( m_Width ) / static_cast<float>( m_Height );

	STATICHASH( Fullscreen );
	m_Fullscreen = ConfigManager::GetBool( sFullscreen );

#if BUILD_LINUX
	// On Linux, changing the screen resolution isn't really supported, so always upscale.
	m_UpscaleFullscreen = true;
#else
	STATICHASH( UpscaleFullscreen );
	m_UpscaleFullscreen = ConfigManager::GetBool( sUpscaleFullscreen );
#endif

	STATICHASH( VSync );
	m_VSync = ConfigManager::GetBool( sVSync );

	PRINTF( "Desired display mode: %dx%d", m_Width, m_Height );
	PRINTF( m_Fullscreen		? " / fullscreen" : " / windowed" );
	PRINTF( m_UpscaleFullscreen	? " / upscaling" : " / no upscaling" );
	PRINTF( m_VSync				? " / vsync\n" : " / no vsync\n" );

	RefreshFrameDimensions();
	UpdateDisplay();
}

Display::~Display()
{
	if( m_Fullscreen )
	{
		SetDisplay( true );
	}

	m_DisplayModes.Clear();
}

void Display::SetFullscreen( bool Fullscreen )
{
	m_Fullscreen = Fullscreen;

	// Keep config var in sync with display
	STATICHASH( Fullscreen );
	ConfigManager::SetBool( sFullscreen, Fullscreen );

	RefreshFrameDimensions();
}

void Display::SetVSync( const bool VSync )
{
	m_VSync = VSync;

	// Keep config var in sync with display
	STATICHASH( VSync );
	ConfigManager::SetBool( sVSync, VSync );
}

void Display::SetResolution( uint Width, uint Height )
{
	m_Width = Width;
	m_Height = Height;
	m_AspectRatio = static_cast<float>( m_Width ) / static_cast<float>( m_Height );

	// Keep config vars in sync with display
	STATICHASH( DisplayWidth );
	STATICHASH( DisplayHeight );
	ConfigManager::SetInt( sDisplayWidth, m_Width );
	ConfigManager::SetInt( sDisplayHeight, m_Height );

	RefreshFrameDimensions();
}

void Display::RefreshFrameDimensions()
{
	if( m_Fullscreen && m_UpscaleFullscreen )
	{
		m_FrameWidth	= m_ScreenWidth;
		m_FrameHeight	= m_ScreenHeight;
	}
	else
	{
		m_FrameWidth	= m_Width;
		m_FrameHeight	= m_Height;
	}

	// Cache proportional frame dimensions
	const uint	PropWidth		= ( m_FrameHeight * m_Width ) / m_Height;
	const uint	PropHeight		= ( m_FrameWidth * m_Height ) / m_Width;
	const float	fDisplayWidth	= static_cast<float>( m_Width );
	const float	fDisplayHeight	= static_cast<float>( m_Height );
	const float	fFrameWidth		= static_cast<float>( m_FrameWidth );
	const float	fFrameHeight	= static_cast<float>( m_FrameHeight );
	const float	DisplayAspect	= fDisplayWidth / fDisplayHeight;
	const float	FrameAspect		= fFrameWidth / fFrameHeight;
	const bool	WideFrame		= FrameAspect >= DisplayAspect;
	m_PropWidth					= WideFrame ? PropWidth : m_FrameWidth;
	m_PropHeight				= WideFrame ? m_FrameHeight : PropHeight;
}

bool Display::ShouldUpscaleFullscreen() const
{
	if( m_Width != m_FrameWidth || m_Height != m_FrameHeight )
	{
		DEVASSERT( m_Fullscreen && m_UpscaleFullscreen );
		return true;
	}
	else
	{
		return false;
	}
}

void Display::UpdateDisplay()
{
	// DLP 24 Sep 2020: If we're doing upscale fullscreen, the display mode should never change, right?
	// This also fixes a very weird bug where ChangeDisplaySettings was causing the main window border
	// to change visual style.
	if( m_UpscaleFullscreen )
	{
		return;
	}

	int CurrentWidth = 0;
	int CurrentHeight = 0;
	int CurrentRefreshRate = 0;
	GetCurrentDisplayMode( CurrentWidth, CurrentHeight, CurrentRefreshRate );

	STATICHASH( DisplayDepth );
	const uint DisplayDepth = ConfigManager::GetInt( sDisplayDepth, 32 );

	// Used to default to 60Hz; now defaults to whatever the display is already set to
	STATICHASH( DisplayRate );
	const uint DisplayRate = ConfigManager::GetInt( sDisplayRate, CurrentRefreshRate );

	if( m_Fullscreen )
	{
		SetDisplay( false, m_Fullscreen, m_FrameWidth, m_FrameHeight, DisplayDepth, DisplayRate );
	}
	else
	{
		SetDisplay( true );
	}
}

bool Display::NeedsUpdate()
{
#if BUILD_WINDOWS_NO_SDL
	DEVMODE DevMode;
	ZeroMemory( &DevMode, sizeof( DEVMODE ) );

	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &DevMode );

	return m_Fullscreen && ( DevMode.dmPelsWidth != m_FrameWidth || DevMode.dmPelsHeight != m_FrameHeight );
#endif
#if BUILD_SDL
	SDL_DisplayMode DisplayMode;
	SDL_GetCurrentDisplayMode( 0, &DisplayMode );

	return m_Fullscreen && ( DisplayMode.w != static_cast<int>( m_FrameWidth ) || DisplayMode.h != static_cast<int>( m_FrameHeight ) );
#endif
}

void Display::GetCurrentDisplayMode( int& Width, int& Height, int& RefreshRate )
{
#if BUILD_WINDOWS_NO_SDL
	DEVMODE DevMode;
	ZeroMemory( &DevMode, sizeof( DEVMODE ) );

	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &DevMode );

	Width = DevMode.dmPelsWidth;
	Height = DevMode.dmPelsHeight;
	RefreshRate = DevMode.dmDisplayFrequency;
#elif BUILD_SDL
	SDL_DisplayMode DisplayMode;
	SDL_GetCurrentDisplayMode( 0, &DisplayMode );

	Width = DisplayMode.w;
	Height = DisplayMode.h;
	RefreshRate = DisplayMode.refresh_rate;
#else
#error Unknown display system
#endif
}

// TODO: Check return value of ChangeDisplaySettings and react appropriately
/*static*/ void Display::SetDisplay( bool Reset /*= true*/, bool Fullscreen /*= false*/, int Width /*= 0*/, int Height /*= 0*/, int BitDepth /*= 0*/, int Frequency /*= 0*/ )
{
	XTRACE_FUNCTION;

#if BUILD_SDL
	// SDL manages display through its window system.
	Unused( Reset );
	Unused( Fullscreen );
	Unused( Width );
	Unused( Height );
	Unused( BitDepth );
	Unused( Frequency );
	return;
#endif

#if BUILD_WINDOWS_NO_SDL
	if( Reset )
	{
		PRINTF( "Resetting display\n" );

		ChangeDisplaySettings( NULL, 0 );
	}
	else
	{
		PRINTF( "Setting display: %dx%d %dbpp %dHz", Width, Height, BitDepth, Frequency );
		PRINTF( Fullscreen ? " fullscreen\n" : " windowed\n" );

		DEVMODE DevMode;
		ZeroMemory( &DevMode, sizeof( DEVMODE ) );

		DevMode.dmSize = sizeof( DEVMODE );
		DevMode.dmPelsWidth = Width;
		DevMode.dmPelsHeight = Height;
		DevMode.dmBitsPerPel = BitDepth;
		DevMode.dmDisplayFrequency = Frequency;
		DevMode.dmFields = 0;
		DevMode.dmFields |= ( Width > 0 ) ? DM_PELSWIDTH : 0;
		DevMode.dmFields |= ( Height > 0 ) ? DM_PELSHEIGHT : 0;
		DevMode.dmFields |= ( BitDepth > 0 ) ? DM_BITSPERPEL : 0;
		DevMode.dmFields |= ( Frequency > 0 ) ? DM_DISPLAYFREQUENCY : 0;

		DWORD Flags = ( Fullscreen ) ? CDS_FULLSCREEN : 0;
		if( DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettings( &DevMode, Flags | CDS_TEST ) )
		{
			ChangeDisplaySettings( &DevMode, Flags );
		}
	}
#endif
}

/*static*/ void Display::EnumerateDisplayModes( Array<SDisplayMode>& DisplayModes )
{
	DisplayModes.Clear();

#if BUILD_WINDOWS_NO_SDL
	PRINTF( "Enumerating Windows display modes...\n" );
	DISPLAY_DEVICE	DisplayDevice;
	DEVMODE			DevMode;
	for( uint DisplayIndex = 0; ; ++DisplayIndex )
	{
		DisplayDevice.cb		= sizeof( DISPLAY_DEVICE );
		const BOOL DisplayValid	= EnumDisplayDevices( NULL, DisplayIndex, &DisplayDevice, 0 /*flags*/ );
		if( !DisplayValid )
		{
			break;
		}

		if( 0 == ( DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) )
		{
			continue;
		}

		for( uint ModeIndex = 0; ; ++ModeIndex )
		{
			DevMode.dmSize			= sizeof( DEVMODE );
			const BOOL ModeValid	= EnumDisplaySettings( DisplayDevice.DeviceName, ModeIndex, &DevMode );
			if( !ModeValid )
			{
				break;
			}

			// Some users have problems if their refresh rate is set to 59 Hz. Maybe I should reconsider this?
			if( DevMode.dmBitsPerPel == 32 )
			{
				SDisplayMode Mode;
				Mode.Width	= DevMode.dmPelsWidth;
				Mode.Height	= DevMode.dmPelsHeight;

				if( DisplayModes.PushBackUnique( Mode ) )
				{
					PRINTF( "\tEnumerated mode %dx%d\n", Mode.Width, Mode.Height );
				}
			}
		}
	}
#endif
#if BUILD_SDL
	PRINTF( "Enumerating SDL display modes...\n" );
	const int NumDisplays		= SDL_GetNumVideoDisplays();
	for( int DisplayIndex = 0; DisplayIndex < NumDisplays; ++DisplayIndex )
	{
		const int NumModes = SDL_GetNumDisplayModes( DisplayIndex );
		for( int ModeIndex = 0; ModeIndex < NumModes; ++ModeIndex )
		{
			SDL_DisplayMode DisplayMode;
			SDL_GetDisplayMode( DisplayIndex, ModeIndex, &DisplayMode );

			if( SDL_BYTESPERPIXEL( DisplayMode.format ) == 4 )
			{
				SDisplayMode Mode;
				Mode.Width	= DisplayMode.w;
				Mode.Height	= DisplayMode.h;

				if( DisplayModes.PushBackUnique( Mode ) )
				{
					PRINTF( "Enumerated mode %dx%d\n", Mode.Width, Mode.Height );
				}
			}
		}
	}
#endif

	ASSERT( DisplayModes.Size() );
}

/*static*/ SDisplayMode Display::GetBestDisplayMode( const uint DesiredWidth, const uint DesiredHeight )
{
	STATICHASH( IgnoreDisplayEnum );
	const bool IgnoreDisplayEnum = ConfigManager::GetBool( sIgnoreDisplayEnum );
	if( IgnoreDisplayEnum )
	{
		PRINTF( "IgnoreDisplayEnum is true, using desired display mode %dx%d\n", DesiredWidth, DesiredHeight );
		SDisplayMode DesiredDisplayMode;
		DesiredDisplayMode.Width	= DesiredWidth;
		DesiredDisplayMode.Height	= DesiredHeight;
		return DesiredDisplayMode;
	}

	Array<SDisplayMode> DisplayModes;
	EnumerateDisplayModes( DisplayModes );

	SDisplayMode BestDisplayMode;
	FOR_EACH_ARRAY( ModeIter, DisplayModes, SDisplayMode )
	{
		const SDisplayMode& Mode = ModeIter.GetValue();
		if( Mode.Width	>= BestDisplayMode.Width &&
			Mode.Height	>= BestDisplayMode.Height &&
			Mode.Width	<= DesiredWidth &&
			Mode.Height	<= DesiredHeight )
		{
			BestDisplayMode = Mode;
		}
	}

	// This case can occur if our desired dimensions are *smaller* than any available display mode.
	if( BestDisplayMode.Width == 0 ||
		BestDisplayMode.Height == 0 )
	{
		PRINTF( "Could not find a more suitable display mode.\n" );
		BestDisplayMode.Width   = DesiredWidth;
		BestDisplayMode.Height  = DesiredHeight;
	}

	PRINTF( "Using display mode %dx%d\n", BestDisplayMode.Width, BestDisplayMode.Height );
	return BestDisplayMode;
}
