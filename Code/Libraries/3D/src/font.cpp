#include "core.h"
#include "font.h"
#include "mesh.h"
#include "irenderer.h"
#include "texturemanager.h"
#include "vector2.h"
#include "ivertexdeclaration.h"
#include "idatastream.h"
#include "mathcore.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "configmanager.h"
#include "reversehash.h"

#include <stdio.h>
#include <string.h>

Font::Font()
:	m_Renderer( NULL )
,	m_CapHeight( 0.0f )
,	m_Leading( 0.0f )
,	m_InternalLeading( 0.0f )
,	m_VerticalA( 0.0f )
,	m_VerticalC( 0.0f )
,	m_Height( 0.0f )
,	m_FontLocales()
,	m_FontLanguages()
{
}

Font::~Font()
{
}

void Font::Initialize( const IDataStream& Stream, IRenderer* const pRenderer )
{
	ASSERT( pRenderer );
	m_Renderer = pRenderer;
	TextureManager* const pTextureManager = pRenderer->GetTextureManager();

	m_CapHeight			= static_cast<float>( Stream.ReadInt32() );
	m_Leading			= static_cast<float>( Stream.ReadInt32() );
	m_InternalLeading	= static_cast<float>( Stream.ReadInt32() );
	m_VerticalA			= static_cast<float>( Stream.ReadInt32() );
	m_VerticalC			= static_cast<float>( Stream.ReadInt32() );
	m_Height			= static_cast<float>( Stream.ReadInt32() );

	const uint NumLocales = Stream.ReadUInt32();
	FOR_EACH_INDEX( LocaleIndex, NumLocales )
	{
		const HashedString LocaleTag = Stream.ReadHashedString();
		DEBUGASSERT( m_FontLocales.Search( LocaleTag ).IsNull() );
		SFontLocale& Locale = m_FontLocales.Insert( LocaleTag );

		const SimpleString ImageFilename = Stream.ReadString();
		Locale.m_Texture = pTextureManager->GetTextureNoMips( ImageFilename.CStr() );

		const uint NumLanguages = Stream.ReadUInt32();
		FOR_EACH_INDEX( LanguageIndex, NumLanguages )
		{
			const HashedString Language = Stream.ReadHashedString();
			DEBUGASSERT( m_FontLanguages.Search( Language ).IsNull() );
			m_FontLanguages.Insert( Language, LocaleTag );
		}

		Map<unicode_t, SFontCharProp>& RegularCharProps = Locale.m_StyleCharProps[ EFS_Regular ];
		const uint NumCodePoints = Stream.ReadUInt32();
		FOR_EACH_INDEX( CodePointIndex, NumCodePoints )
		{
			const unicode_t CodePoint = Stream.ReadUInt32();
			DEBUGASSERT( RegularCharProps.Search( CodePoint ).IsNull() );
			SFontCharProp& CharProp = RegularCharProps.Insert( CodePoint );

			Stream.Read( sizeof( SFontCharProp ), &CharProp );
		}

		Map<unicode_t, SFontCharProp>& BoldCharProps = Locale.m_StyleCharProps[ EFS_Bold ];
		const uint NumCodePointsBold = Stream.ReadUInt32();
		FOR_EACH_INDEX( CodePointIndex, NumCodePointsBold )
		{
			const unicode_t CodePoint = Stream.ReadUInt32();
			DEBUGASSERT( BoldCharProps.Search( CodePoint ).IsNull() );
			SFontCharProp& CharProp = BoldCharProps.Insert( CodePoint );

			Stream.Read( sizeof( SFontCharProp ), &CharProp );
		}

		Map<unicode_t, SFontCharProp>& ItalicCharProps = Locale.m_StyleCharProps[ EFS_Italic ];
		const uint NumCodePointsItalic = Stream.ReadUInt32();
		FOR_EACH_INDEX( CodePointIndex, NumCodePointsItalic )
		{
			const unicode_t CodePoint = Stream.ReadUInt32();
			DEBUGASSERT( ItalicCharProps.Search( CodePoint ).IsNull() );
			SFontCharProp& CharProp = ItalicCharProps.Insert( CodePoint );

			Stream.Read( sizeof( SFontCharProp ), &CharProp );
		}
	}
}

const SFontCharProp& Font::GetCharProp( const SFontLocale& Locale, const unicode_t CodePoint, const EFontStyle Style ) const
{
	const Map<unicode_t, SFontCharProp>&			UsingCharProps	= Locale.m_StyleCharProps[ Style ];
	const Map<unicode_t, SFontCharProp>::Iterator	PropIter		= UsingCharProps.Search( CodePoint );

	if( PropIter.IsNull() )
	{
#if BUILD_DEV
		Map<HashedString, SFontLocale>::Iterator LocaleIter = m_FontLocales.SearchValue( Locale );
		ASSERT( LocaleIter.IsValid() );
		const HashedString LocaleHash		= LocaleIter.GetKey();
		const SimpleString LocaleString		= ReverseHash::ReversedHash( LocaleHash );
		const SimpleString CodePointString	= SimpleString::GetCodePointString( CodePoint );
		PRINTF( "Code point %s not found in locale \"%s\".\n", CodePointString.CStr(), LocaleString.CStr() );
#endif

		// Return a ?
		if( CodePoint != 0x003F )
		{
			return GetCharProp( Locale, 0x003F, Style );
		}
		else
		{
			// Bail, return anything present
			return UsingCharProps.First().GetValue();
		}
	}

	DEVASSERT( PropIter.IsValid() );
	return PropIter.GetValue();
}

const SFontLocale& Font::GetCurrentLocale() const
{
	// HACKHACK: If we only have one locale, just use it and don't bother looking up the language.
	// (Adding this so that purely numeric fonts don't need to specify languages.)
	// NEONNOTE: This also trivially resolves the current case, where every font has one locale and
	// localization provides the mapping from the tag to the desired font.
	if( m_FontLocales.Size() == 1 )
	{
		return m_FontLocales.First().GetValue();
	}

	const HashedString Language = ConfigManager::GetLanguageHash();

	Map<HashedString, HashedString>::Iterator LanguageIter = m_FontLanguages.Search( Language );
	ASSERT( LanguageIter.IsValid() );
	const HashedString Locale = LanguageIter.GetValue();

	Map<HashedString, SFontLocale>::Iterator LocaleIter = m_FontLocales.Search( Locale );
	ASSERT( LocaleIter.IsValid() );
	return LocaleIter.GetValue();
}

Font::EWordResult Font::GetNextWord(
	const Array<unicode_t>& CodePoints,
	const SFontLocale& Locale,
	uint& Index,
	Array<unicode_t>& WordBuffer,
	float& WordWidth,
	Array<uint>& ColorBuffer,
	uint& CurrentColor,
	const uint DefaultColor,
	Array<EFontStyle>& StyleBuffer,
	EFontStyle& CurrentStyle
) const
{
	WordBuffer.Clear();
	WordWidth = 0.0f;
	ColorBuffer.Clear();
	StyleBuffer.Clear();

#define ADDCODEPOINT												\
	WordBuffer.PushBack( CodePoint );								\
	WordWidth += ( CharProp.m_A + CharProp.m_B + CharProp.m_C );	\
	ColorBuffer.PushBack( CurrentColor );							\
	StyleBuffer.PushBack( CurrentStyle );							\
	++Index

	const uint NumCodePoints = CodePoints.Size();
	for( ; Index < NumCodePoints; )
	{
		const unicode_t CodePoint = CodePoints[ Index ];

		// HACKHACK: also parse for color tags and bold/italic
		if( CodePoint == '#' && ( Index + 1 ) < CodePoints.Size() )
		{
			const unicode_t NextCodePoint = CodePoints[ Index + 1 ];
			if( NextCodePoint == '<' )
			{
				if( '>' == CodePoints[ Index + 2] )
				{
					// "#<>" resets to default color/style
					CurrentColor = DefaultColor;
					CurrentStyle = EFS_Regular;
					Index += 3;
					continue;
				}
				else
				{
					if( 'c' == CodePoints[ Index + 2 ] && '>' == CodePoints[ Index + 3 ] )
					{
						// "Clear" (to clear color without clearing bold/italic)
						CurrentColor = DefaultColor;
						Index += 4;
						continue;
					}
					else if( 'r' == CodePoints[ Index + 2 ] )
					{
						// Regular (to clear bold/italic without clearing color)
						CurrentStyle = EFS_Regular;
						Index += 4;
						continue;
					}
					else if( 'b' == CodePoints[ Index + 2 ] && '>' == CodePoints[ Index + 3 ] )
					{
						// Bold
						CurrentStyle = EFS_Bold;
						Index += 4;
						continue;
					}
					else if( 'i' == CodePoints[ Index + 2 ] )
					{
						// Italic
						CurrentStyle = EFS_Italic;
						Index += 4;
						continue;
					}
					else
					{
						// We're parsing a color, with a very rigid format, e.g. "#<ff8000>"
						CurrentColor = 0;
						for( uint ColorIndex = Index + 2; ColorIndex < Index + 8; ++ColorIndex )
						{
							CurrentColor <<= 4;

							const unicode_t ColorCodePoint = CodePoints[ ColorIndex ];
							if( ColorCodePoint >= '0' && ColorCodePoint <= '9' )
							{
								CurrentColor |= ColorCodePoint - '0';
							}
							else if( ColorCodePoint >= 'a' && ColorCodePoint <= 'f' )
							{
								CurrentColor |= 10 + ColorCodePoint - 'a';
							}
							else if( ColorCodePoint >= 'A' && ColorCodePoint <= 'F' )
							{
								CurrentColor |= 10 + ColorCodePoint - 'F';
							}
						}
						CurrentColor |= 0xff000000; // Always use full alpha
						Index += 9;
						continue;
					}
				}
			}
		}

		if( CodePoint == '\n' )
		{
			if( WordBuffer.Empty() )
			{
				++Index;
				return EWR_Linebreak;
			}
			else
			{
				return EWR_Word;
			}
		}
		else if( CodePoint == ' ' || CodePoint == '\t' )	// Just treat Tab as a whitespace character
		{
			if( WordBuffer.Empty() )
			{
				const SFontCharProp& CharProp = GetCharProp( Locale, CodePoint, CurrentStyle );
				ADDCODEPOINT;
			}
			return EWR_Word;
		}
		else if( CodePoint == '-' || CodePoint == 0x2014 )	// U+2014 == em dash
		{
			// Add this breaking character to the current word, then return.
			const SFontCharProp& CharProp = GetCharProp( Locale, CodePoint, CurrentStyle );
			ADDCODEPOINT;
			return EWR_Word;
		}
		else
		{
			const SFontCharProp& CharProp = GetCharProp( Locale, CodePoint, CurrentStyle );
			ADDCODEPOINT;
		}
	}

#undef ADDCODEPOINT

	return WordBuffer.Empty() ? EWR_None : EWR_Word;
}

// TODO LOC LATER: Add support for center/right alignment. This could be done by adjusting
// typeset positions after each line is closed (or as a post process after all typesetting).
// I wouldn't be too worried about performance on that, since I almost never use it. (Using
// a center or right origin on the UI widget is sufficient for single-line elements.)
void Font::Arrange( const Array<unicode_t>& CodePoints, const SRect& Bounds, const uint Flags, Array<STypesetGlyph>& OutTypesetting, Vector2& OutDimensions, const uint DefaultColor ) const
{
	// We know we can't exceed the number of code points, at least.
	OutTypesetting.Reserve( CodePoints.Size() );

	// Cache the locale so we don't do multiple lookups while typesetting.
	const SFontLocale& Locale = GetCurrentLocale();

	// Initialize typeset cursor; type will later be adjusted so (0,0)
	// is relative to ( Bounds.m_Left - 1.0f, Bounds.m_Top - 1.0f ).
	Vector2 Cursor;

	const float	BoundWidth		= Bounds.m_Right - Bounds.m_Left;
	const bool	Bounded			= BoundWidth > 0.0f;
	const float	PositionAdjust	= 0.5f;	// Subtract 1/2 pixel from position for padding in font sheet

	// Initialize temporary word buffer
	Array<unicode_t> WordBuffer;
	WordBuffer.SetDeflate( false );
	Array<uint> ColorBuffer;
	ColorBuffer.SetDeflate( false );
	Array<EFontStyle> StyleBuffer;
	StyleBuffer.SetDeflate( false );
	float	WordWidth	= 0.0f;
	uint	Index		= 0;

	// Array of line widths, so I can center- and right-align after setting
	Array<float>	LineWidths;
	Array<uint>		LineSpaces;
	Array<bool>		LineParagraphs;	// Which lines are paragraph ends, for justified text
	LineWidths.PushBack( 0.0f );
	LineSpaces.PushBack( 0 );
	LineParagraphs.PushBack( false );

	EWordResult WordResult = EWR_None;
	uint CurrentColor = DefaultColor;
	EFontStyle CurrentStyle = EFS_Regular;
	while( EWR_None != ( WordResult = GetNextWord( CodePoints, Locale, Index, WordBuffer, WordWidth, ColorBuffer, CurrentColor, DefaultColor, StyleBuffer, CurrentStyle ) ) )
	{
		DEBUGASSERT( WordBuffer.Size() == ColorBuffer.Size() );
		DEBUGASSERT( WordBuffer.Size() == StyleBuffer.Size() );

		if( WordResult == EWR_Linebreak )
		{
			LineParagraphs.Last() = true;

			LineWidths.PushBack( 0.0f );
			LineSpaces.PushBack( 0 );
			LineParagraphs.PushBack( false );

			// Advance cursor to new line but don't draw anything
			Cursor.x = 0.0f;
			Cursor.y += m_Leading;
		}
		else
		{
			ASSERT( WordResult == EWR_Word );

			// Advance cursor to new line if word won't fit.
			if( Bounded &&							// Are we doing a bounded print?
				Cursor.x > 0.0f &&					// Is this not the first word on the line?
				Cursor.x + WordWidth > BoundWidth )	// Does the word not fit within bounds?
			{
				LineWidths.PushBack( 0.0f );
				LineSpaces.PushBack( 0 );
				LineParagraphs.PushBack( false );

				Cursor.x = 0.0f;
				Cursor.y += m_Leading;

				// Special case: If the next word is a single space, drop it.
				// We don't want a leading space on the line in this case.
				if( WordBuffer.Size() == 1 && WordBuffer[0] == ' ' )
				{
					continue;
				}

				// HACKHACK: If the previous character was a space, drop it.
				// We don't want a trailing space in center- or right-aligned layouts.
				if( OutTypesetting.Size() > 0 &&
					OutTypesetting.Last().m_CodePoint == ' ' )
				{
					const EFontStyle SpaceStyle = OutTypesetting.Last().m_Style;
					OutTypesetting.PopBack();
					LineSpaces[ LineSpaces.Size() - 2 ]--;

					// And subtract the space from the line width
					const SFontCharProp& SpaceProp = GetCharProp( Locale, ' ', SpaceStyle );
					float& PrevLineWidth = LineWidths[ LineWidths.Size() - 2 ];
					PrevLineWidth -= Max( 0, SpaceProp.m_A );
					PrevLineWidth -= SpaceProp.m_B;
					PrevLineWidth -= SpaceProp.m_C;
				}
			}

			if( WordBuffer.Size() == 1 && WordBuffer[0] == ' ' )
			{
				LineSpaces.Last()++;
			}

			// Add chars to typesetting array and advance cursor
			FOR_EACH_ARRAY( WordIter, WordBuffer, unicode_t )
			{
				const unicode_t			CodePoint	= WordIter.GetValue();
				const EFontStyle		WordStyle	= StyleBuffer[ WordIter.GetIndex() ];
				const SFontCharProp&	CharProp	= GetCharProp( Locale, CodePoint, WordStyle );

				// DLP 9 May 2019/FONTTODO: Why do we not also advance the leading underhang here? It seems wrong, Worth investigating.
				Cursor.x += Min( 0, CharProp.m_A );	// Retreat cursor if there is leading overhang (because the font sheet has the negative A width added)

				float& LineWidth	= LineWidths.Last();
				LineWidth			= Max( LineWidth, Cursor.x + CharProp.m_Width );
				OutDimensions.x		= Max( OutDimensions.x, LineWidth );
				OutDimensions.y		= Max( OutDimensions.y, Cursor.y + m_Height );

				STypesetGlyph& TypesetGlyph	= OutTypesetting.PushBack();
				TypesetGlyph.m_CodePoint	= CodePoint;
				TypesetGlyph.m_Position.x	= Bounds.m_Left + Cursor.x - PositionAdjust;
				TypesetGlyph.m_Position.y	= Bounds.m_Top + Cursor.y - PositionAdjust;
				TypesetGlyph.m_LineNumber	= LineWidths.Size() - 1;
				TypesetGlyph.m_Color		= ColorBuffer[ WordIter.GetIndex() ];
				TypesetGlyph.m_Style		= StyleBuffer[ WordIter.GetIndex() ];

				// DLP 9 May 2019/FONTTODO: Likewise, why add this after? Doesn't make sense with what FontGenerator is doing.
				Cursor.x += Max( 0, CharProp.m_A );	// Advance cursor if there is leading padding
				Cursor.x += CharProp.m_B;
				Cursor.x += CharProp.m_C;
			}
		}
	}

	// Final line is always a paragraph end.
	LineParagraphs.Last() = true;

	const bool	AlignCenter		= ( Flags & FONT_PRINT_CENTER ) != 0;
	const bool	AlignRight		= ( Flags & FONT_PRINT_RIGHT ) != 0;
	const bool	AlignJustify	= ( Flags & FONT_PRINT_JUSTIFY ) != 0;
	const float	MaxLineWidth	= Bounded ? BoundWidth : OutDimensions.x;

	if( AlignCenter || AlignRight )
	{
		FOR_EACH_ARRAY( GlyphIter, OutTypesetting, STypesetGlyph )
		{
			STypesetGlyph&	TypesetGlyph	= GlyphIter.GetValue();
			const float		LineWidth		= LineWidths[ TypesetGlyph.m_LineNumber ];
			const float		LineDifference	= MaxLineWidth - LineWidth;
			DEVASSERT( LineDifference >= 0.0f );
			const float		Adjustment		= AlignRight ? LineDifference : LineDifference * 0.5f;
			TypesetGlyph.m_Position.x		+= Adjustment;
		}
	}
	else if( AlignJustify )
	{
		uint CurrentLine	= 0;
		uint SpacesPassed	= 0;

		FOR_EACH_ARRAY( GlyphIter, OutTypesetting, STypesetGlyph )
		{
			STypesetGlyph&	TypesetGlyph	= GlyphIter.GetValue();

			if( CurrentLine != TypesetGlyph.m_LineNumber )
			{
				CurrentLine		= TypesetGlyph.m_LineNumber;
				SpacesPassed	= 0;
			}

			if( TypesetGlyph.m_CodePoint == ' ' )
			{
				SpacesPassed++;
			}

			const bool		IsParagraphEnd	= LineParagraphs[ TypesetGlyph.m_LineNumber ];
			const float		LineWidth		= LineWidths[ TypesetGlyph.m_LineNumber ];
			const uint		LineSpace		= LineSpaces[ TypesetGlyph.m_LineNumber ];
			const float		LineDifference	= MaxLineWidth - LineWidth;
			const float		SpacePadding	= ( LineSpace > 0 ) ? ( LineDifference / static_cast<float>( LineSpace ) ) : LineDifference;
			const float		Adjustment		= IsParagraphEnd ? 0.0f : ( SpacesPassed * SpacePadding );
			TypesetGlyph.m_Position.x		+= Adjustment;
		}
	}
}

void Font::Arrange( const SimpleString& StringUTF8, const SRect& Bounds, const uint Flags, Array<STypesetGlyph>& OutTypesetting, Vector2& OutDimensions, const uint DefaultColor ) const
{
	Array<unicode_t> CodePoints;
	StringUTF8.UTF8ToUnicode( CodePoints );
	Arrange( CodePoints, Bounds, Flags, OutTypesetting, OutDimensions, DefaultColor );
}

Mesh* Font::Print( const SimpleString& StringUTF8, const SRect& Bounds, const uint Flags, const uint DefaultColor ) const
{
	Array<unicode_t> CodePoints;
	StringUTF8.UTF8ToUnicode( CodePoints );
	return Print( CodePoints, Bounds, Flags, DefaultColor );
}

Mesh* Font::Print( const Array<unicode_t>& CodePoints, const SRect& Bounds, const uint Flags, const uint DefaultColor ) const
{
	// Cache the locale so we don't do multiple lookups while typesetting.
	const SFontLocale& Locale = GetCurrentLocale();

	Array<STypesetGlyph> TypesetGlyphs;
	Vector2 Dimensions;
	Arrange( CodePoints, Bounds, Flags, TypesetGlyphs, Dimensions, DefaultColor );

	const uint NumVertices	= TypesetGlyphs.Size() * 4;
	const uint NumIndices	= TypesetGlyphs.Size() * 6;

	Array<Vector> Positions;
	Positions.Reserve( NumVertices );

	Array<Vector2> UVs;
	UVs.Reserve( NumVertices );

	Array<uint> Colors;
	Colors.Reserve( NumVertices );

	Array<index_t> Indices;
	Indices.Reserve( NumIndices );

	FOR_EACH_ARRAY( GlyphIter, TypesetGlyphs, STypesetGlyph )
	{
		const STypesetGlyph& Glyph = GlyphIter.GetValue();

		const SFontCharProp& CharProp = GetCharProp( Locale, Glyph.m_CodePoint, Glyph.m_Style );

		Positions.PushBack( Vector( Glyph.m_Position.x,						0.0f, Glyph.m_Position.y + m_Height ) );
		Positions.PushBack( Vector( Glyph.m_Position.x + CharProp.m_Width,	0.0f, Glyph.m_Position.y + m_Height ) );
		Positions.PushBack( Vector( Glyph.m_Position.x,						0.0f, Glyph.m_Position.y ) );
		Positions.PushBack( Vector( Glyph.m_Position.x + CharProp.m_Width,	0.0f, Glyph.m_Position.y ) );

		UVs.PushBack( Vector2( CharProp.m_U1, CharProp.m_V2 ) );
		UVs.PushBack( Vector2( CharProp.m_U2, CharProp.m_V2 ) );
		UVs.PushBack( Vector2( CharProp.m_U1, CharProp.m_V1 ) );
		UVs.PushBack( Vector2( CharProp.m_U2, CharProp.m_V1 ) );

		Colors.PushBack( Glyph.m_Color );
		Colors.PushBack( Glyph.m_Color );
		Colors.PushBack( Glyph.m_Color );
		Colors.PushBack( Glyph.m_Color );

		// Ordered:
		// 2 3
		// 0 1
		const index_t BaseIndex = static_cast<index_t>( Positions.Size() - 4 );
		Indices.PushBack( BaseIndex + 0 );
		Indices.PushBack( BaseIndex + 1 );
		Indices.PushBack( BaseIndex + 2 );
		Indices.PushBack( BaseIndex + 1 );
		Indices.PushBack( BaseIndex + 3 );
		Indices.PushBack( BaseIndex + 2 );
	}

	IVertexBuffer*		pVertexBuffer		= m_Renderer->CreateVertexBuffer();
	IVertexDeclaration*	pVertexDeclaration	= m_Renderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS | VD_COLORS );
	IIndexBuffer*		pIndexBuffer		= m_Renderer->CreateIndexBuffer();
	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= NumVertices;
	InitStruct.Positions	= Positions.GetData();
	InitStruct.UVs			= UVs.GetData();
	InitStruct.Colors		= Colors.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( NumIndices, Indices.GetData() );
	pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

	Mesh* StringMesh		= new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	AABB MeshBounds;
	MeshBounds.m_Min.x = Bounds.m_Left - 1.0f;
	MeshBounds.m_Min.z = Bounds.m_Top - 1.0f;
	MeshBounds.m_Max.x = MeshBounds.m_Min.x + Dimensions.x;
	MeshBounds.m_Max.z = MeshBounds.m_Min.z + Dimensions.y;

	StringMesh->SetTexture( 0, Locale.m_Texture );
	StringMesh->SetAABB( MeshBounds );
#if BUILD_DEBUG
	StringMesh->m_DEBUG_Name	= "String";
#endif

	return StringMesh;
}
