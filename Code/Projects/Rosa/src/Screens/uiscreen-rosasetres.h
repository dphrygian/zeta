#ifndef UISCREENROSASETRES_H
#define UISCREENROSASETRES_H

#include "uiscreen.h"
#include "map.h"
#include "hashedstring.h"
#include "display.h"

class UIScreenRosaSetRes : public UIScreen
{
public:
	UIScreenRosaSetRes();
	virtual ~UIScreenRosaSetRes();

	DEFINE_UISCREEN_FACTORY( RosaSetRes );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Pushed();

	void			SetUICallback( const SUICallback& Callback );

	SDisplayMode	GetRes( const HashedString& Name );

protected:
	Map<HashedString, SDisplayMode>	m_ResMap;
	SUICallback						m_Callback;
};

#endif // UISCREENROSASETRES_H
