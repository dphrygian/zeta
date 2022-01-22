#ifndef UIWIDGETSPACER_H
#define UIWIDGETSPACER_H

#include "uiwidget.h"

class UIWidgetSpacer : public UIWidget
{
public:
	UIWidgetSpacer();
	virtual ~UIWidgetSpacer();

	DEFINE_UIWIDGET_FACTORY( Spacer );
};

#endif // UIWIDGETSPACER_H
