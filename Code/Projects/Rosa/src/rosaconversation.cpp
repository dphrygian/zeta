#include "core.h"
#include "rosaconversation.h"
#include "configmanager.h"
#include "uiscreen.h"
#include "Widgets/uiwidget-text.h"
#include "Widgets/uiwidget-image.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "Common/uimanagercommon.h"
#include "uistack.h"
#include "mesh.h"
#include "mathcore.h"
#include "clock.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "irenderer.h"
#include "texturemanager.h"
#include "wbeventmanager.h"
#include "keyboard.h"
#include "rosacampaign.h"
#include "Components/wbcomprosacharacterconfig.h"
#include "stringmanager.h"
#include "mathfunc.h"

/*static*/ RosaConversation::EPortraitLocation RosaConversation::GetPortraitLocation( const HashedString& Location )
{
	STATIC_HASHED_STRING( Left );
	STATIC_HASHED_STRING( Right );

	if( Location == sLeft )
	{
		return EPL_Left;
	}
	else if( Location == sRight )
	{
		return EPL_Right;
	}
	else
	{
		WARN;
		return EPL_Left;
	}
}

RosaConversation::RosaConversation()
:	m_UIScreen( NULL )
,	m_UISpeaker( NULL )
,	m_UIText( NULL )
,	m_UIButton( NULL )
,	m_UIPortraitLeftBackdrop( NULL )
,	m_UIPortraitRightBackdrop( NULL )
,	m_UIPortraitLeft( NULL )
,	m_UIPortraitRight( NULL )
,	m_UIChoiceButtons()
,	m_Portraits()
,	m_StartTime( 0.0f )
,	m_CharsPerSec( 0.0f )
,	m_LastNumChars( 0 )
,	m_CurrentString()
,	m_SkippedLine( false )
,	m_FinishedLine( false )
,	m_ShowingChoices( false )
,	m_CurrentPortraitImage( NULL )
,	m_CurrentPortraitLocation( EPL_Left )
,	m_ChoiceSpeaker()
,	m_DefaultChoiceSpeaker()
,	m_CurrentLine( 0 )
,	m_Lines()
,	m_Actions()
,	m_Choices()
,	m_ChoiceMap()
,	m_NextConvo()
,	m_NextConvoPE()
,	m_ConvoTarget()
{
	m_Lines.SetDeflate( false );
	m_Actions.SetDeflate( false );
	m_Choices.SetDeflate( false );
	m_ChoiceMap.SetDeflate( false );

	InitializeFromDefinition( "RosaConversation" );
}

RosaConversation::~RosaConversation()
{
	WBActionFactory::ClearActionArray( m_Actions );
}

void RosaConversation::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( UIScreen );
	const HashedString	UIScreenName		= ConfigManager::GetHash( sUIScreen, HashedString::NullString, sDefinitionName );
	m_UIScreen								= RosaFramework::GetInstance()->GetUIManager()->GetScreen( UIScreenName );
	ASSERT( m_UIScreen );

	STATICHASH( UISpeaker );
	const HashedString	UISpeakerName		= ConfigManager::GetHash( sUISpeaker, HashedString::NullString, sDefinitionName );
	m_UISpeaker								= m_UIScreen->GetWidget<UIWidgetText>( UISpeakerName );
	ASSERT( m_UISpeaker );

	STATICHASH( UIText );
	const HashedString	UITextName			= ConfigManager::GetHash( sUIText, HashedString::NullString, sDefinitionName );
	m_UIText								= m_UIScreen->GetWidget<UIWidgetText>( UITextName );
	ASSERT( m_UIText );

	STATICHASH( UIButton );
	const HashedString	UIButtonName		= ConfigManager::GetHash( sUIButton, HashedString::NullString, sDefinitionName );
	m_UIButton								= m_UIScreen->GetWidget( UIButtonName );
	ASSERT( m_UIButton );

	STATICHASH( UIPortraitLeftBackdrop );
	const HashedString	UIPortraitLeftBackdrop	= ConfigManager::GetHash( sUIPortraitLeftBackdrop, HashedString::NullString, sDefinitionName );
	m_UIPortraitLeftBackdrop					= m_UIScreen->GetWidget<UIWidgetImage>( UIPortraitLeftBackdrop );

	STATICHASH( UIPortraitRightBackdrop );
	const HashedString	UIPortraitRightBackdrop	= ConfigManager::GetHash( sUIPortraitRightBackdrop, HashedString::NullString, sDefinitionName );
	m_UIPortraitRightBackdrop					= m_UIScreen->GetWidget<UIWidgetImage>( UIPortraitRightBackdrop );

	STATICHASH( UIPortraitLeft );
	const HashedString	UIPortraitLeft		= ConfigManager::GetHash( sUIPortraitLeft, HashedString::NullString, sDefinitionName );
	m_UIPortraitLeft						= m_UIScreen->GetWidget<UIWidgetImage>( UIPortraitLeft );

	STATICHASH( UIPortraitRight );
	const HashedString	UIPortraitRight		= ConfigManager::GetHash( sUIPortraitRight, HashedString::NullString, sDefinitionName );
	m_UIPortraitRight						= m_UIScreen->GetWidget<UIWidgetImage>( UIPortraitRight );

	STATICHASH( NumUIChoiceButtons );
	const uint			NumUIChoiceButtons	= ConfigManager::GetInt( sNumUIChoiceButtons, 0, sDefinitionName );
	for( uint UIChoiceButtonIndex = 0; UIChoiceButtonIndex < NumUIChoiceButtons; ++UIChoiceButtonIndex )
	{
		const HashedString	UIChoiceButtonName	= ConfigManager::GetSequenceHash( "UIChoiceButton%d", UIChoiceButtonIndex, HashedString::NullString, sDefinitionName );
		UIWidgetText* const	pUIChoiceButton		= m_UIScreen->GetWidget<UIWidgetText>( UIChoiceButtonName );
		ASSERT( pUIChoiceButton );
		m_UIChoiceButtons.PushBack( pUIChoiceButton );
	}

	m_ChoiceMap.Resize( NumUIChoiceButtons );

	STATICHASH( CharsPerSec );
	m_CharsPerSec							= ConfigManager::GetFloat( sCharsPerSec, 0.0f, sDefinitionName );

	TextureManager* const pTextureManager	= RosaFramework::GetInstance()->GetRenderer()->GetTextureManager();
	STATICHASH( NumPortraits );
	const uint NumPortraits					= ConfigManager::GetInt( sNumPortraits, 0, sDefinitionName );
	for( uint PortraitIndex = 0; PortraitIndex < NumPortraits; ++PortraitIndex )
	{
		const HashedString	Speaker		= ConfigManager::GetSequenceHash( "Portrait%dSpeaker", PortraitIndex, HashedString::NullString, sDefinitionName );
		SPortrait& Portrait				= m_Portraits.Insert( Speaker );

		const SimpleString	Image		= ConfigManager::GetSequenceString( "Portrait%dImage", PortraitIndex, "", sDefinitionName );
		Portrait.m_Image				= pTextureManager->GetTexture( Image.CStr() );

		const HashedString	Location	= ConfigManager::GetSequenceHash( "Portrait%dLocation", PortraitIndex, HashedString::NullString, sDefinitionName );
		Portrait.m_Location				= GetPortraitLocation( Location );
	}

	STATICHASH( ChoiceSpeaker );
	m_DefaultChoiceSpeaker				= ConfigManager::GetHash( sChoiceSpeaker, HashedString::NullString, sDefinitionName );
}

void RosaConversation::Tick()
{
	const float CurrentTime			= RosaFramework::GetInstance()->GetClock()->GetMachineCurrentTime();
	const float DeltaTime			= CurrentTime - m_StartTime;
	const uint	NumChars			= static_cast<uint>( DeltaTime * m_CharsPerSec );
	const uint	OriginalNumIndices	= m_UIText->GetOriginalNumIndices();
	const uint	NumIndices			= Min( ( m_SkippedLine ? OriginalNumIndices : ( NumChars * 6 ) ), OriginalNumIndices );

	SetUINumIndices( NumIndices );

	if( NumIndices == OriginalNumIndices && !m_FinishedLine )
	{
		m_FinishedLine = true;
		OnLineFinished();
	}

	// If we've added any chars since the last tick,
	// and any of those chars are spaces, play a sound.
	const uint StringSize		= m_CurrentString.Length();
	const uint ActualNumChars	= Min( ( m_SkippedLine ? StringSize : NumChars ), StringSize );
	for( uint CharIndex = m_LastNumChars; CharIndex < ActualNumChars; ++CharIndex )
	{
		if( m_CurrentString.GetChar( CharIndex ) == ' '	||	// Play sound on spaces
			CharIndex == 0								||	// Play sound on first character
			CharIndex == ( StringSize - 1 )				)	// Play sound on last character
		{
			// HACKHACK: Hard-coded sound
			STATIC_HASHED_STRING( Sound_Convo );
			WB_MAKE_EVENT( PlaySoundDef, NULL );
			WB_SET_AUTO( PlaySoundDef, Hash, Sound, sSound_Convo );
			WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), PlaySoundDef, RosaGame::GetPlayer() );

			// Only play one sound even if there's multiple spaces in this tick
			break;
		}
	}
	m_LastNumChars = ActualNumChars;

#if BUILD_DEV
	// Alt + H ends conversation instantly
	Keyboard* const pKeyboard = RosaFramework::GetInstance()->GetKeyboard();
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_H ) )
	{
		EndConversation();
	}
#endif // BUILD_DEV
}

void RosaConversation::SkipLine()
{
	if( m_ShowingChoices )
	{
		// Ignore this input
	}
	else if( m_FinishedLine )
	{
		ProgressConversation();
	}
	else
	{
		m_SkippedLine = true;
	}
}

void RosaConversation::OnLineFinished()
{
	m_UIButton->SetHidden( false );
	m_UIButton->SetDisabled( false );
}

void RosaConversation::ContinueConversation()
{
	// Check for a following conversation.
	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = RosaGame::GetPlayer();

	m_NextConvoPE.Evaluate( PEContext );
	const HashedString Conversation = ( m_NextConvoPE.GetType() == WBParamEvaluator::EPT_String ) ? m_NextConvoPE.GetString() : m_NextConvo;

	if( Conversation == HashedString::NullString )
	{
		// No conversation follows. We're all done!
		EndConversation();
	}
	else
	{
		StartConversation( Conversation, m_ConvoTarget.Get() );
	}
}

void RosaConversation::ProgressConversation()
{
	ASSERT( !m_ShowingChoices );

	m_CurrentLine++;

	if( m_CurrentLine < m_Lines.Size() )
	{
		// Move to next line
		StartLine( m_Lines[ m_CurrentLine ] );
	}
	else if( m_Choices.Size() > 0 )
	{
		// Move to choices
		StartChoices();
	}
	else
	{
		ContinueConversation();
	}
}

void RosaConversation::SelectChoice( const uint ButtonIndex )
{
	if( !m_ShowingChoices )
	{
		// Received input while not showing choices, ignore and skip line if possible
		SkipLine();
		return;
	}

	if( ButtonIndex >= m_ChoiceMap.Size() )
	{
		// Received input outside valid choice range, ignore
		return;
	}

	const uint ChoiceIndex = m_ChoiceMap[ ButtonIndex ];
	if( ChoiceIndex >= m_Choices.Size() )
	{
		// Received input outside current choice range, ignore
		return;
	}

	SChoice& Choice = m_Choices[ ChoiceIndex ];
	if( Choice.m_Hidden || Choice.m_Disabled )
	{
		// Received for a hidden or disabled choice, ignore
		return;
	}

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = RosaGame::GetPlayer();

	Choice.m_ConvoPE.Evaluate( PEContext );
	const HashedString Conversation = ( Choice.m_ConvoPE.GetType() == WBParamEvaluator::EPT_String ) ? Choice.m_ConvoPE.GetString() : Choice.m_Convo;

	if( Conversation == HashedString::NullString )
	{
		// No conversation follows. We're all done!
		EndConversation();
	}
	else
	{
		StartConversation( Conversation, m_ConvoTarget.Get() );
	}
}

void RosaConversation::InitializeConversationFromDefinition( const HashedString& DefinitionName )
{
	// NOTE: This will fail to compile if I use PARANOID_HASH_CHECK. If I ever
	// need to test that, I can pass a string instead of a hash for the name.

	m_Lines.Clear();
	WBActionFactory::ClearActionArray( m_Actions );
	m_Choices.Clear();

	// HACKHACK: For Rosa, I want to implement a lot of one-line responses without all the actual "conversation" stuff.
	// For this purpose, if nothing is defined in the convo context, treat the convo name as a dynamic line.
	if( !ConfigManager::ContextExists( DefinitionName ) )
	{
		SLine& Line			= m_Lines.PushBack();
		Line.m_IsDynamic	= true;
		Line.m_Speaker		= HashedString::NullString;
		Line.m_Line			= DefinitionName;
		return;
	}

	STATICHASH( NumRandomConvos );
	const uint NumRandomConvos = ConfigManager::GetInt( sNumRandomConvos, 0, DefinitionName );
	if( NumRandomConvos )
	{
		const uint			RandomConvoIndex	= Math::Random( NumRandomConvos );
		const HashedString	RandomConvoDef		= ConfigManager::GetSequenceHash( "RandomConvo%d", RandomConvoIndex, HashedString::NullString, DefinitionName );
		InitializeConversationFromDefinition( RandomConvoDef );
		return;
	}

	STATICHASH( NumLines );
	const uint NumLines = ConfigManager::GetInt( sNumLines, 0, DefinitionName );
	m_Lines.Reserve( NumLines );
	for( uint LineIndex = 0; LineIndex < NumLines; ++LineIndex )
	{
		SLine& Line			= m_Lines.PushBack();
		Line.m_IsDynamic	= ConfigManager::GetSequenceBool( "Line%dIsDynamic", LineIndex, true, DefinitionName );	// DLP 31 Aug 2016: Defaults to true now because why not, and I can stop having to flag lines as such
		Line.m_Speaker		= ConfigManager::GetSequenceHash( "Line%dSpeaker", LineIndex, HashedString::NullString, DefinitionName );
		Line.m_Line			= ConfigManager::GetSequenceHash( "Line%d", LineIndex, HashedString::NullString, DefinitionName );
	}

	WBActionFactory::InitializeActionArray( DefinitionName, m_Actions );

	STATICHASH( ChoiceSpeaker );
	m_ChoiceSpeaker			= ConfigManager::GetHash( sChoiceSpeaker, m_DefaultChoiceSpeaker, DefinitionName );

	STATICHASH( NumChoices );
	const uint NumChoices = ConfigManager::GetInt( sNumChoices, 0, DefinitionName );
	m_Choices.Reserve( NumChoices );
	for( uint ChoiceIndex = 0; ChoiceIndex < NumChoices; ++ChoiceIndex )
	{
		SChoice&			Choice		= m_Choices.PushBack();
		Choice.m_IsDynamic				= ConfigManager::GetSequenceBool( "Choice%dIsDynamic", ChoiceIndex, false, DefinitionName );
		Choice.m_Line					= ConfigManager::GetSequenceHash( "Choice%d", ChoiceIndex, HashedString::NullString, DefinitionName );
		Choice.m_Convo					= ConfigManager::GetSequenceHash( "Choice%dConvo", ChoiceIndex, HashedString::NullString, DefinitionName );

		const SimpleString	ConvoPE		= ConfigManager::GetSequenceString( "Choice%dConvoPE", ChoiceIndex, "", DefinitionName );
		Choice.m_ConvoPE.InitializeFromDefinition( ConvoPE );

		const SimpleString	HiddenPE	= ConfigManager::GetSequenceString( "Choice%dHiddenPE", ChoiceIndex, "", DefinitionName );
		Choice.m_HiddenPE.InitializeFromDefinition( HiddenPE );

		const SimpleString	ShownPE		= ConfigManager::GetSequenceString( "Choice%dShownPE", ChoiceIndex, "", DefinitionName );
		Choice.m_ShownPE.InitializeFromDefinition( ShownPE );

		const SimpleString	DisabledPE	= ConfigManager::GetSequenceString( "Choice%dDisabledPE", ChoiceIndex, "", DefinitionName );
		Choice.m_DisabledPE.InitializeFromDefinition( DisabledPE );

		const SimpleString	EnabledPE	= ConfigManager::GetSequenceString( "Choice%dEnabledPE", ChoiceIndex, "", DefinitionName );
		Choice.m_EnabledPE.InitializeFromDefinition( EnabledPE );
	}

	STATICHASH( NextConvo );
	m_NextConvo						= ConfigManager::GetHash( sNextConvo, HashedString::NullString, DefinitionName );

	STATICHASH( NextConvoPE );
	const SimpleString	NextConvoPE	= ConfigManager::GetString( sNextConvoPE, "", DefinitionName );
	m_NextConvoPE.InitializeFromDefinition( NextConvoPE );
}

// HACKHACK: Define active conversation by the UI.
bool RosaConversation::IsConversationActive() const
{
	UIStack* const pUIStack = RosaFramework::GetInstance()->GetUIManager()->GetUIStack();
	return pUIStack->IsOnStack( m_UIScreen );
}

void RosaConversation::StartConversation( const HashedString& Conversation, WBEntity* const pConvoTarget )
{
	InitializeConversationFromDefinition( Conversation );

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	m_ConvoTarget = pConvoTarget;

	// Tell the player that they are speaking with pConvoTarget.
	STATIC_HASHED_STRING( ConvoTarget );
	WB_MAKE_EVENT( SetVariable, NULL );
	WB_SET_AUTO( SetVariable, Hash, Name, sConvoTarget );
	WB_SET_AUTO( SetVariable, Entity, Value, m_ConvoTarget );
	WB_DISPATCH_EVENT( pEventManager, SetVariable, RosaGame::GetPlayer() );

	if( !IsConversationActive() )
	{
		// Disable frobbing to hide the prompt, but only once per conversation
		{
			WB_MAKE_EVENT( DisableFrob, NULL );
			WB_DISPATCH_EVENT( pEventManager, DisableFrob, RosaGame::GetPlayer() );
		}

		// Hide the reticle
		{
			STATIC_HASHED_STRING( HUD );
			STATIC_HASHED_STRING( Crosshair );
			WB_MAKE_EVENT( SetWidgetHidden, NULL );
			WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
			WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
			WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
			WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
		}

		// HACKHACK: Hide the title card
		{
			STATIC_HASHED_STRING( HUD );
			STATIC_HASHED_STRING( TitleCard );
			WB_MAKE_EVENT( SetWidgetHidden, NULL );
			WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
			WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sTitleCard );
			WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
			WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
		}
		{
			STATIC_HASHED_STRING( HUD );
			STATIC_HASHED_STRING( TitleCardSub );
			WB_MAKE_EVENT( SetWidgetHidden, NULL );
			WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
			WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sTitleCardSub );
			WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
			WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
		}
		{
			STATIC_HASHED_STRING( HUD );
			STATIC_HASHED_STRING( TitleCardSub_ShadowBox );
			WB_MAKE_EVENT( SetWidgetHidden, NULL );
			WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
			WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sTitleCardSub_ShadowBox );
			WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
			WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
		}
	}

	// Push the conversation screen if it isn't already
	// (Don't Repush, it causes mouse seizing problems)
	UIStack* const pUIStack = RosaFramework::GetInstance()->GetUIManager()->GetUIStack();
	if( !pUIStack->IsOnStack( m_UIScreen ) )
	{
		pUIStack->Push( m_UIScreen );
	}

	m_CurrentLine		= 0;
	m_ShowingChoices	= false;

	if( m_CurrentLine < m_Lines.Size() )
	{
		// Move to next line
		StartLine( m_Lines[ m_CurrentLine ] );
	}
	else if( m_Choices.Size() > 0 )
	{
		// Move to choices
		StartChoices();
	}
	else
	{
		ContinueConversation();
	}

	ExecuteActions();
}

void RosaConversation::EndConversation()
{
	if( !IsConversationActive() )
	{
		return;
	}

	// Pop the screen. (This assumes conversation is always on top, which should be safe
	// because the game is paused and the player can't bring up the pause screen.)
	RosaFramework::GetInstance()->GetUIManager()->GetUIStack()->Pop();

	// Enable frobbing again
	{
		WB_MAKE_EVENT( EnableFrob, NULL );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), EnableFrob, RosaGame::GetPlayer() );
	}

	// Show the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, false );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetWidgetHidden, NULL );
	}
}

void RosaConversation::ExecuteActions()
{
	// ROSANOTE: Changed to execute actions with the convo target's context instead of the player.
	WBEntity* const pConvoTarget = m_ConvoTarget.Get();
	WB_MAKE_EVENT( ConversationActionsEvent, pConvoTarget );
	WBActionFactory::ExecuteActionArray( m_Actions, WB_AUTO_EVENT( ConversationActionsEvent ), pConvoTarget );
}

void RosaConversation::StartLine( const SLine& Line )
{
	// NOTE: This will fail to compile if I use PARANOID_HASH_CHECK. If I ever need
	// to test that, I can pass strings instead of hashes for the line and speaker.

	m_FinishedLine = false;

	// If we don't have a speaker defined, try using the convo target's full name (if any)
	// OLDVAMP
	//RosaCampaign* const					pCampaign			= RosaCampaign::GetCampaign();
	const bool							UseActorFullName	= ( Line.m_Speaker == HashedString::NullString );
	//WBCompRosaCharacterConfig* const	pCharacterConfig	= WB_GETCOMP_SAFE( m_ConvoTarget.Get(), RosaCharacterConfig );
	//const bool							IsValidActor		= false; //pCharacterConfig && INVALID_ACTOR != pCharacterConfig->IsActor();
	const SimpleString					ActorFullName		= ""; //IsValidActor ? pCampaign->GetFullName( pCharacterConfig->GetActorID() ) : "";
	const SimpleString					SpeakerName			= ConfigManager::GetLocalizedString( Line.m_Speaker, "" );

	m_UISpeaker->m_String			= UseActorFullName ? ActorFullName : SpeakerName;
	m_UISpeaker->SetHidden( false );
	m_UISpeaker->Reinitialize();

	m_CurrentString					= ConfigManager::GetLocalizedString( Line.m_Line, "" );
	if( Line.m_IsDynamic )
	{
		// Resolve the string here instead of in UI code, so we can use it for sound effects too
		m_CurrentString				= StringManager::ParseConfigString( m_CurrentString.CStr() );
	}

	m_UIText->m_String				= m_CurrentString;
	m_UIText->SetHidden( false );
	m_UIText->Reinitialize();

	// Hide the choices
	FOR_EACH_ARRAY( UIChoiceButtonIter, m_UIChoiceButtons, UIWidgetText* )
	{
		UIWidgetText* const pUIChoiceButton = UIChoiceButtonIter.GetValue();
		pUIChoiceButton->SetHidden( true );
		pUIChoiceButton->SetDisabled( true );
	}

	ShowPortrait( Line.m_Speaker );

	m_LastNumChars	= 0;
	m_SkippedLine	= false;
	m_StartTime		= RosaFramework::GetInstance()->GetClock()->GetMachineCurrentTime();
	SetUINumIndices( 0 );

	// Hide the close/OK button
	m_UIButton->SetHidden( true );
	m_UIButton->SetDisabled( true );
}

void RosaConversation::StartChoices()
{
	m_ShowingChoices = true;

	// Hide the speaker and text
	m_UISpeaker->SetHidden( true );
	m_UIText->SetHidden( true );

	ShowPortrait( m_ChoiceSpeaker );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = RosaGame::GetPlayer();

	// Show the valid choices
	uint ButtonIndex = 0;
	for( uint ChoiceIndex = 0; ChoiceIndex < m_Choices.Size(); ++ChoiceIndex )
	{
		SChoice&			Choice			= m_Choices[ ChoiceIndex ];
		UIWidgetText* const	pUIChoiceButton	= m_UIChoiceButtons[ ButtonIndex ];

		Choice.m_HiddenPE.Evaluate( PEContext );
		Choice.m_ShownPE.Evaluate( PEContext );
		Choice.m_Hidden = Choice.m_HiddenPE.GetBool() || ( Choice.m_ShownPE.IsInitialized() && !Choice.m_ShownPE.GetBool() );

		Choice.m_DisabledPE.Evaluate( PEContext );
		Choice.m_EnabledPE.Evaluate( PEContext );
		Choice.m_Disabled = Choice.m_DisabledPE.GetBool() || ( Choice.m_EnabledPE.IsInitialized() && !Choice.m_EnabledPE.GetBool() );

		if( Choice.m_Hidden )
		{
			// Don't do anything with this button, we'll move the next element up into this slot
			continue;
		}

		if( Choice.m_IsDynamic )
		{
			pUIChoiceButton->m_String			= "";
			pUIChoiceButton->m_DynamicString	= SimpleString::PrintF( "%d. %s", ButtonIndex + 1, ConfigManager::GetLocalizedString( Choice.m_Line, "" ) );
		}
		else
		{
			pUIChoiceButton->m_String			= SimpleString::PrintF( "%d. %s", ButtonIndex + 1, ConfigManager::GetLocalizedString( Choice.m_Line, "" ) );
			pUIChoiceButton->m_DynamicString	= "";
		}

		pUIChoiceButton->SetHidden( false );
		pUIChoiceButton->SetDisabled( Choice.m_Disabled );
		pUIChoiceButton->Reinitialize();

		// Map the button to this choice.
		m_ChoiceMap[ ButtonIndex ] = ChoiceIndex;

		++ButtonIndex;
	}

	// Hide the remaining buttons, if any
	for( ; ButtonIndex < m_UIChoiceButtons.Size(); ++ButtonIndex )
	{
		UIWidgetText* const	pUIChoiceButton	= m_UIChoiceButtons[ ButtonIndex ];

		pUIChoiceButton->SetHidden( true );
		pUIChoiceButton->SetDisabled( true );

		// Map the button to an invalid choice.
		m_ChoiceMap[ ButtonIndex ] = INVALID_ARRAY_INDEX;
	}

	// Hide the close/OK button
	m_UIButton->SetHidden( true );
	m_UIButton->SetDisabled( true );
}

void RosaConversation::ShowPortrait( const HashedString& Speaker )
{
	// Hide all portraits, then show the appropriate one
	if( m_UIPortraitLeftBackdrop )	{ m_UIPortraitLeftBackdrop->SetHidden( true ); }
	if( m_UIPortraitRightBackdrop )	{ m_UIPortraitRightBackdrop->SetHidden( true ); }
	if( m_UIPortraitLeft )			{ m_UIPortraitLeft->SetHidden( true ); }
	if( m_UIPortraitRight )			{ m_UIPortraitRight->SetHidden( true ); }

	Map<HashedString, SPortrait>::Iterator PortraitIterator = m_Portraits.Search( Speaker );

	// HACKHACK for Neon DLC: if the requested speaker doesn't exist, try latently loading it
	if( PortraitIterator.IsNull() )
	{
		STATICHASH( Image );
		const SimpleString	Image		= ConfigManager::GetString( sImage, "", Speaker );

		STATICHASH( Location );
		const HashedString	Location	= ConfigManager::GetHash( sLocation, HashedString::NullString, Speaker );

		if( Image != "" )
		{
			TextureManager* const pTextureManager = RosaFramework::GetInstance()->GetRenderer()->GetTextureManager();
			SPortrait& Portrait	= m_Portraits.Insert( Speaker );
			Portrait.m_Image	= pTextureManager->GetTexture( Image.CStr() );
			Portrait.m_Location	= GetPortraitLocation( Location );

			PortraitIterator	= m_Portraits.Search( Speaker );
			ASSERT( PortraitIterator.IsValid() );
		}
	}

	if( PortraitIterator.IsValid() )
	{
		const SPortrait&		Portrait	= PortraitIterator.GetValue();

		m_CurrentPortraitImage		= Portrait.m_Image;
		m_CurrentPortraitLocation	= Portrait.m_Location;

		if( Portrait.m_Location == EPL_Left )
		{
			if( m_UIPortraitLeft )
			{
				m_UIPortraitLeft->SetTexture( Portrait.m_Image, 0 );
				m_UIPortraitLeft->SetHidden( false );
			}

			if( m_UIPortraitLeftBackdrop )
			{
				m_UIPortraitLeftBackdrop->SetHidden( false );
			}
		}
		else if( Portrait.m_Location == EPL_Right )
		{
			if( m_UIPortraitRight )
			{
				m_UIPortraitRight->SetTexture( Portrait.m_Image, 0 );
				m_UIPortraitRight->SetHidden( false );
			}

			if( m_UIPortraitRightBackdrop )
			{
				m_UIPortraitRightBackdrop->SetHidden( false );
			}
		}
	}
}

void RosaConversation::SetUINumIndices( const uint NumIndices )
{
	m_UIText->GetMesh()->SetNumIndices( NumIndices );
	if( m_UIText->GetDropShadowMesh() )
	{
		m_UIText->GetDropShadowMesh()->SetNumIndices( NumIndices );
	}
}
