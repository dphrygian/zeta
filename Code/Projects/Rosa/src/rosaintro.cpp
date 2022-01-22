#include "core.h"
#include "rosaintro.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "meshfactory.h"
#include "simplestring.h"
#include "material.h"
#include "mesh.h"
#include "mathcore.h"
#include "configmanager.h"
#include "view.h"
#include "wbworld.h"
#include "wbeventmanager.h"
#include "itexture.h"
#include "texturemanager.h"
#include "inputsystem.h"
#include "Components/wbcomprosamesh.h"

#define SKIP_INTRO BUILD_DEV && 0

RosaIntro::RosaIntro()
:	m_Phase( EIP_None )
,	m_CurrentTime( 0.0f )
,	m_FadeInDuration( 0.0f )
,	m_FinishedTime( 0.0f )
,	m_IntroFadeOutTime( 0.0f )
,	m_IntroFadeOutDuration( 0.0f )
,	m_TitleFadeInDuration( 0.0f )
,	m_ColorGrading()
,	m_OldColorGrading()
{
	Initialize();
}

RosaIntro::~RosaIntro()
{
	ShutDown();
}

void RosaIntro::StartPhaseRunning( const float InitialDelay )
{
	m_Phase = EIP_Running;

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	RosaGame* const			pGame		= pFramework->GetGame();

	if( m_ColorGrading != "" )
	{
		pGame->SetColorGradingTexture( m_ColorGrading );
	}

	WB_MAKE_EVENT( FadeIn, NULL );
	WB_SET_AUTO( FadeIn, Float, Duration, m_FadeInDuration );
	if( InitialDelay == 0.0f )	// If there's no delay, we want to dispatch immediately, NOT queue and suffer the one-frame flicker
	{
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), FadeIn, NULL );
	}
	else
	{
		WB_QUEUE_EVENT_DELAY( WBWorld::GetInstance()->GetEventManager(), FadeIn, NULL, InitialDelay );
	}

	// Push the logo screen immediately; it will be rendered at zero alpha at first
	STATIC_HASHED_STRING( MKGLogoScreen );
	WB_MAKE_EVENT( PushUIScreen, NULL );
	WB_SET_AUTO( PushUIScreen, Hash, Screen, sMKGLogoScreen );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), PushUIScreen, NULL );

	// Let in-level script know we're starting
	WB_MAKE_EVENT( OnIntroStarted, NULL );
	WB_QUEUE_EVENT_DELAY( WBWorld::GetInstance()->GetEventManager(), OnIntroStarted, NULL, InitialDelay );
}

void RosaIntro::StartPhaseFinished()
{
	m_Phase = EIP_Finished;

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	InputSystem* const		pInputSystem	= pFramework->GetInputSystem();

	if( m_OldColorGrading != "" )
	{
		pGame->SetColorGradingTexture( m_OldColorGrading );
	}

	pInputSystem->PopAllContexts();

	RestoreView();

	// Queue this so we'll continue to render the logo until the next tick
	WB_MAKE_EVENT( ResetToInitialScreens, NULL );
	WB_QUEUE_EVENT( WBWorld::GetInstance()->GetEventManager(), ResetToInitialScreens, NULL );

	WB_MAKE_EVENT( FadeIn, NULL );
	WB_SET_AUTO( FadeIn, Float, Duration, m_TitleFadeInDuration );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), FadeIn, NULL );
}

void RosaIntro::Tick( const float DeltaTime )
{
	if( !IsRunning() )
	{
		return;
	}

	// Accumulates drift, I don't care
	const float LastTime = m_CurrentTime;
	m_CurrentTime += DeltaTime;

	if( m_Phase == EIP_Running && m_CurrentTime >= m_FinishedTime )
	{
		StartPhaseFinished();
	}

	if( LastTime < m_IntroFadeOutTime && m_CurrentTime >= m_IntroFadeOutTime )
	{
		WB_MAKE_EVENT( FadeOut, NULL );
		WB_SET_AUTO( FadeOut, Float, Duration, m_IntroFadeOutDuration );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), FadeOut, NULL );
	}

	// Constantly slam the FOV back, too
	if( m_Phase != EIP_Finished )
	{
		SetView();
	}
}

void RosaIntro::Initialize()
{
#if BUILD_DEV
	// (Safeguard against making intro skippable in final)
#if SKIP_INTRO
	const bool SkipIntro = true;
#else
	STATICHASH( SkipIntro );
	const bool SkipIntro = ConfigManager::GetBool( sSkipIntro );
#endif

	if( SkipIntro )
	{
		// Let in-level script know we're skipping the intro
		// Queue because stuff hasn't finished initializing in framework yet
		WB_MAKE_EVENT( OnIntroSkipped, NULL );
		WB_QUEUE_EVENT( WBWorld::GetInstance()->GetEventManager(), OnIntroSkipped, NULL );

		StartPhaseFinished();
		return;
	}
#endif

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	InputSystem* const		pInputSystem	= pFramework->GetInputSystem();

	// Need to set this before StartPhaseFinished
	m_OldColorGrading		= pGame->GetColorGradingTexture();

	STATICHASH( RosaIntro );
	STATICHASH( ColorGrading );
	m_ColorGrading			= ConfigManager::GetString( sColorGrading, "", sRosaIntro );

	m_FadeInDuration		= 0.5f;

	// HACKHACK for video capture
	STATICHASH( IntroDelay );
	const float InitialDelay	= ConfigManager::GetFloat( sIntroDelay, 0.0f );

	m_IntroFadeOutDuration			= 0.5f;
	const float FadeOutWaitDuration	= 1.0f;
	m_IntroFadeOutTime				= m_FadeInDuration + FadeOutWaitDuration;

	const float FinishedWaitDuration	= 0.25f;
	m_FinishedTime						= m_IntroFadeOutTime + m_IntroFadeOutDuration + FinishedWaitDuration;

	m_TitleFadeInDuration			= 0.5f;

	STATIC_HASHED_STRING( Context_Null );
	pInputSystem->PushContext( sContext_Null );

	StartPhaseRunning( InitialDelay );
}

void RosaIntro::ShutDown()
{
}

void RosaIntro::SetView()
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	pFramework->SetFOV( 90.0f );
	pFramework->SetVanishingPointY( 0.0f );
}

void RosaIntro::RestoreView()
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();

	STATICHASH( FOV );
	const float FOV = ConfigManager::GetFloat( sFOV );
	pFramework->SetFOV( FOV );

	STATICHASH( VanishingPointY );
	const float VanishingPointY = ConfigManager::GetFloat( sVanishingPointY );
	pFramework->SetVanishingPointY( VanishingPointY );
}
