#include "core.h"
#include "uiscreen-rosasetres.h"
#include "configmanager.h"
#include "display.h"
#include "3d.h"
#include "irenderer.h"
#include "array.h"
#include "uimanager.h"
#include "stringmanager.h"
#include "Widgets/uiwidget-text.h"
#include "uifactory.h"

#define DEBUG_SETRES 1

UIScreenRosaSetRes::UIScreenRosaSetRes()
:	m_ResMap()
,	m_Callback()
{
}

UIScreenRosaSetRes::~UIScreenRosaSetRes()
{
}

void UIScreenRosaSetRes::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Flush();
	m_ResMap.Clear();

	UIScreen::InitializeFromDefinition( DefinitionName );

	Display* const pDisplay = m_UIManager->GetDisplay();
	Array<SDisplayMode> DisplayModes;
	m_UIManager->GetRenderer()->EnumerateDisplayModes( DisplayModes );

	DisplayModes.InsertionSort();
	ASSERT( DisplayModes.Size() );

	MAKEHASH( DefinitionName );

	// Dynamically create definitions for each text button

	STATICHASH( Rules );
	SimpleString UsingRules = ConfigManager::GetString( sRules, "", sDefinitionName );

	MAKEHASH( UsingRules );

	STATICHASH( Archetype );
	const SimpleString Archetype = ConfigManager::GetString( sArchetype, "", sUsingRules );

	STATICHASH( Parent );
	const SimpleString Parent = ConfigManager::GetString( sParent, "", sUsingRules );

	STATICHASH( ParentHYBase );
	const float YBase = ConfigManager::GetFloat( sParentHYBase, 0.0f, sUsingRules );

	STATICHASH( ParentHYStep );
	const float YStep = ConfigManager::GetFloat( sParentHYStep, 0.0f, sUsingRules );

	STATICHASH( Column0ParentWX );
	const float Column0X = ConfigManager::GetFloat( sColumn0ParentWX, 0.0f, sUsingRules );

	STATICHASH( Column1ParentWX );
	const float Column1X = ConfigManager::GetFloat( sColumn1ParentWX, 0.0f, sUsingRules );

#if DEBUG_SETRES
	PRINTF( "UIScreenRosaSetRes info:\n" );
	PRINTF( "  ScreenWidth:  %d\n", pDisplay->m_ScreenWidth );
	PRINTF( "  ScreenHeight: %d\n", pDisplay->m_ScreenHeight );
#endif

	FOR_EACH_ARRAY_REVERSE( DisplayIter, DisplayModes, SDisplayMode )
	{
		const SDisplayMode& DisplayMode = DisplayIter.GetValue();
		if( !pDisplay->m_Fullscreen &&
			(	DisplayMode.Width > pDisplay->m_ScreenWidth ||
				DisplayMode.Height > pDisplay->m_ScreenHeight ) )
		{
#if DEBUG_SETRES
			PRINTF( "  Skipping mode %dx%d:\n", DisplayMode.Width, DisplayMode.Height );
#endif
			DisplayModes.FastRemove( DisplayIter );
		}
	}

	const int NumDisplayModes	= static_cast<int>( DisplayModes.Size() );
	const int LeftColumnSize	= ( NumDisplayModes + 1 ) / 2;
	const int RightColumnSize	= ( NumDisplayModes + 1 ) - LeftColumnSize;	// Add one for the Back button

	for( int DisplayModeIndex = 0; DisplayModeIndex < NumDisplayModes; ++DisplayModeIndex )
	{
		const SDisplayMode& DisplayMode = DisplayModes[ DisplayModeIndex ];

		float ScreenX = 0.0f;
		float ScreenY = 0.0f;

		int FocusShiftUp = 0;
		int FocusShiftDown = 0;
		int FocusShiftLeft = 0;
		int FocusShiftRight = 0;

		if( DisplayModeIndex < LeftColumnSize )
		{
			// Display mode is in left column
			ScreenY			= YBase + DisplayModeIndex * YStep;
			ScreenX			= Column0X;
			FocusShiftLeft	= -RightColumnSize;
			FocusShiftRight	= LeftColumnSize;
		}
		else
		{
			// Display mode is in right column
			ScreenY			= YBase + ( DisplayModeIndex - LeftColumnSize ) * YStep;
			ScreenX			= Column1X;
			FocusShiftLeft	= -LeftColumnSize;
			FocusShiftRight	= RightColumnSize;
		}

		if( DisplayModeIndex == 0 )
		{
			// Display mode is on top left
			FocusShiftUp = -( RightColumnSize + 1 );
		}
		else if( DisplayModeIndex == ( LeftColumnSize - 1 ) )
		{
			// Display mode is on bottom left
			FocusShiftDown = ( RightColumnSize + 1 );

			// If columns are the same size (*including* the back button),
			// this widget doesn't have a pair in the right column.
			if( LeftColumnSize == RightColumnSize )
			{
				// Setting focus override to 0 uses the screen's shift values, so loop instead.
				FocusShiftLeft = -( NumDisplayModes + 1 );
				FocusShiftRight = ( NumDisplayModes + 1 );
			}
		}
		else if( DisplayModeIndex == LeftColumnSize )
		{
			// Display mode is on top right left
			FocusShiftUp = -( LeftColumnSize + 1 );
		}
		// Back button shift down is handled below this loop.

		const SimpleString NewDefinitionName	= SimpleString::PrintF( "_Res%d", DisplayModeIndex );
		// HACKHACK: Insert the UTF-8 codes for the U+00D7 (multiplication sign)
		const SimpleString ResolutionString		= SimpleString::PrintF( "%d \xc3\x97 %d", DisplayMode.Width, DisplayMode.Height );

		MAKEHASH( NewDefinitionName );

		STATICHASH( UIWidgetType );
		ConfigManager::SetString( sUIWidgetType, "Text", sNewDefinitionName );

		STATICHASH( Extends );
		ConfigManager::SetString( sExtends, Archetype.CStr(), sNewDefinitionName );

		ConfigManager::SetString( sParent, Parent.CStr(), sNewDefinitionName );

		STATICHASH( String );
		ConfigManager::SetString( sString, ResolutionString.CStr(), sNewDefinitionName );

		STATICHASH( ParentWX );
		ConfigManager::SetFloat( sParentWX, ScreenX, sNewDefinitionName );

		STATICHASH( ParentHY );
		ConfigManager::SetFloat( sParentHY, ScreenY, sNewDefinitionName );

		STATICHASH( FocusShiftUp );
		ConfigManager::SetInt( sFocusShiftUp, FocusShiftUp, sNewDefinitionName );

		STATICHASH( FocusShiftDown );
		ConfigManager::SetInt( sFocusShiftDown, FocusShiftDown, sNewDefinitionName );

		STATICHASH( FocusShiftLeft );
		ConfigManager::SetInt( sFocusShiftLeft, FocusShiftLeft, sNewDefinitionName );

		STATICHASH( FocusShiftRight );
		ConfigManager::SetInt( sFocusShiftRight, FocusShiftRight, sNewDefinitionName );

		UIWidget* const pResWidget = UIFactory::CreateWidget( NewDefinitionName, this, NULL );
		ASSERT( pResWidget );

		pResWidget->m_Callback = m_Callback;

		AddWidget( pResWidget );

		m_ResMap[ NewDefinitionName ] = DisplayMode;
	}

	STATIC_HASHED_STRING( SetResBackButton );
	UIWidget* const pBackButtonWidget = GetWidget( sSetResBackButton );
	ASSERT( pBackButtonWidget );
	pBackButtonWidget->m_FocusShiftLeft		= -( NumDisplayModes + 1 );
	pBackButtonWidget->m_FocusShiftRight	=  ( NumDisplayModes + 1 );
	pBackButtonWidget->m_FocusShiftDown		=  ( LeftColumnSize + 1 );

	UpdateRender();
	ResetFocus();
	RefreshWidgets();
}

/*virtual*/ void UIScreenRosaSetRes::Pushed()
{
	UIScreen::Pushed();

	// Reinitialize whenever this screen is pushed. This re-enumerates the resolutions
	// in the current mode and pushes the callback to dynamic widgets.
	InitializeFromDefinition( m_Name );
}

void UIScreenRosaSetRes::SetUICallback( const SUICallback& Callback )
{
	m_Callback = Callback;
}

SDisplayMode UIScreenRosaSetRes::GetRes( const HashedString& Name )
{
	ASSERT( m_ResMap.Search( Name ).IsValid() );
	return m_ResMap[ Name ];
}
