#ifndef UISCREENROSAHACKING_H
#define UISCREENROSAHACKING_H

#include "uiscreen.h"

class UIScreenRosaHacking : public UIScreen
{
public:
	UIScreenRosaHacking();
	virtual ~UIScreenRosaHacking();

	DEFINE_UISCREEN_FACTORY( RosaHacking );

	virtual void		Popped();
};

#endif // UISCREENROSAHACKING_H
