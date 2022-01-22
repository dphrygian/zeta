#include "core.h"
#include "rosasupertitles.h"
#include "configmanager.h"
#include "uiscreen.h"
#include "Widgets/uiwidget-text.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "Common/uimanagercommon.h"
#include "uistack.h"
#include "mesh.h"
#include "mathcore.h"
#include "clock.h"
#include "wbactionfactory.h"
#include "wbeventmanager.h"
#include "stringmanager.h"
#include "mathfunc.h"

RosaSupertitles::RosaSupertitles()
:	m_UIScreen( NULL )
,	m_UIText( NULL )
,	m_StartTime( 0.0f )
,	m_CharsPerSec( 0.0f )
,	m_TimeToAutoProgress( 0.0f )
,	m_AutoProgressTime( 0.0f )
,	m_CurrentString()
,	m_FinishedLine( false )
,	m_CurrentLine( 0 )
,	m_Lines()
,	m_Actions()
,	m_FinishedActions()
,	m_NextSupertitles()
,	m_NextSupertitlesPE()
{
	m_Lines.SetDeflate( false );
	m_Actions.SetDeflate( false );
	m_FinishedActions.SetDeflate( false );

	InitializeFromDefinition( "RosaSupertitles" );
}

RosaSupertitles::~RosaSupertitles()
{
	WBActionFactory::ClearActionArray( m_Actions );
	WBActionFactory::ClearActionArray( m_FinishedActions );
}

void RosaSupertitles::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( UIScreen );
	const HashedString	UIScreenName	= ConfigManager::GetHash( sUIScreen, HashedString::NullString, sDefinitionName );
	m_UIScreen							= RosaFramework::GetInstance()->GetUIManager()->GetScreen( UIScreenName );
	ASSERT( m_UIScreen );

	STATICHASH( UIText );
	const HashedString	UITextName		= ConfigManager::GetHash( sUIText, HashedString::NullString, sDefinitionName );
	m_UIText							= m_UIScreen->GetWidget<UIWidgetText>( UITextName );
	ASSERT( m_UIText );

	STATICHASH( CharsPerSec );
	m_CharsPerSec						= ConfigManager::GetFloat( sCharsPerSec, 0.0f, sDefinitionName );

	STATICHASH( TimeToAutoProgress );
	m_TimeToAutoProgress				= ConfigManager::GetFloat( sTimeToAutoProgress, 0.0f, sDefinitionName );
}

void RosaSupertitles::Tick()
{
	if( !IsSupertitlesActive() )
	{
		return;
	}

	const float CurrentTime			= RosaFramework::GetInstance()->GetClock()->GetGameCurrentTime();
	const float DeltaTime			= CurrentTime - m_StartTime;
	const uint	NumChars			= static_cast<uint>( DeltaTime * m_CharsPerSec );
	const uint	OriginalNumIndices	= m_UIText->GetOriginalNumIndices();
	const uint	NumIndices			= Min( NumChars * 6, OriginalNumIndices );

	SetUINumIndices( NumIndices );

	if( OriginalNumIndices > 0 && NumIndices == OriginalNumIndices && !m_FinishedLine )
	{
		m_FinishedLine = true;
		m_AutoProgressTime = CurrentTime + m_TimeToAutoProgress;
	}

	if( m_AutoProgressTime > 0.0f && CurrentTime > m_AutoProgressTime )
	{
		m_AutoProgressTime = 0.0f;
		ProgressSupertitles();
	}
}

void RosaSupertitles::ContinueSupertitles()
{
	ExecuteFinishedActions();

	// Check for following supertitles.
	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = RosaGame::GetPlayer();

	m_NextSupertitlesPE.Evaluate( PEContext );
	const HashedString Supertitles = ( m_NextSupertitlesPE.GetType() == WBParamEvaluator::EPT_String ) ? m_NextSupertitlesPE.GetString() : m_NextSupertitles;

	if( Supertitles == HashedString::NullString )
	{
		// No supertitles follow. We're all done!
		EndSupertitles();
	}
	else
	{
		StartSupertitles( Supertitles );
	}
}

void RosaSupertitles::ProgressSupertitles()
{
	m_CurrentLine++;

	if( m_CurrentLine < m_Lines.Size() )
	{
		// Move to next line
		StartLine( m_Lines[ m_CurrentLine ] );
	}
	else
	{
		ContinueSupertitles();
	}
}

void RosaSupertitles::InitializeSupertitlesFromDefinition( const HashedString& DefinitionName )
{
	// NOTE: This will fail to compile if I use PARANOID_HASH_CHECK. If I ever
	// need to test that, I can pass a string instead of a hash for the name.

	m_Lines.Clear();
	WBActionFactory::ClearActionArray( m_Actions );
	WBActionFactory::ClearActionArray( m_FinishedActions );

	// HACKHACK: If nothing is defined in the context, treat the name as a dynamic line.
	// This is a borrowed hack from convos, could still be useful I guess.
	if( !ConfigManager::ContextExists( DefinitionName ) )
	{
		SLine& Line			= m_Lines.PushBack();
		Line.m_IsDynamic	= true;
		Line.m_Line			= DefinitionName;
		return;
	}

	STATICHASH( NumRandomSupertitles );
	const uint NumRandomSupertitles = ConfigManager::GetInt( sNumRandomSupertitles, 0, DefinitionName );
	if( NumRandomSupertitles )
	{
		const uint			RandomSupertitlesIndex	= Math::Random( NumRandomSupertitles );
		const HashedString	RandomSupertitlesDef	= ConfigManager::GetSequenceHash( "RandomSupertitles%d", RandomSupertitlesIndex, HashedString::NullString, DefinitionName );
		InitializeSupertitlesFromDefinition( RandomSupertitlesDef );
		return;
	}

	STATICHASH( NumLines );
	const uint NumLines = ConfigManager::GetInt( sNumLines, 0, DefinitionName );
	m_Lines.Reserve( NumLines );
	for( uint LineIndex = 0; LineIndex < NumLines; ++LineIndex )
	{
		SLine& Line			= m_Lines.PushBack();
		Line.m_IsDynamic	= ConfigManager::GetSequenceBool( "Line%dIsDynamic", LineIndex, true, DefinitionName );	// DLP 31 Aug 2016: Defaults to true now because why not, and I can stop having to flag lines as such
		Line.m_Line			= ConfigManager::GetSequenceHash( "Line%d", LineIndex, HashedString::NullString, DefinitionName );
	}

	WBActionFactory::InitializeActionArray( DefinitionName, m_Actions );
	WBActionFactory::InitializeActionArray( DefinitionName, "Finished", m_FinishedActions );

	STATICHASH( NextSupertitles );
	m_NextSupertitles						= ConfigManager::GetHash( sNextSupertitles, HashedString::NullString, DefinitionName );

	STATICHASH( NextSupertitlesPE );
	const SimpleString	NextSupertitlesPE	= ConfigManager::GetString( sNextSupertitlesPE, "", DefinitionName );
	m_NextSupertitlesPE.InitializeFromDefinition( NextSupertitlesPE );
}

bool RosaSupertitles::IsSupertitlesActive() const
{
	UIStack* const pUIStack = RosaFramework::GetInstance()->GetUIManager()->GetUIStack();
	return pUIStack->IsOnStack( m_UIScreen );
}

void RosaSupertitles::StartSupertitles( const HashedString& Supertitles )
{
	InitializeSupertitlesFromDefinition( Supertitles );

	//WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	if( !IsSupertitlesActive() )
	{
		// Disable frobbing to hide the prompt, but only once per supertitles
		// Not for Zeta!
		//{
		//	WB_MAKE_EVENT( DisableFrob, NULL );
		//	WB_DISPATCH_EVENT( pEventManager, DisableFrob, RosaGame::GetPlayer() );
		//}
	}

	// Push the supertitles screen if it isn't already
	// (Don't Repush, it causes mouse seizing problems)
	UIStack* const pUIStack = RosaFramework::GetInstance()->GetUIManager()->GetUIStack();
	if( !pUIStack->IsOnStack( m_UIScreen ) )
	{
		pUIStack->Push( m_UIScreen );
	}

	m_CurrentLine		= 0;
	m_AutoProgressTime	= 0.0f;

	if( m_CurrentLine < m_Lines.Size() )
	{
		// Move to next line
		StartLine( m_Lines[ m_CurrentLine ] );
	}
	else
	{
		ContinueSupertitles();
	}

	ExecuteActions();
}

void RosaSupertitles::EndSupertitles()
{
	if( !IsSupertitlesActive() )
	{
		return;
	}

	// Unlike the convo screen, it's not safe to assume this screen is on top and pop.
	RosaFramework::GetInstance()->GetUIManager()->GetUIStack()->Remove( m_UIScreen );

	// Enable frobbing again
	//{
	//	WB_MAKE_EVENT( EnableFrob, NULL );
	//	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), EnableFrob, RosaGame::GetPlayer() );
	//}
}

void RosaSupertitles::ExecuteActions()
{
	WB_MAKE_EVENT( SupertitlesActionsEvent, NULL );
	WBActionFactory::ExecuteActionArray( m_Actions, WB_AUTO_EVENT( SupertitlesActionsEvent ), NULL );
}

void RosaSupertitles::ExecuteFinishedActions()
{
	WB_MAKE_EVENT( SupertitlesFinishedActionsEvent, NULL );
	WBActionFactory::ExecuteActionArray( m_FinishedActions, WB_AUTO_EVENT( SupertitlesFinishedActionsEvent ), NULL );
}

void RosaSupertitles::StartLine( const SLine& Line )
{
	// NOTE: This will fail to compile if I use PARANOID_HASH_CHECK. If I ever need
	// to test that, I can pass strings instead of hashes for the line and speaker.

	m_FinishedLine = false;

	m_CurrentString					= ConfigManager::GetLocalizedString( Line.m_Line, "" );
	if( Line.m_IsDynamic )
	{
		// Resolve the string here instead of in UI code, so we can use it for sound effects too
		m_CurrentString				= StringManager::ParseConfigString( m_CurrentString.CStr() );
	}

	m_UIText->m_String				= m_CurrentString;
	m_UIText->SetHidden( false );
	m_UIText->Reinitialize();

	m_StartTime		= RosaFramework::GetInstance()->GetClock()->GetGameCurrentTime();
	SetUINumIndices( 0 );
}

void RosaSupertitles::SetUINumIndices( const uint NumIndices )
{
	m_UIText->GetMesh()->SetNumIndices( NumIndices );
	if( m_UIText->GetDropShadowMesh() )
	{
		m_UIText->GetDropShadowMesh()->SetNumIndices( NumIndices );
	}
}
