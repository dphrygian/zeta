#ifndef UISCREENROSACREDITS_H
#define UISCREENROSACREDITS_H

#include "uiscreen.h"

class UIScreenRosaCredits : public UIScreen
{
public:
	UIScreenRosaCredits();
	virtual ~UIScreenRosaCredits();

	DEFINE_UISCREEN_FACTORY( RosaCredits );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickReturn	Tick( const float DeltaTime, bool HasFocus );
	virtual void		Pushed();

private:
	bool			m_Repeat;
	HashedString	m_TextWidget;
};

#endif // UISCREENROSACREDITS_H
