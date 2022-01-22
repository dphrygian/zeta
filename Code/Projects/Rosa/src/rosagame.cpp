#include "core.h"
#include "rosagame.h"
#include "wbeventmanager.h"
#include "rosaframework.h"
#include "rosaworld.h"
#include "wbworld.h"
#include "rosasaveload.h"
#include "rosapersistence.h"
#include "rosacampaign.h"
#include "rosamusic.h"
#include "rosaconversation.h"
#include "rosasupertitles.h"
#include "rosahudlog.h"
#include "rosawardrobe.h"
#include "rosalockpicking.h"
#include "configmanager.h"
#include "texturemanager.h"
#include "irenderer.h"
#include "mesh.h"
#include "rosatargetmanager.h"
#include "meshfactory.h"
#include "irendertarget.h"
#include "shadermanager.h"
#include "Common/uimanagercommon.h"
#include "uiscreen.h"
#include "uiwidget.h"
#include "uistack.h"
#include "Components/wbcomprosahealth.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacamera.h"
#include "Components/wbcomprosaplayer.h"
#include "Components/wbcomprosavisible.h"
#include "Components/wbcomprosacollision.h"
#include "Widgets/uiwidget-image.h"
#include "allocator.h"
#include "wbcomponentarrays.h"
#include "ivertexdeclaration.h"
#include "rosadifficulty.h"
#include "mathcore.h"
#include "iaudiosystem.h"
#include "hsv.h"

#if BUILD_WINDOWS
#include <Windows.h>	// For ShellExecute
#endif

RosaGame::RosaGame()
:	m_GoToLevelInNumTicks( 0 )
,	m_IsRestarting( false )
,	m_IsReturningToHub( false )
,	m_RestoreSpawnPoint( false )
,	m_TeleportLabel()
,	m_NextLevelName()
,	m_LoadSlotInNumTicks( 0 )
,	m_LoadSlotName()
,	m_CurrentLevelName()
,	m_TravelPersistence()
,	m_SaveLoad( NULL )
,	m_GenerationPersistence( NULL )
,	m_Campaign( NULL )
,	m_Music( NULL )
,	m_Conversation( NULL )
,	m_Supertitles( NULL )
,	m_HUDLog( NULL )
,	m_Wardrobe( NULL )
,	m_Lockpicking( NULL )
,	m_Gamma( 0.0f )
,	m_GlobalCubemap( NULL )
,	m_GlobalAmbientQuad( NULL )
,	m_UpscaleQuad( NULL )
,	m_PostQuad( NULL )
,	m_PostCheapQuad( NULL )
,	m_UsePostCheapQuad( false )
#if BUILD_ROSA_TOOLS
,	m_PostToolsQuad( NULL )
#endif
,	m_SSAOQuad( NULL )
,	m_GradientQuad( NULL )
,	m_EdgeQuad( NULL )
,	m_LightCombineQuad( NULL )
#if ROSA_USE_WATERCOLOR_POST
,	m_EdgeQuad( NULL )
#endif
,	m_BloomQuads()
,	m_FXAAQuad( NULL )
,	m_MinimapBQuad( NULL )
,	m_MinimapFXAAQuad( NULL )
,	m_MinimapTonesTexture()
,	m_MinimapFloorTexture()
,	m_MinimapSolidTexture()
,	m_MinimapHeightThreshold( 0.0f )
,	m_MinimapHeightOffset( 0.0f )
,	m_MinimapHeightDiffScale( 0.0f )
,	m_MinimapHeightToneScale( 0.0f )
,	m_MinimapRenderEdges( 0.0f )
,	m_MinimapRcpTileSize( 0.0f )
#if ROSA_USE_MAXIMAP
,	m_MaximapBQuad( NULL )
,	m_MaximapFXAAQuad( NULL )
#endif
,	m_ColorGradingTexture()
,	m_NoiseTexture()
,	m_NoiseScaleRange()
,	m_NoiseRange( 0.0f )
,	m_HalosEnabled( false )
,	m_DirtyLensTexture()
,	m_BloomKernelTexture()
,	m_DisplaceTexture()
,	m_BlotTexture()
,	m_CanvasTexture()
,	m_EdgeColorHSV()
,	m_EdgeBackColor()
,	m_EdgeColor()
,	m_EdgeLuminanceMul( 0.0f )
,	m_EdgeLuminanceAdd( 0.0f )
,	m_WatercolorParams()
,	m_DisplacePct( 0.0f )
,	m_FogEnabled( false )
,	m_FogColors()
,	m_FogNearFarCurve()
,	m_FogLoHiCurve()
,	m_FogParams()
,	m_HeightFogParams()
,	m_RegionFogScalar()
,	m_FogLightParams()
,	m_SunVector()
,	m_SkyColorHi()
,	m_SkyColorLo()
,	m_WindMatrix()
,	m_WindPhaseTime()
,	m_WindPhaseSpace()
,	m_WindWaterVector()
,	m_InvExposure( 0.0f )
,	m_BloomVerticalRadius( 0.0f )
,	m_BloomAspectRatio( 0.0f )
,	m_BloomStepRadiusH()
,	m_BloomStepRadiusV()
,	m_BloomParams()
,	m_ReturnVerticalBloomRadius( false )
,	m_CurrentMinimapScalar( 0.0f )
,	m_CurrentMusic()
,	m_CurrentAmbience()
,	m_CurrentReverbDef()
{
	m_MinimapTonesTexture	= DEFAULT_TEXTURE;
	m_MinimapFloorTexture	= DEFAULT_TEXTURE;
	m_MinimapSolidTexture	= DEFAULT_TEXTURE;
	m_ColorGradingTexture	= DEFAULT_TEXTURE;
	m_NoiseTexture			= DEFAULT_TEXTURE;
	m_DirtyLensTexture		= DEFAULT_TEXTURE;
	m_BloomKernelTexture	= DEFAULT_TEXTURE;
	m_DisplaceTexture		= DEFAULT_TEXTURE;
	m_BlotTexture			= DEFAULT_TEXTURE;
	m_CanvasTexture			= DEFAULT_TEXTURE;
	m_SaveLoad				= new RosaSaveLoad;
	m_GenerationPersistence	= new RosaPersistence;
	m_Campaign				= new RosaCampaign;
	m_Music					= new RosaMusic;
	m_Conversation			= new RosaConversation;
	m_Supertitles			= new RosaSupertitles;
	m_HUDLog				= new RosaHUDLog;
	m_Wardrobe				= new RosaWardrobe;
	m_Lockpicking			= new RosaLockpicking;
}

RosaGame::~RosaGame()
{
	SafeDelete( m_SaveLoad );
	SafeDelete( m_GenerationPersistence );
	SafeDelete( m_Campaign );
	SafeDelete( m_Music );
	SafeDelete( m_Conversation );
	SafeDelete( m_Supertitles );
	SafeDelete( m_HUDLog );
	SafeDelete( m_Wardrobe );
	SafeDelete( m_Lockpicking );

	SafeDelete( m_GlobalAmbientQuad );
	SafeDelete( m_UpscaleQuad );
	SafeDelete( m_PostQuad );
	SafeDelete( m_PostCheapQuad );
#if BUILD_ROSA_TOOLS
	SafeDelete( m_PostToolsQuad );
#endif
	SafeDelete( m_SSAOQuad );
	SafeDelete( m_GradientQuad );
	SafeDelete( m_EdgeQuad );
	SafeDelete( m_LightCombineQuad );
#if ROSA_USE_WATERCOLOR_POST
	SafeDelete( m_EdgeQuad );
#endif
	SafeDelete( m_FXAAQuad );
	SafeDelete( m_MinimapBQuad );
	SafeDelete( m_MinimapFXAAQuad );
#if ROSA_USE_MAXIMAP
	SafeDelete( m_MaximapBQuad );
	SafeDelete( m_MaximapFXAAQuad );
#endif

	FOR_EACH_ARRAY( BloomQuadIter, m_BloomQuads, Mesh* )
	{
		Mesh* pBloomQuad = BloomQuadIter.GetValue();
		SafeDelete( pBloomQuad );
	}
}

void RosaGame::SetCurrentLevelName( const SimpleString& LevelName )
{
	XTRACE_FUNCTION;

	m_CurrentLevelName = LevelName;

	const SimpleString LevelFriendlyName = GetCurrentFriendlyLevelName();

	STATICHASH( HUD );
	STATICHASH( WorldName );
	ConfigManager::SetString( sWorldName, LevelFriendlyName.CStr(), sHUD );
}

SimpleString RosaGame::GetCurrentFriendlyLevelName() const
{
	XTRACE_FUNCTION;

	return GetFriendlyLevelName( m_CurrentLevelName );
}

/*static*/ SimpleString RosaGame::GetFriendlyLevelName( const SimpleString& LevelName )
{
	XTRACE_FUNCTION;

	MAKEHASH( LevelName );
	STATICHASH( Name );
	return ConfigManager::GetString( sName, "", sLevelName );
}

void RosaGame::ClearTravelPersistence()
{
	m_TravelPersistence.Clear();
}

/*static*/ TPersistence& RosaGame::StaticGetTravelPersistence()
{
	return RosaFramework::GetInstance()->GetGame()->GetTravelPersistence();
}

// NOTE: Player component forwards all its events to this function,
// so the game can be targeted from script via the player, and it
// doesn't have to register for events.
/*virtual*/ void RosaGame::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( GoToInitialLevel );
	STATIC_HASHED_STRING( GoToHubLevel );
	STATIC_HASHED_STRING( GoToNextLevel );
	STATIC_HASHED_STRING( GoToPrevLevel );
	STATIC_HASHED_STRING( GoToLevel );
	STATIC_HASHED_STRING( GoToTitleLevel );
	STATIC_HASHED_STRING( Checkpoint );
	STATIC_HASHED_STRING( Autosave );
	STATIC_HASHED_STRING( FlushWorldFiles );
	STATIC_HASHED_STRING( PlayMusic );
	STATIC_HASHED_STRING( PlayMusicAndAmbience );
	STATIC_HASHED_STRING( StopMusic );
	STATIC_HASHED_STRING( StopMusicAndAmbience );
	STATIC_HASHED_STRING( SetMusicLevels );
	STATIC_HASHED_STRING( LaunchWebSite );
	STATIC_HASHED_STRING( OpenUserDataPath );
	STATIC_HASHED_STRING( GoToLevelImmediate );
	STATIC_HASHED_STRING( StartConversation );
	STATIC_HASHED_STRING( SkipConversationLine );
	STATIC_HASHED_STRING( ProgressConversation );
	STATIC_HASHED_STRING( SelectConversationChoice );
	STATIC_HASHED_STRING( StartSupertitles );
	STATIC_HASHED_STRING( CycleMenuDifficulty );
	STATIC_HASHED_STRING( SetMenuDifficulty );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sGoToInitialLevel )
	{
		RequestGoToInitialLevel();
	}
	else if( EventName == sGoToHubLevel )
	{
		RequestGoToHubLevel();
	}
	else if( EventName == sGoToNextLevel )
	{
		STATIC_HASHED_STRING( TeleportLabel );
		const HashedString TeleportLabel = Event.GetHash( sTeleportLabel );

		RequestGoToNextLevel( TeleportLabel );
	}
	else if( EventName == sGoToPrevLevel )
	{
		STATIC_HASHED_STRING( TeleportLabel );
		const HashedString TeleportLabel = Event.GetHash( sTeleportLabel );

		RequestGoToPrevLevel( TeleportLabel );
	}
	else if( EventName == sGoToLevel )
	{
		STATIC_HASHED_STRING( Level );
		const SimpleString	Level					= Event.GetString( sLevel );

		STATIC_HASHED_STRING( TeleportLabel );
		const HashedString	TeleportLabel			= Event.GetHash( sTeleportLabel );

		STATIC_HASHED_STRING( SuppressLoadingScreen );
		const bool			SuppressLoadingScreen	= Event.GetBool( sSuppressLoadingScreen );

		RequestGoToLevel( Level, TeleportLabel, SuppressLoadingScreen );
	}
	else if( EventName == sGoToTitleLevel )
	{
		RequestReturnToTitle();
	}
	else if( EventName == sCheckpoint || EventName == sAutosave )
	{
		Autosave();
	}
	else if( EventName == sFlushWorldFiles )
	{
		GetSaveLoad()->FlushWorldFiles();
	}
	else if( EventName == sLaunchWebSite )
	{
		STATIC_HASHED_STRING( URL );
		const SimpleString URL = Event.GetString( sURL );
		LaunchWebSite( URL );
	}
	else if( EventName == sOpenUserDataPath )
	{
		OpenUserDataPath();
	}
	else if( EventName == sPlayMusic )
	{
		STATIC_HASHED_STRING( Music );
		const SimpleString	Music		= Event.GetString( sMusic );
		const SimpleString	MusicDef	= ( Music == "" ) ? m_CurrentMusic : Music;

		STATIC_HASHED_STRING( TrackBits );
		const uint			TrackBits	= Event.GetInt( sTrackBits );

		STATIC_HASHED_STRING( TrackMask );
		const uint			TrackMask	= Event.GetInt( sTrackMask, -1 );

		m_Music->PlayMusic( MusicDef, TrackBits, TrackMask );
	}
	else if( EventName == sPlayMusicAndAmbience )
	{
		STATIC_HASHED_STRING( Music );
		const SimpleString MusicDef = Event.GetString( sMusic, m_CurrentMusic );

		STATIC_HASHED_STRING( TrackBits );
		const uint TrackBits = Event.GetInt( sTrackBits );

		STATIC_HASHED_STRING( TrackMask );
		const uint TrackMask = Event.GetInt( sTrackMask, -1 );

		STATIC_HASHED_STRING( Ambience );
		const SimpleString AmbienceDef = Event.GetString( sAmbience, m_CurrentAmbience );

		m_Music->PlayMusic( MusicDef, TrackBits, TrackMask );
		m_Music->PlayAmbience( AmbienceDef );
	}
	else if( EventName == sStopMusic )
	{
		m_Music->StopMusic();
	}
	else if( EventName == sStopMusicAndAmbience )
	{
		m_Music->StopMusic();
		m_Music->StopAmbience();
	}
	else if( EventName == sSetMusicLevels )
	{
		STATIC_HASHED_STRING( TrackBits );
		const uint TrackBits = Event.GetInt( sTrackBits );

		STATIC_HASHED_STRING( TrackMask );
		const uint TrackMask = Event.GetInt( sTrackMask, -1 );

		STATIC_HASHED_STRING( Duration );
		const float Duration = Event.GetFloat( sDuration );

		m_Music->SetMusicLevels( TrackBits, TrackMask, Duration );
	}
	else if( EventName == sGoToLevelImmediate )
	{
		// This should only be called after we've queued a go-to.
		ASSERT( m_GoToLevelInNumTicks > 0 );
		if( m_GoToLevelInNumTicks > 0 )
		{
			GoToLevel();
		}
	}
	else if( EventName == sStartConversation )
	{
		STATIC_HASHED_STRING( Conversation );
		const HashedString Conversation = Event.GetHash( sConversation );

		STATIC_HASHED_STRING( ConvoTarget );
		WBEntity* const pConvoTarget = Event.GetEntity( sConvoTarget );

		m_Conversation->StartConversation( Conversation, pConvoTarget );
	}
	else if( EventName == sSkipConversationLine )
	{
		m_Conversation->SkipLine();
	}
	else if( EventName == sProgressConversation )
	{
		m_Conversation->ProgressConversation();
	}
	else if( EventName == sSelectConversationChoice )
	{
		STATIC_HASHED_STRING( ChoiceIndex );
		const uint ChoiceIndex = Event.GetInt( sChoiceIndex );
		m_Conversation->SelectChoice( ChoiceIndex );
	}
	else if( EventName == sStartSupertitles )
	{
		STATIC_HASHED_STRING( Supertitles );
		const HashedString Supertitles = Event.GetHash( sSupertitles );

		m_Supertitles->StartSupertitles( Supertitles );
	}
	else if( EventName == sCycleMenuDifficulty )
	{
		RosaDifficulty::CycleMenuDifficulty();
	}
	else if( EventName == sSetMenuDifficulty )
	{
		STATIC_HASHED_STRING( Difficulty );
		const uint Difficulty = Event.GetInt( sDifficulty );

		RosaDifficulty::SetMenuDifficulty( Difficulty );
	}
}

void RosaGame::Initialize()
{
	// If we need to load any state on the title screen (like
	// campaign state for a single-profile game), do it here.
	//GetSaveLoad()->TryLoadAutosave( 0 );

	SetCurrentLevelName( GetTitleScreenLevelName() );

	MAKEHASH( m_CurrentLevelName );

	STATICHASH( WorldDef );
	const SimpleString TitleScreenWorldDef = ConfigManager::GetString( sWorldDef, "", sm_CurrentLevelName );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	RosaWorld* const pWorld = pFramework->GetWorld();
	pWorld->SetCurrentWorld( TitleScreenWorldDef );
	pWorld->Create();
	pFramework->InitializeTools();

	RefreshUIRetreatEnabled();
}

void RosaGame::ShutDown()
{
	PRINTF( "Shutting down game\n" );

	m_Lockpicking->ShutDown();

	Autosave();
	GetSaveLoad()->FlushWorldFiles();
}

void RosaGame::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;

	GetSupertitles()->Tick();

	m_Lockpicking->Tick( DeltaTime );
	m_HUDLog->Tick();
	m_Music->Tick( DeltaTime );

	// This is done in a tick instead of being event-driven,
	// because it needs to happen before the world tick.
	if( m_GoToLevelInNumTicks > 0 &&
		--m_GoToLevelInNumTicks == 0 )
	{
		GoToLevel();
	}

	if( m_LoadSlotInNumTicks > 0 &&
		--m_LoadSlotInNumTicks == 0 )
	{
		LoadSlot();
	}
}

void RosaGame::TickPaused( const float DeltaTime )
{
	// Tick conversation when we're paused because the convo screen pauses the game
	GetConversation()->Tick();
	m_Music->Tick( DeltaTime );

	if( m_GoToLevelInNumTicks > 0 &&
		--m_GoToLevelInNumTicks == 0 )
	{
		GoToLevel();
	}

	if( m_LoadSlotInNumTicks > 0 &&
		--m_LoadSlotInNumTicks == 0 )
	{
		LoadSlot();
	}
}

void RosaGame::Render() const
{
	XTRACE_FUNCTION;

	// HACKHACK so I'm not doing so many lookups for player feet in minimap SDPs
	CachePlayerFeetLocation();

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	IRenderer* const		pRenderer		= pFramework->GetRenderer();
	ASSERT( pRenderer );

	pRenderer->AddMesh( GetGlobalAmbientQuad() );
	pRenderer->AddMesh( GetSSAOQuad() );
	pRenderer->AddMesh( GetGradientQuad() );
	pRenderer->AddMesh( GetEdgeQuad() );
	pRenderer->AddMesh( GetLightCombineQuad() );
#if ROSA_USE_WATERCOLOR_POST
	pRenderer->AddMesh( GetEdgeQuad() );
#endif
	pRenderer->AddMesh( m_UsePostCheapQuad ? GetPostCheapQuad() : GetPostQuad() );
#if BUILD_ROSA_TOOLS
	pRenderer->AddMesh( m_PostToolsQuad );
#endif
	pRenderer->AddMesh( GetFXAAQuad() );
	pRenderer->AddMesh( GetMinimapBQuad() );
	pRenderer->AddMesh( GetMinimapFXAAQuad() );
#if ROSA_USE_MAXIMAP
	pRenderer->AddMesh( GetMaximapBQuad() );
	pRenderer->AddMesh( GetMaximapFXAAQuad() );
#endif
	pRenderer->AddMesh( GetUpscaleQuad() );

	FOR_EACH_ARRAY( BloomQuadIter, m_BloomQuads, Mesh* )
	{
		Mesh* const pBloomQuad = BloomQuadIter.GetValue();
		pRenderer->AddMesh( pBloomQuad );
	}
}

void RosaGame::RequestReturnToTitle()
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::RequestReturnToTitle\n" );

	const SimpleString TitleScreenLevelName = GetTitleScreenLevelName();

	const HashedString	TeleportLabel			= HashedString::NullString;
	const bool			RestoreSpawnPoint		= false;
	const bool			SuppressLoadingScreen	= false;
	RequestGoToLevelInternal( TitleScreenLevelName, TeleportLabel, RestoreSpawnPoint, SuppressLoadingScreen );

	m_IsRestarting = true;
	ClearTravelPersistence();	// Make sure we don't pull persistence over any default inventory, etc.
}

void RosaGame::RequestGoToInitialLevel()
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::RequestGoToInitialLevel\n" );

	const SimpleString InitialLevelName = GetInitialLevelName();

	const HashedString	TeleportLabel		=	 HashedString::NullString;
	const bool			RestoreSpawnPoint		= false;
	const bool			SuppressLoadingScreen	= false;
	RequestGoToLevelInternal( InitialLevelName, TeleportLabel, RestoreSpawnPoint, SuppressLoadingScreen );

	m_IsRestarting = true;
	ClearTravelPersistence();	// Make sure we don't pull persistence over any default inventory, etc.
}

void RosaGame::RequestGoToHubLevel()
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::RequestGoToHubLevel\n" );

	const SimpleString HubLevelName = GetHubLevelName();

	const HashedString	TeleportLabel			= HashedString::NullString;
	const bool			RestoreSpawnPoint		= false;
	const bool			SuppressLoadingScreen	= false;
	RequestGoToLevelInternal( HubLevelName, TeleportLabel, RestoreSpawnPoint, SuppressLoadingScreen );

	m_IsReturningToHub = true;
}

void RosaGame::RequestGoToNextLevel( const HashedString& TeleportLabel )
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::RequestGoToNextLevel\n" );

	MAKEHASH( m_CurrentLevelName );

	STATICHASH( NextLevel );
	const SimpleString NextLevelName = ConfigManager::GetString( sNextLevel, "", sm_CurrentLevelName );

	const bool	RestoreSpawnPoint		= true;
	const bool	SuppressLoadingScreen	= false;
	RequestGoToLevelInternal( NextLevelName, TeleportLabel, RestoreSpawnPoint, SuppressLoadingScreen );
}

void RosaGame::RequestGoToPrevLevel( const HashedString& TeleportLabel )
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::RequestGoToPrevLevel\n" );

	MAKEHASH( m_CurrentLevelName );

	STATICHASH( PrevLevel );
	const SimpleString PrevLevelName = ConfigManager::GetString( sPrevLevel, "", sm_CurrentLevelName );

	const bool	RestoreSpawnPoint		= false;
	const bool	SuppressLoadingScreen	= false;
	RequestGoToLevelInternal( PrevLevelName, TeleportLabel, RestoreSpawnPoint, SuppressLoadingScreen );
}

void RosaGame::RequestGoToLevel( const SimpleString& NextLevel, const HashedString& TeleportLabel, const bool SuppressLoadingScreen )
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::RequestGoToLevel %s\n", NextLevel.CStr() );

	const bool RestoreSpawnPoint = false;
	RequestGoToLevelInternal( NextLevel, TeleportLabel, RestoreSpawnPoint, SuppressLoadingScreen );
}

void RosaGame::RequestGoToLevelInternal( const SimpleString& NextLevel, const HashedString& TeleportLabel, const bool RestoreSpawnPoint, const bool SuppressLoadingScreen )
{
	XTRACE_FUNCTION;

	if( NextLevel == "" )
	{
		WARN;
		return;
	}

	if( m_GoToLevelInNumTicks > 0 || m_LoadSlotInNumTicks > 0 )
	{
		// We have something scheduled already!
		return;
	}

	// NOTE: No longer doing a crash protection checkpoint save;
	// level transitions should be stable, and the player may be
	// intersecting a level end trigger at this point so the
	// checkpoint state would not be good!

	ASSERT( m_GoToLevelInNumTicks == 0 );
	m_GoToLevelInNumTicks	= 2;	// Add an extra tick so we get a chance to show the "Loading..." UI before starting the load
	m_RestoreSpawnPoint		= RestoreSpawnPoint;
	m_TeleportLabel			= TeleportLabel;
	m_NextLevelName			= NextLevel;

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	WB_MAKE_EVENT( PreLevelTransition, NULL );
	WB_DISPATCH_EVENT( pEventManager, PreLevelTransition, NULL );

	if( !SuppressLoadingScreen )
	{
		// Push the loading screen
		STATIC_HASHED_STRING( LoadingScreen );
		WB_MAKE_EVENT( PushUIScreen, NULL );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sLoadingScreen );
		WB_DISPATCH_EVENT( pEventManager, PushUIScreen, NULL );

		// HACKHACK: Force the loading screen to render
		RosaFramework::GetInstance()->RequestRenderTick();
	}
}

void RosaGame::RequestLoadSlot( const SimpleString& Slot )
{
	XTRACE_FUNCTION;

	if( Slot == "" )
	{
		WARN;
		return;
	}

	RosaSaveLoad::SSaveSlotInfo SlotInfo;
	RosaSaveLoad::GetSaveSlotInfo( Slot, SlotInfo );
	if( SlotInfo.m_Empty )
	{
		return;
	}

	if( m_GoToLevelInNumTicks > 0 || m_LoadSlotInNumTicks > 0 )
	{
		// We have something scheduled already!
		return;
	}

	// *Don't* checkpoint when loading a slot

	ASSERT( m_LoadSlotInNumTicks == 0 );
	m_LoadSlotInNumTicks	= 2;	// Add an extra tick so we get a chance to show the "Loading..." UI before starting the load
	m_LoadSlotName			= Slot;

	// Push the loading screen
	STATIC_HASHED_STRING( LoadingScreen );
	WB_MAKE_EVENT( PushUIScreen, NULL );
	WB_SET_AUTO( PushUIScreen, Hash, Screen, sLoadingScreen );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), PushUIScreen, NULL );

	// HACKHACK: Force the loading screen to render
	RosaFramework::GetInstance()->RequestRenderTick();
}

void RosaGame::LoadSlot()
{
	m_LoadSlotInNumTicks	= 0;

	const bool ForceResetToGameScreens = true;
	RosaFramework::GetInstance()->PrepareForLoad( ForceResetToGameScreens );
	GetSaveLoad()->TryLoadSlot( m_LoadSlotName );
}

// ROSANOTE: No longer decorating the world filename with the save/load path,
// because that prevents migration of the saved game to other accounts/systems.
SimpleString RosaGame::DecorateWorldFileName( const SimpleString& LevelName ) const
{
	return SimpleString::PrintF( "%s.rosaworldsave", LevelName.CStr() );
}

void RosaGame::Autosave() const
{
	XTRACE_FUNCTION;

	if( GetSaveLoad()->ShouldSaveCurrentWorld() )
	{
#if ROSA_USE_ACTIVESAVESLOT
		// Save directly to slot instead of autosaves
		GetSaveLoad()->SaveSlot( GetSaveLoad()->GetActiveSaveSlot() );
#else
		GetSaveLoad()->SaveAutosave();
#endif
	}
}

void RosaGame::GoToLevel()
{
	XTRACE_FUNCTION;

	DEVPRINTF( "RosaGame::GoToLevel: %s\n", m_NextLevelName.CStr() );

	// This may modify m_NextLevelName
	PreGoToLevel();

	PRINTF( "Traveling to level %s\n", m_NextLevelName.CStr() );

	m_GoToLevelInNumTicks	= 0;
	m_IsRestarting			= false;
	m_IsReturningToHub		= false;
	SetCurrentLevelName( m_NextLevelName );

	// ZETANOTE: I don't expect the player to return to serialized/persistent
	// worlds in this game, so no need to try to load the world from file.
#if ROSA_USE_PERSISTENT_WORLDS
	// Try to load persistent world data first, else create the world.
	if( GetSaveLoad()->TryLoadWorld( DecorateWorldFileName( m_NextLevelName ) ) )
	{
		// We're good!
	}
	else
#endif
	{
		// HACKHACK: Allow the campaign to override the world def; this lets me evolve the hub
		// without it being influenced by e.g. dark/dusk/foggy twists from the selected mission.
		// CAMTODO: Revisit this, it may not be needed and could simplify campaign code by getting rid of the ForceModify stuff.
		// (Basically, do I need to modify the hub or can I use actually different hub levels? Or maybe I want both?)
		STATICHASH( WorldDef );
		const SimpleString WorldDef = GetCampaign()->OverrideString( m_NextLevelName, ConfigManager::GetString( sWorldDef, "", m_NextLevelName ), true /*ForceModify*/ );
		DEVASSERT( WorldDef != "" );

		RosaFramework::GetInstance()->GoToLevel( WorldDef );
	}

	PostGoToLevel();

	PRINTF( "Travel finished\n" );
}

void RosaGame::PreGoToLevel()
{
	// CAMTODO: I may want to introduce a third option here, between "restarting" (resetting everything)
	// and "returning to hub" (advancing to the next stage), for failing and restarting in the roguelike
	// while keeping persistence. This was sort of handled in Vamp by failing at the campaign level and
	// advancing to the next legacy, but here it'll probably also be linked to loading into a different
	// map so I might as well model it at the game level.

	if( m_IsRestarting )			// Completely restarting game, clearing persistence (i.e., returning to title or starting a new game)
	{
		DEVPRINTF( "RosaGame::PreGoToLevel: m_IsRestarting\n" );

		// Don't save the current world, and flush everything including persistence.
		GetSaveLoad()->FlushWorldFiles();

		// I think this sort of persistence is only used for tutorials and achievements.
		// If those should be persistent between profiles/slots, don't clear this, but I
		// think this is the best option.
		GetPersistence()->Reset();

		// Reset the campaign and take the first turn
		GetCampaign()->Reset();
		GetCampaign()->TakePreGenTurn();
	}
	else if( m_IsReturningToHub )	// Returning to hub within a game, flushing world files but keeping persistence
	{
		DEVPRINTF( "RosaGame::PreGoToLevel: m_IsReturningToHub\n" );

#if !ROSA_USE_PERSISTENT_WORLDS
		// CAMTODO: Figure out what the "hub" will be in light of roguelike campaign/persistence structure.
		// I'll probably need separate initial level and hub maps, which will do different campaign things.
		// Probably get rid of "Overworld" map and make "Start" and "Hub" maps instead. Or call them, like,
		// "Home" and "Camp" or whatever fictional purpose they serve.

		// CAMTODO: Get rid of this warning when the above part is done. Or make a different #define
		// instead of ROSA_USE_PERSISTENT_WORLDS that suits what I'm trying to do here.

		// ZETANOTE: There is no hub anymore, we should never get here
		WARNDESC( "Returning to hub; this should never happen in Zeta." );
#endif

		// Don't save the current world, and flush any other world files.
		// ROSANOTE: In Eldritch, I had to defer this flush so I could return to the
		// previous instance of the hub. In Rosa, I'm building a new hub every time.
		GetSaveLoad()->FlushWorldFiles();

		// The campaign needs to run some things in the hub before world generation
		GetCampaign()->TakePreGenTurn();
	}
	else
	{
		DEVPRINTF( "RosaGame::PreGoToLevel\n" );

		// ZETANOTE: I don't expect the player to return to serialized/persistent
		// worlds in this game, so no need to save the world (which also means no
		// need to manually flush world files.
		// ZETANOTE: Except I _might_ serialize the overworld. Probably best to
		// avoid that unless there's a compelling reason, though.
#if ROSA_USE_PERSISTENT_WORLDS
		// Store a record of the world we're leaving so we can come back to it.
		if( GetSaveLoad()->ShouldSaveCurrentWorld() )
		{
			GetSaveLoad()->SaveWorld( DecorateWorldFileName( m_CurrentLevelName ) );
		}
#endif
	}
}

void RosaGame::PostGoToLevel()
{
	RefreshUIRetreatEnabled();

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	WB_MAKE_EVENT( PostLevelTransition, NULL );
	WB_SET_AUTO( PostLevelTransition, Bool, RestoreSpawnPoint, m_RestoreSpawnPoint );
	WB_SET_AUTO( PostLevelTransition, Hash, TeleportLabel, m_TeleportLabel );
	WB_DISPATCH_EVENT( pEventManager, PostLevelTransition, NULL );

	// Make a checkpoint save, for crash protection
	// (Queued because we want to pump the event queue once to initialize the world.)
	WB_MAKE_EVENT( Checkpoint, NULL );
	WB_LOG_EVENT( Checkpoint );
	WB_QUEUE_EVENT( pEventManager, Checkpoint, this );

	// Clear travel persistence now that we've successfully traveled and don't need it.
	ClearTravelPersistence();
}

void RosaGame::RefreshRTDependentSystems()
{
	XTRACE_FUNCTION;

	CreateGlobalAmbientQuad();
	CreatePostQuad();
	CreateSSAOQuad();
	CreateGradientQuad();
	CreateEdgeQuad();
	CreateLightCombineQuad();
#if ROSA_USE_WATERCOLOR_POST
	CreateEdgeQuad();
#endif
	CreateBloomQuads();
	CreateFXAAQuad();
	CreateMinimapBQuad();
	CreateMinimapFXAAQuad();
#if ROSA_USE_MAXIMAP
	CreateMaximapBQuad();
	CreateMaximapFXAAQuad();
#endif
	CreateUpscaleQuad();

	UpdateMinimap();
#if ROSA_USE_MAXIMAP
	UpdateMaximap();
#endif
	RefreshBloomParams();

	// Notify the world and any other observers that RTs are updated
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	RosaWorld* const		pWorld			= RosaFramework::GetInstance()->GetWorld();

	WB_MAKE_EVENT( OnRenderTargetsUpdated, NULL );
	WB_DISPATCH_EVENT( pEventManager, OnRenderTargetsUpdated, pWorld );
}

void RosaGame::UpdateMinimap()
{
	XTRACE_FUNCTION;

	UIWidgetImage* const		pMinimapImage	= GetMinimapImage();
	if( !pMinimapImage )
	{
		return;
	}

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	DEVASSERT( pTargetManager );

	STATIC_HASHED_STRING( MinimapA );
	IRenderTarget* const		pMinimapART		= pTargetManager->GetRenderTarget( sMinimapA );
	DEVASSERT( pMinimapART );

	ITexture* const				pTexture		= pMinimapART->GetColorTextureHandle( 0 );
	DEVASSERT( pTexture );

	pMinimapImage->SetTexture( pTexture, 0 );
}

#if ROSA_USE_MAXIMAP
void RosaGame::UpdateMaximap()
{
	XTRACE_FUNCTION;

	UIWidgetImage* const		pMaximapImage	= GetMaximapImage();
	if( !pMaximapImage )
	{
		return;
	}

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	DEVASSERT( pTargetManager );

	STATIC_HASHED_STRING( MaximapA );
	IRenderTarget* const		pMaximapART		= pTargetManager->GetRenderTarget( sMaximapA );
	DEVASSERT( pMaximapART );

	ITexture* const				pTexture		= pMaximapART->GetColorTextureHandle( 0 );
	DEVASSERT( pTexture );

	pMaximapImage->SetTexture( pTexture, 0 );
}
#endif

void RosaGame::SetGlobalCubemap( ITexture* const pCubemap )
{
	XTRACE_FUNCTION;

	m_GlobalCubemap = pCubemap;

	DEVASSERT( m_GlobalAmbientQuad );
	if( !m_GlobalAmbientQuad )
	{
		return;
	}

	m_GlobalAmbientQuad->SetTexture( 3, GetGlobalCubemap() );
}

void RosaGame::CreateGlobalAmbientQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_GlobalAmbientQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();

	m_GlobalAmbientQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "GlobalAmbient", "Material_GlobalAmbient" );
	m_GlobalAmbientQuad->SetVertexDeclaration( pRenderer->GetVertexDeclaration( VD_POSITIONS ) );	// We don't use UVs for ambient light shaders
	m_GlobalAmbientQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "GB_Albedo" )->GetColorTextureHandle( 0 ) );
	m_GlobalAmbientQuad->SetTexture( 1, pTargetManager->GetRenderTarget( "GB_Normal" )->GetColorTextureHandle( 0 ) );
	m_GlobalAmbientQuad->SetTexture( 2, pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 ) );
	m_GlobalAmbientQuad->SetTexture( 3, GetGlobalCubemap() );
}

void RosaGame::CreateUpscaleQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_UpscaleQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();

	// Accomodate different aspect ratios by using the proportional dimensions
	const float					LocationX		= static_cast<float>( ( pDisplay->m_FrameWidth - pDisplay->m_PropWidth ) / 2 );
	const float					LocationY		= static_cast<float>( ( pDisplay->m_FrameHeight - pDisplay->m_PropHeight ) / 2 );

	m_UpscaleQuad = CreateFullscreenQuad( pDisplay->m_PropWidth, pDisplay->m_PropHeight, "Upscale", "Material_Upscale" );
	m_UpscaleQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "UI" )->GetColorTextureHandle( 0 ) );
	m_UpscaleQuad->m_Location.x += LocationX;
	m_UpscaleQuad->m_Location.z += LocationY;
}

void RosaGame::CreatePostQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_PostQuad );
	SafeDelete( m_PostCheapQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	TextureManager* const		pTextureManager	= pRenderer->GetTextureManager();

#if ROSA_USE_FILMIC_POST
	m_PostQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "Post", "Material_Post" );
	m_PostQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "Primary" )->GetColorTextureHandle( 0 ) );
	m_PostQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_ColorGradingTexture.CStr() ) );
	m_PostQuad->SetTexture( 2, pTextureManager->GetTextureNoMips( m_NoiseTexture.CStr() ) );
	m_PostQuad->SetTexture( 3, pTargetManager->GetRenderTarget( "BloomAV" )->GetColorTextureHandle( 0 ) );
	m_PostQuad->SetTexture( 4, pTargetManager->GetRenderTarget( "BloomCV" )->GetColorTextureHandle( 0 ) );		// Use the lowest blur level for dirty lens effect, it's the most blurry (can switch to BloomBV if I see artifacts)
	m_PostQuad->SetTexture( 5, pTextureManager->GetTextureNoMips( m_DirtyLensTexture.CStr() ) );

	UpdateNoiseEnabled();
	UpdateDirtyLensEnabled();
	UpdateColorGradingEnabled();
#elif ROSA_USE_WATERCOLOR_POST
	m_PostQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "Post", "Material_PostWatercolor" );
	m_PostQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "Primary" )->GetColorTextureHandle( 0 ) );		// Main
	m_PostQuad->SetTexture( 1, pTargetManager->GetRenderTarget( "BloomAV" )->GetColorTextureHandle( 0 ) );		// Bloom/blur
	m_PostQuad->SetTexture( 2, pTextureManager->GetTextureNoMips( m_DisplaceTexture.CStr() ) );					// Displacement
	m_PostQuad->SetTexture( 3, pTextureManager->GetTextureNoMips( m_BlotTexture.CStr() ) );						// Blotting
	m_PostQuad->SetTexture( 4, pTextureManager->GetTextureNoMips( m_CanvasTexture.CStr() ) );					// Canvas
	m_PostQuad->SetTexture( 5, pTargetManager->GetRenderTarget( "GB_Albedo" )->GetColorTextureHandle( 0 ) );	// GB_Albedo is repurposed to store edges
	m_PostQuad->SetTexture( 6, pTextureManager->GetTextureNoMips( m_ColorGradingTexture.CStr() ) );				// Color grading
	m_PostQuad->SetTexture( 7, pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 ) );		// Depth

	UpdateDisplaceEnabled();
	UpdateBlurEnabled();
	UpdateBlotEnabled();
	UpdateCanvasEnabled();
	UpdateEdgeEnabled();
	UpdateColorGradingEnabled();
#endif

	m_PostCheapQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "Post", "Material_PostCheap" );
	m_PostCheapQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "Primary" )->GetColorTextureHandle( 0 ) );		// Main

#if BUILD_ROSA_TOOLS
	SafeDelete( m_PostToolsQuad );

	m_PostToolsQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "PostTools", "Material_PostTools" );
	m_PostToolsQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "Primary" )->GetColorTextureHandle( 0 ) );
#endif
}

void RosaGame::CreateSSAOQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_SSAOQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	TextureManager* const		pTextureManager	= pRenderer->GetTextureManager();

	m_SSAOQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "SSAO", "Material_SSAO" );
	m_SSAOQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 ) );
	m_SSAOQuad->SetTexture( 1, pTextureManager->GetTexture( "Textures/Post/random-cos-sin.tga" ) );	// HACKHACK: Hard-coded asset!
}

void RosaGame::CreateGradientQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_GradientQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();

	m_GradientQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "Gradient", "Material_Gradient" );
	m_GradientQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "GB_Normal" )->GetColorTextureHandle( 0 ) );
	m_GradientQuad->SetTexture( 1, pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 ) );
}

void RosaGame::CreateEdgeQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_EdgeQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();

	m_EdgeQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "Edge", "Material_Edge" );
	m_EdgeQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "GB_Normal" )->GetColorTextureHandle( 0 ) );
	m_EdgeQuad->SetTexture( 1, pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 ) );
}

void RosaGame::CreateLightCombineQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_LightCombineQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	IRenderTarget* const		pGB_LAccum		= pTargetManager->GetRenderTarget( "GB_LAccum" );
	IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
	IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );

	m_LightCombineQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "LightCombine", "Material_LightCombine" );
	m_LightCombineQuad->SetTexture( 0, pGB_LAccum->GetColorTextureHandle( 0 ) );
	m_LightCombineQuad->SetTexture( 1, pGB_Albedo->GetColorTextureHandle( 0 ) );
	m_LightCombineQuad->SetTexture( 2, pGB_Depth->GetColorTextureHandle( 0 ) );
}

#if ROSA_USE_WATERCOLOR_POST
void RosaGame::CreateEdgeQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_EdgeQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
	IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );

	m_EdgeQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "Edge", "Material_Edge" );
	m_EdgeQuad->SetTexture( 0, pGB_Normal->GetColorTextureHandle( 0 ) );
	m_EdgeQuad->SetTexture( 1, pGB_Depth->GetColorTextureHandle( 0 ) );
}
#endif

void RosaGame::CreateBloomQuads()
{
	XTRACE_FUNCTION;

	FOR_EACH_ARRAY( BloomQuadIter, m_BloomQuads, Mesh* )
	{
		Mesh* pBloomQuad = BloomQuadIter.GetValue();
		SafeDelete( pBloomQuad );
	}
	m_BloomQuads.Clear();

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	IRenderer* const			pRenderer		= pFramework->GetRenderer();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	TextureManager* const		pTextureManager	= pRenderer->GetTextureManager();

	const uint BloomWidthA	= pDisplay->m_Width		/ ROSA_BLOOMA_SCALE;
	const uint BloomHeightA	= pDisplay->m_Height	/ ROSA_BLOOMA_SCALE;
	const uint BloomWidthB	= pDisplay->m_Width		/ ROSA_BLOOMB_SCALE;
	const uint BloomHeightB	= pDisplay->m_Height	/ ROSA_BLOOMB_SCALE;
	const uint BloomWidthC	= pDisplay->m_Width		/ ROSA_BLOOMC_SCALE;
	const uint BloomHeightC	= pDisplay->m_Height	/ ROSA_BLOOMC_SCALE;

	// Bloom A
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthA, BloomHeightA, "BloomA", "Material_BloomClip" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "Primary" )->GetColorTextureHandle( 0 ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthA, BloomHeightA, "BloomAH", "Material_Bloom" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomAV" )->GetColorTextureHandle( 0 ) );
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthA, BloomHeightA, "BloomAV", "Material_BloomSum" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomAH" )->GetColorTextureHandle( 0 ) );
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
		pBloomQuad->SetTexture( 2, pTargetManager->GetRenderTarget( "BloomBV" )->GetColorTextureHandle( 0 ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}

	// Bloom B
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthB, BloomHeightB, "BloomB", "Material_BloomCopy" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomAV" )->GetColorTextureHandle( 0 ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthB, BloomHeightB, "BloomBH", "Material_Bloom" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomBV" )->GetColorTextureHandle( 0 ) );
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthB, BloomHeightB, "BloomBV", "Material_BloomSum" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomBH" )->GetColorTextureHandle( 0 ) );
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
		pBloomQuad->SetTexture( 2, pTargetManager->GetRenderTarget( "BloomCV" )->GetColorTextureHandle( 0 ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}

	// Bloom C
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthC, BloomHeightC, "BloomC", "Material_BloomCopy" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomBV" )->GetColorTextureHandle( 0 ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthC, BloomHeightC, "BloomCH", "Material_Bloom" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomCV" )->GetColorTextureHandle( 0 ) );
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
	{
		Mesh* const pBloomQuad = CreateFullscreenQuad( BloomWidthC, BloomHeightC, "BloomCV", "Material_Bloom" );
		pBloomQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "BloomCH" )->GetColorTextureHandle( 0 ) );
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
		m_BloomQuads.PushBack( pBloomQuad );
	}
}

void RosaGame::CreateFXAAQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_FXAAQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	Display* const				pDisplay		= pFramework->GetDisplay();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();

	m_FXAAQuad = CreateFullscreenQuad( pDisplay->m_Width, pDisplay->m_Height, "FXAA", "Material_FXAA" );
	m_FXAAQuad->SetTexture( 0, pTargetManager->GetRenderTarget( "Post" )->GetColorTextureHandle( 0 ) );
}

void RosaGame::CreateMinimapBQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_MinimapBQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	TextureManager* const		pTextureManager	= pFramework->GetRenderer()->GetTextureManager();
	IRenderTarget* const		pMinimapART		= pTargetManager->GetRenderTarget( "MinimapA" );

	const uint					MinimapRTWidth	= pMinimapART->GetWidth();
	const uint					MinimapRTHeight	= pMinimapART->GetHeight();

	m_MinimapBQuad = CreateFullscreenQuad( MinimapRTWidth, MinimapRTHeight, "MinimapB", "Material_MinimapB" );
	m_MinimapBQuad->SetTexture( 0, pMinimapART->GetColorTextureHandle( 0 ) );
	m_MinimapBQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_MinimapTonesTexture.CStr() ) );
	m_MinimapBQuad->SetTexture( 2, pTextureManager->GetTextureNoMips( m_MinimapFloorTexture.CStr() ) );
	m_MinimapBQuad->SetTexture( 3, pTextureManager->GetTextureNoMips( m_MinimapSolidTexture.CStr() ) );
}

void RosaGame::CreateMinimapFXAAQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_MinimapFXAAQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	IRenderTarget* const		pMinimapBRT		= pTargetManager->GetRenderTarget( "MinimapB" );

	const uint					MinimapRTWidth	= pMinimapBRT->GetWidth();
	const uint					MinimapRTHeight	= pMinimapBRT->GetHeight();

	m_MinimapFXAAQuad = CreateFullscreenQuad( MinimapRTWidth, MinimapRTHeight, "MinimapFXAA", "Material_MinimapFXAA" );
	m_MinimapFXAAQuad->SetTexture( 0, pMinimapBRT->GetColorTextureHandle( 0 ) );
}

#if ROSA_USE_MAXIMAP
void RosaGame::CreateMaximapBQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_MaximapBQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	TextureManager* const		pTextureManager	= pFramework->GetRenderer()->GetTextureManager();
	IRenderTarget* const		pMaximapART		= pTargetManager->GetRenderTarget( "MaximapA" );

	const uint					MaximapRTWidth	= pMaximapART->GetWidth();
	const uint					MaximapRTHeight	= pMaximapART->GetHeight();

	m_MaximapBQuad = CreateFullscreenQuad( MaximapRTWidth, MaximapRTHeight, "MaximapB", "Material_MinimapB" );
	m_MaximapBQuad->SetTexture( 0, pMaximapART->GetColorTextureHandle( 0 ) );
	m_MaximapBQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_MinimapTonesTexture.CStr() ) );
	m_MaximapBQuad->SetTexture( 2, pTextureManager->GetTextureNoMips( m_MinimapFloorTexture.CStr() ) );
	m_MaximapBQuad->SetTexture( 3, pTextureManager->GetTextureNoMips( m_MinimapSolidTexture.CStr() ) );
}
#endif

#if ROSA_USE_MAXIMAP
void RosaGame::CreateMaximapFXAAQuad()
{
	XTRACE_FUNCTION;

	SafeDelete( m_MaximapFXAAQuad );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
	IRenderTarget* const		pMaximapBRT		= pTargetManager->GetRenderTarget( "MaximapB" );

	const uint					MaximapRTWidth	= pMaximapBRT->GetWidth();
	const uint					MaximapRTHeight	= pMaximapBRT->GetHeight();

	m_MaximapFXAAQuad = CreateFullscreenQuad( MaximapRTWidth, MaximapRTHeight, "MaximapFXAA", "Material_MinimapFXAA" );
	m_MaximapFXAAQuad->SetTexture( 0, pMaximapBRT->GetColorTextureHandle( 0 ) );
}
#endif

Mesh* RosaGame::CreateFullscreenQuad( const uint Width, const uint Height, const HashedString& PrescribedBucket, const SimpleString& MaterialDef )
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	IRenderer* const		pRenderer	= pFramework->GetRenderer();

	const float QuadWidth		= static_cast<float>( Width );
	const float QuadHeight		= static_cast<float>( Height );
	const float OffsetWidth		= 0.5f * QuadWidth;
	const float OffsetHeight	= 0.5f * QuadHeight;

	Mesh* const pFSQuadMesh = pRenderer->GetMeshFactory()->CreatePlane( QuadWidth, QuadHeight, 1, 1, XZ_PLANE, false );
	pFSQuadMesh->m_Location = Vector( OffsetWidth, 0.0f, OffsetHeight );
	pFSQuadMesh->SetVertexDeclaration( pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS ) );	// Override what CreatePlane gives us
	pFSQuadMesh->SetBucket( PrescribedBucket );
	pFSQuadMesh->SetMaterialDefinition( MaterialDef, pRenderer );

	return pFSQuadMesh;
}

void RosaGame::SetMinimapTextures( const SimpleString& TonesFilename, const SimpleString& FloorFilename, const SimpleString& SolidFilename )
{
	XTRACE_FUNCTION;

	m_MinimapTonesTexture = TonesFilename;
	m_MinimapFloorTexture = FloorFilename;
	m_MinimapSolidTexture = SolidFilename;

	// HACKHACK: Do this here because why not. It's sure to be called before minimap is rendered.
	STATICHASH( RosaMinimap );
	STATICHASH( MinimapHeightThreshold );
	m_MinimapHeightThreshold = ConfigManager::GetFloat( sMinimapHeightThreshold, 0.0f, sRosaMinimap );

	STATICHASH( MinimapHeightOffset );
	m_MinimapHeightOffset = ConfigManager::GetFloat( sMinimapHeightOffset, 0.0f, sRosaMinimap );

	STATICHASH( MinimapHeightDiffScale );
	m_MinimapHeightDiffScale = ConfigManager::GetFloat( sMinimapHeightDiffScale, 1.0f, sRosaMinimap );

	STATICHASH( MinimapHeightToneScale );
	m_MinimapHeightToneScale = ConfigManager::GetFloat( sMinimapHeightToneScale, 1.0f, sRosaMinimap );

	STATICHASH( MinimapRenderEdges );
	m_MinimapRenderEdges = ConfigManager::GetBool( sMinimapRenderEdges, false, sRosaMinimap ) ? 1.0f : 0.0f;

	STATICHASH( MinimapTileSize );
	m_MinimapRcpTileSize = 1.0f / ConfigManager::GetFloat( sMinimapTileSize, 1.0f, sRosaMinimap );

	if( m_MinimapBQuad )
	{
		RosaFramework* const pFramework = RosaFramework::GetInstance();
		DEVASSERT( pFramework );

		TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
		DEVASSERT( pTextureManager );

		m_MinimapBQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_MinimapTonesTexture.CStr() ) );
		m_MinimapBQuad->SetTexture( 2, pTextureManager->GetTextureNoMips( m_MinimapFloorTexture.CStr() ) );
		m_MinimapBQuad->SetTexture( 3, pTextureManager->GetTextureNoMips( m_MinimapSolidTexture.CStr() ) );
	}

#if ROSA_USE_MAXIMAP
	if( m_MaximapBQuad )
	{
		RosaFramework* const pFramework = RosaFramework::GetInstance();
		DEVASSERT( pFramework );

		TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
		DEVASSERT( pTextureManager );

		m_MaximapBQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_MinimapTonesTexture.CStr() ) );
		m_MaximapBQuad->SetTexture( 2, pTextureManager->GetTextureNoMips( m_MinimapFloorTexture.CStr() ) );
		m_MaximapBQuad->SetTexture( 3, pTextureManager->GetTextureNoMips( m_MinimapSolidTexture.CStr() ) );
	}
#endif
}

void RosaGame::SetColorGradingTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	m_ColorGradingTexture = TextureFilename;

	UpdateColorGradingEnabled();
}

void RosaGame::UpdateColorGradingEnabled()
{
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( ColorGrading );
	const bool ColorGrading = ConfigManager::GetBool( sColorGrading );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	STATICHASH( ColorGradingDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sColorGradingDisabledTexture, "" );
	const SimpleString&	TextureFilename	= ColorGrading ? m_ColorGradingTexture : DisabledTexture;
	ITexture* const		pTexture		= pTextureManager->GetTextureNoMips( TextureFilename.CStr() );
#if ROSA_USE_FILMIC_POST
	m_PostQuad->SetTexture( 1, pTexture );
#elif ROSA_USE_WATERCOLOR_POST
	m_PostQuad->SetTexture( 6, pTexture );
#endif
}

void RosaGame::SetNoiseTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	m_NoiseTexture = TextureFilename;

	UpdateNoiseEnabled();
}

void RosaGame::UpdateNoiseEnabled()
{
#if ROSA_USE_FILMIC_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( FilmGrain );
	const bool FilmGrain = ConfigManager::GetBool( sFilmGrain );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	STATICHASH( FilmGrainDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sFilmGrainDisabledTexture, "" );
	const SimpleString&	TextureFilename	= FilmGrain ? m_NoiseTexture : DisabledTexture;
	ITexture* const		pTexture		= pTextureManager->GetTextureNoMips( TextureFilename.CStr() );
	m_PostQuad->SetTexture( 2, pTexture );
#endif
}

void RosaGame::SetNoiseScaleRange( const float NoiseScaleLo, const float NoiseScaleHi )
{
	m_NoiseScaleRange.x = NoiseScaleLo;
	m_NoiseScaleRange.y = NoiseScaleHi;
}

void RosaGame::SetNoiseRange( const float NoiseRange )
{
	m_NoiseRange = NoiseRange;
}

void RosaGame::UpdateGraphicsOptionWidgets()
{
	STATIC_HASHED_STRING( GraphicsOptionsScreen );

	// Filmic
	STATIC_HASHED_STRING( BloomButton );
	STATIC_HASHED_STRING( DirtyLensButton );
	STATIC_HASHED_STRING( HalosButton );
	STATIC_HASHED_STRING( FilmGrainButton );

	// Watercolor
	STATIC_HASHED_STRING( DisplaceButton );
	STATIC_HASHED_STRING( BlurButton );
	STATIC_HASHED_STRING( BlotButton );
	STATIC_HASHED_STRING( EdgeButton );
	STATIC_HASHED_STRING( CanvasButton );

	STATIC_HASHED_STRING( ColorGradingButton );

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	const bool				Disabled		= m_UsePostCheapQuad;
	
	STATICHASH( Bloom );
	const bool				BloomDisabled	= Disabled || !ConfigManager::GetBool( sBloom );

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sBloomButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sDirtyLensButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, BloomDisabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sHalosButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, BloomDisabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sFilmGrainButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sDisplaceButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sBlurButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sBlotButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

#if ROSA_USE_WATERCOLOR_POST
	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sEdgeButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}
#endif

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sCanvasButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}

	{
		WB_MAKE_EVENT( SetWidgetDisabled, NULL );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sGraphicsOptionsScreen );
		WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sColorGradingButton );
		WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetDisabled, NULL );
	}
}

void RosaGame::UpdateBloomEnabled()
{
	UpdateGraphicsOptionWidgets();
}

void RosaGame::SetDirtyLensTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	m_DirtyLensTexture = TextureFilename;

	UpdateDirtyLensEnabled();
}

void RosaGame::UpdateDirtyLensEnabled()
{
#if ROSA_USE_FILMIC_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( DirtyLens );
	const bool DirtyLens = ConfigManager::GetBool( sDirtyLens );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	STATICHASH( DirtyLensDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sDirtyLensDisabledTexture, "" );
	const SimpleString&	TextureFilename	= DirtyLens ? m_DirtyLensTexture : DisabledTexture;
	ITexture* const		pTexture		= pTextureManager->GetTextureNoMips( TextureFilename.CStr() );
	m_PostQuad->SetTexture( 5, pTexture );
#endif
}

void RosaGame::UpdateHalosEnabled()
{
#if ROSA_USE_FILMIC_POST
	STATICHASH( Halos );
	m_HalosEnabled = ConfigManager::GetBool( sHalos );
#endif
}

void RosaGame::SetBloomKernelTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	m_BloomKernelTexture = TextureFilename;

	FOR_EACH_ARRAY( BloomQuadIter, m_BloomQuads, Mesh* )
	{
		// HACKHACK: Skip clip/copy bloom quads
		if( BloomQuadIter.GetIndex() % 3 == 0 )
		{
			continue;
		}

		Mesh* pBloomQuad = BloomQuadIter.GetValue();
		pBloomQuad->SetTexture( 1, pTextureManager->GetTextureNoMips( m_BloomKernelTexture.CStr() ) );
	}
}

void RosaGame::SetDisplaceTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	m_DisplaceTexture = TextureFilename;

	UpdateDisplaceEnabled();
}

void RosaGame::UpdateBlurEnabled()
{
#if ROSA_USE_WATERCOLOR_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( Blur );
	const bool Blur = ConfigManager::GetBool( sBlur );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaTargetManager* const pTargetManager = pFramework->GetTargetManager();
	ASSERT( pTargetManager );

	const SimpleString	TargetName		= Blur ? "BloomAV" : "Primary";
	ITexture* const		pTexture		= pTargetManager->GetRenderTarget( TargetName )->GetColorTextureHandle( 0 );
	m_PostQuad->SetTexture( 1, pTexture );
#endif
}

void RosaGame::UpdateEdgeEnabled()
{
#if ROSA_USE_WATERCOLOR_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( Edge );
	const bool Edge = ConfigManager::GetBool( sEdge );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	RosaTargetManager* const pTargetManager = pFramework->GetTargetManager();
	ASSERT( pTargetManager );

	STATICHASH( EdgeDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sEdgeDisabledTexture, "" );
	ITexture* const		pNoEdgeTexture	= pTextureManager->GetTextureNoMips( DisabledTexture.CStr() );
	ITexture* const		pEdgeTexture	= pTargetManager->GetRenderTarget( "GB_Albedo" )->GetColorTextureHandle( 0 );
	ITexture* const		pTexture		= Edge ? pEdgeTexture : pNoEdgeTexture;
	m_PostQuad->SetTexture( 5, pTexture );
#endif
}

void RosaGame::UpdateDisplaceEnabled()
{
#if ROSA_USE_WATERCOLOR_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( Displace );
	const bool Displace = ConfigManager::GetBool( sDisplace );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	STATICHASH( DisplaceDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sDisplaceDisabledTexture, "" );
	const SimpleString&	TextureFilename	= Displace ? m_DisplaceTexture : DisabledTexture;
	ITexture* const		pTexture		= pTextureManager->GetTextureNoMips( TextureFilename.CStr() );
	m_PostQuad->SetTexture( 2, pTexture );
#endif
}

void RosaGame::SetBlotTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	m_BlotTexture = TextureFilename;

	UpdateBlotEnabled();
}

void RosaGame::UpdateBlotEnabled()
{
#if ROSA_USE_WATERCOLOR_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( Blot );
	const bool Blot = ConfigManager::GetBool( sBlot );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	STATICHASH( BlotDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sBlotDisabledTexture, "" );
	const SimpleString&	TextureFilename	= Blot ? m_BlotTexture : DisabledTexture;
	ITexture* const		pTexture		= pTextureManager->GetTextureNoMips( TextureFilename.CStr() );
	m_PostQuad->SetTexture( 3, pTexture );
#endif
}

void RosaGame::SetCanvasTexture( const SimpleString& TextureFilename )
{
	XTRACE_FUNCTION;

	m_CanvasTexture = TextureFilename;

	UpdateCanvasEnabled();
}

void RosaGame::UpdateCanvasEnabled()
{
#if ROSA_USE_WATERCOLOR_POST
	DEVASSERT( m_PostQuad );
	if( !m_PostQuad )
	{
		return;
	}

	STATICHASH( Canvas );
	const bool Canvas = ConfigManager::GetBool( sCanvas );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	TextureManager* const pTextureManager = pFramework->GetRenderer()->GetTextureManager();
	ASSERT( pTextureManager );

	STATICHASH( CanvasDisabledTexture );
	const SimpleString	DisabledTexture	= ConfigManager::GetString( sCanvasDisabledTexture, "" );
	const SimpleString&	TextureFilename	= Canvas ? m_CanvasTexture : DisabledTexture;
	ITexture* const		pTexture		= pTextureManager->GetTextureNoMips( TextureFilename.CStr() );
	m_PostQuad->SetTexture( 4, pTexture );
#endif
}

void RosaGame::UpdatePostCheapEnabled()
{
	STATICHASH( PostCheap );
	m_UsePostCheapQuad = ConfigManager::GetBool( sPostCheap );

	// HACKHACK: Also set UI button state
	UpdateGraphicsOptionWidgets();
}

void RosaGame::SetFogColors( const Vector4& FogColorNearLo, const Vector4& FogColorFarLo, const Vector4& FogColorNearHi, const Vector4& FogColorFarHi )
{
	m_FogColors = Matrix( FogColorNearLo, FogColorFarLo, FogColorNearHi, FogColorFarHi );

#if BUILD_WINDOWS_NO_SDL
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	IRenderer* const		pRenderer		= pFramework->GetRenderer();
	DEVASSERT( pRenderer );

	if( pRenderer->IsOpenGL() )
#endif // BUILD_WINDOWS_NO_SDL
	{
		// HACKHACK: Transpose this on GL, since we're just using this as a container of 4 vec4s and GLSL uses column-major indexing.
		m_FogColors.Transpose();
	}
}

void RosaGame::SetFogCurves( const Vector4& FogNearFarCurve, const Vector4& FogLoHiCurve )
{
	// HACKHACK: Save this off, it's used for height fog
	const float	HeightFogExp	= m_FogLoHiCurve.y;

	m_FogNearFarCurve			= FogNearFarCurve;
	m_FogLoHiCurve				= FogLoHiCurve;

	// HACKHACK: Restore this, it's  used for height fog
	m_FogLoHiCurve.y			= HeightFogExp;
}

void RosaGame::SetFogParams( const float FogNear, const float FogFar, const float EmissiveMax, const float Exposure, const float FogLightDensity )
{
	XTRACE_FUNCTION;

	const float	FogRange		= FogFar - FogNear;
	const bool	HasValidRange	= Abs( FogRange ) > EPSILON;

	m_FogParams.x		= FogNear;
	m_FogParams.y		= HasValidRange ? ( 1.0f / FogRange ) : 0.0f;
	m_FogParams.z		= EmissiveMax;

	m_FogLightParams.x	= FogLightDensity;

	SetExposure( Exposure );
}

void RosaGame::UpdateFogEnabled()
{
	STATICHASH( Fog );
	m_FogEnabled = ConfigManager::GetBool( sFog );
}

void RosaGame::SetHeightFogParams( const float HeightFogLo, const float HeightFogHi, const float HeightFogExp, const float HeightFogLoExp, const float HeightFogHiExp )
{
	XTRACE_FUNCTION;

	const float	HeightFogRange	= HeightFogHi - HeightFogLo;
	const bool	HasValidRange	= Abs( HeightFogRange ) > EPSILON;

	m_HeightFogParams.x	= HeightFogLo;
	m_HeightFogParams.y	= HasValidRange ? ( 1.0f / HeightFogRange ) : 0.0f;
	m_HeightFogParams.z	= HeightFogLoExp;
	m_HeightFogParams.w	= HeightFogHiExp;

	// HACKHACK: Shove height fog exp (the curve between lo/hi points) into unused member here
	m_FogLoHiCurve.y	= HeightFogExp;
}

void RosaGame::AdjustFogRegionScalar( const Vector4& RegionFogScalar, const float DeltaTime )
{
	const float		ClampedDT	= Min( DeltaTime, 1.0f );	// Prevent overshooting
	const Vector4	Deltas		= RegionFogScalar - m_RegionFogScalar;
	const Vector4	NewScalar	= m_RegionFogScalar + Deltas * ClampedDT;

	SetRegionFogScalar( NewScalar );
}

void RosaGame::AdjustExposure( const float Exposure, const float DeltaTime )
{
	const float		OldExposure	= m_FogParams.w;
	const float		LoExposure	= Min( OldExposure, Exposure );
	const float		HiExposure	= Max( OldExposure, Exposure );

	// HACKHACK: Hard-coded velocity scalar, relative to exposure delta
	// This makes the eye adjust quickly at first and then ease toward stability
	// Use a lower velocity if exposure is increasing (i.e. we're moving into a darker region);
	// this emulates the eye adjusting more slowly to darkness than to light.
	const float		Delta		= Exposure - OldExposure;
	const bool		Increasing	= ( Delta > 0.0f );
	const float		Velocity	= Delta * ( Increasing ? 0.5f : 2.0f );
	const float		NewExposure	= Clamp( OldExposure + Velocity * DeltaTime, LoExposure, HiExposure );

	SetExposure( NewExposure );
}

void RosaGame::AdjustMinimapScalar( const float MinimapScalar, const float DeltaTime )
{
	const float	OldMinimapScalar	= m_CurrentMinimapScalar;
	const float	LoMinimapScalar		= Min( OldMinimapScalar, MinimapScalar );
	const float	HiMinimapScalar		= Max( OldMinimapScalar, MinimapScalar );

	// HACKHACK: Hard-coded velocity scalar, relative to delta
	const float	Delta				= MinimapScalar - OldMinimapScalar;
	const float	Velocity			= Delta * 5.0f;
	const float	NewMinimapScalar	= Clamp( OldMinimapScalar + Velocity * DeltaTime, LoMinimapScalar, HiMinimapScalar );

	SetMinimapScalar( NewMinimapScalar );
}

void RosaGame::AdjustEdgeColorHSV( const Vector& EdgeColorHSV, const float DeltaTime )
{
	const float		Velocity		= 4.0f;	// TODO: Configure?
	const float		ClampedDT		= Min( Velocity * DeltaTime, 1.0f );	// Prevent overshooting
	const Vector	Deltas			= EdgeColorHSV - m_EdgeColorHSV;
	const Vector	NewEdgeColorHSV	= m_EdgeColorHSV + Deltas * ClampedDT;

	SetEdgeColorHSV( NewEdgeColorHSV );
}

void RosaGame::SetAmbience( const SimpleString& Ambience )
{
	if( Ambience == m_CurrentAmbience )
	{
		return;
	}

	m_CurrentAmbience = Ambience;
	m_Music->PlayAmbience( Ambience );
}

void RosaGame::SetReverb( const SimpleString& ReverbDef )
{
	if( ReverbDef == m_CurrentReverbDef )
	{
		return;
	}

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	IAudioSystem* const		pAudioSystem	= pFramework->GetAudioSystem();
	DEVASSERT( pAudioSystem );

	m_CurrentReverbDef = ReverbDef;
	pAudioSystem->SetReverbParams( ReverbDef );
}

void RosaGame::SetSkyParams( const Vector4& SunVector, const Vector4& SkyColorHi, const Vector4& SkyColorLo )
{
	m_SunVector		= SunVector;
	m_SkyColorHi	= SkyColorHi;
	m_SkyColorLo	= SkyColorLo;
}

void RosaGame::SetEdgeColorHSV( const Vector& EdgeColorHSV )
{
	m_EdgeColorHSV					= EdgeColorHSV;

	const Vector	EdgeColorRGB	= HSV::HSVToRGB( EdgeColorHSV );
	m_EdgeColor						= Vector4( EdgeColorRGB, 1.0f );
	m_EdgeBackColor					= Vector4( EdgeColorRGB, 0.0f );
}

void RosaGame::SetBloomRadius( const float VerticalRadius, const float AspectRatio )
{
	m_BloomVerticalRadius	= VerticalRadius;
	m_BloomAspectRatio		= AspectRatio;
	RefreshBloomParams();
}

void RosaGame::RefreshBloomParams()
{
	RosaFramework* const	pFramework			= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	Display* const			pDisplay			= pFramework->GetDisplay();
	DEVASSERT( pDisplay );

	const float				DisplayWidth		= static_cast<float>( pDisplay->m_Width );
	const float				DisplayHeight		= static_cast<float>( pDisplay->m_Height );
	const float				AspectRatio		= m_BloomAspectRatio * ( DisplayHeight / DisplayWidth );
	const float				BloomStepRadius	= m_BloomVerticalRadius / ROSA_BLOOM_TAPS;	// NOTE: Must be kept in tune with the number of steps in shader!

	m_BloomStepRadiusH = Vector2( AspectRatio * BloomStepRadius, 0.0f );
	m_BloomStepRadiusV = Vector2( 0.0f, BloomStepRadius );
}

Vector2 RosaGame::GetBloomRadius()
{
	// HACKHACK: Alternate between providing the horizontal and vertical params
	const bool ReturnVertical = m_ReturnVerticalBloomRadius;
	m_ReturnVerticalBloomRadius = !m_ReturnVerticalBloomRadius;
	return ReturnVertical ? m_BloomStepRadiusV : m_BloomStepRadiusH;
}

SimpleString RosaGame::GetTitleScreenLevelName() const
{
	STATICHASH( RosaWorld );
	STATICHASH( TitleScreenLevel );
	return ConfigManager::GetString( sTitleScreenLevel, "", sRosaWorld );
}

bool RosaGame::IsInTitleScreen() const
{
	return m_CurrentLevelName == GetTitleScreenLevelName();
}

SimpleString RosaGame::GetInitialLevelName() const
{
	STATICHASH( RosaWorld );
	STATICHASH( InitialLevel );
	return ConfigManager::GetString( sInitialLevel, "", sRosaWorld );
}

SimpleString RosaGame::GetHubLevelName() const
{
	STATICHASH( RosaWorld );
	STATICHASH( HubLevel );
	return ConfigManager::GetString( sHubLevel, "", sRosaWorld );
}

bool RosaGame::IsInHub() const
{
	STATICHASH( IsHub );
	return ConfigManager::GetBool( sIsHub, false, m_CurrentLevelName );
}

bool RosaGame::ShouldLowerWeapon() const
{
	STATICHASH( LowerWeapon );
	return ConfigManager::GetBool( sLowerWeapon, false, m_CurrentLevelName );
}

void RosaGame::SetUIRetreatDisabled( const bool Disabled )
{
	STATIC_HASHED_STRING( PauseScreen );
	STATIC_HASHED_STRING( PausedRetreatButton );

	WB_MAKE_EVENT( SetWidgetDisabled, NULL );
	WB_SET_AUTO( SetWidgetDisabled, Hash, Screen, sPauseScreen );
	WB_SET_AUTO( SetWidgetDisabled, Hash, Widget, sPausedRetreatButton );
	WB_SET_AUTO( SetWidgetDisabled, Bool, Disabled, Disabled );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetDisabled, NULL );
}

void RosaGame::RefreshUIRetreatEnabled()
{
	SetUIRetreatDisabled( IsInHub() );
}

// For a purchase link in the demo, if I do that.
void RosaGame::LaunchWebSite( const SimpleString& URL )
{
#if BUILD_WINDOWS
	ShellExecute( NULL, "open", URL.CStr(), NULL, NULL, SW_SHOWNORMAL );
#elif BUILD_MAC
	const SimpleString Command = SimpleString::PrintF( "open %s", URL.CStr() );
	system( Command.CStr() );
#elif BUILD_LINUX
	const SimpleString Command = SimpleString::PrintF( "xdg-open %s", URL.CStr() );
	system( Command.CStr() );
#endif
}

void RosaGame::OpenUserDataPath()
{
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	const SimpleString		UserDataPath	= pFramework->GetUserDataPath();

#if BUILD_WINDOWS
	ShellExecute( NULL, "open", UserDataPath.CStr(), NULL, NULL, SW_SHOWNORMAL );
#elif BUILD_MAC
	// HACKHACK: Escape spaces on Mac because of "Application\ Support"
	const SimpleString EscapedPath = UserDataPath.Replace( " ", "\\ " );
	const SimpleString Command = SimpleString::PrintF( "open %s", EscapedPath.CStr() );
	system( Command.CStr() );
#elif BUILD_LINUX
	// HACKHACK: Probably need to escape spaces Linux too (untested)
	const SimpleString EscapedPath = UserDataPath.Replace( " ", "\\ " );
	const SimpleString Command = SimpleString::PrintF( "xdg-open %s", EscapedPath.CStr() );
	system( Command.CStr() );
#endif
}

/*static*/ WBEntity* RosaGame::GetPlayer()
{
	const Array<WBCompRosaPlayer*>* const pPlayers = WBComponentArrays::GetComponents<WBCompRosaPlayer>();
	if( !pPlayers )
	{
		return NULL;
	}

	const Array<WBCompRosaPlayer*>& Players = *pPlayers;
	if( Players.Empty() )
	{
		return NULL;
	}

	WBCompRosaPlayer* const pPlayer = Players[0];
	DEVASSERT( pPlayer );

	return pPlayer->GetEntity();
}

/*static*/ Vector RosaGame::GetPlayerLocation()
{
	WBEntity* const pPlayer = GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaTransform* const pTransform = pPlayer->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	return pTransform->GetLocation();
}

/*static*/ Angles RosaGame::GetPlayerOrientation()
{
	WBEntity* const pPlayer = GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaTransform* const pTransform = pPlayer->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	return pTransform->GetOrientation();
}

/*static*/ Vector RosaGame::GetPlayerViewLocation()
{
	WBEntity* const pPlayer = GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaTransform* const pTransform = pPlayer->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCamera* const pCamera = WB_GETCOMP( pPlayer, RosaCamera );
	DEVASSERT( pCamera );

	return pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, pTransform->GetLocation() );
}

/*static*/ Angles RosaGame::GetPlayerViewOrientation()
{
	WBEntity* const pPlayer = GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaTransform* const pTransform = pPlayer->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCamera* const pCamera = WB_GETCOMP( pPlayer, RosaCamera );
	DEVASSERT( pCamera );

	return pCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, pTransform->GetOrientation() );
}

/*static*/ Vector RosaGame::GetPlayerFeetLocation()
{
	WBEntity* const				pPlayer		= GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaTransform* const	pTransform	= pPlayer->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pPlayer, RosaCollision );
	DEVASSERT( pCollision );

	return pTransform->GetLocation() - Vector( 0.0f, 0.0f, pCollision->GetExtents().z );
}

// HACKHACK so I'm not doing so many lookups for player feet in minimap SDPs
static Vector sCachedPlayerFeetLocation;
/*static*/ void RosaGame::CachePlayerFeetLocation()
{
	sCachedPlayerFeetLocation = GetPlayerFeetLocation();
}

/*static*/ Vector& RosaGame::GetCachedPlayerFeetLocation()
{
	return sCachedPlayerFeetLocation;
}

/*static*/ bool RosaGame::IsPlayerAlive()
{
	WBEntity* const pPlayer = GetPlayer();
	if( !pPlayer )
	{
		return false;
	}

	WBCompRosaHealth* const pHealth = WB_GETCOMP( pPlayer, RosaHealth );
	DEVASSERT( pHealth );

	if( pHealth->IsDead() )
	{
		return false;
	}

	return true;
}

/*static*/ bool RosaGame::IsPlayerDisablingPause()
{
	WBEntity* const pPlayer = GetPlayer();
	if( !pPlayer )
	{
		return false;
	}

	WBCompRosaPlayer* const pPlayerComponent = WB_GETCOMP( pPlayer, RosaPlayer );
	DEVASSERT( pPlayerComponent );

	return pPlayerComponent->IsDisablingPause();
}

/*static*/ bool RosaGame::IsPlayerVisible()
{
	WBEntity* const pPlayer = GetPlayer();
	if( !pPlayer )
	{
		return false;
	}

	WBCompRosaVisible* const pVisible = WB_GETCOMP( pPlayer, RosaVisible );
	DEVASSERT( pVisible );

	return pVisible->IsVisible();
}

/*static*/ bool RosaGame::IsGamePaused()
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	UIManager* const			pUIManager	= pFramework->GetUIManager();
	DEVASSERT( pUIManager );

	UIStack* const				pUIStack	= pUIManager->GetUIStack();
	DEVASSERT( pUIStack );

	return pUIStack->PausesGame();
}

/*static*/ UIWidgetImage* RosaGame::GetMinimapImage()
{
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	UIManager* const		pUIManager		= pFramework->GetUIManager();
	DEVASSERT( pUIManager );

	STATIC_HASHED_STRING( HUD );
	UIScreen* const			pHUDScreen		= pUIManager->GetScreen( sHUD );
	DEVASSERT( pHUDScreen );

	STATIC_HASHED_STRING( Minimap );
	UIWidgetImage* const	pMinimapImage	= pHUDScreen->GetWidget<UIWidgetImage>( sMinimap );

	return pMinimapImage;
}

#if ROSA_USE_MAXIMAP
/*static*/ UIWidgetImage* RosaGame::GetMaximapImage()
{
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	UIManager* const		pUIManager		= pFramework->GetUIManager();
	DEVASSERT( pUIManager );

	STATIC_HASHED_STRING( HUD );
	UIScreen* const			pHUDScreen		= pUIManager->GetScreen( sHUD );
	DEVASSERT( pHUDScreen );

	STATIC_HASHED_STRING( Maximap );
	UIWidgetImage* const	pMaximapImage	= pHUDScreen->GetWidget<UIWidgetImage>( sMaximap );

	return pMaximapImage;
}
#endif
