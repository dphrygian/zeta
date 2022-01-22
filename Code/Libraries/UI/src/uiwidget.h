#ifndef UIWIDGET_H
#define UIWIDGET_H

#include "ui.h"
#include "simplestring.h"
#include "vector2.h"
#include "vector4.h"

class UIManager;
class UIScreen;
class WBAction;
struct SRect;

#define DEFINE_UIWIDGET_FACTORY( type ) static class UIWidget* Factory() { return new UIWidget##type; }
typedef class UIWidget* ( *UIWidgetFactoryFunc )( void );

class UIWidget
{
public:
	enum EWidgetOrigin
	{
		EWO_TopLeft,
		EWO_TopCenter,
		EWO_TopRight,
		EWO_CenterLeft,
		EWO_Center,
		EWO_CenterRight,
		EWO_BottomLeft,
		EWO_BottomCenter,
		EWO_BottomRight,
	};

	UIWidget();
	virtual ~UIWidget();

	virtual void		Render( bool HasFocus );
	virtual void		UpdateRender();				// Pushes widget changes to renderable
	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void		GetBounds( SRect& OutBounds );
	virtual void		OnTrigger();
	virtual void		Released();
	virtual void		Drag( float X, float Y );
	virtual bool		HandleInput();
	virtual void		GetFocus();
	virtual void		Refresh();
	virtual void		Tick( const float DeltaTime );

	bool				HasFocus() const;

	void				Reinitialize( const bool WithState );
	void				Reinitialize()				{ Reinitialize( true ); }
	void				Reinitialize_Stateless()	{ Reinitialize( false ); }

	int					OverrideFocusUp( int FocusShift );
	int					OverrideFocusDown( int FocusShift );
	int					OverrideFocusLeft( int FocusShift );
	int					OverrideFocusRight( int FocusShift );

	bool				IsHidden() const { return m_Hidden; }
	void				SetHidden( const bool Hidden );
	void				Show();
	void				Hide() { m_Hidden = true; }

	bool				IsDisabled() const { return m_IsDisabled; }
	virtual void		SetDisabled( const bool Disabled );

	bool				IsUnfocusable() const { return m_IsDisabled || m_Hidden; }

	// X and Y are screen values in range [0,1]
	void				SetScreenLocation( const float X, const float Y );
	void				SetParentLocation( const float X, const float Y );
	// HACKHACK for list spacing
	virtual void		SetListY( const float ListY );

	void				SetExtentsScalar( const float W, const float H );

	void				SetColor( const Vector4 Color ){ m_Color = Color; }
	void				SetAlpha( const float Alpha ){ m_Color.a = Alpha; }

	void				ClearActions();

	// For maintaining state when reinitializing widget.
	virtual void		PushState();
	virtual void		PullState();

	virtual float		GetX() { return m_Location.x; }
	virtual float		GetY() { return m_Location.y; }
	virtual float		GetWidth() { return m_Extents.x; }
	virtual float		GetHeight() { return m_Extents.y; }

	UIScreen*			GetOwnerScreen() const						{ return m_OwnerScreen; }
	void				SetOwnerScreen( UIScreen* const pScreen )	{ m_OwnerScreen = pScreen; }

	UIWidget*			GetParentWidget() const						{ return m_ParentWidget; }
	void				SetParentWidget( UIWidget* const pWidget )	{ m_ParentWidget = pWidget; }

	void				GetOrigin( const HashedString& Origin );
	void				SetPositionFromOrigin( const Vector2& Location, const Vector2& Extents );

	void				SetTransform( const Vector2& Location, const Vector2& Extents );
	void				SetConfigTransform( const SimpleString& DefinitionName );
	void				GetConfigTransform( const SimpleString& DefinitionName, Vector2& OutLocation, Vector2& OutExtents ) const;
	void				AdjustDimensionsToParent( const SimpleString& DefinitionName, Vector2& Location, Vector2& Extents );

	Vector4				GetHighlightColor() const;

	void				ClampAlignForPixelGrid();

	bool				GetUseVanishingPointY( const bool CheckParents ) const;

	SimpleString		m_Name;
	int					m_RenderPriority;	// Low numbers draw first, so high numbers draw on top
	bool				m_CanBeFocused;
	bool				m_RenderInWorld;
	bool				m_IsDisabled;
	bool				m_IsDisabled_SavedState;
	int					m_FocusOrder;
	UIScreen*			m_OwnerScreen;

	int					m_FocusShiftUp;
	int					m_FocusShiftDown;
	int					m_FocusShiftLeft;
	int					m_FocusShiftRight;

	HashedString		m_EventName;	// Event that gets fired when this widget is triggered
	SUICallback			m_Callback;		// Callback that is invoked when this widget is triggered
	Array<WBAction*>	m_Actions;		// Actions that are executed when this widget is triggered
	Array<WBAction*>	m_FocusActions;	// Actions that are executed when this widget receives focus
	bool				m_OwnsActions;	// Does this widget own its m_Actions?

	bool				m_Hidden;
	bool				m_Hidden_SavedState;
	Vector4				m_Color;		// Multiply color
	Vector4				m_ScreenColor;	// Screen color
	Vector4				m_Highlight;
	Vector4				m_DisabledColor;
	Vector2				m_Location;		// Top left position
	Vector2				m_Extents;
	Vector2				m_Velocity;
	bool				m_AllowNegativeOrigin;	// If true, negative origin is used literally instead of being used as relative to parent's high bound. Useful for spacers.
	EWidgetOrigin		m_Origin;
	UIWidget*			m_ParentWidget;	// Must be defined before this widget, obviously
	bool				m_ClampToPixelGrid;
	bool				m_UseVanishingPointY;

	bool				m_UsePulsingHighlight;
	float				m_PulsingHighlightMul;
	float				m_PulsingHighlightAdd;
	float				m_PulsingHighlightTimeScalar;

	// HACKHACK for list spacing
	bool				m_ListStep;	// ListStep means increment the list Y and use it to position this widget (e.g., for the composite button containing a label and a setting or slider)
	bool				m_UseListY;	// UseListY means use the current list Y to position this widget (e.g., for the label, setting, or slider within a composite button)

#if BUILD_DEBUG
	debug_int64_t		m_LastRenderedTime;
#endif

	static void			SetUIManager( UIManager* pManager );
	static UIManager*	m_UIManager;
};

#endif // UIWIDGET_H
