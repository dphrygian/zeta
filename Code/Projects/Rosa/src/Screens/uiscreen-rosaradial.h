#ifndef UISCREENROSARADIAL_H
#define UISCREENROSARADIAL_H

#include "uiscreen.h"
#include "vector2.h"
#include "array.h"

class UIWidgetImage;

class UIScreenRosaRadial : public UIScreen
{
public:
	UIScreenRosaRadial();
	virtual ~UIScreenRosaRadial();

	DEFINE_UISCREEN_FACTORY( RosaRadial );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickReturn	Tick( const float DeltaTime, bool HasFocus );
	virtual void		Pushed();

private:
	void				SelectSlot( const uint SlotIndex );

	struct SSlot
	{
		SSlot()
		:	m_Name()
		,	m_Direction()
		,	m_Widget( NULL )
		,	m_Hidden( false )
		{
		}

		HashedString	m_Name;
		Vector2			m_Direction;
		UIWidgetImage*	m_Widget;
		bool			m_Hidden;
	};

	Array<SSlot>	m_Slots;
	float			m_SlotDotThreshold;
	uint			m_SelectedSlot;
};

#endif // UISCREENROSARADIAL_H
