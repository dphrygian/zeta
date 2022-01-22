#ifndef ROSASUPERTITLES_H
#define ROSASUPERTITLES_H

#include "array.h"
#include "wbparamevaluator.h"
#include "wbentityref.h"

class UIScreen;
class UIWidget;
class UIWidgetText;
class UIWidgetImage;
class WBAction;
class ITexture;

class RosaSupertitles
{
public:
	RosaSupertitles();
	~RosaSupertitles();

	void	InitializeFromDefinition( const SimpleString& DefinitionName );
	void	Tick();

	void	StartSupertitles( const HashedString& Supertitles );
	void	EndSupertitles();

	void	ProgressSupertitles();
	void	ContinueSupertitles();

	bool	IsSupertitlesActive() const;

private:
	struct SLine
	{
		SLine()
		:	m_IsDynamic( false )
		,	m_Line()
		{
		}

		bool			m_IsDynamic;
		HashedString	m_Line;
	};

	void	InitializeSupertitlesFromDefinition( const HashedString& DefinitionName );
	void	StartLine( const SLine& Line );

	void	ExecuteActions();
	void	ExecuteFinishedActions();

	void	SetUINumIndices( const uint NumIndices );

	UIScreen*				m_UIScreen;
	UIWidgetText*			m_UIText;

	// Text revealing
	float			m_StartTime;
	float			m_CharsPerSec;			// Config
	float			m_TimeToAutoProgress;	// Config
	float			m_AutoProgressTime;
	SimpleString	m_CurrentString;
	bool			m_FinishedLine;

	// Current supertitles properties
	uint				m_CurrentLine;			// Iterating index into m_Lines
	Array<SLine>		m_Lines;				// Tags for each line and speaker in the supertitles
	Array<WBAction*>	m_Actions;				// Actions executed at the start of these supertitles (optional)
	Array<WBAction*>	m_FinishedActions;		// Actions executed at the end of these supertitles (optional)
	HashedString		m_NextSupertitles;		// Tag for the supertitles which immediately follows this one (optional, mutually exclusive with choices)
	WBParamEvaluator	m_NextSupertitlesPE;	// Evaluator for the next supertitles, where logic should be executed
};

#endif // ROSASUPERTITLES_H
