#include "core.h"
#include "3d.h"
#include "uiwidget-text.h"
#include "uimanager.h"
#include "font.h"
#include "mesh.h"
#include "shadermanager.h"
#include "configmanager.h"
#include "stringmanager.h"
#include "irenderer.h"
#include "mathcore.h"
#include "fontmanager.h"
#include "texturemanager.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "ivertexdeclaration.h"
#include "hsv.h"

UIWidgetText::UIWidgetText()
:	m_Font( NULL )
,	m_IsLiteral( false )
,	m_IsDynamicPosition( false )
,	m_String()
,	m_DynamicString()
,	m_Mesh( NULL )
,	m_Material()
,	m_FontPrintFlags( FONT_PRINT_LEFT )
,	m_TextColor( 0 )
,	m_Scale( 0.0f )
,	m_WrapWidth( 0.0f )
,	m_OriginalNumIndices( 0 )
,	m_HasDropShadow( false )
,	m_DropShadowOffset()
,	m_DropShadowColor( ARGB_TO_COLOR( 255, 0, 0, 0 ) )
,	m_DropShadowMesh( NULL )
,	m_HasShadowBox( false )
,	m_ShadowBoxTexture( NULL )
,	m_ShadowBoxBorder( 0.0f )
,	m_ShadowBoxMargin( 0.0f )
,	m_ShadowBoxMesh( NULL )
{
}

UIWidgetText::UIWidgetText( const SimpleString& DefinitionName )
:	m_Font( NULL )
,	m_IsLiteral( false )
,	m_IsDynamicPosition( false )
,	m_String()
,	m_DynamicString()
,	m_Mesh( NULL )
,	m_Material()
,	m_FontPrintFlags( FONT_PRINT_LEFT )
,	m_TextColor( 0 )
,	m_Scale( 0.0f )
,	m_WrapWidth( 0.0f )
,	m_OriginalNumIndices( 0 )
,	m_HasDropShadow( false )
,	m_DropShadowOffset()
,	m_DropShadowColor( ARGB_TO_COLOR( 255, 0, 0, 0 ) )
,	m_DropShadowMesh( NULL )
,	m_HasShadowBox( false )
,	m_ShadowBoxTexture( NULL )
,	m_ShadowBoxBorder( 0.0f )
,	m_ShadowBoxMargin( 0.0f )
,	m_ShadowBoxMesh( NULL )
{
	InitializeFromDefinition( DefinitionName );
}

UIWidgetText::~UIWidgetText()
{
	SafeDelete( m_Mesh );
	SafeDelete( m_DropShadowMesh );
	SafeDelete( m_ShadowBoxMesh );
}

void UIWidgetText::UpdateRenderPosition()
{
	ASSERT( m_Mesh );

	m_Mesh->m_Location = Vector( m_Location.x, 0.0f, m_Location.y );
	m_Mesh->m_AABB.m_Min.x += m_Location.x;
	m_Mesh->m_AABB.m_Min.z += m_Location.y;
	m_Mesh->m_AABB.m_Max.x += m_Location.x;
	m_Mesh->m_AABB.m_Max.z += m_Location.y;

	if( m_HasDropShadow )
	{
		ASSERT( m_DropShadowMesh );

		m_DropShadowMesh->m_Location		= m_Mesh->m_Location;
		m_DropShadowMesh->m_Location.x		+= m_DropShadowOffset.x;
		m_DropShadowMesh->m_Location.z		+= m_DropShadowOffset.y;

		m_DropShadowMesh->m_AABB			= m_Mesh->m_AABB;
		m_DropShadowMesh->m_AABB.m_Min.x	+= m_DropShadowOffset.x;
		m_DropShadowMesh->m_AABB.m_Min.z	+= m_DropShadowOffset.y;
		m_DropShadowMesh->m_AABB.m_Max.x	+= m_DropShadowOffset.x;
		m_DropShadowMesh->m_AABB.m_Max.z	+= m_DropShadowOffset.y;
	}

	// ROSATODO: Properly handle shadow box location/AABB.
	// (This currently doesn't matter because I'm always rebuilding the mesh when
	// the string changes, which is currently the only time anything moves anyway.)
}

void UIWidgetText::UpdateRender()
{
	// This will fail if UpdateRender is called between submitting UI meshes to the renderer and actually
	// rendering the scene. It may actually be safe if this widget was not submitted to the renderer.
	DEBUGASSERT( m_UIManager->DEBUGIsSafeToUpdateRender( m_LastRenderedTime ) );

	SafeDelete( m_Mesh );
	SafeDelete( m_DropShadowMesh );
	SafeDelete( m_ShadowBoxMesh );

	IRenderer* pRenderer = m_UIManager->GetRenderer();

	m_Mesh = m_Font->Print( m_String.CStr(), SRect( 0.0f, 0.0f, m_WrapWidth, 0.0f ), m_FontPrintFlags, m_TextColor );
	ASSERT( m_Mesh );

	// HACKHACK: Cache the num indices so if a client adjusts it (e.g. for conversation text), we can
	// always query the original without needing to make assumptions or know when it was updated.
	m_OriginalNumIndices = m_Mesh->GetNumIndices();

	m_Mesh->m_Location		= Vector( m_Location.x, 0.0f, m_Location.y );
	m_Mesh->m_Scale			= Vector( m_Scale, 1.0f, m_Scale );
	m_Mesh->m_AABB.m_Min.x	= m_Mesh->m_AABB.m_Min.x * m_Scale + m_Location.x;
	m_Mesh->m_AABB.m_Min.z	= m_Mesh->m_AABB.m_Min.z * m_Scale + m_Location.y;
	m_Mesh->m_AABB.m_Max.x	= m_Mesh->m_AABB.m_Max.x * m_Scale + m_Location.x;
	m_Mesh->m_AABB.m_Max.z	= m_Mesh->m_AABB.m_Max.z * m_Scale + m_Location.y;

	m_Mesh->SetMaterialDefinition( m_Material, pRenderer );
	m_Mesh->SetMaterialFlags( m_RenderInWorld ? MAT_INWORLDHUD : MAT_HUD );
	m_Mesh->m_ConstantColor = m_Color;

	// Enable vertex colors, disable constant color
	static const Vector4 skDefaultHUDParams = Vector4( 1.0f, 0.0f, 0.0f, 0.0f );
	STATIC_HASHED_STRING( HUDParams );
	m_Mesh->SetShaderConstant( sHUDParams, skDefaultHUDParams );

	if( m_HasDropShadow )
	{
		m_DropShadowMesh = new Mesh;
		m_DropShadowMesh->Initialize( m_Mesh->m_VertexBuffer, m_Mesh->m_VertexDeclaration, m_Mesh->m_IndexBuffer, NULL );
		m_DropShadowMesh->m_Material		= m_Mesh->m_Material;

		m_DropShadowMesh->m_Location		= m_Mesh->m_Location;
		m_DropShadowMesh->m_Location.x		+= m_DropShadowOffset.x;
		m_DropShadowMesh->m_Location.z		+= m_DropShadowOffset.y;
		m_DropShadowMesh->m_Scale			= m_Mesh->m_Scale;

		m_DropShadowMesh->m_AABB			= m_Mesh->m_AABB;
		m_DropShadowMesh->m_AABB.m_Min.x	= m_DropShadowMesh->m_AABB.m_Min.x * m_Scale + m_DropShadowOffset.x;
		m_DropShadowMesh->m_AABB.m_Min.z	= m_DropShadowMesh->m_AABB.m_Min.z * m_Scale + m_DropShadowOffset.y;
		m_DropShadowMesh->m_AABB.m_Max.x	= m_DropShadowMesh->m_AABB.m_Max.x * m_Scale + m_DropShadowOffset.x;
		m_DropShadowMesh->m_AABB.m_Max.z	= m_DropShadowMesh->m_AABB.m_Max.z * m_Scale + m_DropShadowOffset.y;

		m_DropShadowMesh->m_ConstantColor	= m_DropShadowColor;
	}

	if( m_HasShadowBox && m_String.Length() )
	{
		m_ShadowBoxMesh = CreateShadowBoxMesh();
		m_ShadowBoxMesh->m_Material			= m_Mesh->m_Material;
		m_ShadowBoxMesh->SetTexture( 0, m_ShadowBoxTexture );
	}
}

void UIWidgetText::Render( bool HasFocus )
{
	XTRACE_FUNCTION;

	UIWidget::Render( HasFocus );

	if( IsDisabled() )
	{
		m_Mesh->m_ConstantColor = m_DisabledColor;
	}
	else if( HasFocus )
	{
		m_Mesh->m_ConstantColor = GetHighlightColor();
	}
	else
	{
		m_Mesh->m_ConstantColor = m_Color;
	}

	STATIC_HASHED_STRING( ScreenColor );
	m_Mesh->SetShaderConstant( sScreenColor, m_ScreenColor );

	// Don't draw mesh if it's going to be invisible
	if( m_Mesh->m_ConstantColor.a > 0.0f )
	{
		if( NULL != m_ShadowBoxMesh )
		{
			m_UIManager->GetRenderer()->AddMesh( m_ShadowBoxMesh );
		}

		if( m_HasDropShadow )
		{
			m_UIManager->GetRenderer()->AddMesh( m_DropShadowMesh );
		}

		m_UIManager->GetRenderer()->AddMesh( m_Mesh );
	}
}

void UIWidgetText::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	UIWidget::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( DisplayWidth );
	const float DisplayWidth	= ConfigManager::GetFloat( sDisplayWidth );
	const float ParentWidth		= m_ParentWidget ? m_ParentWidget->GetWidth() : DisplayWidth;

	STATICHASH( DisplayHeight );
	const float DisplayHeight	= ConfigManager::GetFloat( sDisplayHeight );
	const float ParentHeight	= m_ParentWidget ? m_ParentWidget->GetHeight() : DisplayHeight;

	// If LoadString is false, we're expecting to dynamically set the string in code somewhere
	STATICHASH( LoadString );
	if( ConfigManager::GetInheritedBool( sLoadString, true, sDefinitionName ) )
	{
		STATICHASH( String );
		STATICHASH( DynamicString );

		STATICHASH( IsLiteral );
		m_IsLiteral = ConfigManager::GetInheritedBool( sIsLiteral, false, sDefinitionName );
		if( m_IsLiteral )
		{
			m_String		= ConfigManager::GetInheritedString( sString, "", sDefinitionName );
			m_DynamicString	= ConfigManager::GetInheritedString( sDynamicString, "", sDefinitionName );
		}
		else
		{
			m_String		= ConfigManager::GetLocalizedString( ConfigManager::GetInheritedHash( sString, HashedString::NullString, sDefinitionName ), "" );
			m_DynamicString	= ConfigManager::GetLocalizedString( ConfigManager::GetInheritedHash( sDynamicString, HashedString::NullString, sDefinitionName ), "" );
		}
	}

	const bool IsDynamicString = ( m_DynamicString != "" );
	STATICHASH( IsDynamicPosition );
	m_IsDynamicPosition = ConfigManager::GetInheritedBool( sIsDynamicPosition, IsDynamicString, sDefinitionName );

	STATICHASH( Material );
	const SimpleString DefaultMaterial = m_RenderInWorld ? "Material_HUDInWorld" : "Material_HUD";
	m_Material = ConfigManager::GetInheritedString( sMaterial, DefaultMaterial.CStr(), sDefinitionName );

	STATICHASH( Alignment );
	GetFontPrintFlags( ConfigManager::GetInheritedHash( sAlignment, HashedString::NullString, sDefinitionName ) );

	const Vector TextColorHSV	= HSV::GetConfigHSV( "TextColor", sDefinitionName, Vector( 0.0f, 0.0f, 1.0f ) );
	const Vector TextColor		= HSV::GetConfigRGB( "TextColor", sDefinitionName, HSV::HSVToRGB( TextColorHSV ) );
	m_TextColor = TextColor.ToColor();

	STATICHASH( HasDropShadow );
	m_HasDropShadow = ConfigManager::GetInheritedBool( sHasDropShadow, false, sDefinitionName );
	m_DropShadowColor = HSV::GetConfigRGBA( "DropShadowColor", sDefinitionName, Vector4( 1.0f, 1.0f, 1.0f, 1.0f ) );

	STATICHASH( HasShadowBox );
	m_HasShadowBox = ConfigManager::GetInheritedBool( sHasShadowBox, false, sDefinitionName );

	STATICHASH( ShadowBoxPixelBorder );
	STATICHASH( ShadowBoxParentHBorder );
	STATICHASH( ShadowBoxScreenHBorder );
	STATICHASH( ShadowBoxParentWBorder );
	STATICHASH( ShadowBoxScreenWBorder );
	m_ShadowBoxBorder = Pick(
						  ConfigManager::GetInheritedFloat( sShadowBoxPixelBorder,   0.0f, sDefinitionName ),
		ParentHeight	* ConfigManager::GetInheritedFloat( sShadowBoxParentHBorder, 0.0f, sDefinitionName ),
		DisplayHeight	* ConfigManager::GetInheritedFloat( sShadowBoxScreenHBorder, 0.0f, sDefinitionName ),
		ParentWidth		* ConfigManager::GetInheritedFloat( sShadowBoxParentWBorder, 0.0f, sDefinitionName ),
		DisplayWidth	* ConfigManager::GetInheritedFloat( sShadowBoxScreenWBorder, 0.0f, sDefinitionName ) );

	STATICHASH( ShadowBoxPixelMargin );
	STATICHASH( ShadowBoxParentHMargin );
	STATICHASH( ShadowBoxScreenHMargin );
	STATICHASH( ShadowBoxParentWMargin );
	STATICHASH( ShadowBoxScreenWMargin );
	m_ShadowBoxMargin = Pick(
						  ConfigManager::GetInheritedFloat( sShadowBoxPixelMargin,   0.0f, sDefinitionName ),
		ParentHeight	* ConfigManager::GetInheritedFloat( sShadowBoxParentHMargin, 0.0f, sDefinitionName ),
		DisplayHeight	* ConfigManager::GetInheritedFloat( sShadowBoxScreenHMargin, 0.0f, sDefinitionName ),
		ParentWidth		* ConfigManager::GetInheritedFloat( sShadowBoxParentWMargin, 0.0f, sDefinitionName ),
		DisplayWidth	* ConfigManager::GetInheritedFloat( sShadowBoxScreenWMargin, 0.0f, sDefinitionName ) );
	DEVASSERT( m_ShadowBoxMargin >= 0.0f );

	STATICHASH( ShadowBoxImage );
	const char* const Filename = ConfigManager::GetInheritedString( sShadowBoxImage, DEFAULT_TEXTURE, sDefinitionName );
	m_ShadowBoxTexture = m_UIManager->GetRenderer()->GetTextureManager()->GetTexture( Filename, TextureManager::ETL_Permanent );

	STATICHASH( PixelH );
	STATICHASH( ParentHH );
	STATICHASH( ScreenHH );
	STATICHASH( ParentWH );
	STATICHASH( ScreenWH );
	const float Height = Pick(
		ConfigManager::GetInheritedFloat( sPixelH, 0.0f, sDefinitionName ),
		ParentHeight	* ConfigManager::GetInheritedFloat( sParentHH, 0.0f, sDefinitionName ),
		DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHH, 0.0f, sDefinitionName ),
		ParentWidth		* ConfigManager::GetInheritedFloat( sParentWH, 0.0f, sDefinitionName ),
		DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWH, 0.0f, sDefinitionName ) );

	STATICHASH( FontTag );
	const char* const pFontTag = ConfigManager::GetInheritedString( sFontTag, DEFAULT_FONT_TAG, sDefinitionName );
	m_Font = m_UIManager->GetRenderer()->GetFontManager()->GetFont( pFontTag, Height );

	m_Scale = Height / m_Font->GetCapHeight();

	STATICHASH( PixelWrap );
	STATICHASH( ParentWWrap );
	STATICHASH( ScreenWWrap );
	STATICHASH( ParentHWrap );
	STATICHASH( ScreenHWrap );
	m_WrapWidth = Pick(
			ConfigManager::GetInheritedFloat( sPixelWrap, 0.0f, sDefinitionName ),
			ParentWidth		* ConfigManager::GetInheritedFloat( sParentWWrap, 0.0f, sDefinitionName ),
			DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWWrap, 0.0f, sDefinitionName ),
			ParentHeight	* ConfigManager::GetInheritedFloat( sParentHWrap, 0.0f, sDefinitionName ),
			DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHWrap, 0.0f, sDefinitionName ) );
	m_WrapWidth /= m_Scale;	// Wrap tests are performed relative to base font size

	STATICHASH( PixelShadowX );
	STATICHASH( ParentWShadowX );
	STATICHASH( ScreenWShadowX );
	STATICHASH( ParentHShadowX );
	STATICHASH( ScreenHShadowX );
	m_DropShadowOffset.x = Pick(
		ConfigManager::GetInheritedFloat( sPixelShadowX, 0.0f, sDefinitionName ),
		ParentWidth		* ConfigManager::GetInheritedFloat( sParentWShadowX, 0.0f, sDefinitionName ),
		DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWShadowX, 0.0f, sDefinitionName ),
		ParentHeight	* ConfigManager::GetInheritedFloat( sParentHShadowX, 0.0f, sDefinitionName ),
		DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHShadowX, 0.0f, sDefinitionName ) );

	STATICHASH( PixelShadowY );
	STATICHASH( ParentHShadowY );
	STATICHASH( ScreenHShadowY );
	STATICHASH( ParentWShadowY );
	STATICHASH( ScreenWShadowY );
	m_DropShadowOffset.y = Pick(
		ConfigManager::GetInheritedFloat( sPixelShadowY, 0.0f, sDefinitionName ),
		ParentHeight	* ConfigManager::GetInheritedFloat( sParentHShadowY, 0.0f, sDefinitionName ),
		DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHShadowY, 0.0f, sDefinitionName ),
		ParentWidth		* ConfigManager::GetInheritedFloat( sParentWShadowY, 0.0f, sDefinitionName ),
		DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWShadowY, 0.0f, sDefinitionName ) );

	UpdatePosition();

	UpdateRender();
}

void UIWidgetText::GetBounds( SRect& OutBounds )
{
	OutBounds = SRect( m_Mesh->m_AABB.m_Min.x, m_Mesh->m_AABB.m_Min.z, m_Mesh->m_AABB.m_Max.x, m_Mesh->m_AABB.m_Max.z );
}

/*virtual*/ float UIWidgetText::GetWidth()
{
	return m_Mesh->m_AABB.m_Max.x - m_Mesh->m_AABB.m_Min.x;
}

/*virtual*/ float UIWidgetText::GetHeight()
{
	return m_Mesh->m_AABB.m_Max.y - m_Mesh->m_AABB.m_Min.y;
}

void UIWidgetText::Refresh()
{
	UIWidget::Refresh();

	if( m_DynamicString != "" )
	{
		const SimpleString	ResolvedDynamicString	= StringManager::ParseConfigString( m_DynamicString.CStr() );
		const bool			StringChanged			= ( ResolvedDynamicString != m_String );

		m_String = ResolvedDynamicString;

		if( StringChanged )
		{
			if( m_IsDynamicPosition )
			{
				UpdatePosition();
				UpdateRenderPosition();
			}

			UpdateRender();
		}
	}
}

/*virtual*/ void UIWidgetText::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;

	if( !m_Velocity.IsZero() )
	{
		m_Location += m_Velocity * DeltaTime;

		ClampAlignForPixelGrid();

		UpdateRenderPosition();
	}
}

void UIWidgetText::UpdatePosition()
{
	Vector2 Location;
	Vector2 Extents;
	GetConfigTransform( m_Name, Location, Extents );

	// Get dimensions so we can do different origins
	Array<STypesetGlyph>	UnusedTypesetting;
	m_Font->Arrange( m_String, SRect( 0.0f, 0.0f, m_WrapWidth, 0.0f ), m_FontPrintFlags, UnusedTypesetting, Extents );
	Extents *= m_Scale;

	AdjustDimensionsToParent( m_Name, Location, Extents );

	// Add or subtract internal leading or all space below the baseline to center relative to em height (M height/capheight)
	// Add or subtract the font's vertical A from the location to account for the leading/height difference
	if( m_Origin == EWO_TopLeft || m_Origin == EWO_TopCenter || m_Origin == EWO_TopRight )
	{
		Location.y -= m_Font->GetInternalLeading() * m_Scale;
		Location.y -= ( m_Font->GetVerticalA() * m_Scale );
	}
	else if( m_Origin == EWO_BottomLeft || m_Origin == EWO_BottomCenter || m_Origin == EWO_BottomRight )
	{
		Location.y += ( m_Font->GetFontHeight() - m_Font->GetAscent() ) * m_Scale;
		Location.y += ( m_Font->GetVerticalC() * m_Scale );
	}
	else
	{
		Location.y += 0.5f * ( ( m_Font->GetFontHeight() - m_Font->GetAscent() ) - m_Font->GetInternalLeading() ) * m_Scale;
	}

	SetTransform( Location, Extents );
}

void UIWidgetText::GetFontPrintFlags( const HashedString& Alignment )
{
	STATIC_HASHED_STRING( Right );
	STATIC_HASHED_STRING( Center );
	STATIC_HASHED_STRING( Justify );

	if( Alignment == sRight )
	{
		m_FontPrintFlags = FONT_PRINT_RIGHT;
	}
	else if( Alignment == sCenter )
	{
		m_FontPrintFlags = FONT_PRINT_CENTER;
	}
	else if( Alignment == sJustify )
	{
		m_FontPrintFlags = FONT_PRINT_JUSTIFY;
	}
	else
	{
		DEVASSERT( m_FontPrintFlags == FONT_PRINT_LEFT );
	}
}

Mesh* UIWidgetText::CreateShadowBoxMesh() const
{
	static const uint kNumVertices	= 16;
	static const uint kNumIndices	= 54;

	const Vector TopLeft		= Vector( m_Location.x, 0.0f, m_Location.y );
	const Vector BottomRight	= TopLeft + Vector( m_Extents.x, 0.0f, m_Extents.y );

	Array<Vector> Positions;
	Positions.Reserve( kNumVertices );

	{
		Array<float> PositionsX;
		PositionsX.Reserve( 4 );

		if( m_ShadowBoxBorder < 0.0f )
		{
			PositionsX.PushBack( TopLeft.x - m_ShadowBoxMargin );
			PositionsX.PushBack( TopLeft.x - m_ShadowBoxMargin - m_ShadowBoxBorder );
			PositionsX.PushBack( BottomRight.x + m_ShadowBoxMargin + m_ShadowBoxBorder );
			PositionsX.PushBack( BottomRight.x + m_ShadowBoxMargin );
		}
		else
		{
			PositionsX.PushBack( TopLeft.x - m_ShadowBoxMargin - m_ShadowBoxBorder );
			PositionsX.PushBack( TopLeft.x - m_ShadowBoxMargin );
			PositionsX.PushBack( BottomRight.x + m_ShadowBoxMargin );
			PositionsX.PushBack( BottomRight.x + m_ShadowBoxMargin + m_ShadowBoxBorder );
		}

		Array<float> PositionsY;
		PositionsY.Reserve( 4 );

		if( m_ShadowBoxBorder < 0.0f )
		{
			PositionsY.PushBack( TopLeft.z );
			PositionsY.PushBack( TopLeft.z - m_ShadowBoxBorder );
			PositionsY.PushBack( BottomRight.z + m_ShadowBoxBorder );
			PositionsY.PushBack( BottomRight.z );
		}
		else
		{
			PositionsY.PushBack( TopLeft.z - m_ShadowBoxBorder );
			PositionsY.PushBack( TopLeft.z );
			PositionsY.PushBack( BottomRight.z );
			PositionsY.PushBack( BottomRight.z + m_ShadowBoxBorder );
		}

		for( uint Y = 0; Y < 4; ++Y )
		{
			for( uint X = 0; X < 4; ++X )
			{
				Positions.PushBack( Vector( PositionsX[ X ], 0.0f, PositionsY[ Y ] ) );
			}
		}
	}

	Array<Vector2> UVs;
	UVs.Reserve( kNumVertices );

	{
		Array<float> UVValues;
		UVValues.Reserve( 4 );

		UVValues.PushBack( 0.0f );
		UVValues.PushBack( 0.25f );
		UVValues.PushBack( 0.75f );
		UVValues.PushBack( 1.0f );

		for( uint V = 0; V < 4; ++V )
		{
			for( uint U = 0; U < 4; ++U )
			{
				UVs.PushBack( Vector2( UVValues[ U ], UVValues[ V ] ) );
			}
		}
	}

	Array<uint> Colors;
	Colors.Reserve( kNumVertices );

	{
		for( uint I = 0; I < kNumVertices; ++I )
		{
			Colors.PushBack( 0xffffffff );
		}
	}

	Array<index_t> Indices;
	Indices.Reserve( kNumIndices );

	for( index_t Y = 0; Y < 3; ++Y )
	{
		for( index_t X = 0; X < 3; ++X )
		{
			const index_t Base = X + Y * 4;
			Indices.PushBack( Base + 0 );
			Indices.PushBack( Base + 4 );
			Indices.PushBack( Base + 1 );
			Indices.PushBack( Base + 4 );
			Indices.PushBack( Base + 5 );
			Indices.PushBack( Base + 1 );
		}
	}

	IRenderer* const			pRenderer			= m_UIManager->GetRenderer();
	IVertexBuffer* const		VertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration* const	VertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS | VD_COLORS );
	IIndexBuffer* const			IndexBuffer			= pRenderer->CreateIndexBuffer();

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= kNumVertices;
	InitStruct.Positions	= Positions.GetData();
	InitStruct.UVs			= UVs.GetData();
	InitStruct.Colors		= Colors.GetData();

	VertexBuffer->Init( InitStruct );
	IndexBuffer->Init( kNumIndices, Indices.GetData() );
	IndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

	Mesh* const pMesh = new Mesh( VertexBuffer, VertexDeclaration, IndexBuffer );

	pMesh->SetAABB( AABB( TopLeft, BottomRight ) );

#if BUILD_DEBUG
	pMesh->m_DEBUG_Name = "UI Text Shadow Box";
#endif

	return pMesh;
}
