#ifndef UISCREENROSAKEYPAD_H
#define UISCREENROSAKEYPAD_H

#include "uiscreen.h"
#include "simplestring.h"
#include "wbeventmanager.h"

class UIScreenRosaKeypad : public UIScreen, IWBEventObserver
{
public:
	UIScreenRosaKeypad();
	virtual ~UIScreenRosaKeypad();

	DEFINE_UISCREEN_FACTORY( RosaKeypad );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void		RegisterForEvents();

	virtual ETickReturn	TickInput();

	virtual void		Pushed();
	virtual void		Popped();

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

private:
	void			ResetCode();
	void			PublishCode();
	void			PushNumber( uint Number );
	void			PopNumber();

	void			HandleKeycode();
	void			SucceedKeycode();
	void			FailKeycode();

	void			Succeed();
	void			Fail();

	uint			m_TargetKeycode;
	uint			m_Keycode;
	SimpleString	m_KeycodeString;
	uint			m_NumbersPushed;

	Array<WBAction*>	m_SuccessActions;
	Array<WBAction*>	m_FailureActions;
	TEventUID			m_KeycodeEventUID;
	float				m_HandleKeycodeDelay;

	HashedString		m_ClosedLightImage;
	HashedString		m_OpenLightImage;
};

#endif // UISCREENROSAKEYPAD_H
