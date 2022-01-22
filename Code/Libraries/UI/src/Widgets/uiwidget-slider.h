#ifndef UIWIDGETSLIDER_H
#define UIWIDGETSLIDER_H

#include "uiwidget.h"
#include "vector2.h"
#include "simplestring.h"

class Font;
class Mesh;
class UIWidgetText;
class UIWidgetImage;

class UIWidgetSlider : public UIWidget
{
public:
	UIWidgetSlider();
	UIWidgetSlider( const SimpleString& DefinitionName );
	virtual ~UIWidgetSlider();

	DEFINE_UIWIDGET_FACTORY( Slider );

	virtual void	Render( bool HasFocus );
	virtual void	UpdateRender();
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	GetBounds( SRect& OutBounds );
	virtual void	Refresh();
	virtual void	Released();
	virtual void	Drag( float X, float Y );
	virtual bool	HandleInput();

	virtual void	OnTrigger();

	void			SendSliderEvent() const;

	void			SetSliderValue( const float T );	// T clamped to range [0,1]
	float			GetSliderValue() const;				// Returns range [0,1]

	void			SetSliderNotch( const float T );
	void			TryNotch();

	// For maintaining state when reinitializing widget.
	virtual void	PushState();
	virtual void	PullState();

	UIWidgetText*	m_SliderLabel;
	UIWidgetText*	m_SliderValue;
	UIWidgetImage*	m_SliderBack;
	UIWidgetImage*	m_Slider;

	float			m_ShiftAmount;

	float			m_SlideValue_SavedState;

	float			m_NotchRadius;
	float			m_NotchValue;		// 0-1
};

#endif // UIWIDGETSLIDER_H
