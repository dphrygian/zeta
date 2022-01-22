#ifndef UIMANAGERCOMMON_H
#define UIMANAGERCOMMON_H

#include "../uimanager.h"
#include "array.h"

class WBAction;

class UIManagerCommon : public UIManager
{
public:
	UIManagerCommon();
	virtual ~UIManagerCommon();

	virtual void	Initialize();
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

	virtual void	RegisterForEvents();

	void			ResetToInitialScreens();
	void			ResetToGameScreens();

	// "GameFade" is an optional screen at an arbitrary place on the UI stack.
	// A normal (non-game) fade is always rendered above the entire stack.
	void	Fade( const Vector4& StartColor, const Vector4& EndColor, const float Duration, const bool GameFade );

	class UIScreenOKDialog*	GetOKDialog() const;
	void	ShowOKDialog(
		bool				PauseGame,
		const SimpleString&	OKString,
		const SimpleString&	OKDynamicString,
		const SimpleString&	OKEvent,
		const SUICallback&	OKCallback = SUICallback() );

	class UIScreenWaitDialog*	GetWaitDialog() const;
	void	ShowWaitDialog(
		bool				PauseGame,
		const SimpleString&	WaitString,
		const SimpleString&	WaitDynamicString );

	void	HideWaitDialog();

	class UIScreenYesNoDialog*	GetYesNoDialog() const;
	void	ShowYesNoDialog(
		bool				PauseGame,
		const SimpleString&	YesNoString,
		const SimpleString&	YesNoDynamicString,
		const SimpleString&	YesEvent,
		const SimpleString&	NoEvent,
		const SUICallback&	YesCallback = SUICallback(),
		const SUICallback&	NoCallback = SUICallback() );

	// Customized versions for integration with event system.
	void	ShowOKDialog(
		bool							PauseGame,
		const SimpleString&				OKString,
		const SimpleString&				OKDynamicString,
		const Array<WBAction*>* const	pOKActions );
	void	ShowYesNoDialog(
		bool							PauseGame,
		const SimpleString&				YesNoString,
		const SimpleString&				YesNoDynamicString,
		const Array<WBAction*>* const	pYesActions,
		const Array<WBAction*>* const	pNoActions );

protected:
	Array<UIScreen*>	m_InitialScreens;
	Array<UIScreen*>	m_GameScreens;
};

#endif // UIMANAGERCOMMON_H
