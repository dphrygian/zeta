#ifndef UIINPUTMAPROSA_H
#define UIINPUTMAPROSA_H

#include "iuiinputmap.h"

class RosaFramework;

class UIInputMapRosa : public IUIInputMap
{
public:
	UIInputMapRosa( RosaFramework* const pFramework );
	virtual ~UIInputMapRosa();

	virtual bool OnNext();
	virtual bool OnPrevious();
	virtual bool OnUp();
	virtual bool OnDown();
	virtual bool OnLeft();
	virtual bool OnRight();

	virtual bool OnPrevPage();
	virtual bool OnNextPage();

	virtual bool OnAccept();
	virtual bool OnCancel();

protected:
	RosaFramework*	m_Framework;
};

#endif // UIINPUTMAPROSA_H
