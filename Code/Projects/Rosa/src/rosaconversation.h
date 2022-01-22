#ifndef ROSACONVERSATION_H
#define ROSACONVERSATION_H

#include "array.h"
#include "wbparamevaluator.h"
#include "wbentityref.h"

class UIScreen;
class UIWidget;
class UIWidgetText;
class UIWidgetImage;
class WBAction;
class ITexture;

class RosaConversation
{
public:
	RosaConversation();
	~RosaConversation();

	void	InitializeFromDefinition( const SimpleString& DefinitionName );
	void	Tick();

	void	StartConversation( const HashedString& Conversation, WBEntity* const pConvoTarget );
	void	EndConversation();

	void	SkipLine();
	void	ProgressConversation();
	void	ContinueConversation();
	void	SelectChoice( const uint ButtonIndex );

	bool	IsConversationActive() const;

private:
	struct SLine
	{
		SLine()
		:	m_IsDynamic( false )
		,	m_Line()
		,	m_Speaker()
		{
		}

		bool			m_IsDynamic;
		HashedString	m_Line;
		HashedString	m_Speaker;
	};

	struct SChoice
	{
		SChoice()
		:	m_IsDynamic( false )
		,	m_Hidden( false )
		,	m_Disabled( false )
		,	m_Line()
		,	m_Convo()
		,	m_ConvoPE()
		,	m_HiddenPE()
		,	m_ShownPE()
		,	m_DisabledPE()
		,	m_EnabledPE()
		{
		}

		bool				m_IsDynamic;
		bool				m_Hidden;	// Cached for validating choice input later
		bool				m_Disabled;	// Cached for validating choice input later
		HashedString		m_Line;
		HashedString		m_Convo;
		WBParamEvaluator	m_ConvoPE;
		WBParamEvaluator	m_HiddenPE;
		WBParamEvaluator	m_ShownPE;
		WBParamEvaluator	m_DisabledPE;
		WBParamEvaluator	m_EnabledPE;
	};

	enum EPortraitLocation
	{
		EPL_Left,
		EPL_Right,
	};

	struct SPortrait
	{
		SPortrait()
		:	m_Image( NULL )
		,	m_Location( EPL_Left )
		{
		}

		ITexture*			m_Image;
		EPortraitLocation	m_Location;
	};

	void	InitializeConversationFromDefinition( const HashedString& DefinitionName );
	void	StartLine( const SLine& Line );
	void	ExecuteActions();
	void	StartChoices();
	void	SetUINumIndices( const uint NumIndices );
	void	OnLineFinished();
	void	ShowPortrait( const HashedString& Speaker );

	static EPortraitLocation	GetPortraitLocation( const HashedString& Location );

	UIScreen*				m_UIScreen;
	UIWidgetText*			m_UISpeaker;
	UIWidgetText*			m_UIText;
	UIWidget*				m_UIButton;
	UIWidgetImage*			m_UIPortraitLeftBackdrop;
	UIWidgetImage*			m_UIPortraitRightBackdrop;
	UIWidgetImage*			m_UIPortraitLeft;
	UIWidgetImage*			m_UIPortraitRight;
	Array<UIWidgetText*>	m_UIChoiceButtons;

	Map<HashedString, SPortrait>	m_Portraits;	// Map from speaker tag to portrait

	// Text revealing
	float			m_StartTime;
	float			m_CharsPerSec;			// Config
	uint			m_LastNumChars;
	SimpleString	m_CurrentString;
	bool			m_SkippedLine;
	bool			m_FinishedLine;
	bool			m_ShowingChoices;

	ITexture*			m_CurrentPortraitImage;
	EPortraitLocation	m_CurrentPortraitLocation;
	HashedString		m_ChoiceSpeaker;
	HashedString		m_DefaultChoiceSpeaker;

	// Current conversation properties
	uint				m_CurrentLine;	// Iterating index into m_Lines
	Array<SLine>		m_Lines;		// Tags for each line and speaker in the conversation
	Array<WBAction*>	m_Actions;		// Actions executed at the start of this conversation (optional)
	Array<SChoice>		m_Choices;		// Tags for each choice at the end of this conversation (optional)
	Array<uint>			m_ChoiceMap;	// HACKHACK for hidden choices, map button index to choice index
	HashedString		m_NextConvo;	// Tag for the conversation which immediately follows this one (optional, mutually exclusive with choices)
	WBParamEvaluator	m_NextConvoPE;	// Evaluator for the next conversation, where logic should be executed

	WBEntityRef			m_ConvoTarget;
};

#endif // ROSACONVERSATION_H
