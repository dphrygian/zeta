#include "core.h"

#include "rosaframework.h"

#if BUILD_WINDOWS
#include "../resource.h"
#endif

#if BUILD_MAC
#include <CoreFoundation/CoreFoundation.h>
#include "objcjunk.h"
#endif

#if BUILD_LINUX
#include <stdlib.h>
#endif

#include "array.h"
#include "vector.h"
#include "view.h"
#include "bucket.h"
#include "angles.h"
#include "keyboard.h"
#include "mouse.h"
#include "xinputcontroller.h"
#include "rosagame.h"
#include "rosaworld.h"
#include "rosasaveload.h"
#include "rosapersistence.h"
#include "rosacampaign.h"
#include "rosaconversation.h"
#include "rosahudlog.h"
#include "rosaintro.h"
#include "material.h"
#include "wbevent.h"
#include "wbeventmanager.h"
#include "reversehash.h"
#include "clock.h"
#include "frameworkutil.h"
#include "packstream.h"
#include "configmanager.h"
#include "filestream.h"
#include "audio.h"
#include "iaudiosystem.h"
#include "rosamesh.h"
#include "dynamicmeshmanager.h"
#include "inputsystem.h"
#include "fileutil.h"
#include "windowwrapper.h"
#include "irenderer.h"
#include "irendertarget.h"
#include "rosasound3dlistener.h"
#include "rosaparticles.h"
#include "uiinputmaprosa.h"
#include "rosatargetmanager.h"
#include "rosacloudmanager.h"
#include "shadermanager.h"
#include "texturemanager.h"
#include "wbcomponentarrays.h"
#include "Achievements/achievementmanager_steam.h"
#include "rosadifficulty.h"
#include "rosalockpicking.h"
#include "stringmanager.h"
#include "animationstate.h"

#include "rosatools.h"

#include "sdpfactory.h"
#include "rosasdps.h"

#include "Common/uimanagercommon.h"
#include "uistack.h"

#include "uifactory.h"
#include "uiscreens.h"
#include "rosauiscreens.h"

#include "wbentity.h"
#include "wbcomponent.h"
#include "rodinwbcomponents.h"
#include "rosawbcomponents.h"

#include "wbparamevaluatorfactory.h"
#include "rodinwbpes.h"
#include "rosawbpes.h"

#include "wbactionfactory.h"
#include "uiwbactions.h"
#include "rodinwbactions.h"
#include "rosawbactions.h"

#include "rodinbtnodefactory.h"
#include "rosarodinbtnodes.h"

#include "animeventfactory.h"
#include "rosaanimevents.h"

#include "wbactionstack.h"

#if BUILD_WINDOWS_NO_SDL
#include "D3D9/d3d9renderer.h"
#endif

#if BUILD_STEAM
extern "C" void __cdecl SteamAPIDebugTextHook( int Severity, const char* DebugText )
{
	Unused( Severity );	// 0 = message, 1 = warning

	if( !DebugText )
	{
		return;
	}

	PRINTF( "[STEAM]: %s\n", DebugText );
}
#endif // BUILD_STEAM

// Singleton accessor
/*static*/ RosaFramework* RosaFramework::m_Instance = NULL;

/*static*/ RosaFramework* RosaFramework::GetInstance()
{
	return m_Instance;
}

/*static*/ void RosaFramework::SetInstance( RosaFramework* const pFramework )
{
	ASSERT( m_Instance == NULL );
	m_Instance = pFramework;
}

RosaFramework::RosaFramework()
:	m_Game( NULL )
,	m_World( NULL )
,	m_Intro( NULL )
#if BUILD_DEV
,	m_Tools( NULL )
#endif
,	m_Controller( NULL )
,	m_InputSystem( NULL )
,	m_DisplayWidth( 0 )
,	m_DisplayHeight( 0 )
,	m_TargetManager( NULL )
,	m_CloudManager( NULL )
,	m_MainView( NULL )
,	m_FGView( NULL )
,	m_HUDView( NULL )
,	m_UpscaleView( NULL )
,	m_BloomViewA( NULL )
,	m_BloomViewB( NULL )
,	m_BloomViewC( NULL )
,	m_MinimapViewExtent( 0.0f )
,	m_MinimapViewOffset( 0.0f )
,	m_MinimapViewHeight( 0.0f )
,	m_MinimapAView( NULL )
,	m_MinimapBView( NULL )
#if ROSA_USE_MAXIMAP
,	m_MaximapViewExtent( 0.0f )
,	m_MaximapViewOffset( 0.0f )
,	m_MaximapAView( NULL )
,	m_MaximapBView( NULL )
#endif
,	m_SkyView( NULL )
,	m_SkylineView( NULL )
,	m_LightView( NULL )
,	m_Audio3DListener( NULL )
,	m_AchievementManager( NULL )
,	m_SimTickHasRequestedRenderTick( false )
,	m_PauseOnLostFocus( false )
,	m_MuteWhenUnfocused( false )
#if BUILD_STEAM
,	m_CallbackItemInstalled( this, &RosaFramework::OnItemInstalled )
,	m_CallbackOverlayActivated( this, &RosaFramework::OnOverlayActivated )
#endif
{
	RosaFramework::SetInstance( this );
	Framework3D::SetInstance( this );
}

RosaFramework::~RosaFramework()
{
	ASSERT( m_Instance == this );
	m_Instance = NULL;
}

/*virtual*/ void RosaFramework::GetInitialWindowTitle( SimpleString& WindowTitle )
{
	STATICHASH( WindowTitle );
	WindowTitle = ConfigManager::GetLocalizedString( sWindowTitle, "Rosa" );

	STATICHASH( Version );
	WindowTitle += SimpleString::PrintF( " (%s)", StringManager::ParseConfigString( ConfigManager::GetLocalizedString( sVersion, "" ) ).CStr() );

#if BUILD_WINDOWS_NO_SDL
	// HACKHACK because renderer doesn't actually exist yet
	STATICHASH( OpenGL );
	const bool OpenGL = ConfigManager::GetBool( sOpenGL );
	WindowTitle += OpenGL ? " [OpenGL 2.1]" : " [Direct3D 9]";
#endif

#if BUILD_WINDOWS
	const bool Is32Bit = sizeof( void* ) == sizeof( int );
	WindowTitle += Is32Bit ? " [32-bit]" : " [64-bit]";
#endif

#if BUILD_DEV
	WindowTitle += " [BUILD_DEV]";
#endif
}

/*virtual*/ void RosaFramework::GetInitialWindowIcon( uint& WindowIcon )
{
#if BUILD_WINDOWS
	WindowIcon = IDI_ICON1;
#else
	Unused( WindowIcon );
#endif
}

/*virtual*/ void RosaFramework::GetUIManagerDefinitionName( SimpleString& DefinitionName )
{
	DefinitionName = "RosaUI";
}

/*virtual*/ void RosaFramework::InitializeUIInputMap()
{
	m_UIInputMap = new UIInputMapRosa( this );
}

void RosaFramework::InitializePackages()
{
	STATICHASH( NumPackages );
	const uint NumPackages = ConfigManager::GetInt( sNumPackages );
	for( uint PackageIndex = 0; PackageIndex < NumPackages; ++PackageIndex )
	{
		const SimpleString PackageName = ConfigManager::GetSequenceString( "Package%d", PackageIndex, "" );

		// Don't preempt the base package, there should be no conflicts.
		const bool PreemptExistingPackages = false;
		PackStream::StaticAddPackageFile( PackageName.CStr(), PreemptExistingPackages );
	}
}

#if BUILD_STEAM
bool RosaFramework::GetPublishedFileFolder( const PublishedFileId_t FileId, SimpleString& OutFolder )
{
	if( !SteamUGC() )
	{
		return false;
	}

	static const uint	sFolderNameSize	= 512;
	char				Folder[ sFolderNameSize ];
	uint64				SizeOnDisk		= 0;
	uint32				Timestamp		= 0;

	if( SteamUGC()->GetItemInstallInfo( FileId, &SizeOnDisk, Folder, sFolderNameSize, &Timestamp ) )
	{
		OutFolder = Folder;
		return true;
	}

	return false;
}

void RosaFramework::OnItemInstalled( ItemInstalled_t* pParam )
{
	ISteamUtils* const	pSteamUtils = SteamUtils();
	if( !pSteamUtils )
	{
		return;
	}

	// Ignore if the item isn't for this game
	const uint32		AppID		= pSteamUtils->GetAppID();
	if( AppID != pParam->m_unAppID )
	{
		return;
	}

	SimpleString Folder;
	if( !GetPublishedFileFolder( pParam->m_nPublishedFileId, Folder ) )
	{
		return;
	}

	Array<SimpleString> ItemConfigFiles;
	FileUtil::GetFilesInFolder( Folder, true, ".config", ItemConfigFiles );

	// Add the mod path to resource folders
	FileStream::StaticAddResPath( Folder );

	// Load config files for new item
	FOR_EACH_ARRAY( ConfigFileIter, ItemConfigFiles, SimpleString )
	{
		const SimpleString& ConfigFile = ConfigFileIter.GetValue();
		ConfigManager::Load( FileStream( ConfigFile.CStr(), FileStream::EFM_Read ) );
	}
}

void RosaFramework::OnOverlayActivated( GameOverlayActivated_t* pParam )
{
	STATIC_HASHED_STRING( SteamOverlayPauseScreen );

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	if( pParam->m_bActive )
	{
		WB_MAKE_EVENT( PushUIScreen, NULL );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sSteamOverlayPauseScreen );
		WB_DISPATCH_EVENT( pEventManager, PushUIScreen, NULL );
	}
	else
	{
		WB_MAKE_EVENT( RemoveUIScreen, NULL );
		WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sSteamOverlayPauseScreen );
		WB_DISPATCH_EVENT( pEventManager, RemoveUIScreen, NULL );
	}
}
#endif // BUILD_STEAM

void RosaFramework::InitializeMods()
{
	Array<SimpleString> ModConfigFiles;
	Array<SimpleString> ModPaths;
#if BUILD_WINDOWS || BUILD_LINUX
	FileUtil::GetFilesInFolder( "Mods/", true, ".config", ModConfigFiles );
	FileUtil::GetFoldersInFolder( "Mods/", false, ModPaths );
#elif BUILD_MAC
	CFBundleRef MainBundle  = CFBundleGetMainBundle();
	CFURLRef    BundleURL   = CFBundleCopyBundleURL( MainBundle );
	char        szBundlePath[ PATH_MAX ];
	CFURLGetFileSystemRepresentation( BundleURL, TRUE, reinterpret_cast<UInt8*>( szBundlePath ), PATH_MAX );
	CFRelease( BundleURL );

	// HACKHACK: I can't just get the bundle's path without the file in it,
	// so I'll just lop off the end manually. Garbage API is garbage.
	char* const szSubStr = strstr( szBundlePath, "/Zeta.app" );
	ASSERT( szSubStr );
	( *szSubStr ) = '\0';

	const SimpleString ModsPath = SimpleString::PrintF( "%s/Mods/", szBundlePath );
	PRINTF( ModsPath.CStr() );
	FileUtil::GetFilesInFolder( ModsPath, true, ".config", ModConfigFiles );
	FileUtil::GetFoldersInFolder( ModsPath, false, ModPaths );
#else
#error Need to port this!
#endif

#if BUILD_STEAM
	// Get paths to all currently installed mod folders
	if( SteamUGC() )
	{
		const uint32 NumSubscribedItems = SteamUGC()->GetNumSubscribedItems();
		Array<PublishedFileId_t> FileIds;
		FileIds.Resize( NumSubscribedItems );
		SteamUGC()->GetSubscribedItems( FileIds.GetData(), NumSubscribedItems );

		FOR_EACH_ARRAY( FileIdIter, FileIds, PublishedFileId_t )
		{
			const PublishedFileId_t FileId = FileIdIter.GetValue();

			SimpleString Folder;
			if( !GetPublishedFileFolder( FileId, Folder ) )
			{
				continue;
			}

			ModPaths.PushBack( Folder );
			FileUtil::GetFilesInFolder( Folder, true, ".config", ModConfigFiles );
		}
	}
#endif

	// Add mod folders as FileStream resource paths
	FOR_EACH_ARRAY( ModPathIter, ModPaths, SimpleString )
	{
		const SimpleString& ModPath = ModPathIter.GetValue();
		FileStream::StaticAddResPath( ModPath );
	}

	// Load config files for all mods
	FOR_EACH_ARRAY( ConfigFileIter, ModConfigFiles, SimpleString )
	{
		const SimpleString& ConfigFile = ConfigFileIter.GetValue();
		ConfigManager::Load( FileStream( ConfigFile.CStr(), FileStream::EFM_Read ) );
	}
}

void RosaFramework::InitializePackagesAndConfig()
{
	// HACKHACK: Hard-coded base package file!
	PackStream::StaticAddPackageFile( "zeta-base.cpk" );
	FrameworkUtil::MinimalLoadConfigFiles( "Config/default.ccf" );

	// HACKHACK: Initialize Steam as soon as possible after loading default config.
	// This way, we can query SteamUGC while loading mods.
#if BUILD_STEAM
	STATICHASH( LoadSteam );
	const bool LoadSteam = ConfigManager::GetBool( sLoadSteam );
	if( LoadSteam )
	{
		PRINTF( "Initializing Steam API.\n" );
		const bool SteamInited = SteamAPI_Init();
		if( SteamInited )
		{
			PRINTF( "Steam API initialized successfully.\n" );
		}
		else
		{
			PRINTF( "Steam API failed to initialize. Steamworks functionality will be disabled.\n" );
		}

		if( SteamClient() )
		{
			SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );
		}

		if( SteamUtils() )
		{
			SteamUtils()->SetOverlayNotificationPosition( k_EPositionBottomRight );
		}
	}
	else
	{
		PRINTF( "Running Steam build without Steam API. Is this intended?\n" );
	}
#endif

	// This loads the base packages along with any Steam DLC (which will be
	// installed in the same location, and should always be included in the
	// packages list; it will be gracefully ignored if not present).
	InitializePackages();

	// This loads anything in the user's Mods folder, as well as any Steam UGC.
	// Mods are expected to be declared in raw .config files, and the game must
	// know how to utilize them. (Steam mods can also be installed during play,
	// and will arrive via via RosaFramework::OnItemInstalled.)
	InitializeMods();

	// Load prefs over everything else.
	LoadPrefsConfig();

	// Push defaults to uninitialized options after loading prefs.
	PushDefaultOptions();
}

void RosaFramework::PushDefaultOptions()
{
	// Anything that is undefined due to a missing prefs.cfg will be initialized from the default contexts
	// Note that the input device sections are ignore if context exists; this is so that an unbound input
	// does not get stomped by the default (which could cause inputs to be double-bound, because I'm not
	// safety checking anything like that). The downside to that is that it means I have to put *all*
	// inputs into prefs.cfg, even ones which are not rebindable (or intentionally hidden, like Quicksave/
	// Quickload in Zeta).
	// DLP 21 May 2020: This was also causing problems with controller config stuff; in that case, I can
	// just put it into the [RosaController] context in input.loom, because nothing in the game can ever
	// change it. (I can't do that for non-bindable inputs, because they still won't save properly and the
	// user might again end up with double-bound keys.)
	//																IgnoreIfContextExists	ResetTargetContext	Overwrite	Target
	STATICHASH( Defaults_Game );
	ConfigManager::PushDefaultsToContext( sDefaults_Game,			false,					false,				false );

	STATICHASH( Defaults_Framework );
	STATICHASH( Framework );
	ConfigManager::PushDefaultsToContext( sDefaults_Framework,		false,					false,				false,		sFramework );

	STATICHASH( Defaults_Controls );
	ConfigManager::PushDefaultsToContext( sDefaults_Controls,		false,					false,				false );

	STATICHASH( Defaults_RosaInput );
	STATICHASH( RosaInput );
	ConfigManager::PushDefaultsToContext( sDefaults_RosaInput,		false,					false,				false,		sRosaInput );

	STATICHASH( Defaults_RosaKeyboard );
	STATICHASH( RosaKeyboard );
	ConfigManager::PushDefaultsToContext( sDefaults_RosaKeyboard,	true,					false,				false,		sRosaKeyboard );

	STATICHASH( Defaults_RosaMouse );
	STATICHASH( RosaMouse );
	ConfigManager::PushDefaultsToContext( sDefaults_RosaMouse,		true,					false,				false,		sRosaMouse );

	STATICHASH( Defaults_RosaController );
	STATICHASH( RosaController );
	ConfigManager::PushDefaultsToContext( sDefaults_RosaController,	true,					false,				false,		sRosaController );

	// RosaController has immutable defaults that must be applied even if the context exists;
	// these used to be in a default [RosaController] context but that made the above fail, oops.
	STATICHASH( Consts_RosaController );
	ConfigManager::PushDefaultsToContext( sConsts_RosaController,	false,					false,				false,		sRosaController );

	STATICHASH( Defaults_Display );
	ConfigManager::PushDefaultsToContext( sDefaults_Display,		false,					false,				false );

	STATICHASH( Defaults_Graphics );
	ConfigManager::PushDefaultsToContext( sDefaults_Graphics,		false,					false,				false );

	STATICHASH( Defaults_Audio );
	ConfigManager::PushDefaultsToContext( sDefaults_Audio,			false,					false,				false );
}

#if BUILD_STEAM
ENotificationPosition GetSteamNoticePos( const HashedString& SteamNoticePosName )
{
#define TRYPOS( pos ) STATIC_HASHED_STRING( pos ); if( SteamNoticePosName == s##pos ) { return k_EPosition##pos; }
	TRYPOS( TopLeft );
	TRYPOS( TopRight );
	TRYPOS( BottomLeft );
	TRYPOS( BottomRight );
#undef TRYPOS

	return k_EPositionTopRight;
}
#endif

/*virtual*/ void RosaFramework::Initialize()
{
	XTRACE_FUNCTION;

	XTRACE_BEGIN( PreFramework3D );
		ReverseHash::Initialize();

		PRINTF( "Checking user data path...\n" );
		const SimpleString UserDataPath = GetUserDataPath();
		PRINTF( "  %s\n", UserDataPath.CStr() );
		if( !FileUtil::PathExists( UserDataPath.CStr() ) )
		{
			PRINTF( "Creating user data game path...\n" );
			FileUtil::MakePath( UserDataPath.CStr() );

			if( !FileUtil::PathExists( UserDataPath.CStr() ) )
			{
				PRINTF( "Could not create user data game path!\n" );
			}
			else
			{
				PRINTF( "  ...done.\n" );
			}
		}

		InitializePackagesAndConfig();

		LOADPRINTLEVELS;

		// Load save index file if it exists
		const SimpleString SaveIndexFile = RosaSaveLoad::GetSaveIndexFile();
		if( FileUtil::Exists( SaveIndexFile.CStr() ) )
		{
			ConfigManager::Load( FileStream( SaveIndexFile.CStr(), FileStream::EFM_Read ) );
		}

		STATICHASH( Version );
		STATICHASH( BuildNumber );
		SimpleString LocalVersion = ConfigManager::GetString( sBuildNumber, "", sVersion );
		PRINTF( "Version: %s\n", LocalVersion.CStr() );

		XTRACE_BEGIN( InitializeFactories );
			PRINTF( "Initializing factories...\n" );

			PRINTF( "Initializing SDP factories.\n" );
			SDPFactory::InitializeBaseFactories();
#define ADDSDPFACTORY( type ) SDPFactory::RegisterSDPFactory( #type, SDP##type::Factory )
#include "rosasdps.h"
#undef ADDSDPFACTORY

			PRINTF( "Initializing UI factories.\n" );
			UIFactory::InitializeBaseFactories();
#define ADDUISCREENFACTORY( type ) UIFactory::RegisterUIScreenFactory( #type, UIScreen##type::Factory )
#include "rosauiscreens.h"
#undef ADDUISCREENFACTORY

			PRINTF( "Initializing anim event factories.\n" );
#define ADDANIMEVENTFACTORY( type ) AnimEventFactory::GetInstance()->Register( #type, AnimEvent##type::Factory )
#include "rosaanimevents.h"
#undef ADDANIMEVENTFACTORY

			PRINTF( "Initializing PE factories.\n" );
			WBParamEvaluatorFactory::InitializeBaseFactories();
#define ADDWBPEFACTORY( type ) WBParamEvaluatorFactory::RegisterFactory( #type, WBPE##type::Factory )
#include "rodinwbpes.h"
#include "rosawbpes.h"
#undef ADDWBPEFACTORY

			PRINTF( "Initializing action factories.\n" );
			WBActionFactory::InitializeBaseFactories();
#define ADDWBACTIONFACTORY( type ) WBActionFactory::RegisterFactory( #type, WBAction##type::Factory )
#include "uiwbactions.h"
#include "rodinwbactions.h"
#include "rosawbactions.h"
#undef ADDWBPEFACTORY

			PRINTF( "Initializing BT factories.\n" );
			RodinBTNodeFactory::InitializeBaseFactories();
#define ADDRODINBTNODEFACTORY( type ) RodinBTNodeFactory::RegisterFactory( #type, RodinBTNode##type::Factory )
#include "rosarodinbtnodes.h"
#undef ADDRODINBTNODEFACTORY

			// Initialize core and Rosa Workbench component factories.
			PRINTF( "Initializing component factories.\n" );
			WBComponent::InitializeBaseFactories();
#define ADDWBCOMPONENT( type ) WBComponent::RegisterWBCompFactory( #type, WBComp##type::Factory )
#include "rodinwbcomponents.h"
#include "rosawbcomponents.h"
#undef ADDWBCOMPONENT
		XTRACE_END;

		PRINTF( "Factories initialized.\n" );

		// Create input system before framework so it will exist for UI. But don't attach devices yet, as they don't exist.
		PRINTF( "Initializing input system.\n" );
		m_InputSystem = new InputSystem;
		m_InputSystem->Initialize( "RosaInput" );
	XTRACE_END;

	Framework3D::Initialize();

	m_InputSystem->SetRelativeFrameTime( m_FixedFrameTime );

	STATICHASH( DisplayWidth );
	STATICHASH( DisplayHeight );
	m_DisplayWidth	= ConfigManager::GetInt( sDisplayWidth );
	m_DisplayHeight	= ConfigManager::GetInt( sDisplayHeight );

	STATICHASH( PauseOnLostFocus );
	m_PauseOnLostFocus = ConfigManager::GetBool( sPauseOnLostFocus );

	STATICHASH( MuteWhenUnfocused );
	m_MuteWhenUnfocused = ConfigManager::GetBool( sMuteWhenUnfocused );

#if USE_DIRECTINPUT
	m_Controller = new XInputController( m_Window->GetHWnd() );
#else
	m_Controller = new XInputController;
#endif

	m_TargetManager = new RosaTargetManager( m_Renderer );
	m_TargetManager->CreateTargets( m_Display->m_Width, m_Display->m_Height );

	m_Audio3DListener = new RosaSound3DListener;
	m_Audio3DListener->Initialize();

	ASSERT( m_AudioSystem );
	m_AudioSystem->Set3DListener( m_Audio3DListener );

	m_AchievementManager = new AchievementManager_Steam;

	PublishDisplayedBrightness();

	STATICHASH( FOV );
	const float FOV = ConfigManager::GetFloat( sFOV, 90.0f );
	PublishDisplayedFOV();

	STATICHASH( ForegroundFOV );
	const float FGFOV = ConfigManager::GetFloat( sForegroundFOV, 60.0f );

	STATICHASH( NearClip );
	const float NearClip = ConfigManager::GetFloat( sNearClip, 0.1f );

	STATICHASH( FarClip );
	const float FarClip = ConfigManager::GetFloat( sFarClip, 0.1f );

	STATICHASH( VanishingPointY );
	const float VanishingPointY = ConfigManager::GetFloat( sVanishingPointY, 0.0f );

	STATICHASH( LeftyMode );
	const bool LeftyMode = ConfigManager::GetBool( sLeftyMode, 0.0f );

	const float	fDisplayWidth	= static_cast<float>( m_DisplayWidth );
	const float	fDisplayHeight	= static_cast<float>( m_DisplayHeight );
	const float	AspectRatio		= fDisplayWidth / fDisplayHeight;
	const bool	OpenGL			= m_Renderer->IsOpenGL();

	m_MainView		= new View( Vector(), Angles(), FOV,	AspectRatio,	NearClip, FarClip, VanishingPointY,	false,		OpenGL );
	m_FGView		= new View( Vector(), Angles(), FGFOV,	AspectRatio,	NearClip, FarClip, VanishingPointY,	LeftyMode,	OpenGL );
	m_SkyView		= new View( Vector(), Angles(), FOV,	AspectRatio,	NearClip, FarClip, VanishingPointY,	false,		OpenGL );
	m_SkylineView	= new View( Vector(), Angles(), FOV,	AspectRatio,	NearClip, FarClip, VanishingPointY,	false,		OpenGL );
	m_LightView		= new View( Vector(), Angles(), 90.0f,	1.0f,			NearClip, FarClip, 0.0f,			false,		OpenGL );
	CreateHUDView();
	CreateMinimapViews();

	CreateBuckets();

	m_InputSystem->SetKeyboard( m_Keyboard );
	m_InputSystem->SetMouse( m_Mouse );
	m_InputSystem->SetController( m_Controller );
	m_InputSystem->SetClock( m_Clock );

	WBActionStack::Initialize();

	PRINTF( "Checking saved game path\n" );
	const SimpleString SaveLoadPath = GetSaveLoadPath();
	if( !FileUtil::PathExists( SaveLoadPath.CStr() ) )
	{
		PRINTF( "Creating saved game path\n" );
		FileUtil::MakePath( SaveLoadPath.CStr() );
	}

	PRINTF( "Initializing game.\n" );

	// Initialize difficulty system
	RosaDifficulty::Initialize();

	InitializeWorld( "", false );

	m_Game = new RosaGame;
	m_Game->RefreshRTDependentSystems();
	m_Game->Initialize();

	m_Intro = new RosaIntro;

	// Initialize config stuff
	OnShowHUDChanged();
	OnShowMinimapChanged();
#if ROSA_USE_MAXIMAP
	OnShowMaximapChanged();
#endif
	OnLeftyModeChanged();
	OnStylizedAnimChanged();

	// HACKHACK to init game and UI to correct state
	m_Game->UpdatePostCheapEnabled();

	// ROSATODO: Move this into input system init?
	OnInvertYChanged();
	{
		STATICHASH( ControllerPower );
		const float ControllerPower = ConfigManager::GetFloat( sControllerPower );

		STATIC_HASHED_STRING( MoveX );
		m_InputSystem->SetControllerPower( sMoveX, ControllerPower );
		STATIC_HASHED_STRING( MoveY );
		m_InputSystem->SetControllerPower( sMoveY, ControllerPower );
		STATIC_HASHED_STRING( TurnX );
		m_InputSystem->SetControllerPower( sTurnX, ControllerPower );
		STATIC_HASHED_STRING( TurnY );
		m_InputSystem->SetControllerPower( sTurnY, ControllerPower );
	}

	// Initialize UI sliders. This could be neater.
	// This also pushes the initial values to their respective systems, which is pret-ty terrible design.
	m_UIManager->SetSliderValue( "ControlsOptionsScreen",	"MouseSpeedSlider",			GetSliderValueFromMouseSpeed(		ConfigManager::GetFloat( "MouseSpeed",		1.0f ) ) );
	m_UIManager->SetSliderValue( "ControlsOptionsScreen",	"ControllerSpeedSlider",	GetSliderValueFromControllerSpeed(	ConfigManager::GetFloat( "ControllerSpeed",	1.0f ) ) );
	m_UIManager->SetSliderValue( "DisplayOptionsScreen",	"BrightnessSlider",			GetSliderValueFromBrightness(		ConfigManager::GetFloat( "Brightness",		1.0f ) ) );
	m_UIManager->SetSliderValue( "GraphicsOptionsScreen",	"FOVSlider",				GetSliderValueFromFOV(				FOV ) );
	m_UIManager->SetSliderValue( "GraphicsOptionsScreen",	"VanishingPointYSlider",	GetSliderValueFromVanishingPointY(	VanishingPointY ) );
	m_UIManager->SetSliderValue( "GraphicsOptionsScreen",	"LightDistanceSlider",		GetSliderValueFromLightDistance(	ConfigManager::GetFloat( "LightDistance",	1.0f ) ) );
	m_UIManager->SetSliderValue( "AudioOptionsScreen",		"VolumeSlider",													ConfigManager::GetFloat( "MasterVolume" ) );
	m_UIManager->SetSliderValue( "AudioOptionsScreen",		"MusicVolumeSlider",											ConfigManager::GetFloat( "MusicVolume" ) );
	m_UIManager->SetSliderValue( "AudioOptionsScreen",		"AmbienceVolumeSlider",											ConfigManager::GetFloat( "AmbienceVolume" ) );

	// Also set the slider notches where desired (in all current cases, to whatever the defaults are)
	m_UIManager->SetSliderNotch( "ControlsOptionsScreen",	"MouseSpeedSlider",			GetSliderValueFromMouseSpeed(		ConfigManager::GetFloat( "MouseSpeed",		0.0f, "Defaults_Controls" ) ) );
	m_UIManager->SetSliderNotch( "ControlsOptionsScreen",	"ControllerSpeedSlider",	GetSliderValueFromControllerSpeed(	ConfigManager::GetFloat( "ControllerSpeed",	0.0f, "Defaults_Controls" ) ) );
	m_UIManager->SetSliderNotch( "DisplayOptionsScreen",	"BrightnessSlider",			GetSliderValueFromBrightness(		ConfigManager::GetFloat( "Brightness",		0.0f, "Defaults_Display" ) ) );
	m_UIManager->SetSliderNotch( "GraphicsOptionsScreen",	"FOVSlider",				GetSliderValueFromFOV(				ConfigManager::GetFloat( "FOV",				0.0f, "Defaults_Graphics" ) ) );
	m_UIManager->SetSliderNotch( "GraphicsOptionsScreen",	"VanishingPointYSlider",	GetSliderValueFromVanishingPointY(	ConfigManager::GetFloat( "VanishingPointY",	0.0f, "Defaults_Graphics" ) ) );
	m_UIManager->SetSliderNotch( "GraphicsOptionsScreen",	"LightDistanceSlider",		GetSliderValueFromLightDistance(	ConfigManager::GetFloat( "LightDistance",	0.0f, "Defaults_Graphics" ) ) );
	m_UIManager->SetSliderNotch( "AudioOptionsScreen",		"VolumeSlider",													ConfigManager::GetFloat( "MasterVolume",	0.0f, "Defaults_Audio" ) );
	m_UIManager->SetSliderNotch( "AudioOptionsScreen",		"MusicVolumeSlider",											ConfigManager::GetFloat( "MusicVolume",		0.0f, "Defaults_Audio" ) );
	m_UIManager->SetSliderNotch( "AudioOptionsScreen",		"AmbienceVolumeSlider",											ConfigManager::GetFloat( "AmbienceVolume",	0.0f, "Defaults_Audio" ) );

	// Initialize UI callbacks
	UIScreenRosaSetRes* pSetRes = m_UIManager->GetScreen<UIScreenRosaSetRes>( "SetResScreen" );
	pSetRes->SetUICallback( SUICallback( RosaFramework::OnSetRes, NULL ) );

	// Initialize cloud stuff last, there's no rush and it needs the WBWorld to exist first
	m_CloudManager = new RosaCloudManager();
	m_CloudManager->Initialize();

	// All done, show the window finally.
	SafeDelete( m_SplashWindow );
#if BUILD_WINDOWS_NO_SDL
	m_Window->Show( m_CmdShow );
#elif BUILD_SDL
	m_Window->Show();
#endif

	// Reattach GL context if needed.
	m_Renderer->Refresh();

	// Add a unique player stat
	SET_STAT( "NumPlayers", 1 );

	PRINTF( "Rosa initialization complete.\n" );
}

/*static*/ void RosaFramework::OnSetRes( void* pUIElement, void* pVoid )
{
	Unused( pVoid );

	RosaFramework* const		pFramework	= RosaFramework::GetInstance();
	Display* const				pDisplay	= pFramework->GetDisplay();
	UIWidget* const				pWidget		= static_cast<UIWidget*>( pUIElement );
	UIScreenRosaSetRes* const	pSetRes		= pFramework->GetUIManager()->GetScreen<UIScreenRosaSetRes>( "SetResScreen" );
	const SDisplayMode			ChosenRes	= pSetRes->GetRes( pWidget->m_Name );

	if( !pDisplay->m_Fullscreen && ( ChosenRes.Width > pDisplay->m_ScreenWidth || ChosenRes.Height > pDisplay->m_ScreenHeight ) )
	{
		WARNDESC( "Mode too large for screen." );
	}
	else
	{
		pFramework->SetResolution( ChosenRes.Width, ChosenRes.Height );
	}
}

/*virtual*/ void RosaFramework::InitializeAudioSystem()
{
	m_AudioSystem = CreateSoLoudAudioSystem();
}

// Also used to reinitialize world.
void RosaFramework::InitializeWorld( const SimpleString& WorldDef, const bool CreateWorld )
{
	XTRACE_FUNCTION;

	const bool ForceResetToGameScreens = false;
	PrepareForLoad( ForceResetToGameScreens );

	if( WBWorld::HasInstance() )
	{
		DEVASSERT( m_World );

		// Reinitialize WBWorld
		WBWorld::GetInstance()->ShutDown();
		WBWorld::GetInstance()->Initialize();
	}
	else
	{
		WBWorld::CreateInstance();
		WBWorld::GetInstance()->SetClock( GetClock() );
		RegisterForEvents();
		m_UIManager->RegisterForEvents();
	}

	if( !m_World )
	{
		m_World = new RosaWorld;
		m_World->Initialize();
	}

	m_World->SetCurrentWorld( WorldDef );

	m_Audio3DListener->SetWorld( m_World );

	if( CreateWorld )
	{
		m_World->Create();
		InitializeTools();
	}
}

void RosaFramework::RegisterForEvents()
{
	STATIC_HASHED_STRING( QuitGame );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sQuitGame, this, NULL );

	STATIC_HASHED_STRING( TogglePlayIntroLevel );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sTogglePlayIntroLevel, this, NULL );

	STATIC_HASHED_STRING( ToggleShowTutorials );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleShowTutorials, this, NULL );

	STATIC_HASHED_STRING( ToggleInvertY );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleInvertY, this, NULL );

	STATIC_HASHED_STRING( ToggleAutoAim );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleAutoAim, this, NULL );

	STATIC_HASHED_STRING( ToggleShowHUD );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleShowHUD, this, NULL );

	STATIC_HASHED_STRING( ToggleHUDMarkers );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleHUDMarkers, this, NULL );

	STATIC_HASHED_STRING( ToggleMinimap );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleMinimap, this, NULL );

	STATIC_HASHED_STRING( ToggleMinimapMarkers );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleMinimapMarkers, this, NULL );

#if ROSA_USE_MAXIMAP
	STATIC_HASHED_STRING( ToggleMaximap );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleMaximap, this, NULL );
#endif

	STATIC_HASHED_STRING( CycleControllerType );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sCycleControllerType, this, NULL );

	STATIC_HASHED_STRING( ToggleFullscreen );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleFullscreen, this, NULL );

	STATIC_HASHED_STRING( ToggleVSync );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleVSync, this, NULL );

	STATIC_HASHED_STRING( ToggleFixedDT );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleFixedDT, this, NULL );

	STATIC_HASHED_STRING( ToggleFXAA );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleFXAA, this, NULL );

	STATIC_HASHED_STRING( ToggleSSAO );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleSSAO, this, NULL );

	STATIC_HASHED_STRING( ToggleBloom );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleBloom, this, NULL );

	STATIC_HASHED_STRING( ToggleViewBob );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleViewBob, this, NULL );

	STATIC_HASHED_STRING( ToggleViewSway );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleViewSway, this, NULL );

	STATIC_HASHED_STRING( ToggleSlideRoll );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleSlideRoll, this, NULL );

	STATIC_HASHED_STRING( ToggleStrafeRoll );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleStrafeRoll, this, NULL );

	STATIC_HASHED_STRING( ToggleSprintFOV );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleSprintFOV, this, NULL );

	STATIC_HASHED_STRING( ToggleHandsVelocity );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleHandsVelocity, this, NULL );

	STATIC_HASHED_STRING( ToggleLeftyMode );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleLeftyMode, this, NULL );

	STATIC_HASHED_STRING( ToggleStylizedAnim );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleStylizedAnim, this, NULL );

	STATIC_HASHED_STRING( ToggleFilmGrain );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleFilmGrain, this, NULL );

	STATIC_HASHED_STRING( ToggleDirtyLens );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleDirtyLens, this, NULL );

	STATIC_HASHED_STRING( ToggleHalos );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleHalos, this, NULL );

	STATIC_HASHED_STRING( ToggleFog );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleFog, this, NULL );

	STATIC_HASHED_STRING( ToggleVolumeFog );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleVolumeFog, this, NULL );

	STATIC_HASHED_STRING( ToggleColorGrading );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleColorGrading, this, NULL );

	STATIC_HASHED_STRING( ToggleDisplace );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleDisplace, this, NULL );

	STATIC_HASHED_STRING( ToggleBlur );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleBlur, this, NULL );

	STATIC_HASHED_STRING( ToggleBlot );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleBlot, this, NULL );

	STATIC_HASHED_STRING( ToggleCanvas );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleCanvas, this, NULL );

	STATIC_HASHED_STRING( ToggleGradient );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleGradient, this, NULL );

	STATIC_HASHED_STRING( ToggleEdge );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sToggleEdge, this, NULL );

	STATIC_HASHED_STRING( TogglePostCheap );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sTogglePostCheap, this, NULL );

	STATIC_HASHED_STRING( ResetGameOptions );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sResetGameOptions, this, NULL );

	STATIC_HASHED_STRING( ResetControlsOptions );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sResetControlsOptions, this, NULL );

	STATIC_HASHED_STRING( ResetInputBindings );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sResetInputBindings, this, NULL );

	STATIC_HASHED_STRING( ResetDisplayOptions );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sResetDisplayOptions, this, NULL );

	STATIC_HASHED_STRING( ResetGraphicsOptions );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sResetGraphicsOptions, this, NULL );

	STATIC_HASHED_STRING( ResetAudioOptions );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sResetAudioOptions, this, NULL );

	STATIC_HASHED_STRING( OnSliderChanged );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sOnSliderChanged, this, NULL );

	STATIC_HASHED_STRING( WritePrefsConfig );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sWritePrefsConfig, this, NULL );

	STATIC_HASHED_STRING( AddClockScalar );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sAddClockScalar, this, NULL );

	STATIC_HASHED_STRING( DownloadCloudPackages );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sDownloadCloudPackages, this, NULL );
}

/*virtual*/ void RosaFramework::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Framework3D::HandleEvent( Event );

	STATIC_HASHED_STRING( TogglePlayIntroLevel );
	STATIC_HASHED_STRING( ToggleShowTutorials );
	STATIC_HASHED_STRING( ToggleInvertY );
	STATIC_HASHED_STRING( ToggleAutoAim );
	STATIC_HASHED_STRING( ToggleShowHUD );
	STATIC_HASHED_STRING( ToggleHUDMarkers );
	STATIC_HASHED_STRING( ToggleMinimap );
	STATIC_HASHED_STRING( ToggleMinimapMarkers );
#if ROSA_USE_MAXIMAP
	STATIC_HASHED_STRING( ToggleMaximap );
#endif
	STATIC_HASHED_STRING( CycleControllerType );
	STATIC_HASHED_STRING( ToggleFullscreen );
	STATIC_HASHED_STRING( ToggleVSync );
	STATIC_HASHED_STRING( ToggleFixedDT );
	STATIC_HASHED_STRING( ToggleFXAA );
	STATIC_HASHED_STRING( ToggleSSAO );
	STATIC_HASHED_STRING( ToggleBloom );
	STATIC_HASHED_STRING( ToggleViewBob );
	STATIC_HASHED_STRING( ToggleViewSway );
	STATIC_HASHED_STRING( ToggleSlideRoll );
	STATIC_HASHED_STRING( ToggleStrafeRoll );
	STATIC_HASHED_STRING( ToggleSprintFOV );
	STATIC_HASHED_STRING( ToggleHandsVelocity );
	STATIC_HASHED_STRING( ToggleLeftyMode );
	STATIC_HASHED_STRING( ToggleStylizedAnim );
	STATIC_HASHED_STRING( ToggleFilmGrain );
	STATIC_HASHED_STRING( ToggleDirtyLens );
	STATIC_HASHED_STRING( ToggleHalos );
	STATIC_HASHED_STRING( ToggleFog );
	STATIC_HASHED_STRING( ToggleVolumeFog );
	STATIC_HASHED_STRING( ToggleColorGrading );
	STATIC_HASHED_STRING( ToggleDisplace );
	STATIC_HASHED_STRING( ToggleBlur );
	STATIC_HASHED_STRING( ToggleBlot );
	STATIC_HASHED_STRING( ToggleCanvas );
	STATIC_HASHED_STRING( ToggleGradient );
	STATIC_HASHED_STRING( ToggleEdge );
	STATIC_HASHED_STRING( TogglePostCheap );
	STATIC_HASHED_STRING( ResetGameOptions );
	STATIC_HASHED_STRING( ResetControlsOptions );
	STATIC_HASHED_STRING( ResetInputBindings );
	STATIC_HASHED_STRING( ResetDisplayOptions );
	STATIC_HASHED_STRING( ResetGraphicsOptions );
	STATIC_HASHED_STRING( ResetAudioOptions );
	STATIC_HASHED_STRING( OnSliderChanged );
	STATIC_HASHED_STRING( WritePrefsConfig );
	STATIC_HASHED_STRING( AddClockScalar );
	STATIC_HASHED_STRING( DownloadCloudPackages );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sTogglePlayIntroLevel )
	{
		XTRACE_NAMED( TogglePlayIntroLevel );

		STATICHASH( PlayIntroLevel );
		const bool PlayIntroLevel = !ConfigManager::GetBool( sPlayIntroLevel );
		ConfigManager::SetBool( sPlayIntroLevel, PlayIntroLevel );
	}
	else if( EventName == sToggleShowTutorials )
	{
		XTRACE_NAMED( ToggleShowTutorials );

		STATICHASH( ShowTutorials );
		const bool ShowTutorials = !ConfigManager::GetBool( sShowTutorials );
		ConfigManager::SetBool( sShowTutorials, ShowTutorials );

		// ROSATODO: Notify tutorial system if needed
	}
	else if( EventName == sToggleInvertY )
	{
		XTRACE_NAMED( ToggleInvertY );

		STATIC_HASHED_STRING( TurnY );
		const bool InvertY = !m_InputSystem->GetMouseInvert( sTurnY );

		// Publish config var so UI can reflect the change.
		// I could make input system publish config vars for each adjustment, but
		// that seems wasteful since most inputs currently have no adjustment.
		STATICHASH( InvertY );
		ConfigManager::SetBool( sInvertY, InvertY );

		OnInvertYChanged();
	}
	else if( EventName == sToggleAutoAim )
	{
		XTRACE_NAMED( ToggleAutoAim );

		STATICHASH( AutoAim );
		const bool AutoAim = !ConfigManager::GetBool( sAutoAim );
		ConfigManager::SetBool( sAutoAim, AutoAim );

		OnAutoAimChanged();
	}
	else if( EventName == sToggleShowHUD )
	{
		STATICHASH( ShowHUD );
		const bool ShowHUD = !ConfigManager::GetBool( sShowHUD );

		ConfigManager::SetBool( sShowHUD, ShowHUD );

		OnShowHUDChanged();
	}
	else if( EventName == sToggleHUDMarkers )
	{
		STATICHASH( HUDMarkers );
		const bool ShowHUDMarkers = !ConfigManager::GetBool( sHUDMarkers );

		ConfigManager::SetBool( sHUDMarkers, ShowHUDMarkers );

		OnShowHUDMarkersChanged();
	}
	else if( EventName == sToggleMinimap )
	{
		STATICHASH( Minimap );
		const bool ShowMinimap = !ConfigManager::GetBool( sMinimap );

		ConfigManager::SetBool( sMinimap, ShowMinimap );

		OnShowMinimapChanged();
	}
	else if( EventName == sToggleMinimapMarkers )
	{
		STATICHASH( MinimapMarkers );
		const bool ShowMinimapMarkers = !ConfigManager::GetBool( sMinimapMarkers );

		ConfigManager::SetBool( sMinimapMarkers, ShowMinimapMarkers );

		OnShowMinimapMarkersChanged();
	}
#if ROSA_USE_MAXIMAP
	else if( EventName == sToggleMaximap )
	{
		STATICHASH( Maximap );
		const bool ShowMaximap = !ConfigManager::GetBool( sMaximap );

		ConfigManager::SetBool( sMaximap, ShowMaximap );

		OnShowMaximapChanged();
	}
#endif
	else if( EventName == sCycleControllerType )
	{
		GetInputSystem()->CycleControllerType();
	}
	else if( EventName == sToggleFullscreen )
	{
		XTRACE_NAMED( ToggleFullscreen );
		ToggleFullscreen();
	}
	else if( EventName == sToggleVSync )
	{
		XTRACE_NAMED( ToggleVSync );
		ToggleVSync();
	}
	else if( EventName == sToggleFixedDT )
	{
		XTRACE_NAMED( ToggleFixedDT );
		ToggleFixedDT();
	}
	else if( EventName == sToggleFXAA )
	{
		XTRACE_NAMED( ToggleFXAA );
		ToggleFXAA();
	}
	else if( EventName == sToggleSSAO )
	{
		XTRACE_NAMED( ToggleSSAO );
		ToggleSSAO();
	}
	else if( EventName == sToggleBloom )
	{
		XTRACE_NAMED( ToggleBloom );
		ToggleBloom();
	}
	else if( EventName == sToggleViewBob )
	{
		XTRACE_NAMED( ToggleViewBob );

		STATICHASH( ViewBob );
		const bool ViewBob = !ConfigManager::GetBool( sViewBob );
		ConfigManager::SetBool( sViewBob, ViewBob );

		OnViewBobChanged();
	}
	else if( EventName == sToggleViewSway )
	{
		XTRACE_NAMED( ToggleViewSway );

		STATICHASH( ViewSway );
		const bool ViewSway = !ConfigManager::GetBool( sViewSway );
		ConfigManager::SetBool( sViewSway, ViewSway );

		OnViewSwayChanged();
	}
	else if( EventName == sToggleSlideRoll )
	{
		XTRACE_NAMED( ToggleSlideRoll );

		STATICHASH( SlideRoll );
		const bool SlideRoll = !ConfigManager::GetBool( sSlideRoll );
		ConfigManager::SetBool( sSlideRoll, SlideRoll );

		OnSlideRollChanged();
	}
	else if( EventName == sToggleStrafeRoll )
	{
		XTRACE_NAMED( ToggleStrafeRoll );

		STATICHASH( StrafeRoll );
		const bool StrafeRoll = !ConfigManager::GetBool( sStrafeRoll );
		ConfigManager::SetBool( sStrafeRoll, StrafeRoll );

		OnStrafeRollChanged();
	}
	else if( EventName == sToggleSprintFOV )
	{
		STATICHASH( SprintFOV );
		const bool SprintFOV = !ConfigManager::GetBool( sSprintFOV );
		ConfigManager::SetBool( sSprintFOV, SprintFOV );

		OnSprintFOVChanged();
	}
	else if( EventName == sToggleHandsVelocity )
	{
		XTRACE_NAMED( ToggleHandsVelocity );

		STATICHASH( HandsVelocity );
		const bool HandsVelocity = !ConfigManager::GetBool( sHandsVelocity );
		ConfigManager::SetBool( sHandsVelocity, HandsVelocity );

		OnHandsVelocityChanged();
	}
	else if( EventName == sToggleLeftyMode )
	{
		STATICHASH( LeftyMode );
		const bool LeftyMode = !ConfigManager::GetBool( sLeftyMode );
		ConfigManager::SetBool( sLeftyMode, LeftyMode );

		OnLeftyModeChanged();
	}
	else if( EventName == sToggleStylizedAnim )
	{
		STATICHASH( StylizedAnim );
		const bool StylizedAnim = !ConfigManager::GetBool( sStylizedAnim );
		ConfigManager::SetBool( sStylizedAnim, StylizedAnim );

		OnStylizedAnimChanged();
	}
	else if( EventName == sToggleFilmGrain )
	{
		XTRACE_NAMED( ToggleFilmGrain );

		STATICHASH( FilmGrain );
		const bool FilmGrain = !ConfigManager::GetBool( sFilmGrain );
		ConfigManager::SetBool( sFilmGrain, FilmGrain );

		DEVASSERT( m_Game );
		m_Game->UpdateNoiseEnabled();
	}
	else if( EventName == sToggleDirtyLens )
	{
		XTRACE_NAMED( ToggleDirtyLens );

		STATICHASH( DirtyLens );
		const bool DirtyLens = !ConfigManager::GetBool( sDirtyLens );
		ConfigManager::SetBool( sDirtyLens, DirtyLens );

		DEVASSERT( m_Game );
		m_Game->UpdateDirtyLensEnabled();
	}
	else if( EventName == sToggleHalos )
	{
		XTRACE_NAMED( ToggleHalos );

		STATICHASH( Halos );
		const bool Halos = !ConfigManager::GetBool( sHalos );
		ConfigManager::SetBool( sHalos, Halos );

		DEVASSERT( m_Game );
		m_Game->UpdateHalosEnabled();
	}
	else if( EventName == sToggleFog )
	{
		XTRACE_NAMED( ToggleFog );

		STATICHASH( Fog );
		const bool Fog = !ConfigManager::GetBool( sFog );
		ConfigManager::SetBool( sFog, Fog );

		DEVASSERT( m_Game );
		m_Game->UpdateFogEnabled();
	}
	else if( EventName == sToggleVolumeFog )
	{
		XTRACE_NAMED( ToggleVolumeFog );

		STATICHASH( VolumeFog );
		const bool VolumeFog = !ConfigManager::GetBool( sVolumeFog );
		ConfigManager::SetBool( sVolumeFog, VolumeFog );

		// Recreate buckets because we have a new render path
		CreateBuckets();
	}
	else if( EventName == sToggleColorGrading )
	{
		XTRACE_NAMED( ToggleColorGrading );

		STATICHASH( ColorGrading );
		const bool ColorGrading = !ConfigManager::GetBool( sColorGrading );
		ConfigManager::SetBool( sColorGrading, ColorGrading );

		DEVASSERT( m_Game );
		m_Game->UpdateColorGradingEnabled();
	}
	else if( EventName == sToggleDisplace )
	{
		XTRACE_NAMED( ToggleDisplace );

		STATICHASH( Displace );
		const bool Displace = !ConfigManager::GetBool( sDisplace );
		ConfigManager::SetBool( sDisplace, Displace );

		DEVASSERT( m_Game );
		m_Game->UpdateDisplaceEnabled();
	}
	else if( EventName == sToggleBlur )
	{
		XTRACE_NAMED( ToggleBlur );

		STATICHASH( Blur );
		const bool Blur = !ConfigManager::GetBool( sBlur );
		ConfigManager::SetBool( sBlur, Blur );

		// HACKHACK for Zeta: also set bloom (which provides the blur)
		STATICHASH( Bloom );
		ConfigManager::SetBool( sBloom, Blur );

		DEVASSERT( m_Game );
		m_Game->UpdateBlurEnabled();
		m_Game->UpdateBloomEnabled();

		// Recreate buckets because we have a new render path for bloom
		CreateBuckets();
	}
	else if( EventName == sToggleBlot )
	{
		XTRACE_NAMED( ToggleBlot );

		STATICHASH( Blot );
		const bool Blot = !ConfigManager::GetBool( sBlot );
		ConfigManager::SetBool( sBlot, Blot );

		DEVASSERT( m_Game );
		m_Game->UpdateBlotEnabled();
	}
	else if( EventName == sToggleCanvas )
	{
		XTRACE_NAMED( ToggleCanvas );

		STATICHASH( Canvas );
		const bool Canvas = !ConfigManager::GetBool( sCanvas );
		ConfigManager::SetBool( sCanvas, Canvas );

		DEVASSERT( m_Game );
		m_Game->UpdateCanvasEnabled();
	}
	else if( EventName == sToggleGradient )
	{
		XTRACE_NAMED( ToggleGradient );

		STATICHASH( Gradient );
		const bool Gradient = !ConfigManager::GetBool( sGradient );
		ConfigManager::SetBool( sGradient, Gradient );

		// Recreate buckets because we have a new render path
		CreateBuckets();
	}
	else if( EventName == sToggleEdge )
	{
		XTRACE_NAMED( ToggleEdge );

		STATICHASH( Edge );
		const bool Edge = !ConfigManager::GetBool( sEdge );
		ConfigManager::SetBool( sEdge, Edge );

		DEVASSERT( m_Game );
		m_Game->UpdateEdgeEnabled();

		// Recreate buckets because we have a new render path
		CreateBuckets();
	}
	else if( EventName == sTogglePostCheap )
	{
		XTRACE_NAMED( TogglePostCheap );

		STATICHASH( PostCheap );
		const bool PostCheap = !ConfigManager::GetBool( sPostCheap );
		ConfigManager::SetBool( sPostCheap, PostCheap );

		m_Game->UpdatePostCheapEnabled();

#if ROSA_USE_FILMIC_POST
		// Disable bloom if we're using cheap post, so we don't pay the cost
		STATICHASH( Bloom );
		ConfigManager::SetBool( sBloom, !PostCheap );

		// Disable other settings to make it clear which ones are part of post
		STATICHASH( FilmGrain );
		ConfigManager::SetBool( sFilmGrain, !PostCheap );

		STATICHASH( DirtyLens );
		ConfigManager::SetBool( sDirtyLens, !PostCheap );

		STATICHASH( Halos );
		ConfigManager::SetBool( sHalos, !PostCheap );

		STATICHASH( ColorGrading );
		ConfigManager::SetBool( sColorGrading, !PostCheap );

		DEVASSERT( m_Game );
		m_Game->UpdateBloomEnabled();
		m_Game->UpdateNoiseEnabled();
		m_Game->UpdateDirtyLensEnabled();
		m_Game->UpdateHalosEnabled();
		m_Game->UpdateColorGradingEnabled();
#elif ROSA_USE_WATERCOLOR_POST
		// HACKHACK: also set bloom (which provides the blur)
		STATICHASH( Bloom );
		ConfigManager::SetBool( sBloom, !PostCheap );

		// HACKHACK: Also set all the watercolor stuff to match, and update everything
		STATICHASH( Displace );
		ConfigManager::SetBool( sDisplace, !PostCheap );

		STATICHASH( Blur );
		ConfigManager::SetBool( sBlur, !PostCheap );

		STATICHASH( Blot );
		ConfigManager::SetBool( sBlot, !PostCheap );

		STATICHASH( Canvas );
		ConfigManager::SetBool( sCanvas, !PostCheap );

		STATICHASH( Edge );
		ConfigManager::SetBool( sEdge, !PostCheap );

		STATICHASH( ColorGrading );
		ConfigManager::SetBool( sColorGrading, !PostCheap );

		DEVASSERT( m_Game );
		m_Game->UpdateBloomEnabled();
		m_Game->UpdateDisplaceEnabled();
		m_Game->UpdateBlurEnabled();
		m_Game->UpdateBlotEnabled();
		m_Game->UpdateCanvasEnabled();
		m_Game->UpdateEdgeEnabled();
		m_Game->UpdateColorGradingEnabled();
#endif

		// Recreate buckets because we have a new render path
		CreateBuckets();
	}
	else if( EventName == sResetGameOptions )
	{
		STATICHASH( Defaults_Game );
		ConfigManager::PushDefaultsToContext( sDefaults_Game, false, false, true );

		STATICHASH( Defaults_Framework );
		STATICHASH( Framework );
		ConfigManager::PushDefaultsToContext( sDefaults_Framework, false, false, true, sFramework );

		// Tell systems we've done this
		OnViewBobChanged();
		OnViewSwayChanged();
		OnSlideRollChanged();
		OnStrafeRollChanged();
		OnSprintFOVChanged();
		OnHandsVelocityChanged();
		OnLeftyModeChanged();
		OnShowHUDChanged();
		OnShowMinimapChanged();
		OnShowMinimapMarkersChanged();
#if ROSA_USE_MAXIMAP
		OnShowMaximapChanged();
#endif
		OnFixedFrameTimeChanged();
	}
	else if( EventName == sResetControlsOptions )
	{
		STATICHASH( Defaults_Controls );
		ConfigManager::PushDefaultsToContext( sDefaults_Controls, false, false, true );

		STATICHASH( Defaults_RosaInput );
		STATICHASH( RosaInput );
		ConfigManager::PushDefaultsToContext( sDefaults_RosaInput, false, false, true, sRosaInput );

		// Tell systems we've done this
		m_UIManager->SetSliderValue( "ControlsOptionsScreen",	"MouseSpeedSlider",			GetSliderValueFromMouseSpeed(		ConfigManager::GetFloat( "MouseSpeed",		1.0f ) ) );
		m_UIManager->SetSliderValue( "ControlsOptionsScreen",	"ControllerSpeedSlider",	GetSliderValueFromControllerSpeed(	ConfigManager::GetFloat( "ControllerSpeed",	1.0f ) ) );

		OnInvertYChanged();
		OnAutoAimChanged();
		OnControllerTypeChanged();
	}
	else if( EventName == sResetInputBindings )
	{
		STATICHASH( Defaults_RosaKeyboard );
		STATICHASH( RosaKeyboard );
		ConfigManager::PushDefaultsToContext( sDefaults_RosaKeyboard,	false,	true,	true,	sRosaKeyboard );

		STATICHASH( Defaults_RosaMouse );
		STATICHASH( RosaMouse );
		ConfigManager::PushDefaultsToContext( sDefaults_RosaMouse,		false,	true,	true,	sRosaMouse );

		STATICHASH( Defaults_RosaController );
		STATICHASH( RosaController );
		ConfigManager::PushDefaultsToContext( sDefaults_RosaController,	false,	true,	true,	sRosaController );

		// RosaController has immutable defaults that should be re-applied after the reset.
		// (This wasn't actually causing problems since those values were used once when
		// initializing the input system, but this seems neater.
		STATICHASH( Consts_RosaController );
		ConfigManager::PushDefaultsToContext( sConsts_RosaController,	false,	false,	false,	sRosaController );

		// Tell input system we've done this
		m_InputSystem->UpdateBindingsFromConfig();
	}
	else if( EventName == sResetDisplayOptions )
	{
		STATICHASH( Defaults_Display );
		ConfigManager::PushDefaultsToContext( sDefaults_Display, false, false, true );

		// Reset slider values; this also invokes their side effects!
		m_UIManager->SetSliderValue( "DisplayOptionsScreen",	"BrightnessSlider",			GetSliderValueFromBrightness(		ConfigManager::GetFloat( "Brightness" ) ) );
	}
	else if( EventName == sResetGraphicsOptions )
	{
		STATICHASH( Defaults_Graphics );
		ConfigManager::PushDefaultsToContext( sDefaults_Graphics, false, false, true );

		// Reset slider values; this also invokes their side effects!
		m_UIManager->SetSliderValue( "GraphicsOptionsScreen",	"FOVSlider",				GetSliderValueFromFOV(				ConfigManager::GetFloat( "FOV" ) ) );
		m_UIManager->SetSliderValue( "GraphicsOptionsScreen",	"VanishingPointYSlider",	GetSliderValueFromVanishingPointY(	ConfigManager::GetFloat( "VanishingPointY" ) ) );
		m_UIManager->SetSliderValue( "GraphicsOptionsScreen",	"LightDistanceSlider",		GetSliderValueFromLightDistance(	ConfigManager::GetFloat( "LightDistance" ) ) );

		// Push updated state to game, same as the individual options above
		DEVASSERT( m_Game );
		m_Game->UpdateNoiseEnabled();
		m_Game->UpdateBloomEnabled();
		m_Game->UpdateDirtyLensEnabled();
		m_Game->UpdateHalosEnabled();
		m_Game->UpdatePostCheapEnabled();
		m_Game->UpdateDisplaceEnabled();
		m_Game->UpdateBlurEnabled();
		m_Game->UpdateBlotEnabled();
		m_Game->UpdateCanvasEnabled();
		m_Game->UpdateEdgeEnabled();
		m_Game->UpdateFogEnabled();
		m_Game->UpdateColorGradingEnabled();

		OnStylizedAnimChanged();

		// Recreate buckets because we may have a new render path
		CreateBuckets();
	}
	else if( EventName == sResetAudioOptions )
	{
		STATICHASH( Defaults_Audio );
		ConfigManager::PushDefaultsToContext( sDefaults_Audio, false, false, true );

		// Reset slider values; this also invokes their side effects!
		m_UIManager->SetSliderValue( "AudioOptionsScreen",		"VolumeSlider",				ConfigManager::GetFloat( "MasterVolume" ) );
		m_UIManager->SetSliderValue( "AudioOptionsScreen",		"MusicVolumeSlider",		ConfigManager::GetFloat( "MusicVolume" ) );
		m_UIManager->SetSliderValue( "AudioOptionsScreen",		"AmbienceVolumeSlider",		ConfigManager::GetFloat( "AmbienceVolume" ) );
	}
	else if( EventName == sOnSliderChanged )
	{
		XTRACE_NAMED( OnSliderChanged );

		STATIC_HASHED_STRING( SliderName );
		const HashedString SliderName = Event.GetHash( sSliderName );

		STATIC_HASHED_STRING( SliderValue );
		const float SliderValue = Event.GetFloat( sSliderValue );

		HandleUISliderEvent( SliderName, SliderValue );
	}
	else if( EventName == sWritePrefsConfig )
	{
		XTRACE_NAMED( WritePrefsConfig );
		WritePrefsConfig();
	}
	else if( EventName == sAddClockScalar )
	{
		STATIC_HASHED_STRING( Duration );
		const float Duration = Event.GetFloat( sDuration );

		STATIC_HASHED_STRING( Scalar );
		const float Scalar = Event.GetFloat( sScalar );

		m_Clock->AddMultiplierRequest( Duration, Scalar );
	}
	else if( EventName == sDownloadCloudPackages )
	{
		if( NULL != m_CloudManager )
		{
			m_CloudManager->DownloadPackages();
		}
	}
}

void RosaFramework::OnShowHUDChanged()
{
	STATICHASH( ShowHUD );
	const bool ShowHUD = ConfigManager::GetBool( sShowHUD );

	{
		STATIC_HASHED_STRING( HUD );
		WB_MAKE_EVENT( SetScreenHidden, NULL );
		WB_SET_AUTO( SetScreenHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetScreenHidden, Bool, Hidden, !ShowHUD );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetScreenHidden, NULL );
	}

	{
		STATIC_HASHED_STRING( LockpickScreen );
		WB_MAKE_EVENT( SetScreenHidden, NULL );
		WB_SET_AUTO( SetScreenHidden, Hash, Screen, sLockpickScreen );
		WB_SET_AUTO( SetScreenHidden, Bool, Hidden, !ShowHUD );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetScreenHidden, NULL );
	}

	WB_MAKE_EVENT( OnShowHUDChanged, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnShowHUDChanged, NULL );
}

void RosaFramework::OnShowHUDMarkersChanged()
{
	WB_MAKE_EVENT( OnToggledHUDMarkers, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnToggledHUDMarkers, NULL );
}

void RosaFramework::OnShowMinimapChanged()
{
	STATICHASH( Minimap );
	const bool ShowMinimap = ConfigManager::GetBool( sMinimap );

	{
		STATIC_HASHED_STRING( HUD );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sMinimap );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, !ShowMinimap );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetHidden, NULL );
	}

	{
		STATICHASH( MinimapFrame );
		STATIC_HASHED_STRING( HUD );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sMinimapFrame );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, !ShowMinimap );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetHidden, NULL );
	}
}

void RosaFramework::OnShowMinimapMarkersChanged()
{
	WB_MAKE_EVENT( OnToggledMinimapMarkers, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnToggledMinimapMarkers, NULL );
}

#if ROSA_USE_MAXIMAP
void RosaFramework::OnShowMaximapChanged()
{
	STATICHASH( Maximap );
	const bool ShowMaximap = ConfigManager::GetBool( sMaximap );

	{
		STATIC_HASHED_STRING( HUD );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sMaximap );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, !ShowMaximap );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetHidden, NULL );
	}
}
#endif

void RosaFramework::OnInvertYChanged()
{
	STATICHASH( InvertY );
	const bool InvertY = ConfigManager::GetBool( sInvertY );

	STATIC_HASHED_STRING( TurnY );
	m_InputSystem->SetMouseInvert( sTurnY, InvertY );
	m_InputSystem->SetControllerInvert( sTurnY, InvertY );
}

void RosaFramework::OnAutoAimChanged()
{
	// TODO: Replace this with an event that player component listens for?
	DEVASSERT( m_Game );
	WBEntity* const			pPlayer		= m_Game->GetPlayer();
	WBCompRosaPlayer* const	pPlayerComp	= WB_GETCOMP_SAFE( pPlayer, RosaPlayer );
	if( pPlayerComp )
	{
		STATICHASH( AutoAim );
		const bool			AutoAim		= ConfigManager::GetBool( sAutoAim );
		pPlayerComp->SetAutoAimEnabled( AutoAim );
	}
}

void RosaFramework::OnControllerTypeChanged()
{
	STATICHASH( RosaInput );
	STATICHASH( ControllerType );
	const HashedString ControllerType = ConfigManager::GetHash( sControllerType, HashedString::NullString, sRosaInput );
	m_InputSystem->SetControllerType( ControllerType );
}

void RosaFramework::OnViewBobChanged()
{
	DEVASSERT( m_Game );

	WBEntity* const			pPlayer		= m_Game->GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaCamera* const	pCamera		= WB_GETCOMP_SAFE( pPlayer, RosaCamera );
	if( NULL == pCamera )
	{
		return;
	}

	STATICHASH( ViewBob );
	const bool				ViewBob		= ConfigManager::GetBool( sViewBob );
	pCamera->SetViewBobEnabled( ViewBob );
}

void RosaFramework::OnViewSwayChanged()
{
	DEVASSERT( m_Game );

	WBEntity* const			pPlayer		= m_Game->GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaCamera* const	pCamera		= WB_GETCOMP_SAFE( pPlayer, RosaCamera );
	if( NULL == pCamera )
	{
		return;
	}

	STATICHASH( ViewSway );
	const bool				ViewSway	= ConfigManager::GetBool( sViewSway );
	pCamera->SetViewSwayEnabled( ViewSway );
}

void RosaFramework::OnSlideRollChanged()
{
	DEVASSERT( m_Game );

	WBEntity* const			pPlayer		= m_Game->GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaCamera* const	pCamera		= WB_GETCOMP_SAFE( pPlayer, RosaCamera );
	if( NULL == pCamera )
	{
		return;
	}

	STATICHASH( SlideRoll );
	const bool			SlideRoll		= ConfigManager::GetBool( sSlideRoll );
	pCamera->SetSlideRollEnabled( SlideRoll );
}

void RosaFramework::OnStrafeRollChanged()
{
	DEVASSERT( m_Game );

	WBEntity* const			pPlayer		= m_Game->GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaCamera* const	pCamera		= WB_GETCOMP_SAFE( pPlayer, RosaCamera );
	if( NULL == pCamera )
	{
		return;
	}

	STATICHASH( StrafeRoll );
	const bool			StrafeRoll		= ConfigManager::GetBool( sStrafeRoll );
	pCamera->SetStrafeRollEnabled( StrafeRoll );
}

void RosaFramework::OnSprintFOVChanged()
{
	DEVASSERT( m_Game );

	WBEntity* const			pPlayer		= m_Game->GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaPlayer*		pPlayerComp	= WB_GETCOMP_SAFE( pPlayer, RosaPlayer );
	if( NULL == pPlayerComp )
	{
		return;
	}

	STATICHASH( SprintFOV );
	const bool				SprintFOV	= ConfigManager::GetBool( sSprintFOV );
	pPlayerComp->SetSprintFOVEnabled( SprintFOV );
}

void RosaFramework::OnHandsVelocityChanged()
{
	DEVASSERT( m_Game );

	WBEntity* const			pPlayer			= m_Game->GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaCamera* const	pCamera			= WB_GETCOMP_SAFE( pPlayer, RosaCamera );
	if( NULL == pCamera )
	{
		return;
	}

	STATICHASH( HandsVelocity );
	const bool				HandsVelocity	= ConfigManager::GetBool( sHandsVelocity );
	pCamera->SetHandsVelocityEnabled( HandsVelocity );
}

void RosaFramework::OnLeftyModeChanged()
{
	STATICHASH( LeftyMode );
	const bool LeftyMode	= ConfigManager::GetBool( sLeftyMode );
	m_FGView->SetMirrorX( LeftyMode );

	{
		WB_MAKE_EVENT( OnToggledLeftyMode, NULL );
		WB_SET_AUTO( OnToggledLeftyMode, Bool, LeftyMode, LeftyMode );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnToggledLeftyMode, NULL );
	}
}

void RosaFramework::OnStylizedAnimChanged()
{
	STATICHASH( StylizedAnim );
	const bool StylizedAnim	= ConfigManager::GetBool( sStylizedAnim );
	AnimationState::StaticSetStylizedAnim( StylizedAnim );
}

void RosaFramework::HandleUISliderEvent( const HashedString& SliderName, const float SliderValue )
{
	STATIC_HASHED_STRING( MouseSpeedSlider );
	STATIC_HASHED_STRING( ControllerSpeedSlider );
	STATIC_HASHED_STRING( BrightnessSlider );
	STATIC_HASHED_STRING( FOVSlider );
	STATIC_HASHED_STRING( VanishingPointYSlider );
	STATIC_HASHED_STRING( LightDistanceSlider );
	STATIC_HASHED_STRING( VolumeSlider );
	STATIC_HASHED_STRING( MusicVolumeSlider );
	STATIC_HASHED_STRING( AmbienceVolumeSlider );

	if( SliderName == sMouseSpeedSlider )
	{
		const float MouseSpeed = GetMouseSpeedFromSliderValue( SliderValue );

		STATIC_HASHED_STRING( TurnX );
		m_InputSystem->SetMouseScale( sTurnX, MouseSpeed );

		STATIC_HASHED_STRING( TurnY );
		m_InputSystem->SetMouseScale( sTurnY, MouseSpeed );

		// Publish config var.
		STATICHASH( MouseSpeed );
		ConfigManager::SetFloat( sMouseSpeed, MouseSpeed );
	}
	else if( SliderName == sControllerSpeedSlider )
	{
		const float ControllerSpeed = GetControllerSpeedFromSliderValue( SliderValue );

		STATICHASH( ControllerSpeedX );
		const float ControllerSpeedX = ControllerSpeed * ConfigManager::GetFloat( sControllerSpeedX );

		STATICHASH( ControllerSpeedY );
		const float ControllerSpeedY = -ControllerSpeed * ConfigManager::GetFloat( sControllerSpeedY ); // HACK! Controller Y axis is inverted from mouse.

		STATIC_HASHED_STRING( TurnX );
		m_InputSystem->SetControllerScale( sTurnX, ControllerSpeedX );

		STATIC_HASHED_STRING( TurnY );
		m_InputSystem->SetControllerScale( sTurnY, ControllerSpeedY );

		// Publish config var.
		STATICHASH( ControllerSpeed );
		ConfigManager::SetFloat( sControllerSpeed, ControllerSpeed );
	}
	else if( SliderName == sBrightnessSlider )
	{
		const float Brightness = GetBrightnessFromSliderValue( SliderValue );
		m_Game->SetGamma( 1.0f / Brightness );

		// Publish config var.
		STATICHASH( Brightness );
		ConfigManager::SetFloat( sBrightness, Brightness );

		PublishDisplayedBrightness();
	}
	else if( SliderName == sFOVSlider )
	{
		const float FOV = GetFOVFromSliderValue( SliderValue );

		SetFOV( FOV );

		// Publish config var.
		STATICHASH( FOV );
		ConfigManager::SetFloat( sFOV, FOV );

		PublishDisplayedFOV();

		// Notify player of changed FOV to fix FOV interpolating when game is unpaused
		WB_MAKE_EVENT( OnFOVChanged, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnFOVChanged, RosaGame::GetPlayer() );

		// Notify anyone who cares that the view has changed
		WB_MAKE_EVENT( OnViewChanged, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnViewChanged, NULL );
	}
	else if( SliderName == sVanishingPointYSlider )
	{
		const float VanishingPointY = GetVanishingPointYFromSliderValue( SliderValue );

		SetVanishingPointY( VanishingPointY );

		// Publish config var.
		STATICHASH( VanishingPointY );
		ConfigManager::SetFloat( sVanishingPointY, VanishingPointY );

		PublishDisplayedVanishingPointY();

		// HACKHACK: Also move UI elements if needed (for crosshairs and any HUD elements relative to them)
		WB_MAKE_EVENT( OnVanishingPointYChanged, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnVanishingPointYChanged, m_UIManager );

		// Notify anyone who cares that the view has changed
		WB_MAKE_EVENT( OnViewChanged, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnViewChanged, NULL );
	}
	else if( SliderName == sLightDistanceSlider )
	{
		const float LightDistance = GetLightDistanceFromSliderValue( SliderValue );

		// Publish config var.
		STATICHASH( LightDistance );
		ConfigManager::SetFloat( sLightDistance, LightDistance );
	}
	else if( SliderName == sVolumeSlider )
	{
		const float MasterVolume = SliderValue;

		m_AudioSystem->SetMasterVolume( MasterVolume );

		// Publish config var.
		STATICHASH( MasterVolume );
		ConfigManager::SetFloat( sMasterVolume, MasterVolume );
	}
	else if( SliderName == sMusicVolumeSlider )
	{
		const float MusicVolume = SliderValue;

		STATIC_HASHED_STRING( Music );
		m_AudioSystem->SetCategoryVolume( sMusic, MusicVolume, 0.0f );

		// Publish config var.
		STATICHASH( MusicVolume );
		ConfigManager::SetFloat( sMusicVolume, MusicVolume );
	}
	else if( SliderName == sAmbienceVolumeSlider )
	{
		const float AmbienceVolume = SliderValue;

		STATIC_HASHED_STRING( Ambience );
		m_AudioSystem->SetCategoryVolume( sAmbience, AmbienceVolume, 0.0f );

		// Publish config var.
		STATICHASH( AmbienceVolume );
		ConfigManager::SetFloat( sAmbienceVolume, AmbienceVolume );
	}
}

void RosaFramework::PublishDisplayedBrightness() const
{
	STATICHASH( Brightness );
	const float	Brightness			= ConfigManager::GetFloat( sBrightness );

	const float	DisplayedBrightness	= Brightness / 2.20f;

	STATICHASH( DisplayedBrightness );
	ConfigManager::SetFloat( sDisplayedBrightness, DisplayedBrightness );
}

void RosaFramework::PublishDisplayedFOV() const
{
	STATICHASH( FOV );
	const float			VerticalFOV				= ConfigManager::GetFloat( sFOV );

	// Display horizontal FOV relative to vertical at a 16:9 aspect ratio
	//const float			AspectRatio				= 16.0f / 9.0f;

	// Display horizontal FOV relative to vertical at actual aspect ratio
	const float			fDisplayWidth			= static_cast<float>( m_DisplayWidth );
	const float			fDisplayHeight			= static_cast<float>( m_DisplayHeight );
	const float			AspectRatio				= fDisplayWidth / fDisplayHeight;

	const float			VerticalFOVRadians		= DEGREES_TO_RADIANS( VerticalFOV );
	const float			HorizontalFOVRadians	= 2.0f * ATan( AspectRatio * Tan( VerticalFOVRadians * 0.5f ) );
	const float			HorizontalFOV			= RADIANS_TO_DEGREES( HorizontalFOVRadians );
	const uint			iHorizontalFOV			= RoundToUInt( HorizontalFOV );
	const uint			iVerticalFOV			= RoundToUInt( VerticalFOV );

	STATICHASH( HorizontalFOV );
	ConfigManager::SetInt( sHorizontalFOV, iHorizontalFOV );

	STATICHASH( VerticalFOV );
	ConfigManager::SetInt( sVerticalFOV, iVerticalFOV );
}

// DLP 12 Oct 2021: Multiply by 2 because 0.5 puts the crosshair on the bottom of the screen, and that should be "100%"
void RosaFramework::PublishDisplayedVanishingPointY() const
{
	STATICHASH( VanishingPointY );
	const float	VanishingPointY		= ConfigManager::GetFloat( sVanishingPointY );

	STATICHASH( VanishingPointYPct );
	ConfigManager::SetFloat( sVanishingPointYPct, 2.0f * VanishingPointY );
}

// TODO: Parameterize these functions and move to mathcore, they're pretty useful.
inline float RosaFramework::GetMouseSpeedFromSliderValue( const float SliderValue )
{
	// TODO: Parameterize this so I can tune it.
	// This should scale neatly from 1/8 to 8 on a power curve, with 1 in the middle.
	static const float	sMouseScaleRange	= 8.0f;
	const float			AdjustedSliderValue	= ( SliderValue * 2.0f ) - 1.0f;
	const float			MouseSpeed			= Pow( sMouseScaleRange, AdjustedSliderValue );
	return MouseSpeed;
}

inline float RosaFramework::GetSliderValueFromMouseSpeed( const float MouseSpeed )
{
	static const float	sMouseScaleRange	= 8.0f;
	const float			AdjustedSliderValue	= LogBase( MouseSpeed, sMouseScaleRange );
	const float			SliderValue			= ( AdjustedSliderValue + 1.0f ) * 0.5f;
	return SliderValue;
}

inline float RosaFramework::GetControllerSpeedFromSliderValue( const float SliderValue )
{
	// TODO: Parameterize this so I can tune it.
	// This should scale neatly from 1/2 to 2 on a power curve, with 1 in the middle.
	static const float	sControllerScaleRange	= 2.0f;
	const float			AdjustedSliderValue	= ( SliderValue * 2.0f ) - 1.0f;
	const float			ControllerSpeed			= Pow( sControllerScaleRange, AdjustedSliderValue );
	return ControllerSpeed;
}

inline float RosaFramework::GetSliderValueFromControllerSpeed( const float ControllerSpeed )
{
	static const float	sControllerScaleRange	= 2.0f;
	const float			AdjustedSliderValue	= LogBase( ControllerSpeed, sControllerScaleRange );
	const float			SliderValue			= ( AdjustedSliderValue + 1.0f ) * 0.5f;
	return SliderValue;
}

inline float RosaFramework::GetBrightnessFromSliderValue( const float SliderValue )
{
	STATICHASH( Gamma_Min );
	STATICHASH( Gamma_Max );

	const float Gamma_Min = ConfigManager::GetFloat( sGamma_Min, 1.8f );
	const float Gamma_Max = ConfigManager::GetFloat( sGamma_Max, 2.6f );

	return Lerp( Gamma_Min, Gamma_Max, SliderValue );
}

inline float RosaFramework::GetSliderValueFromBrightness( const float Brightness )
{
	STATICHASH( Gamma_Min );
	STATICHASH( Gamma_Max );

	const float Gamma_Min = ConfigManager::GetFloat( sGamma_Min, 1.8f );
	const float Gamma_Max = ConfigManager::GetFloat( sGamma_Max, 2.6f );

	return InvLerp( Brightness, Gamma_Min, Gamma_Max );
}

inline float RosaFramework::GetFOVFromSliderValue( const float SliderValue )
{
	STATICHASH( FOV_Min );
	STATICHASH( FOV_Max );

	const float FOV_Min = ConfigManager::GetFloat( sFOV_Min, 60.0f );
	const float FOV_Max = ConfigManager::GetFloat( sFOV_Max, 120.0f );
	return Lerp( FOV_Min, FOV_Max, SliderValue );
}

inline float RosaFramework::GetSliderValueFromFOV( const float FOV )
{
	STATICHASH( FOV_Min );
	STATICHASH( FOV_Max );

	const float FOV_Min = ConfigManager::GetFloat( sFOV_Min, 60.0f );
	const float FOV_Max = ConfigManager::GetFloat( sFOV_Max, 120.0f );

	return InvLerp( FOV, FOV_Min, FOV_Max );
}

inline float RosaFramework::GetVanishingPointYFromSliderValue( const float SliderValue )
{
	STATICHASH( VanishingPointY_Min );
	STATICHASH( VanishingPointY_Max );

	const float VanishingPointY_Min = ConfigManager::GetFloat( sVanishingPointY_Min, 0.0f );
	const float VanishingPointY_Max = ConfigManager::GetFloat( sVanishingPointY_Max, 0.5f );
	return Lerp( VanishingPointY_Min, VanishingPointY_Max, SliderValue );
}

inline float RosaFramework::GetSliderValueFromVanishingPointY( const float VanishingPointY )
{
	STATICHASH( VanishingPointY_Min );
	STATICHASH( VanishingPointY_Max );

	const float VanishingPointY_Min = ConfigManager::GetFloat( sVanishingPointY_Min, 0.0f );
	const float VanishingPointY_Max = ConfigManager::GetFloat( sVanishingPointY_Max, 0.5f );

	return InvLerp( VanishingPointY, VanishingPointY_Min, VanishingPointY_Max );
}

inline float RosaFramework::GetLightDistanceFromSliderValue( const float SliderValue )
{
	STATICHASH( LightDistance_Min );
	STATICHASH( LightDistance_Max );

	const float LightDistance_Min = ConfigManager::GetFloat( sLightDistance_Min, 0.0f );
	const float LightDistance_Max = ConfigManager::GetFloat( sLightDistance_Max, 2.0f );
	return Lerp( LightDistance_Min, LightDistance_Max, SliderValue );
}

inline float RosaFramework::GetSliderValueFromLightDistance( const float LightDistance )
{
	STATICHASH( LightDistance_Min );
	STATICHASH( LightDistance_Max );

	const float LightDistance_Min = ConfigManager::GetFloat( sLightDistance_Min, 0.0f );
	const float LightDistance_Max = ConfigManager::GetFloat( sLightDistance_Max, 2.0f );

	return InvLerp( LightDistance, LightDistance_Min, LightDistance_Max );
}

void RosaFramework::InitializeTools()
{
#if BUILD_ROSA_TOOLS
	if( !m_Tools )
	{
		m_Tools = new RosaTools;
		m_Tools->InitializeFromDefinition( "RosaTools" );
	}
#endif
}

void RosaFramework::ShutDownWorld()
{
	XTRACE_FUNCTION;

	SafeDelete( m_World );
	SafeDelete( m_Intro );
#if BUILD_ROSA_TOOLS
	SafeDelete( m_Tools );
#endif

	m_Audio3DListener->SetWorld( NULL );

	WBWorld::DeleteInstance();
}

/*virtual*/ void RosaFramework::ShutDown()
{
	XTRACE_FUNCTION;

	// HACKHACK: Clear the UI stack so everything gets popped;
	// this fixes state problems that can occur when things
	// depend on a UI screen closing to restore some state.
	GetUIManager()->GetUIStack()->Clear();

	// Shutting down game also saves the game in progress.
	m_Game->ShutDown();

	WritePrefsConfig();

	FileStream::StaticShutDown();
	PackStream::StaticShutDown();

	WBComponent::ShutDownBaseFactories();
	RodinBTNodeFactory::ShutDown();
	WBActionFactory::ShutDown();
	WBParamEvaluatorFactory::ShutDown();
	AnimEventFactory::DeleteInstance();
	UIFactory::ShutDown();
	WBActionStack::ShutDown();
	RosaParticles::FlushConfigCache();
	SDPFactory::ShutDown();

	ShutDownWorld();

	SafeDelete( m_Game );
	SafeDelete( m_MainView );
	SafeDelete( m_FGView );
	SafeDelete( m_HUDView );
	SafeDelete( m_UpscaleView );
	SafeDelete( m_BloomViewA );
	SafeDelete( m_BloomViewB );
	SafeDelete( m_BloomViewC );
	SafeDelete( m_MinimapAView );
	SafeDelete( m_MinimapBView );
#if ROSA_USE_MAXIMAP
	SafeDelete( m_MaximapAView );
	SafeDelete( m_MaximapBView );
#endif
	SafeDelete( m_SkyView );
	SafeDelete( m_SkylineView );
	SafeDelete( m_LightView );
	SafeDelete( m_TargetManager );
	SafeDelete( m_CloudManager );
	SafeDelete( m_InputSystem );
	SafeDelete( m_Controller );
	SafeDelete( m_Audio3DListener );
	SafeDelete( m_AchievementManager );

	DynamicMeshManager::DeleteInstance();

#if BUILD_STEAM
	PRINTF( "Shutting down Steam API.\n" );
	SteamAPI_Shutdown();
#endif

	Framework3D::ShutDown();

	ReverseHash::ShutDown();
}

void RosaFramework::LoadPrefsConfig()
{
	const SimpleString PrefsConfigFilename = GetUserDataPath() + SimpleString( "prefs.cfg" );
	if( FileUtil::Exists( PrefsConfigFilename.CStr() ) )
	{
		const FileStream PrefsConfigStream = FileStream( PrefsConfigFilename.CStr(), FileStream::EFM_Read );
		ConfigManager::Load( PrefsConfigStream );
	}
}

void RosaFramework::WritePrefsConfig()
{
	const SimpleString PrefsConfigFilename = GetUserDataPath() + SimpleString( "prefs.cfg" );
	const FileStream PrefsConfigStream = FileStream( PrefsConfigFilename.CStr(), FileStream::EFM_Write );

	PrefsConfigStream.PrintF( "# This file is automatically generated.\n# You may delete it to restore defaults.\n\n" );

	ConfigManager::BeginWriting();

	ConfigManager::Write( PrefsConfigStream, "ShowTutorials" );
	ConfigManager::Write( PrefsConfigStream, "Difficulty" );
	ConfigManager::Write( PrefsConfigStream, "ShowHUD" );
	ConfigManager::Write( PrefsConfigStream, "HUDMarkers" );
	ConfigManager::Write( PrefsConfigStream, "Minimap" );
	ConfigManager::Write( PrefsConfigStream, "MinimapMarkers" );
	ConfigManager::Write( PrefsConfigStream, "Language" );
	ConfigManager::Write( PrefsConfigStream, "PauseOnLostFocus" );
	ConfigManager::Write( PrefsConfigStream, "MuteWhenUnfocused" );
	ConfigManager::Write( PrefsConfigStream, "OpenGL" );
	ConfigManager::Write( PrefsConfigStream, "DisplayWidth" );
	ConfigManager::Write( PrefsConfigStream, "DisplayHeight" );
	ConfigManager::Write( PrefsConfigStream, "Fullscreen" );
	ConfigManager::Write( PrefsConfigStream, "UpscaleFullscreen" );
	ConfigManager::Write( PrefsConfigStream, "IgnoreDisplayEnum" );
	ConfigManager::Write( PrefsConfigStream, "DisplayX" );
	ConfigManager::Write( PrefsConfigStream, "DisplayY" );
	ConfigManager::Write( PrefsConfigStream, "HUDDisplayWidth" );
	ConfigManager::Write( PrefsConfigStream, "HUDDisplayHeight" );
	ConfigManager::Write( PrefsConfigStream, "VSync" );
	ConfigManager::Write( PrefsConfigStream, "FXAA" );
	ConfigManager::Write( PrefsConfigStream, "SSAO" );
	ConfigManager::Write( PrefsConfigStream, "Bloom" );		// Shared by filmic and watercolor but only exposed in filmic
	ConfigManager::Write( PrefsConfigStream, "Brightness" );
	ConfigManager::Write( PrefsConfigStream, "FOV" );
	ConfigManager::Write( PrefsConfigStream, "VanishingPointY" );
	ConfigManager::Write( PrefsConfigStream, "LightDistance" );
	ConfigManager::Write( PrefsConfigStream, "ViewBob" );
	ConfigManager::Write( PrefsConfigStream, "ViewSway" );
	ConfigManager::Write( PrefsConfigStream, "SlideRoll" );
	ConfigManager::Write( PrefsConfigStream, "StrafeRoll" );
	ConfigManager::Write( PrefsConfigStream, "SprintFOV" );
	ConfigManager::Write( PrefsConfigStream, "HandsVelocity" );
	ConfigManager::Write( PrefsConfigStream, "LeftyMode" );
	ConfigManager::Write( PrefsConfigStream, "StylizedAnim" );
#if ROSA_USE_FILMIC_POST
	ConfigManager::Write( PrefsConfigStream, "Gradient" );
	ConfigManager::Write( PrefsConfigStream, "Edge" );
	ConfigManager::Write( PrefsConfigStream, "FilmGrain" );
	ConfigManager::Write( PrefsConfigStream, "DirtyLens" );
	ConfigManager::Write( PrefsConfigStream, "Halos" );
#endif
	ConfigManager::Write( PrefsConfigStream, "Fog" );
	ConfigManager::Write( PrefsConfigStream, "VolumeFog" );
	ConfigManager::Write( PrefsConfigStream, "ColorGrading" );
#if ROSA_USE_WATERCOLOR_POST
	ConfigManager::Write( PrefsConfigStream, "Displace" );
	ConfigManager::Write( PrefsConfigStream, "Blur" );
	ConfigManager::Write( PrefsConfigStream, "Blot" );
	ConfigManager::Write( PrefsConfigStream, "Canvas" );
	ConfigManager::Write( PrefsConfigStream, "Edge" );
#endif
	ConfigManager::Write( PrefsConfigStream, "PostCheap" );
	ConfigManager::Write( PrefsConfigStream, "MaxAnisotropy" );
	ConfigManager::Write( PrefsConfigStream, "DirectInput" );
	ConfigManager::Write( PrefsConfigStream, "MouseSpeed" );
	ConfigManager::Write( PrefsConfigStream, "ControllerSpeed" );
	ConfigManager::Write( PrefsConfigStream, "InvertY" );
	ConfigManager::Write( PrefsConfigStream, "AutoAim" );
	ConfigManager::Write( PrefsConfigStream, "MasterVolume" );
	ConfigManager::Write( PrefsConfigStream, "MusicVolume" );
	ConfigManager::Write( PrefsConfigStream, "AmbienceVolume" );

	ConfigManager::Write( PrefsConfigStream, "UseFixedFrameTime", "Framework" );

	m_InputSystem->WriteConfigBinds( PrefsConfigStream );
}

/*virtual*/ bool RosaFramework::TickSim( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;
	FRAME_PROFILE_FUNCTION;

	m_SimTickHasRequestedRenderTick = false;

#if BUILD_STEAM
	SteamAPI_RunCallbacks();
#endif

	m_CloudManager->Tick();

	// Automatically pause when we lose window focus
	if( m_PauseOnLostFocus &&
		!HasFocus() &&
		!m_UIManager->GetUIStack()->PausesGame() &&
		!m_UIManager->GetUIStack()->SeizesMouse() &&	// DLP 16 May 2020: Adding for Zeta, to fix the issue from Vamp where losing focus would bring the pause screen over shopping menus (which don't pause because we want the weapon to change behind it)
		!m_Game->IsInTitleScreen() )
	{
		TryPause();
	}

	// Auomatically pause if input device is lost
	// DLP 19 Oct 2021: I'm not sure why I'd only want to do this if the game wasn't paused;
	// I'm taking that out and making it always show the notification as long as the OK dialog
	// isn't already present.
	if( m_InputSystem->ShouldPauseDueToLostDevice() &&
		//!m_UIManager->GetUIStack()->PausesGame() &&
		!m_UIManager->GetUIStack()->IsOnStack( m_UIManager->GetOKDialog() ) )
	{
		GetUIManager()->ShowOKDialog( true, "InputDeviceLost", "", NULL );
	}

	if( m_MuteWhenUnfocused )
	{
		m_AudioSystem->SetGlobalMute( !HasFocus() );
	}

	return Framework3D::TickSim( DeltaTime );
}

/*virtual*/ bool RosaFramework::TickPaused( const float DeltaTime )
{
	m_Game->TickPaused( DeltaTime );

	return true;
}

/*virtual*/ bool RosaFramework::TickGame( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;
	FRAME_PROFILE_FUNCTION;

	// Oh this is horrible. But it works. Since InputSystem is a "game" system
	// (i.e., part of the realtime sim and not the UI, etc.), tick it here.
	// This way, we *do* update the input system on the same frame that the
	// game gets unpaused, even though we *don't* call TickInput (because we
	// want to ignore the Escape press). This whole system probably needs to
	// be reconsidered, but whatever. It gets this game finished.
	const bool WasBinding = m_InputSystem->IsBinding();
	m_InputSystem->Tick();
	if( WasBinding )
	{
		TryFinishInputBinding();
	}

	m_Game->Tick( DeltaTime );

#if BUILD_ROSA_TOOLS
	if( m_Tools->IsInToolMode() )
	{
		m_Tools->Tick( DeltaTime );
	}
	else
#endif
	{
		WBWorld::GetInstance()->Tick( DeltaTime );
		m_Intro->Tick( DeltaTime );
		m_World->Tick( DeltaTime );
	}

	return true;
}

/*virtual*/ void RosaFramework::OnUnpaused()
{
	Framework3D::OnUnpaused();

	// HACK: If we've just unpaused, tick input system again so we ignore rising inputs on the next frame.
	m_InputSystem->Tick();
}

/*virtual*/ void RosaFramework::TickDevices()
{
	XTRACE_FUNCTION;

	Framework3D::TickDevices();

	m_Controller->Tick( 0.0f );

	// If we're using either controller or mouse exclusively, allow or disallow the cursor as needed.
	const bool IsUsingController	= m_Controller->ReceivedInputThisTick();
	const bool IsUsingMouse			= m_Mouse->ReceivedInputThisTick();
	if( IsUsingController != IsUsingMouse )
	{
		m_Mouse->SetAllowCursor( IsUsingMouse );
	}
}

bool RosaFramework::CanPause() const
{
	// HACKHACK: Don't allow pausing while the game has queued events!
	// This should fix initialization bugs if the game loses focus and autopauses.
	if( m_Game && WBWorld::GetInstance()->GetEventManager()->HasQueuedEvents( m_Game ) )
	{
		return false;
	}

	// Don't allow pausing when dead
	if( !RosaGame::IsPlayerAlive() )
	{
		return false;
	}

	// Don't allow pausing for scripted reasons
	if( RosaGame::IsPlayerDisablingPause() )
	{
		return false;
	}

	// All checks passed, we can pause
	return true;
}

void RosaFramework::TryPause()
{
	XTRACE_FUNCTION;

	if( CanPause() )
	{
		UIScreen* const pPauseScreen = m_UIManager->GetScreen( "PauseScreen" );
		ASSERT( pPauseScreen );
		m_UIManager->GetUIStack()->Repush( pPauseScreen );
	}
}

/*virtual*/ bool RosaFramework::TickInput( const float DeltaTime, const bool UIHasFocus )
{
	XTRACE_FUNCTION;

	if( !Framework3D::TickInput( DeltaTime, UIHasFocus ) )
	{
		return false;
	}

	if( !UIHasFocus &&
		!m_Intro->IsRunning() &&
		( m_Keyboard->OnRise( Keyboard::EB_Escape ) || m_Controller->OnRise( XInputController::EB_Start ) ) )
	{
		TryPause();
	}

#if BUILD_ROSA_TOOLS
	// Alt+F to lock frustum, Shift+Alt+F to unlock
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_F ) )
	{
		m_Renderer->DEV_SetLockedFrustum( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift ) ? NULL : m_MainView );
	}

	if( m_Keyboard->OnRise( Keyboard::EB_Tab ) )
	{
		m_Tools->ToggleToolMode();

#if 1
		// ROSANOTE: Toggle between editor/game in same location.
		// I'm commenting this out because it doesn't work for procgen worlds
		// and it keeps screwing me up.
		if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
		{
			if( m_Tools->IsInToolMode() )
			{
				m_Tools->SetCameraTransform( RosaGame::GetPlayerLocation(), RosaGame::GetPlayerOrientation() );
			}
			else
			{
				WBEntity* const pPlayer = RosaGame::GetPlayer();
				if( pPlayer )
				{
					WBCompRosaTransform* const pTransform = pPlayer->GetTransformComponent<WBCompRosaTransform>();
					pTransform->SetLocation( m_Tools->GetCameraLocation() );
					pTransform->SetOrientation( m_Tools->GetCameraOrientation() );
				}
			}
		}
#endif
	}

	if( m_Tools->IsInToolMode() )
	{
		m_Tools->TickInput( DeltaTime );
		return true;
	}
#endif

#if ROSA_ALLOW_QUICKSAVELOAD
	STATIC_HASHED_STRING( Quicksave );
	if( m_InputSystem->OnRise( sQuicksave ) )
	{
		m_Game->GetSaveLoad()->SaveQuicksave();

		STATICHASH( Quicksaved );
		RosaHUDLog::StaticAddMessage( sQuicksaved );
	}

	STATIC_HASHED_STRING( Quickload );
	if( m_InputSystem->OnRise( sQuickload ) )
	{
		m_Game->RequestLoadSlot( "Quicksave" );
	}
#endif

#if ROSA_USE_MAXIMAP
	STATIC_HASHED_STRING( Maximap );
	if( m_InputSystem->OnRise( sMaximap ) )
	{
		WB_MAKE_EVENT( ToggleMaximap, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), ToggleMaximap, NULL );
	}
#endif

#if BUILD_DEV

	// TEMP until porting
	// Alt + V: Validate path functions
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_V ) )
	{
		Array<SimpleString> LocalFiles;
		FileUtil::GetFilesInFolder( ".", false, "", LocalFiles );

		Array<SimpleString> ScreensFiles;
		FileUtil::GetFilesInFolder( "Screens", true, "", ScreensFiles );

		Array<SimpleString> AllFiles;
		FileUtil::GetFilesInFolder( ".", true, "", AllFiles );

		Array<SimpleString> TgaFiles;
		FileUtil::GetFilesInFolder( ".", true, ".tga", TgaFiles );

		Array<SimpleString> RosaMissionFiles;
		PackStream::StaticGetFilesInFolder_Master( ".", true, ".rosamission", RosaMissionFiles );

		// HACKHACK: Also redownload cloud stuff why not
		m_CloudManager->DownloadPackages();
	}

	// Alt + Q: Start/stop reverb test mode
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_Q ) )
	{
		m_AudioSystem->ReverbTest_Toggle();
	}

	if( m_AudioSystem->ReverbTest_IsActive() )
	{
		if( m_Keyboard->OnRise( Keyboard::EB_Up ) )
		{
			m_AudioSystem->ReverbTest_PrevSetting();
		}

		if( m_Keyboard->OnRise( Keyboard::EB_Down ) )
		{
			m_AudioSystem->ReverbTest_NextSetting();
		}

		if( m_Keyboard->OnRise( Keyboard::EB_Left ) )
		{
			const float Scalar = m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift ) ? 10.0f : 1.0f;
			m_AudioSystem->ReverbTest_DecrementSetting( Scalar );
		}

		if( m_Keyboard->OnRise( Keyboard::EB_Right ) )
		{
			const float Scalar = m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift ) ? 10.0f : 1.0f;
			m_AudioSystem->ReverbTest_IncrementSetting( Scalar );
		}
	}

	// Ctrl + R: reveal minimap
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Control ) && m_Keyboard->OnRise( Keyboard::EB_R ) )
	{
		m_World->RevealMinimap();
	}

	// Alt + R: Report
	// Alt + Shift + R: Skip world report
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_R ) )
	{
		const bool SkipFullReport = m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift );
		if( SkipFullReport )
		{
			PRINTF( "\n==== REPORT (player only) ====\n\n" );
			// Only report on player
			RosaGame::GetPlayer()->Report();
		}
		else
		{
			PRINTF( "\n==== REPORT (full) ====\n\n" );
			WBWorld::GetInstance()->Report();
		}

		ReverseHash::ReportSize();

		StringManager::Report();

		// Report global stats (obtained when the game was launched)
		if( m_AchievementManager )
		{
			STATICHASH( RosaSteamStats );

			PRINTF( "Steam global stats:\n" );
			STATICHASH( NumStats );
			const uint NumStats = ConfigManager::GetInt( sNumStats, 0, sRosaSteamStats );
			for( uint StatIndex = 0; StatIndex < NumStats; ++StatIndex )
			{
				const SimpleString StatTag = ConfigManager::GetSequenceString( "StatTag%d", StatIndex, "", sRosaSteamStats );
				m_AchievementManager->ReportGlobalStat( StatTag );
			}

			PRINTF( "Steam global achievement rates:\n" );
			STATICHASH( NumAchievements );
			const uint NumAchievements = ConfigManager::GetInt( sNumAchievements, 0, sRosaSteamStats );
			for( uint AchievementIndex = 0; AchievementIndex < NumAchievements; ++AchievementIndex )
			{
				const SimpleString AchievementTag = ConfigManager::GetSequenceString( "AchievementTag%d", AchievementIndex, "", sRosaSteamStats );
				m_AchievementManager->ReportGlobalAchievementRate( AchievementTag );
			}

			// Request updated stats for the next report
			m_AchievementManager->RequestServerUpdate();
		}

		// Count collectibles
		const Array<WBCompRosaCollectible*>* pCollectibleComponents = WBComponentArrays::GetComponents<WBCompRosaCollectible>();
		PRINTF( "%d collectibles\n", pCollectibleComponents ? pCollectibleComponents->Size() : 0 );

		m_Game->GetSaveLoad()->Report();
		m_Game->GetPersistence()->Report();
		m_Game->GetCampaign()->Report();

		PRINTF( "Difficulty sync check: %d (Menu: %d Game: %d)\n", RosaDifficulty::CheckSync(), RosaDifficulty::GetMenuDifficulty(), RosaDifficulty::GetGameDifficulty() );

		PRINTF( "Num visible sectors:   %d\n", m_World->GetNumVisibleSectors() );
		PRINTF( "Num visible geomeshes: %d\n", m_World->CountVisibleGeoMeshes() );

		const IRenderer::SDEV_RenderStats& RenderStats = m_Renderer->DEV_GetStats();
		PRINTF( "Rendered meshes:        %d\n", RenderStats.m_NumMeshes );
		PRINTF( "Rendered draw calls:    %d\n", RenderStats.m_NumDrawCalls );
		PRINTF( "Rendered primitives:    %d\n", RenderStats.m_NumPrimitives );
		PRINTF( "Rendered shadow lights: %d\n", RenderStats.m_NumShadowLights );
		PRINTF( "Rendered shadow meshes: %d\n", RenderStats.m_NumShadowMeshes );

#if BUILD_WINDOWS_NO_SDL
		if( !m_Renderer->IsOpenGL() )
		{
			static_cast<D3D9Renderer*>( m_Renderer )->TestDeviceCapabilities();
		}
#endif

		PackStream::ReportUnusedPackageFiles();
	}

	// Alt + A: reset stats and achievements
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_A ) )
	{
		if( m_AchievementManager )
		{
			const bool ResetAchievements = true;
			m_AchievementManager->ResetAllStats( ResetAchievements );
		}
	}

	// Alt + C: hotload config files
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_C ) )
	{
		InitializePackagesAndConfig();
	}

	// Alt + P: reset persistence
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_P ) )
	{
		m_Game->GetPersistence()->Reset();
	}

	// Alt + U resets UI, mainly so we can enter tools mode on title screen
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_U ) )
	{
		m_UIManager->ResetToGameScreens();
	}

	// Ctrl + Alt + Backspace: Invoke crash
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->IsHigh( Keyboard::EB_Virtual_Control ) && m_Keyboard->OnRise( Keyboard::EB_Backspace ) )
	{
		WBEntity* const pEntity = NULL;
		pEntity->Tick( 0.0f );
	}

	// Shift + Alt + Backspace: Invoke crash by allocating all the memory
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift ) && m_Keyboard->OnRise( Keyboard::EB_Backspace ) )
	{
		for(;;)
		{
			byte* pArray = new byte[ 32 ];
			pArray[0] = pArray[31];
		}
	}

	// Alt + Backspace: Gather worldgen stats
	if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && m_Keyboard->OnRise( Keyboard::EB_Backspace ) )
	{
		m_World->GatherStats();
	}

	// T regenerates current world
	// Shift + T (no Alt, no Ctrl) is the same as T but repeats while held, useful mainly for rapidly testing procgen.
	// Ctrl + T regenerates current world and teleports player back to current location
	// Alt + T is a travel to next level.
	// Shift + Alt + T is a travel to prev level.
	if( m_Keyboard->OnRise( Keyboard::EB_T ) )
	{
		if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Alt ) )
		{
			if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift ) )
			{
				WB_MAKE_EVENT( GoToPrevLevel, NULL );
				WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), GoToPrevLevel, m_Game );
			}
			else
			{
				WB_MAKE_EVENT( GoToNextLevel, NULL );
				WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), GoToNextLevel, m_Game );
			}
		}
		else	// EB_Virtual_Alt is low
		{
#define ALLOWREGENINPLACE 1
#if ALLOWREGENINPLACE
			const Vector OldPlayerLocation		= RosaGame::GetPlayerLocation();
			const Angles OldPlayerOrientation	= RosaGame::GetPlayerOrientation();
#endif

			RegenerateWorld();

#if ALLOWREGENINPLACE
			if( m_Keyboard->IsHigh( Keyboard::EB_Virtual_Control ) )
			{
				WBEntity* const	pPlayer = RosaGame::GetPlayer();

				WB_MAKE_EVENT( SetLocation, pPlayer );
				WB_SET_AUTO( SetLocation, Vector, NewLocation, OldPlayerLocation );
				WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetLocation, pPlayer );

				WB_MAKE_EVENT( SetOrientation, pPlayer );
				WB_SET_AUTO( SetOrientation, Angles, NewOrientation, OldPlayerOrientation );
				WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetOrientation, pPlayer );
			}
#endif
		}
	}
	else if( m_Keyboard->IsHigh( Keyboard::EB_T ) &&
			 m_Keyboard->IsHigh( Keyboard::EB_Virtual_Shift) &&
			 m_Keyboard->IsLow( Keyboard::EB_Virtual_Control) &&
			 m_Keyboard->IsLow( Keyboard::EB_Virtual_Alt) )
	{
		RegenerateWorld();
	}
#endif	// BUILD_DEV

	return true;
}

void RosaFramework::TryFinishInputBinding()
{
	// HACK: If we finished or canceled binding, pop the bind dialog.
	if( !m_InputSystem->IsBinding() )
	{
		STATIC_HASHED_STRING( BindDialog );
		if( m_UIManager->GetUIStack()->Top() == m_UIManager->GetScreen( sBindDialog ) )
		{
			m_UIManager->GetUIStack()->Pop();
		}
	}
}

/*virtual*/ void RosaFramework::TickPausedInput( const float DeltaTime )
{
	XTRACE_FUNCTION;

	Framework3D::TickPausedInput( DeltaTime );

	if( m_InputSystem->IsBinding() )
	{
		m_InputSystem->Tick();
		TryFinishInputBinding();
	}
	else
	{
		m_InputSystem->UpdateIsUsingControllerExclusively();
	}
}

/*virtual*/ void RosaFramework::TickRender()
{
	XTRACE_FUNCTION;

	m_Game->Render();

#if BUILD_ROSA_TOOLS
	if( m_Tools->IsInToolMode() )
	{
		m_Tools->TickRender();
	}
	else
#endif
	{
		// ROSANOTE: For Rosa, it's important to render the RosaWorld before
		// the WBWorld (and entities), because they depend on the visible
		// sectors array that RosaWorld::Render() builds.
		m_World->Render();

#if BUILD_DEV
		m_World->DebugRender();
#endif

		WBWorld::GetInstance()->Render();

#if BUILD_DEV
		WBWorld::GetInstance()->DebugRender();
#endif
	}

#if BUILD_DEV
	if( m_Renderer->DEV_IsLockedFrustum() )
	{
		const Frustum& LockedFrustum = m_Renderer->DEV_GetLockedFrustum();
		m_Renderer->DEBUGDrawFrustum( LockedFrustum, ARGB_TO_COLOR( 255, 0, 255, 255 ) );
	}
#endif

	Framework3D::TickRender();
}

void RosaFramework::SetMainViewTransform( const Vector& Location, const Angles& Orientation )
{
	m_MainView->SetLocation( Location );
	m_MainView->SetRotation( Orientation );

	m_FGView->SetLocation( Location );
	m_FGView->SetRotation( Orientation );

	m_SkyView->SetRotation( Orientation );

	m_SkylineView->SetLocation( m_World->GetSkylineViewScalar() * Location );
	m_SkylineView->SetRotation( Orientation );

	m_MinimapAView->SetLocation( Vector( Location.x, Location.y, Location.z + m_MinimapViewHeight ) + ( Orientation.ToVector2D() * ( 0.5f * m_MinimapViewExtent * m_MinimapViewOffset ) ) );
	m_MinimapAView->SetRotation( Angles( m_MinimapAView->GetRotation().Pitch, 0.0f, Orientation.Yaw ) );

#if ROSA_USE_MAXIMAP
	m_MaximapAView->SetLocation( Vector( Location.x, Location.y, Location.z + m_MinimapViewHeight ) + ( Orientation.ToVector2D() * ( 0.5f * m_MaximapViewExtent * m_MaximapViewOffset ) ) );
	m_MaximapAView->SetRotation( Angles( m_MaximapAView->GetRotation().Pitch, 0.0f, Orientation.Yaw ) );
#endif

	m_Audio3DListener->SetLocation( Location );
	m_Audio3DListener->SetRotation( Orientation );
}

void RosaFramework::SetMinimapViewExtent( const float MinimapViewExtent )
{
	DEVASSERT( m_MinimapAView );

	m_MinimapViewExtent			= MinimapViewExtent;

	const float	HalfExtent		= 0.5f * MinimapViewExtent;
	const SRect	MinimapBoundsA	= SRect( -HalfExtent, HalfExtent, HalfExtent, -HalfExtent );

	m_MinimapAView->SetOrthoBounds( MinimapBoundsA );
}

#if ROSA_USE_MAXIMAP
void RosaFramework::SetMaximapViewExtent( const float MaximapViewExtent )
{
	DEVASSERT( m_MaximapAView );

	m_MaximapViewExtent			= MaximapViewExtent;

	const float	HalfExtent		= 0.5f * MaximapViewExtent;
	const SRect	MaximapBoundsA	= SRect( -HalfExtent, HalfExtent, HalfExtent, -HalfExtent );

	m_MaximapAView->SetOrthoBounds( MaximapBoundsA );
}
#endif

void RosaFramework::SetFOV( const float FOV )
{
	DEVASSERT( m_MainView );
	m_MainView->SetFOV( FOV );

	DEVASSERT( m_SkyView );
	m_SkyView->SetFOV( FOV );

	DEVASSERT( m_SkylineView );
	m_SkylineView->SetFOV( FOV );
}

void RosaFramework::SetFGFOV( const float FGFOV )
{
	DEVASSERT( m_FGView );
	m_FGView->SetFOV( FGFOV );
}

void RosaFramework::SetVanishingPointY( const float VanishingPointY )
{
	DEVASSERT( m_MainView );
	m_MainView->SetVanishingPointY( VanishingPointY );

	DEVASSERT( m_FGView );
	m_FGView->SetVanishingPointY( VanishingPointY );

	DEVASSERT( m_SkyView );
	m_SkyView->SetVanishingPointY( VanishingPointY );

	DEVASSERT( m_SkylineView );
	m_SkylineView->SetVanishingPointY( VanishingPointY );
}

void RosaFramework::PrepareForLoad( const bool ForceResetToGameScreens )
{
	XTRACE_FUNCTION;

	m_InputSystem->PopAllContexts();
	m_Clock->ClearMultiplierRequests();

	// HACK: Before load (and after popping all contexts), tick input system again
	// so we ignore rising inputs on the first new frame.
	m_InputSystem->Tick();

	static bool FirstLoad = true;
	if( FirstLoad )
	{
		FirstLoad = false;
	}
	else
	{
		if( !m_Game->IsInTitleScreen() || ForceResetToGameScreens )
		{
			m_UIManager->ResetToGameScreens();
		}
		else
		{
			m_UIManager->ResetToInitialScreens();
		}

		m_Game->GetHUDLog()->Clear();
		m_Game->GetLockpicking()->Reset();
	}
}

/*virtual*/ void RosaFramework::ToggleFullscreen()
{
	PRINTF( "RosaFramework::ToggleFullscreen\n" );

	Framework3D::ToggleFullscreen();

	// For fullscreen upscaling, we may need a new m_UpscaleView
	UpdateViews();

	ASSERT( m_Game );
	m_Game->RefreshRTDependentSystems();

	// Recreate buckets because we have new views and maybe a new render path
	CreateBuckets();
}

/*virtual*/ void RosaFramework::ToggleVSync()
{
	PRINTF( "RosaFramework::ToggleVSync\n" );

	Framework3D::ToggleVSync();

	ASSERT( m_Game );
	m_Game->RefreshRTDependentSystems();

	// Recreate buckets because we have new render targets
	CreateBuckets();
}

/*virtual*/ void RosaFramework::SetResolution( const uint DisplayWidth, const uint DisplayHeight )
{
	PRINTF( "RosaFramework::SetResolution\n" );

	m_DisplayWidth	= DisplayWidth;
	m_DisplayHeight	= DisplayHeight;

	Framework3D::SetResolution( DisplayWidth, DisplayHeight );

	m_TargetManager->CreateTargets( DisplayWidth, DisplayHeight );

	UpdateViews();
	PublishDisplayedFOV();

	ASSERT( m_Game );
	m_Game->RefreshRTDependentSystems();

	// Recreate buckets because we have new render targets
	CreateBuckets();

	// Notify anyone else who cares that the resolution has changed
	WB_MAKE_EVENT( OnSetRes, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnSetRes, NULL );
}

/*virtual*/ void RosaFramework::ToggleFXAA()
{
	STATICHASH( FXAA );
	const bool FXAA = !ConfigManager::GetBool( sFXAA );
	ConfigManager::SetBool( sFXAA, FXAA );

	PRINTF( "RosaFramework::ToggleFXAA %s\n", FXAA ? "true" : "false" );

	// Recreate buckets because we have a new render path
	CreateBuckets();
}

/*virtual*/ void RosaFramework::ToggleSSAO()
{
	STATICHASH( SSAO );
	const bool SSAO = !ConfigManager::GetBool( sSSAO );
	ConfigManager::SetBool( sSSAO, SSAO );

	PRINTF( "RosaFramework::ToggleSSAO %s\n", SSAO ? "true" : "false" );

	// Recreate buckets because we have a new render path
	CreateBuckets();
}

/*virtual*/ void RosaFramework::ToggleBloom()
{
	STATICHASH( Bloom );
	const bool Bloom = !ConfigManager::GetBool( sBloom );
	ConfigManager::SetBool( sBloom, Bloom );

	PRINTF( "RosaFramework::ToggleBloom %s\n", Bloom ? "true" : "false" );

	// Disable other settings to make it clear which ones are dependent on bloom
	STATICHASH( DirtyLens );
	ConfigManager::SetBool( sDirtyLens, Bloom );

	STATICHASH( Halos );
	ConfigManager::SetBool( sHalos, Bloom );

	DEVASSERT( m_Game );
	m_Game->UpdateBloomEnabled();
	m_Game->UpdateDirtyLensEnabled();
	m_Game->UpdateHalosEnabled();

	// Recreate buckets because we have a new render path
	CreateBuckets();
}

void RosaFramework::CreateBuckets()
{
	PRINTF( "RosaFramework::CreateBuckets\n" );

	STATICHASH( FXAA );
	const bool FXAA = ConfigManager::GetBool( sFXAA );

	STATICHASH( SSAO );
	const bool SSAO = ConfigManager::GetBool( sSSAO );

	STATICHASH( Bloom );
	const bool Bloom = ConfigManager::GetBool( sBloom );

	STATICHASH( VolumeFog );
	const bool VolumeFog = ConfigManager::GetBool( sVolumeFog );

	STATICHASH( Gradient );
	const bool Gradient = ConfigManager::GetBool( sGradient );

	STATICHASH( Edge );
	const bool Edge = ConfigManager::GetBool( sEdge );

#if ROSA_USE_WATERCOLOR_POST
	STATICHASH( Edge );
	const bool Edge = ConfigManager::GetBool( sEdge );
#endif

	IRenderTarget* const pScrnRT = m_TargetManager->GetRenderTarget( "Original" );
	IRenderTarget* const pMainRT = m_TargetManager->GetRenderTarget( "Primary" );
	IRenderTarget* const pMnwZRT = m_TargetManager->GetRenderTarget( "PrimaryDepth" );
	IRenderTarget* const pDclsRT = m_TargetManager->GetRenderTarget( "Decals" );
	IRenderTarget* const pShdwRT = m_TargetManager->GetRenderTarget( "ShadowCube" );
	IRenderTarget* const pBlAHRT = m_TargetManager->GetRenderTarget( "BloomAH" );
	IRenderTarget* const pBlAVRT = m_TargetManager->GetRenderTarget( "BloomAV" );
	IRenderTarget* const pBlBHRT = m_TargetManager->GetRenderTarget( "BloomBH" );
	IRenderTarget* const pBlBVRT = m_TargetManager->GetRenderTarget( "BloomBV" );
	IRenderTarget* const pBlCHRT = m_TargetManager->GetRenderTarget( "BloomCH" );
	IRenderTarget* const pBlCVRT = m_TargetManager->GetRenderTarget( "BloomCV" );
	IRenderTarget* const pGBufRT = m_TargetManager->GetRenderTarget( "GBuffer" );
#if ROSA_USE_WATERCOLOR_POST
	IRenderTarget* const pGB_ART = m_TargetManager->GetRenderTarget( "GB_Albedo" );
#endif
	IRenderTarget* const pLAccRT = m_TargetManager->GetRenderTarget( "GB_LAccum" );
	IRenderTarget* const pMapART = m_TargetManager->GetRenderTarget( "MinimapA" );
	IRenderTarget* const pMapBRT = m_TargetManager->GetRenderTarget( "MinimapB" );
#if ROSA_USE_MAXIMAP
	IRenderTarget* const pMxmART = m_TargetManager->GetRenderTarget( "MaximapA" );
	IRenderTarget* const pMxmBRT = m_TargetManager->GetRenderTarget( "MaximapB" );
#endif
	IRenderTarget* const p_UI_RT = m_Display->ShouldUpscaleFullscreen() ? m_TargetManager->GetRenderTarget( "UI" ) : pScrnRT;	// If we're not upscaling, we can just render UI to the main backbuffer
	IRenderTarget* const pPostRT = FXAA ? m_TargetManager->GetRenderTarget( "Post" ) : p_UI_RT;									// If we're not antialiasing, we can just render post to the UI target

	STATIC_HASHED_STRING( Main );
	STATIC_HASHED_STRING( Tools );
	STATIC_HASHED_STRING( Post );
	STATIC_HASHED_STRING( UI );

	Unused( pMnwZRT );

	m_Renderer->FreeBuckets();

#define ADDBK m_Renderer->AddBucket
#define BK new Bucket
#if BUILD_DEV
	#define DEVBK ADDBK
#else
	#define DEVBK( ... ) DoNothing
#endif

								// View			// RT		// Flags				// Filter				// Tag	// Frus	// Excl	// Clear
	ADDBK( "MainFG",		BK( m_FGView,		pGBufRT,	MAT_FOREGROUND,			MAT_ALPHA,				sMain,	true,	true,	CLEAR_DEPTH|CLEAR_STENCIL ) );	// ROSANOTE: Draw foreground stuff first to reduce overdraw.
	ADDBK( "Main",			BK( m_MainView,		NULL,		MAT_WORLD,				MAT_ALPHA|MAT_DYNAMIC,	sMain,	true,	false ) );
	ADDBK( "DynDecals",		BK( NULL,			NULL,		MAT_DECALS,				MAT_ALPHA,				sMain,	true,	false ) );					// Render entities which should receive decals
	ADDBK( "Decals",		BK( NULL,			pDclsRT,	MAT_PRESCRIBED,			MAT_NONE,				sMain,	true,	true ) );					// Render decals before dynamic objects. Foreground should remain stenciled out.
	ADDBK( "MainDynamic",	BK( NULL,			pGBufRT,	MAT_WORLD|MAT_DYNAMIC,	MAT_ALPHA|MAT_DECALS,	sMain,	true,	false ) );
	ADDBK( "SkyGBuffer",	BK( m_SkyView,		NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	false,	true ) );					// Now we render sky into g-buffer *after* everything else to reduce overdraw
	ADDBK( "Skyline",		BK( m_SkylineView,	NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	false,	true ) );
	ADDBK( "GlobalAmbient",	BK( m_HUDView,		pLAccRT,	MAT_PRESCRIBED,			MAT_NONE,				sMain,	false,	true,	CLEAR_COLOR ) );	// Render the default ambient light first, then draw other ambience regions over it
	ADDBK( "AmbientLights",	BK( m_MainView,		NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	true,	true ) );					// No need to clear stencil in this bucket; each mesh's material does it
	ADDBK( "SSAO",			BK( m_HUDView,		NULL,		MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );							// It's not actually Post, but it's tagged that way so it doesn't get toggled with Main
	ADDBK( "AntiLights",	BK( NULL,			NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	true ) );							// Moved so anti-lights only multiply ambient, not direct lighting (making them a cheapo AO)
	ADDBK( "Lights",		BK( m_MainView,		NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	true ) );							// No need to clear stencil in this bucket; each mesh's material does it
	ADDBK( "ShadowLights",	BK( NULL,			NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	true ) );							// No need to clear stencil in this bucket; each mesh's material does it
	  ADDBK( "ShadowCasts",	BK( m_LightView,	pShdwRT,	MAT_SHADOW,				MAT_NONE,				sMain,	true,	false,	CLEAR_COLOR|CLEAR_DEPTH, 0xffffffff ) );	// This bucket is parented to the prior by a hack
	ADDBK( "SkyLight",		BK( m_SkyView,		NULL,		MAT_PRESCRIBED,			MAT_NONE,				sMain,	false ) );							// Separately, render sky directly into LAccum, because it needs HDR.
	ADDBK( "LightCombine",	BK( m_HUDView,		pMainRT,	MAT_PRESCRIBED,			MAT_NONE,				sMain,	false ) );

	// For Zeta, do the Deos-style gradient+edge thing
	ADDBK( "Gradient",		BK( NULL,			NULL,		MAT_PRESCRIBED,			MAT_NONE,				sPost,	false,	true ) );
	ADDBK( "Edge",			BK( NULL,			NULL,		MAT_PRESCRIBED,			MAT_NONE,				sPost,	false,	true ) );

	ADDBK( "FogMeshes",		BK( m_MainView,		NULL,		MAT_FOG,				MAT_NONE,				sPost,	false ) );							// Non-exclusive so it falls through to EdgeFog
	ADDBK( "FogLights",		BK( NULL,			NULL,		MAT_FOGLIGHTS,			MAT_NONE,				sPost,	false ) );							// No need to clear anything in these buckets; each mesh's material does it (uses Post tag so tools doesn't reenable it)
	ADDBK( "InWorldHUD",	BK( m_HUDView,		pMainRT,	MAT_INWORLDHUD,			MAT_NONE,				sUI,	true,	true ) );					// "In-world" HUD gets bloom and other post (including FXAA) on it; it is not affected by exposure

	// Debug forward pass
	DEVBK( "DevMainFwdDbg",	BK( m_MainView,		pMnwZRT,	MAT_DEBUG_WORLD,		MAT_NONE,				sMain,	true,	false ) );					// Non-exclusive so debug meshes also fall into tools bucket if enabled

	// Separate forward pass for tools
	DEVBK( "DevToolsFwd",	BK( m_MainView,		pMnwZRT,	MAT_PRESCRIBED,			MAT_NONE,				sTools,	true,	true,	CLEAR_COLOR|CLEAR_DEPTH ) );
	DEVBK( "DevToolsNoZ",	BK( NULL,			NULL,		MAT_PRESCRIBED,			MAT_NONE,				sTools,	true,	true ) );
	DEVBK( "DevToolsDbg",	BK( NULL,			NULL,		MAT_DEBUG_WORLD,		MAT_NONE,				sTools,	true,	true ) );

	// Bloom
	if( !Bloom )
	{
		// Proxy bloom bucket to clear the buffer if we've got bloom disabled
		ADDBK( "BloomAOff",	BK( m_BloomViewA,	pBlAVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false,	true,	CLEAR_COLOR ) );
		// Also clear the bottom bloom level because that's what DirtyLens/Halos sample
		ADDBK( "BloomCOff",	BK( m_BloomViewC,	pBlCVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false,	true,	CLEAR_COLOR ) );
	}
	// Downsample
	ADDBK( "BloomA",		BK( m_BloomViewA,	pBlAVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomB",		BK( m_BloomViewB,	pBlBVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomC",		BK( m_BloomViewC,	pBlCVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	// Blur and sum back up
	ADDBK( "BloomCH",		BK( m_BloomViewC,	pBlCHRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomCV",		BK( NULL,			pBlCVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomBH",		BK( m_BloomViewB,	pBlBHRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomBV",		BK( NULL,			pBlBVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomAH",		BK( m_BloomViewA,	pBlAHRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "BloomAV",		BK( NULL,			pBlAVRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );

#if ROSA_USE_WATERCOLOR_POST
	ADDBK( "Edge",			BK( m_HUDView,		pGB_ART,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false,	true ) );					// Putting edges here makes them not "in-world" (they do not go in the bloom mips)
	ADDBK( "EdgeFog",		BK( m_MainView,		NULL,		MAT_FOG,				MAT_NONE,				sPost,	true ) );							// mask edges by fog meshes (using material overrides)
	ADDBK( "EdgeFogLights",	BK( NULL,			NULL,		MAT_FOGLIGHTS,			MAT_NONE,				sPost,	true ) );							// mask edges by fog lights (using material overrides)
#endif

#if ROSA_USE_MAXIMAP
	const bool MinimapExclusive = false;
#else
	const bool MinimapExclusive = true;
#endif
	ADDBK( "MinimapA",		BK( m_MinimapAView,	pMapART,	MAT_MINIMAP,			MAT_NONE,				sUI,	false,	MinimapExclusive,	CLEAR_COLOR|CLEAR_DEPTH ) );
	ADDBK( "MinimapB",		BK( m_MinimapBView,	pMapBRT,	MAT_PRESCRIBED,			MAT_NONE,				sUI,	false,	true ) );
	ADDBK( "MinimapFXAA",	BK( NULL,			pMapART,	MAT_PRESCRIBED,			MAT_NONE,				sUI,	false,	true ) );
	ADDBK( "MinimapC",		BK( m_MinimapAView,	NULL,		MAT_MINIMAPMARKER,		MAT_NONE,				sUI,	false,	MinimapExclusive ) );	// This is minimap markers

#if ROSA_USE_MAXIMAP
	ADDBK( "MaximapA",		BK( m_MaximapAView,	pMxmART,	MAT_MINIMAP,			MAT_NONE,				sUI,	false,	true,	CLEAR_COLOR|CLEAR_DEPTH ) );
	ADDBK( "MaximapB",		BK( m_MaximapBView,	pMxmBRT,	MAT_PRESCRIBED,			MAT_NONE,				sUI,	false,	true ) );
	ADDBK( "MaximapFXAA",	BK( NULL,			pMxmART,	MAT_PRESCRIBED,			MAT_NONE,				sUI,	false,	true ) );
	ADDBK( "MaximapC",		BK( m_MaximapAView,	NULL,		MAT_MINIMAPMARKER,		MAT_NONE,				sUI,	false,	true ) );	// This is maximap markers
#endif

	ADDBK( "Post",			BK( m_HUDView,		pPostRT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );
	ADDBK( "PostTools",		BK( NULL,			NULL,		MAT_PRESCRIBED,			MAT_NONE,				sTools,	false ) );
	ADDBK( "FXAA",			BK( NULL,			p_UI_RT,	MAT_PRESCRIBED,			MAT_NONE,				sPost,	false ) );

	ADDBK( "HUD",			BK( NULL,			NULL,		MAT_HUD,				MAT_NONE,				sUI,	false,	true ) );
	DEVBK( "HUDDebug",		BK( NULL,			NULL,		MAT_DEBUG_HUD,			MAT_NONE,				sUI,	false,	true ) );

	// Final bounce from composited image to backbuffer, if needed
	ADDBK( "Upscale",		BK( m_UpscaleView,	pScrnRT,	MAT_PRESCRIBED,			MAT_NONE,				sUI,	false,	true,	CLEAR_COLOR ) );	// Clear color so we get black bars when aspect doesn't match

#undef DEVBK
#undef ADDBK
#undef BK

	// Get buckets by name and set properties here because I didn't want to change that horrid bucket constructor
	m_Renderer->GetBucket( "SSAO" )->m_Enabled					= SSAO;
	m_Renderer->GetBucket( "FXAA" )->m_Enabled					= FXAA;
	//m_Renderer->GetBucket( "MinimapFXAA" )->m_Enabled			= FXAA;	// Minimap is currently always antialiased, I'd have to change some stuff to make that work and... there's really no point?
#if ROSA_USE_MAXIMAP
	//m_Renderer->GetBucket( "MaximapFXAA" )->m_Enabled			= FXAA;	// Maximap is currently always antialiased, I'd have to change some stuff to make that work and... there's really no point?
#endif

	m_Renderer->GetBucket( "Gradient" )->m_Enabled				= Gradient;
	m_Renderer->GetBucket( "Edge" )->m_Enabled					= Edge;

	m_Renderer->GetBucket( "ShadowLights" )->m_IsShadowMaster	= true;
	m_Renderer->GetBucket( "ShadowCasts" )->SetMaterialOverridesDefinition( "RosaShadowMaterialOverrides" );

	// NOTE: Unlike FogMeshes, FogLights doesn't need to be sorted because they render additively.
	m_Renderer->GetBucket( "FogMeshes" )->m_Enabled				= VolumeFog;
	m_Renderer->GetBucket( "FogMeshes" )->m_AlphaSortMeshes		= true;
	m_Renderer->GetBucket( "FogLights" )->m_Enabled				= VolumeFog;
#if ROSA_USE_WATERCOLOR_POST
	m_Renderer->GetBucket( "EdgeFog" )->m_Enabled				= VolumeFog && Edge;
	m_Renderer->GetBucket( "EdgeFog" )->m_AlphaSortMeshes		= true;
	m_Renderer->GetBucket( "EdgeFog" )->SetMaterialOverridesDefinition( "RosaEdgeFogMaterialOverrides" );
	m_Renderer->GetBucket( "EdgeFogLights" )->m_Enabled			= VolumeFog && Edge;
	m_Renderer->GetBucket( "EdgeFogLights" )->SetMaterialOverridesDefinition( "RosaEdgeFogMaterialOverrides" );
#endif

	m_Renderer->GetBucket( "BloomA" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomB" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomC" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomCH" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomCV" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomBH" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomBV" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomAH" )->m_Enabled				= Bloom;
	m_Renderer->GetBucket( "BloomAV" )->m_Enabled				= Bloom;

#if ROSA_USE_WATERCOLOR_POST
	m_Renderer->GetBucket( "Edge" )->m_Enabled					= Edge;
#endif

	m_Renderer->GetBucket( "Upscale" )->m_Enabled				= m_Display->ShouldUpscaleFullscreen();

	// ROSANOTE: I disabled this during Vamp because sorting was taking too much time in Suburbs. Apparently not worthwhile but could revisit.
	// DLP 16 Oct 2021: Geomeshes are now sorted within their sectors, which isn't exactly the same but only needs to be done once, not each frame.
	//m_Renderer->GetBucket( "Main" )->m_SortByMaterial			= true;
	//m_Renderer->GetBucket( "MainDynamic" )->m_SortByMaterial	= true;
	m_Renderer->GetBucket( "SkyGBuffer" )->m_DepthMin			= 0.999f;
	m_Renderer->GetBucket( "Skyline" )->m_DepthMin				= 0.999f;
	m_Renderer->GetBucket( "SkyLight" )->m_DepthMin				= 0.999f;

#if BUILD_ROSA_TOOLS
	// Tools uses a forward rendering pass
	m_Renderer->SetBucketsEnabled( ( m_Tools && m_Tools->IsInToolMode() ) ? sMain : sTools, false );
#else
	m_Renderer->SetBucketsEnabled( sTools, false );
#endif
}

void RosaFramework::UpdateViews()
{
	PRINTF( "RosaFramework::UpdateViews\n" );

	const float		fDisplayWidth	= static_cast<float>( m_DisplayWidth );
	const float		fDisplayHeight	= static_cast<float>( m_DisplayHeight );
	const float		AspectRatio		= fDisplayWidth / fDisplayHeight;

	m_MainView->SetAspectRatio(		AspectRatio );
	m_FGView->SetAspectRatio(		AspectRatio );
	m_SkyView->SetAspectRatio(		AspectRatio );
	m_SkylineView->SetAspectRatio(	AspectRatio );

	// Recreate HUD view because it is dependent on display dimensions.
	// Some other views may not need to be rebuilt.
	CreateHUDView();
	CreateMinimapViews();	// This is now dependent on RT widget dimensions and needs to be rebuilt too
}

void RosaFramework::CreateHUDView()
{
	PRINTF( "RosaFramework::CreateHUDView\n" );

	SafeDelete( m_HUDView );
	SafeDelete( m_UpscaleView );
	SafeDelete( m_BloomViewA );
	SafeDelete( m_BloomViewB );
	SafeDelete( m_BloomViewC );

	const Vector	EyePosition		= Vector( 0.0f, -1.0f, 0.0f );
	const Angles	EyeOrientation	= Angles( 0.0f, 0.0f, 0.0f );
	const float		NearClip		= 0.01f;
	const float		FarClip			= 2.0f;

	const float		fDisplayWidth	= static_cast<float>( m_DisplayWidth );
	const float		fDisplayHeight	= static_cast<float>( m_DisplayHeight );
	const float		fFrameWidth		= static_cast<float>( m_Display->m_FrameWidth );
	const float		fFrameHeight	= static_cast<float>( m_Display->m_FrameHeight );
	const bool		OpenGL			= m_Renderer->IsOpenGL();

	m_HUDView		= new View( EyePosition, EyeOrientation, SRect( 0.0f, 0.0f, fDisplayWidth, fDisplayHeight ), NearClip, FarClip, OpenGL );
	m_UpscaleView	= new View( EyePosition, EyeOrientation, SRect( 0.0f, 0.0f, fFrameWidth, fFrameHeight ), NearClip, FarClip, OpenGL );
	m_BloomViewA	= new View( EyePosition, EyeOrientation, SRect( 0.0f, 0.0f, fDisplayWidth / ROSA_BLOOMA_SCALE, fDisplayHeight / ROSA_BLOOMA_SCALE ), NearClip, FarClip, OpenGL );
	m_BloomViewB	= new View( EyePosition, EyeOrientation, SRect( 0.0f, 0.0f, fDisplayWidth / ROSA_BLOOMB_SCALE, fDisplayHeight / ROSA_BLOOMB_SCALE ), NearClip, FarClip, OpenGL );
	m_BloomViewC	= new View( EyePosition, EyeOrientation, SRect( 0.0f, 0.0f, fDisplayWidth / ROSA_BLOOMC_SCALE, fDisplayHeight / ROSA_BLOOMC_SCALE ), NearClip, FarClip, OpenGL );
}

void RosaFramework::CreateMinimapViews()
{
	SafeDelete( m_MinimapAView );
	SafeDelete( m_MinimapBView );
#if ROSA_USE_MAXIMAP
	SafeDelete( m_MaximapAView );
	SafeDelete( m_MaximapBView );
#endif

	IRenderTarget* const	pMinimapRT		= m_TargetManager->GetRenderTarget( "MinimapA" );
	const float				MinimapRTWidth	= static_cast<float>( pMinimapRT->GetWidth() );
	const float				MinimapRTHeight	= static_cast<float>( pMinimapRT->GetHeight() );
#if ROSA_USE_MAXIMAP
	IRenderTarget* const	pMaximapRT		= m_TargetManager->GetRenderTarget( "MaximapA" );
	const float				MaximapRTWidth	= static_cast<float>( pMaximapRT->GetWidth() );
	const float				MaximapRTHeight	= static_cast<float>( pMaximapRT->GetHeight() );
#endif

	STATICHASH( RosaMinimap );

	STATICHASH( MinimapViewExtentNear );
	m_MinimapViewExtent				= ConfigManager::GetFloat( sMinimapViewExtentNear, 0.0f, sRosaMinimap );
	const float	MinimapHalfExtent	= 0.5f * m_MinimapViewExtent;

	STATICHASH( MinimapViewOffset );
	m_MinimapViewOffset				= ConfigManager::GetFloat( sMinimapViewOffset, 0.0f, sRosaMinimap );

	STATICHASH( MinimapViewHeight );
	m_MinimapViewHeight				= ConfigManager::GetFloat( sMinimapViewHeight, 0.0f, sRosaMinimap );

	STATICHASH( MinimapViewNearClip );
	const float	MinimapViewNearClip	= ConfigManager::GetFloat( sMinimapViewNearClip, 0.0f, sRosaMinimap );

	STATICHASH( MinimapViewFarClip );
	const float	MinimapViewFarClip	= ConfigManager::GetFloat( sMinimapViewFarClip, 0.0f, sRosaMinimap );

#if ROSA_USE_MAXIMAP
	STATICHASH( MaximapViewExtentNear );
	m_MaximapViewExtent				= ConfigManager::GetFloat( sMaximapViewExtentNear, 0.0f, sRosaMinimap );
	const float	MaximapHalfExtent	= 0.5f * m_MaximapViewExtent;

	STATICHASH( MaximapViewOffset );
	m_MaximapViewOffset				= ConfigManager::GetFloat( sMaximapViewOffset, 0.0f, sRosaMinimap );
#endif

	const Vector	EyePositionA	= Vector( 0.0f, 0.0f, 0.0f );
	const Angles	EyeRotationA	= Angles( PI * -0.5f, 0.0f, 0.0f );
	const SRect		MinimapBoundsA	= SRect( -MinimapHalfExtent, MinimapHalfExtent, MinimapHalfExtent, -MinimapHalfExtent );
#if ROSA_USE_MAXIMAP
	const SRect		MaximapBoundsA	= SRect( -MaximapHalfExtent, MaximapHalfExtent, MaximapHalfExtent, -MaximapHalfExtent );
#endif

	const Vector	EyePositionB	= Vector( 0.0f, -1.0f, 0.0f );
	const Angles	EyeRotationB	= Angles( 0.0f, 0.0f, 0.0f );
	const SRect		MinimapBoundsB	= SRect( 0.0f, 0.0f, MinimapRTWidth, MinimapRTHeight );
#if ROSA_USE_MAXIMAP
	const SRect		MaximapBoundsB	= SRect( 0.0f, 0.0f, MaximapRTWidth, MaximapRTHeight );
#endif
	const float		NearClipB		= 0.01f;
	const float		FarClipB		= 2.0f;
	const bool		OpenGL			= m_Renderer->IsOpenGL();

	m_MinimapAView = new View( EyePositionA, EyeRotationA, MinimapBoundsA, MinimapViewNearClip,	MinimapViewFarClip,	OpenGL );
	m_MinimapBView = new View( EyePositionB, EyeRotationB, MinimapBoundsB, NearClipB,			FarClipB,			OpenGL );

#if ROSA_USE_MAXIMAP
	m_MaximapAView = new View( EyePositionA, EyeRotationA, MaximapBoundsA, MinimapViewNearClip,	MinimapViewFarClip,	OpenGL );
	m_MaximapBView = new View( EyePositionB, EyeRotationB, MaximapBoundsB, NearClipB,			FarClipB,			OpenGL );
#endif
}

/*virtual*/ void RosaFramework::RefreshDisplay( const bool Fullscreen, const bool VSync, const uint DisplayWidth, const uint DisplayHeight )
{
	PRINTF( "RosaFramework::RefreshDisplay\n" );

	Framework3D::RefreshDisplay( Fullscreen, VSync, DisplayWidth, DisplayHeight );

	m_TargetManager->CreateTargets( DisplayWidth, DisplayHeight );

	UpdateViews();
	PublishDisplayedFOV();

	ASSERT( m_Game );
	m_Game->RefreshRTDependentSystems();

	CreateBuckets();
}

void RosaFramework::RegenerateWorld()
{
	ASSERT( m_World );
	InitializeWorld( m_World->GetCurrentWorld(), true );
}

void RosaFramework::GoToLevel( const SimpleString& WorldDef )
{
	InitializeWorld( WorldDef, true );
}

/*virtual*/ SimpleString RosaFramework::GetUserDataPath()
{
	// HACKHACK: Hard-coded so string table *can't* override this!
	const SimpleString UserPathAppName = "NEON STRUCT Desperation Column";

#if BUILD_MAC
	// MACTODO: Make this part of FileUtil::GetUserLocalAppDataPath or whatever.
	return ObjCJunk::GetUserDirectory();
#elif BUILD_LINUX
	const char* const XDGDataHome = getenv( "XDG_DATA_HOME" );
	if( XDGDataHome )
	{
		return SimpleString::PrintF( "%s/%s/", XDGDataHome, UserPathAppName.CStr() );
	}

	const char* const Home = getenv( "HOME" );
	if( Home )
	{
		return SimpleString::PrintF( "%s/.local/share/%s/", Home, UserPathAppName.CStr() );
	}

	// Fall back to the local directory
	return SimpleString( "./" );
#elif BUILD_WINDOWS
	return SimpleString::PrintF( "%s%s/", FileUtil::GetUserLocalAppDataPath().CStr(), UserPathAppName.CStr() );
#else
	// What even platform are we running here?
	return SimpleString( "./" );
#endif
}

// Save to working directory on Windows and Linux. On Mac, save in proper location.
/*virtual*/ SimpleString RosaFramework::GetSaveLoadPath()
{
	return GetUserDataPath() + "Saves/";
}
