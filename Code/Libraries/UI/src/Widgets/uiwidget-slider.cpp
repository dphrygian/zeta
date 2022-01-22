#include "core.h"
#include "3d.h"
#include "uiwidget-slider.h"
#include "uimanager.h"
#include "font.h"
#include "mesh.h"
#include "shadermanager.h"
#include "configmanager.h"
#include "stringmanager.h"
#include "Widgets/uiwidget-text.h"
#include "Widgets/uiwidget-image.h"
#include "mathcore.h"
#include "uifactory.h"
#include "wbeventmanager.h"
#include "wbworld.h"
#include "iuiinputmap.h"

UIWidgetSlider::UIWidgetSlider()
:	m_SliderLabel( NULL )
,	m_SliderValue( NULL )
,	m_SliderBack( NULL )
,	m_Slider( NULL )
,	m_ShiftAmount( 0.0f )
,	m_SlideValue_SavedState( 0.0f )
,	m_NotchRadius( 0.0f )
,	m_NotchValue( 0.0f )
{
}

UIWidgetSlider::UIWidgetSlider( const SimpleString& DefinitionName )
:	m_SliderLabel( NULL )
,	m_SliderValue( NULL )
,	m_SliderBack( NULL )
,	m_Slider( NULL )
,	m_ShiftAmount( 0.0f )
,	m_SlideValue_SavedState( 0.0f )
,	m_NotchRadius( 0.0f )
,	m_NotchValue( 0.0f )
{
	InitializeFromDefinition( DefinitionName );
}

UIWidgetSlider::~UIWidgetSlider()
{
	SafeDelete( m_SliderLabel );
	SafeDelete( m_SliderValue );
	SafeDelete( m_SliderBack );
	SafeDelete( m_Slider );
}

void UIWidgetSlider::UpdateRender()
{
	if( m_SliderLabel )
	{
		m_SliderLabel->UpdateRender();
	}

	if( m_SliderValue )
	{
		m_SliderValue->UpdateRender();
	}

	m_SliderBack->UpdateRender();
	m_Slider->UpdateRender();
}

void UIWidgetSlider::Render( bool HasFocus )
{
	XTRACE_FUNCTION;

	if( m_SliderLabel )
	{
		m_SliderLabel->Render( HasFocus );
	}

	if( m_SliderValue )
	{
		m_SliderValue->Render( HasFocus );
	}

	m_SliderBack->Render( HasFocus );
	m_Slider->Render( HasFocus );
}

void UIWidgetSlider::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	SafeDelete( m_SliderLabel );
	SafeDelete( m_SliderValue );
	SafeDelete( m_SliderBack );
	SafeDelete( m_Slider );

	UIWidget::InitializeFromDefinition( DefinitionName );

	STATICHASH( SliderLabel );
	STATICHASH( SliderValue );
	STATICHASH( SliderBack );
	STATICHASH( Slider );
	MAKEHASH( DefinitionName );

	STATICHASH( ShiftAmount );
	m_ShiftAmount = ConfigManager::GetInheritedFloat( sShiftAmount, 0.1f, sDefinitionName );

	STATICHASH( NotchRadius );
	m_NotchRadius = ConfigManager::GetInheritedFloat( sNotchRadius, 0.1f, sDefinitionName );

	DEVASSERT( m_NotchRadius < m_ShiftAmount );

	// Initialize notch to an invalid value so non-notched sliders don't notch at 0
	m_NotchValue = -1.0f;

	m_SliderLabel	= static_cast<UIWidgetText*>(	UIFactory::CreateWidget( ConfigManager::GetInheritedString( sSliderLabel, "", sDefinitionName ), GetOwnerScreen(), this ) );
	m_SliderValue	= static_cast<UIWidgetText*>(	UIFactory::CreateWidget( ConfigManager::GetInheritedString( sSliderValue, "", sDefinitionName ), GetOwnerScreen(), this ) );
	m_SliderBack	= static_cast<UIWidgetImage*>(	UIFactory::CreateWidget( ConfigManager::GetInheritedString( sSliderBack, "", sDefinitionName ), GetOwnerScreen(), this ) );
	m_Slider		= static_cast<UIWidgetImage*>(	UIFactory::CreateWidget( ConfigManager::GetInheritedString( sSlider, "", sDefinitionName ), GetOwnerScreen(), this ) );

	// We don't need a label or value, but we should have a slider and back.
	ASSERT( m_SliderBack );
	ASSERT( m_Slider );
}

void UIWidgetSlider::GetBounds( SRect& OutBounds )
{
	SRect SliderBackBounds;
	SRect SliderBounds;

	m_SliderBack->GetBounds( SliderBackBounds );
	m_Slider->GetBounds( SliderBounds );

	OutBounds.m_Left	= Min( SliderBackBounds.m_Left,		SliderBounds.m_Left );
	OutBounds.m_Top		= Min( SliderBackBounds.m_Top,		SliderBounds.m_Top );
	OutBounds.m_Right	= Max( SliderBackBounds.m_Right,	SliderBounds.m_Right );
	OutBounds.m_Bottom	= Max( SliderBackBounds.m_Bottom,	SliderBounds.m_Bottom );

	if( m_SliderLabel )
	{
		SRect LabelBounds;

		m_SliderLabel->GetBounds( LabelBounds );

		OutBounds.m_Left	= Min( OutBounds.m_Left,	LabelBounds.m_Left );
		OutBounds.m_Top		= Min( OutBounds.m_Top,		LabelBounds.m_Top );
		OutBounds.m_Right	= Max( OutBounds.m_Right,	LabelBounds.m_Right );
		OutBounds.m_Bottom	= Max( OutBounds.m_Bottom,	LabelBounds.m_Bottom );
	}

	if( m_SliderValue )
	{

		SRect ValueBounds;

		m_SliderValue->GetBounds( ValueBounds );

		OutBounds.m_Left	= Min( OutBounds.m_Left,	ValueBounds.m_Left );
		OutBounds.m_Top		= Min( OutBounds.m_Top,		ValueBounds.m_Top );
		OutBounds.m_Right	= Max( OutBounds.m_Right,	ValueBounds.m_Right );
		OutBounds.m_Bottom	= Max( OutBounds.m_Bottom,	ValueBounds.m_Bottom );
	}
}

void UIWidgetSlider::Refresh()
{
	UIWidget::Refresh();

	if( m_SliderLabel )
	{
		m_SliderLabel->Refresh();
	}

	if( m_SliderValue )
	{
		m_SliderValue->Refresh();
	}

	m_SliderBack->Refresh();
	m_Slider->Refresh();
}

/*virtual*/ void UIWidgetSlider::Released()
{
	OnTrigger();
}

/*virtual*/ void UIWidgetSlider::OnTrigger()
{
	UIWidget::OnTrigger();

	SendSliderEvent();
}

void UIWidgetSlider::SendSliderEvent() const
{
	WB_MAKE_EVENT( OnSliderChanged, NULL );
	WB_SET_AUTO( OnSliderChanged, Hash, SliderName, m_Name );
	WB_SET_AUTO( OnSliderChanged, Float, SliderValue, GetSliderValue() );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnSliderChanged, NULL );
}

/*virtual*/ void UIWidgetSlider::Drag( float X, float Y )
{
	Unused( Y );

	m_Slider->m_Location.x = Clamp( X - ( m_Slider->m_Extents.x / 2 ), m_SliderBack->m_Location.x, m_SliderBack->m_Location.x + m_SliderBack->m_Extents.x - m_Slider->m_Extents.x );
	TryNotch();
	UpdateRender();

	SendSliderEvent();
}

/*virtual*/ bool UIWidgetSlider::HandleInput()
{
	if( UIWidget::HandleInput() )
	{
		return true;
	}

	IUIInputMap* const pInputMap = m_UIManager->GetUIInputMap();

	if( pInputMap )
	{
		const float CurrentValue = GetSliderValue();
		if( pInputMap->OnLeft() )
		{
			const float	TargetValue	= CurrentValue - m_ShiftAmount;
			const bool	ShouldNotch	= ( !Equal( CurrentValue, m_NotchValue ) && CurrentValue > m_NotchValue && TargetValue < m_NotchValue );
			const float	UsingValue	= ShouldNotch ? m_NotchValue : TargetValue;
			SetSliderValue( UsingValue );
			return true;
		}
		else if( pInputMap->OnRight() )
		{
			const float	TargetValue	= CurrentValue + m_ShiftAmount;
			const bool	ShouldNotch	= ( !Equal( CurrentValue, m_NotchValue ) && CurrentValue < m_NotchValue && TargetValue > m_NotchValue );
			const float	UsingValue	= ShouldNotch ? m_NotchValue : TargetValue;
			SetSliderValue( UsingValue );
			return true;
		}
	}

	return false;
}

void UIWidgetSlider::SetSliderValue( const float T )
{
	m_Slider->m_Location.x = Lerp( m_SliderBack->m_Location.x, m_SliderBack->m_Location.x + m_SliderBack->m_Extents.x - m_Slider->m_Extents.x, Saturate( T ) );
	TryNotch();
	UpdateRender();

	SendSliderEvent();
}

void UIWidgetSlider::TryNotch()
{
	// This is called from SetSliderValue, so don't do the other side effects that typically go along with setting slider position
	if( Abs( GetSliderValue() - m_NotchValue ) <= m_NotchRadius )
	{
		m_Slider->m_Location.x = Lerp( m_SliderBack->m_Location.x, m_SliderBack->m_Location.x + m_SliderBack->m_Extents.x - m_Slider->m_Extents.x, m_NotchValue );
	}
}

float UIWidgetSlider::GetSliderValue() const
{
	return Saturate( InvLerp( m_Slider->m_Location.x, m_SliderBack->m_Location.x, m_SliderBack->m_Location.x + m_SliderBack->m_Extents.x - m_Slider->m_Extents.x ) );
}

void UIWidgetSlider::SetSliderNotch( const float T )
{
	m_NotchValue = Saturate( T );
}

/*virtual*/ void UIWidgetSlider::PushState()
{
	UIWidget::PushState();

	m_SlideValue_SavedState = GetSliderValue();
}

/*virtual*/ void UIWidgetSlider::PullState()
{
	UIWidget::PullState();

	SetSliderValue( m_SlideValue_SavedState );
}
