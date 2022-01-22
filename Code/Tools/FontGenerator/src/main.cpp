#include "core.h"
#include "console.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "windowwrapper.h"
#include "keyboard.h"
#include "configmanager.h"
#include "filestream.h"
#include "array.h"
#include "simplestring.h"
#include "filestream.h"
#include "surface.h"
#include "fileutil.h"

#include <Windows.h>
#include <crtdbg.h>
#include <stdio.h>
#include <memory.h>

// Defines properties about a particular glyph or code point in a particular locale in a font.
struct SFGCodePointProps
{
	SFGCodePointProps()
	:	m_CodePoint( 0 )
	,	m_U1( 0.0f )
	,	m_U2( 0.0f )
	,	m_V1( 0.0f )
	,	m_V2( 0.0f )
	,	m_A( 0 )
	,	m_B( 0 )
	,	m_C( 0 )
	,	m_Width( 0.0f )
	{
	}

	unicode_t	m_CodePoint;
	float		m_U1;
	float		m_U2;
	float		m_V1;
	float		m_V2;
	int			m_A;
	uint		m_B;
	int			m_C;
	float		m_Width;
};

void ComputeWidth( SFGCodePointProps& CodePointProps )
{
	uint Width = 1 + Max( 0, CodePointProps.m_A ) + CodePointProps.m_B + Max( 0, CodePointProps.m_C );
	CodePointProps.m_Width = static_cast<float>( Width );
}

// Defines properties about a substitution for a glyph.
// Currently only supports image substitutions.
struct SGlyphSubstitute
{
	enum ESubType
	{
		EST_None,
		EST_Icon,
	};

	SGlyphSubstitute()
	:	m_SubType( EST_None )
	,	m_CodePoint( 0 )
	,	m_IconName()
	,	m_SpacingLeft()
	,	m_SpacingRight()
	,	m_HeightOffset( 0 )
	,	m_PositionAtBaseline( false )
	{
	}

	ESubType		m_SubType;
	unicode_t		m_CodePoint;
	SimpleString	m_IconName;
	int				m_SpacingLeft;
	int				m_SpacingRight;
	int				m_HeightOffset;
	bool			m_PositionAtBaseline;	// Legacy for Eldritch image glyph replacements; if false, center image in height range
};

// Defines properties about a specific instantiated substitution.
struct SBitmapSubstitute
{
	SBitmapSubstitute()
	:	m_X( 0 )
	,	m_Y( 0 )
	,	m_Width( 0 )
	,	m_Height( 0 )
	,	m_Pixels( NULL )
	,	m_HeightOffset( 0 )
	,	m_PositionAtBaseline( false )
	{
	}

	int		m_X;
	int		m_Y;
	int		m_Width;
	int		m_Height;
	byte*	m_Pixels;
	int		m_HeightOffset;
	bool	m_PositionAtBaseline;
};

// Defines properties about a particular locale for the font (e.g. Latin alphabet).
struct SFGLocaleProps
{
	SFGLocaleProps()
	:	m_TextureWidth( 0 )
	,	m_TextureHeight( 0 )
	,	m_BitmapFont( false )
	,	m_BitmapTextureName()
	,	m_BitmapabcA( 0 )
	,	m_BitmapabcB( 0 )
	,	m_BitmapabcC( 0 )
	,	m_LocaleTag()
	,	m_LocaleContext()
	,	m_ImageFilename()
	,	m_RuntimeImageFilename()
	,	m_Languages()
	,	m_CodePointProps()
	,	m_CodePointPropsBold()
	,	m_CodePointPropsItalic()
	,	m_BitmapSubstitutes()
	{
	}

	int							m_TextureWidth;
	int							m_TextureHeight;
	bool						m_BitmapFont;			// If true, we're using provided image(s) instead of drawing a new font sheet. (HACK for Eldritch, remove later.)
	SimpleString				m_BitmapTextureName;	// For bitmap fonts only, passed through without wrangling
	int							m_BitmapabcA;			// For bitmap fonts only.
	int							m_BitmapabcB;			// For bitmap fonts only.
	int							m_BitmapabcC;			// For bitmap fonts only.
	SimpleString				m_LocaleTag;
	SimpleString				m_LocaleContext;		// Config context
	SimpleString				m_ImageFilename;
	SimpleString				m_RuntimeImageFilename;	// Same as m_ImageFilename with leading ../Intermediate/ trimmed, for use as a relative path at runtime.
	Array<SimpleString>			m_Languages;			// NOTE: Deprecated, we look up via localized string now
	Array<SFGCodePointProps>	m_CodePointProps;		// Populated in DrawFont/SaveBitmapFont, of course. >_<
	Array<SFGCodePointProps>	m_CodePointPropsBold;
	Array<SFGCodePointProps>	m_CodePointPropsItalic;
	Array<SBitmapSubstitute>	m_BitmapSubstitutes;	// Locale-specific; can be in different places on different texture sheets.
};

// Defines base properties about a font. This is the root global data.
struct SFGProps
{
	SFGProps()
	:	m_OutputFontFilename()
	,	m_OutputImageTemplate()
	,	m_PointSize( 0 )
	,	m_Weight( 0 )
	,	m_Italics( false )
	,	m_Antialias( false )
	,	m_NoCompress( false )
	,	m_Face()
	,	m_BoldFace()
	,	m_ItalicFace()
	,	m_CapHeight( 0 )
	,	m_Leading( 0 )
	,	m_InternalLeading( 0 )
	,	m_VerticalA( 0 )
	,	m_VerticalC( 0 )
	,	m_Height( 0 )
	,	m_LocaleProps()
	,	m_GlyphSubstitutes()
	,	m_AlphaScalar( 0.0f )
	,	m_AlphaExponent( 0.0f )
	{
	}

	SimpleString			m_OutputFontFilename;
	SimpleString			m_OutputImageTemplate;
	int						m_PointSize;
	int						m_Weight;
	bool					m_Italics;
	bool					m_Antialias;
	bool					m_NoCompress;
	SimpleString			m_Face;
	SimpleString			m_BoldFace;
	SimpleString			m_ItalicFace;
	int						m_CapHeight;			// New member that represents the height of a typical capital letter, better reflects the visible size of the font than m_Leading does
	int						m_Leading;				// Previously called "height", this is the font metric tmHeight + tmExternalLeading, not strictly the spacing on the font sheet
	int						m_InternalLeading;
	int						m_VerticalA;			// Equivalent to the width A (of ABC), used when Leading and Height are different
	int						m_VerticalC;			// Equivalent to the width C (of ABC), used when Leading and Height are different
	int						m_Height;				// This is the spacing on the font sheet which may be larger than the leading
	Array<SFGLocaleProps>	m_LocaleProps;			// NOTE: A font file only contains one locale anymore, to make the system more expansible.
	Array<SGlyphSubstitute>	m_GlyphSubstitutes;		// Not locale-specific. They should generally live in the private use area (U+E000–U+F8FF).

	// For anti-aliased fonts
	float					m_AlphaScalar;
	float					m_AlphaExponent;
};

struct STGAHeader
{
	byte		m_SizeOfIDField;
	byte		m_ColorMapType;
	byte		m_ImageType;		// I'll be concerned with 2 and 10 only

	// Broken up into bytes because of word alignment issues
	// TODO LOC LATER: I could also #pragma pack(push, 2) like I do in bmp-format.h
	byte		m_ColorMapOriginLo;
	byte		m_ColorMapOriginHi;
	byte		m_ColorMapLengthLo;
	byte		m_ColorMapLengthHi;
	byte		m_ColorMapBitDepth;

	c_uint16	m_OriginX;
	c_uint16	m_OriginY;
	c_uint16	m_Width;
	c_uint16	m_Height;
	byte		m_BitDepth;
	byte		m_ImageDescriptor;	// See TGA documentation
};

// Create an in-memory surface from a bitmap. Only supports 32-bit BMP files.
// NOTE: I didn't just use a Surface for this, because Surfaces are explicitly 24-bit, sigh.
void CreateBitmapSubstitute( const SimpleString& Filename, SBitmapSubstitute& OutSub )
{
	SimpleString FixedFilename = Filename;
	if( !FixedFilename.Contains( "../Raw/" ) )
	{
		FixedFilename = SimpleString::PrintF( "../Raw/%s", Filename.CStr() );
	}

	if( FixedFilename.Contains( ".bmp" ) )
	{
		// Legacy support
		{
			FileStream Stream( FixedFilename.CStr(), FileStream::EFM_Read );

			BITMAPFILEHEADER BMPHeader;
			BITMAPINFOHEADER BMPInfo;

			Stream.Read( sizeof( BITMAPFILEHEADER ), &BMPHeader );
			Stream.Read( sizeof( BITMAPINFOHEADER ), &BMPInfo );

			OutSub.m_Width	= BMPInfo.biWidth;
			OutSub.m_Height	= BMPInfo.biHeight;
			const uint Size	= OutSub.m_Width * OutSub.m_Height * 4;
			OutSub.m_Pixels	= new byte[ Size ];	// TODO LOC LATER: Ever free this memory. Or not, it doesn't really matter.

			Stream.Read( Size, OutSub.m_Pixels );
		}

		// HACKHACK: Emit a .tga artifact to patch up legacy assets
		{
			FileStream Stream( FixedFilename.Replace( ".bmp", ".tga" ).CStr(), FileStream::EFM_Write );

			STGAHeader TGAHeader;

			ZeroMemory( &TGAHeader, sizeof( STGAHeader ) );
			TGAHeader.m_ImageType		= 2;
			TGAHeader.m_Width			= static_cast<c_uint16>( OutSub.m_Width );
			TGAHeader.m_Height			= static_cast<c_uint16>( OutSub.m_Height );
			TGAHeader.m_BitDepth		= 32;
			TGAHeader.m_ImageDescriptor	= 8;

			Stream.Write( sizeof( TGAHeader ), &TGAHeader );

			const uint Size	= OutSub.m_Width * OutSub.m_Height * 4;
			Stream.Write( Size, OutSub.m_Pixels );
		}
	}
	else
	{
		// New TGA support
		FileStream Stream( FixedFilename.CStr(), FileStream::EFM_Read );

		STGAHeader TGAHeader;

		Stream.Read( sizeof( STGAHeader ), &TGAHeader );

		ASSERTDESC( 0 == ( TGAHeader.m_ImageDescriptor & 0x20 ), "TGAs are expected to have origin in lower-left." );
		ASSERTDESC( TGAHeader.m_BitDepth == 32, "Expected a 32-bit TGA." );

		OutSub.m_Width	= TGAHeader.m_Width;
		OutSub.m_Height	= TGAHeader.m_Height;
		const uint Size	= OutSub.m_Width * OutSub.m_Height * 4;
		OutSub.m_Pixels	= new byte[ Size ];	// TODO LOC LATER: Ever free this memory. Or not, it doesn't really matter.

		if( 2 == TGAHeader.m_ImageType )
		{
			// Uncompressed true color
			Stream.Read( Size, OutSub.m_Pixels );
		}
		else if( 10 == TGAHeader.m_ImageType )
		{
			// RLE true color (copied from TextureCommon::StaticLoadTGA, could consolidate)
			const uint	NumPixels	= TGAHeader.m_Width * TGAHeader.m_Height;
			uint		ReadPixels	= 0;
			byte*		pDestPixels	= OutSub.m_Pixels;

			while( ReadPixels < NumPixels )
			{
				byte PacketHeader	= Stream.ReadUInt8();
				uint PacketSize		= static_cast<uint>( PacketHeader & 0x7f ) + 1;

				if( PacketHeader & 0x80 )	// RLE packet
				{
					uint RLEValue = Stream.ReadUInt32();
					const byte* const pEndDestPixels = pDestPixels + PacketSize * 4;
					for( ; pDestPixels < pEndDestPixels; pDestPixels += 4 )
					{
						uint* const pDestPixels32 = reinterpret_cast<uint*>( pDestPixels );
						*pDestPixels32 = RLEValue;
					}
				}
				else						// Raw packet
				{
					Stream.Read( PacketSize * 4, pDestPixels );
					pDestPixels += PacketSize * 4;
				}

				ReadPixels += PacketSize;
			}
		}
		else
		{
			WARNDESC( "Unsupported TGA image type in FontGenerator." );
		}
	}
}

void DrawCodePoints( const Array<SFGCodePointProps>& CodePointPropsArray, SFGLocaleProps& LocaleProps, const SFGProps& Props, Surface& LocaleSurface, RECT& DrawRect, const int BaseLine, const bool IncludeGlyphSubstitutes )
{
	const uint Width	= LocaleSurface.GetWidth();
	const uint Height	= LocaleSurface.GetHeight();

	const float RecW = 1.0f / static_cast<float>( Width );
	const float RecH = 1.0f / static_cast<float>( Height );

	const HDC SurfaceHDC = LocaleSurface.GetHDC();

	// Declare temp variables used in the drawing loop below.
	wchar_t WideCharBuffer[2];
	WideCharBuffer[1] = L'\0';

	ABC FontWidths = { 0 };
	int FontWidth;

	int Left;

	FOR_EACH_ARRAY( CodePointPropsIter, CodePointPropsArray, SFGCodePointProps )
	{
		SFGCodePointProps&	CodePointProps	= CodePointPropsIter.GetValue();
		const unicode_t		CodePoint		= CodePointProps.m_CodePoint;

		// Look for a substitute glyph
		SGlyphSubstitute::ESubType CurrentSubType = SGlyphSubstitute::EST_None;
		FOR_EACH_ARRAY( GlyphSubIter, Props.m_GlyphSubstitutes, SGlyphSubstitute )
		{
			SGlyphSubstitute& GlyphSub = GlyphSubIter.GetValue();
			if( GlyphSub.m_CodePoint == CodePoint )
			{
				CurrentSubType = GlyphSub.m_SubType;
				ASSERT( CurrentSubType == SGlyphSubstitute::EST_Icon );

				if( IncludeGlyphSubstitutes )
				{
					SBitmapSubstitute& BitmapSub = LocaleProps.m_BitmapSubstitutes.PushBack();
					CreateBitmapSubstitute( GlyphSub.m_IconName, BitmapSub );

					FontWidths.abcA = GlyphSub.m_SpacingLeft;
					FontWidths.abcB = BitmapSub.m_Width;
					FontWidths.abcC = GlyphSub.m_SpacingRight;

					BitmapSub.m_PositionAtBaseline	= GlyphSub.m_PositionAtBaseline;
					BitmapSub.m_HeightOffset		= GlyphSub.m_HeightOffset;
				}

				break;
			}
		}

		if( !IncludeGlyphSubstitutes && CurrentSubType == SGlyphSubstitute::EST_Icon )
		{
			// Fallback to the default substitute and don't change layout
			CodePointProps = LocaleProps.m_CodePointProps[ CodePointPropsIter.GetIndex() ];
			continue;
		}

		if( CurrentSubType == SGlyphSubstitute::EST_None )
		{
			// For now, assume I'll never need code points beyond the range U+FFFF, and just cast the code point to a wide char.
			ASSERT( CodePoint <= 0xffff );
			WideCharBuffer[0] = static_cast<wchar_t>( CodePoint & 0xffff );
			GetCharABCWidthsW( SurfaceHDC, CodePoint, CodePoint, &FontWidths );
		}

		// Add 1 pixel at the edges and between each glyph to prevent any kind of bleeding
		FontWidth = 1 + FontWidths.abcB;
		FontWidth += Max( 0, FontWidths.abcA );
		FontWidth += Max( 0, FontWidths.abcC );

		Left = DrawRect.left;

		// Accomodate leading overhang
		if( FontWidths.abcA < 0 )
		{
			DrawRect.left -= FontWidths.abcA;
		}

		// Drop to the next line if there's not enough room
		if( DrawRect.left + FontWidth >= DrawRect.right )
		{
			DrawRect.left = 1 - Min( 0, FontWidths.abcA ); // Re-accomodate leading overhang
			DrawRect.top += Props.m_Height + 1;
			Left = 1;
		}

		// Seems like I need this to ensure proper padding, dunno why, but okay.
		if( CurrentSubType == SGlyphSubstitute::EST_None && Props.m_Antialias )
		{
			DrawRect.left += 1;
			Left += 1;
		}

		ASSERTDESC( DrawRect.top + Props.m_Height + 1 < DrawRect.bottom, "Texture is not large enough to contain all specified glyphs." );

		if( CurrentSubType == SGlyphSubstitute::EST_None )
		{
			DrawTextW( SurfaceHDC, WideCharBuffer, 1, &DrawRect, DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_TOP | DT_NOPREFIX );
		}
		else if( CurrentSubType == SGlyphSubstitute::EST_Icon )
		{
			// Don't actually draw the icon now. Just mark its spot in the surface, and copy it in later when exporting the TGA.
			ASSERT( IncludeGlyphSubstitutes );
			ASSERT( LocaleProps.m_BitmapSubstitutes.Size() );
			SBitmapSubstitute& BitmapSub = LocaleProps.m_BitmapSubstitutes.Last();
			BitmapSub.m_X = DrawRect.left + FontWidths.abcA;
			if( BitmapSub.m_PositionAtBaseline )
			{
				BitmapSub.m_Y = ( DrawRect.top + BaseLine ) - BitmapSub.m_Height + BitmapSub.m_HeightOffset;
			}
			else
			{
				BitmapSub.m_Y = ( DrawRect.top - Props.m_VerticalA ) + ( ( Props.m_Height - BitmapSub.m_Height ) / 2 ) + BitmapSub.m_HeightOffset;
			}
		}

		CodePointProps.m_U1	= ( static_cast<float>( Left ) - 0.5f ) * RecW;
		CodePointProps.m_U2	= ( static_cast<float>( Left + FontWidth ) - 0.5f ) * RecW;
		// Using Direct3D-style (top-to-bottom) UVs
		CodePointProps.m_V1	= ( static_cast<float>( DrawRect.top - Props.m_VerticalA ) - 0.5f ) * RecH;
		CodePointProps.m_V2	= ( static_cast<float>( ( DrawRect.top - Props.m_VerticalA ) + Props.m_Height ) + 0.5f ) * RecH;
		CodePointProps.m_A	= FontWidths.abcA;
		CodePointProps.m_B	= FontWidths.abcB;
		CodePointProps.m_C	= FontWidths.abcC;
		ComputeWidth( CodePointProps );

		DrawRect.left += FontWidth;

		// Accomodate leading overhang again.
		// (This is because we've advanced DrawRect by an amount that ignored the negative part, so we've overshot the next mark.)
		if( FontWidths.abcA < 0 )
		{
			DrawRect.left += FontWidths.abcA;
		}

		// Seems like I need this to ensure proper padding, dunno why, but okay.
		// Yes, this means we add a buffer both before and after the glyph.
		if( CurrentSubType == SGlyphSubstitute::EST_None && Props.m_Antialias )
		{
			DrawRect.left += 1;
		}
	}
}

// Big huge side effect: initializes the code point properties, because
// we need to actually have the font loaded to get those values.
void DrawFont( Surface& LocaleSurface, SFGLocaleProps& LocaleProps, SFGProps& Props )
{
	const HDC SurfaceHDC = LocaleSurface.GetHDC();
	const int PixelsY	= GetDeviceCaps( SurfaceHDC, LOGPIXELSY );

	HFONT pMainFont		= NULL;
	HFONT pBoldFont		= NULL;
	HFONT pItalicFont	= NULL;

	LOGFONT LogFont;
	ZeroMemory( &LogFont, sizeof( LOGFONT ) );

	// Convert size to logical units: multiply point size by pixels per inch (PixelsY) and divide by points per inch (72)
	// Negative means we want a font with this character height (not cell height, i.e., ignore the leading).
	// For a pixels per inch of 96, the character height (TextMetric.tmHeight - TextMetric.tmInternalLeading) we get back
	// should be 33% bigger than the point size requested (e.g. 43 for a 32 font). (NOTE: Character height is *not* em
	// height, it includes descent below base line.)
	LogFont.lfHeight			= -(int)( 0.5f + static_cast<float>( Props.m_PointSize * PixelsY ) / 72.0f );
	LogFont.lfWidth				= 0;
	LogFont.lfEscapement		= 0;
	LogFont.lfOrientation		= 0;
	LogFont.lfWeight			= Props.m_Weight;
	LogFont.lfItalic			= Props.m_Italics;
	LogFont.lfUnderline			= 0;
	LogFont.lfStrikeOut			= 0;
	LogFont.lfCharSet			= 0;
	LogFont.lfOutPrecision		= OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision		= CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality			= Props.m_Antialias ? PROOF_QUALITY : NONANTIALIASED_QUALITY;	// ROSANOTE: Switch to PROOF_QUALITY from ANTIALIASED_QUALITY, gives better results on fonts I use now
	LogFont.lfPitchAndFamily	= FF_DONTCARE | DEFAULT_PITCH;

	strcpy_s( LogFont.lfFaceName, LF_FACESIZE, Props.m_Face.CStr() );
	pMainFont = CreateFontIndirect( &LogFont );	// Should really DeleteObject() the returned object...?

	if( Props.m_BoldFace != "" )
	{
		LogFont.lfWeight = FW_BOLD;
		strcpy_s( LogFont.lfFaceName, LF_FACESIZE, Props.m_BoldFace.CStr() );
		pBoldFont = CreateFontIndirect( &LogFont );
	}

	if( Props.m_ItalicFace != "" )
	{
		LogFont.lfWeight = FW_REGULAR;
		LogFont.lfItalic = true;
		strcpy_s( LogFont.lfFaceName, LF_FACESIZE, Props.m_ItalicFace.CStr() );
		pItalicFont = CreateFontIndirect( &LogFont );
	}

	SelectObject( SurfaceHDC, pMainFont );

	const int FontNameLength = GetTextFace( SurfaceHDC, 0, NULL );
	char* const pFontName = new char[ FontNameLength ];
	GetTextFace( SurfaceHDC, FontNameLength, pFontName );
	if( Props.m_Face != pFontName )
	{
		PRINTF( "Desired font name: %s\n", Props.m_Face.CStr() );
		PRINTF( "Created font name: %s\n", pFontName );
		WARNDESC( "Created font name did not match desired font name; do you have the font installed?" );
	}
	SafeDeleteNoNull( pFontName );

	// Side effect: Set the main props font height now that we can select a font.
	TEXTMETRIC TextMetric;
	GetTextMetrics( SurfaceHDC, &TextMetric );

	if( Props.m_CapHeight > 0 )
	{
		PRINTF( "Font em height is %d, overriding with config CapHeight %d\n", TextMetric.tmAscent - TextMetric.tmInternalLeading, Props.m_CapHeight );
	}
	else
	{
		PRINTF( "Font em height is %d\n", TextMetric.tmAscent - TextMetric.tmInternalLeading );
	}

	// ROSANOTE: This is a new field that more properly represents the apparent height of the font regardless of internal leading
	// It can be overridden in config file.
	Props.m_CapHeight		= ( Props.m_CapHeight > 0 ) ? Props.m_CapHeight : ( TextMetric.tmAscent - TextMetric.tmInternalLeading );
	Props.m_Leading			= TextMetric.tmHeight + TextMetric.tmExternalLeading;	// ROSANOTE: This used to just be tmHeight but that doesn't account for the font's intended extra leading
	// ROSANOTE: New field used to position font baselines equivalently when different families have more leading
	// Not using TextMetric.tmInternalLeading just in case cap height is overridden.
	Props.m_InternalLeading	= TextMetric.tmAscent - Props.m_CapHeight;
	Props.m_Height			= Max( Props.m_Height, TextMetric.tmHeight );
	const uint ExtraHeight	= Props.m_Height - TextMetric.tmHeight;
	Props.m_VerticalA		= ( ExtraHeight / 2 );
	Props.m_VerticalC		= ( ExtraHeight - Props.m_VerticalA );	// This should generally be the same as m_VerticalA but accounts for it being an odd number

	const uint Width	= LocaleSurface.GetWidth();
	const uint Height	= LocaleSurface.GetHeight();

	RECT DrawRect;
	DrawRect.left	= 1;
	DrawRect.right	= Width;
	DrawRect.top	= 1 + Props.m_VerticalA;
	DrawRect.bottom	= Height;

	const int BaseLine = TextMetric.tmAscent;

	LocaleSurface.Clear( RGB_TO_COLOR( 255, 255, 255 ) );	// NOTE: Change blue to 0 to see the space between fonts (for testing/debugging)

	DrawCodePoints( LocaleProps.m_CodePointProps, LocaleProps, Props, LocaleSurface, DrawRect, BaseLine, true );

	if( pBoldFont )
	{
		SelectObject( SurfaceHDC, pBoldFont );
		DrawCodePoints( LocaleProps.m_CodePointPropsBold, LocaleProps, Props, LocaleSurface, DrawRect, BaseLine, false );
	}

	if( pItalicFont )
	{
		SelectObject( SurfaceHDC, pItalicFont );
		DrawCodePoints( LocaleProps.m_CodePointPropsItalic, LocaleProps, Props, LocaleSurface, DrawRect, BaseLine, false );
	}
}

// Save target surface to a non-RLE 32-bit TGA.
void SaveTGA( const IDataStream& Stream, const Surface& WindowSurface, const SFGLocaleProps& LocaleProps, const SFGProps& Props )
{
	STGAHeader TGAHeader;

	ZeroMemory( &TGAHeader, sizeof( STGAHeader ) );
	TGAHeader.m_ImageType		= 2;
	TGAHeader.m_Width			= static_cast<c_uint16>( WindowSurface.GetWidth() );
	TGAHeader.m_Height			= static_cast<c_uint16>( WindowSurface.GetHeight() );
	TGAHeader.m_BitDepth		= 32;
	TGAHeader.m_ImageDescriptor	= 8;

	Stream.Write( sizeof( TGAHeader ), &TGAHeader );

	const int Width		= WindowSurface.GetWidth();
	const int Height	= WindowSurface.GetHeight();
	byte r, g, b, a;
	STATICHASH( ColorR );
	STATICHASH( ColorG );
	STATICHASH( ColorB );
	r = (byte)ConfigManager::GetInt( sColorR, 255 );
	g = (byte)ConfigManager::GetInt( sColorG, 255 );
	b = (byte)ConfigManager::GetInt( sColorB, 255 );
	for( int j = Height - 1; j >= 0; --j )
	{
		for( int i = 0; i < Width; ++i )
		{
			// Paste in bitmap substitutes as needed
			bool UsedBitmapSub = false;
			FOR_EACH_ARRAY( BitmapSubIter, LocaleProps.m_BitmapSubstitutes, SBitmapSubstitute )
			{
				const SBitmapSubstitute& Bitmap = BitmapSubIter.GetValue();
				if( i >= Bitmap.m_X &&
					i < Bitmap.m_X + Bitmap.m_Width &&
					j >= Bitmap.m_Y &&
					j < Bitmap.m_Y + Bitmap.m_Height )
				{
					int LocalX = i - Bitmap.m_X;
					int LocalY = Bitmap.m_Height - ( j - Bitmap.m_Y ) - 1;

					if(
						LocalX >= 0 &&
						LocalX < Bitmap.m_Width &&
						LocalY >= 0 &&
						LocalY < Bitmap.m_Height )
					{
						UsedBitmapSub = true;
						byte* const pPixel = Bitmap.m_Pixels + LocalY * Bitmap.m_Width * 4 + LocalX * 4;
						Stream.Write( 4, pPixel );
						break;
					}
				}
			}

			if( !UsedBitmapSub )
			{
				// Use inverse blue of window surface as alpha
				// HACK: Gamma correct the alpha to bolden up antialiasing
				const byte	Blue			= *( WindowSurface.GetPointerAt( i, j ) );
				const float	Alpha			= 1.0f - ( static_cast<float>( Blue ) / 255.0f );
				const float AdjustedAlpha	= Pow( ( Alpha * Props.m_AlphaScalar ), Props.m_AlphaExponent );
				const float FinalAlpha		= Saturate( Props.m_Antialias ? AdjustedAlpha : Alpha );
				a = static_cast<byte>( FinalAlpha * 255.0f );

				Stream.WriteUInt8( b );
				Stream.WriteUInt8( g );
				Stream.WriteUInt8( r );
				Stream.WriteUInt8( a );
			}
		}
	}
}

void SaveCodePointProps( const IDataStream& Stream, const SFGCodePointProps& CodePointProps )
{
	// Just save the whole struct inline, it's densely packed.
	// On the runtime end, the first member (the unicode value)
	// will be serialized first to use as the key.
	Stream.Write( sizeof( SFGCodePointProps ), &CodePointProps );
}

void SaveLocaleProps( const IDataStream& Stream, const SFGLocaleProps& LocaleProps )
{
	// Write locale tag
	Stream.WriteHashedString( LocaleProps.m_LocaleTag );

	// Write relative image filename
	Stream.WriteString( LocaleProps.m_RuntimeImageFilename );

	// Write language array
	Stream.WriteUInt32( LocaleProps.m_Languages.Size() );
	FOR_EACH_ARRAY( LanguageIter, LocaleProps.m_Languages, SimpleString )
	{
		const SimpleString& Language = LanguageIter.GetValue();
		Stream.WriteHashedString( Language );
	}

	// Write code points
	Stream.WriteUInt32( LocaleProps.m_CodePointProps.Size() );
	FOR_EACH_ARRAY( CodePointPropsIter, LocaleProps.m_CodePointProps, SFGCodePointProps )
	{
		const SFGCodePointProps& CodePointProps = CodePointPropsIter.GetValue();
		SaveCodePointProps( Stream, CodePointProps );
	}
	Stream.WriteUInt32( LocaleProps.m_CodePointPropsBold.Size() );
	FOR_EACH_ARRAY( CodePointPropsIter, LocaleProps.m_CodePointPropsBold, SFGCodePointProps )
	{
		const SFGCodePointProps& CodePointProps = CodePointPropsIter.GetValue();
		SaveCodePointProps( Stream, CodePointProps );
	}
	Stream.WriteUInt32( LocaleProps.m_CodePointPropsItalic.Size() );
	FOR_EACH_ARRAY( CodePointPropsIter, LocaleProps.m_CodePointPropsItalic, SFGCodePointProps )
	{
		const SFGCodePointProps& CodePointProps = CodePointPropsIter.GetValue();
		SaveCodePointProps( Stream, CodePointProps );
	}
}

void SaveProps( const IDataStream& Stream, const SFGProps& Props )
{
	// Write global font properties
	Stream.WriteInt32( Props.m_CapHeight );
	Stream.WriteInt32( Props.m_Leading );
	Stream.WriteInt32( Props.m_InternalLeading );
	Stream.WriteInt32( Props.m_VerticalA );
	Stream.WriteInt32( Props.m_VerticalC );
	Stream.WriteInt32( Props.m_Height + 1 );	// Add 1 for the padding between characters (same as in ComputeWidth() for character props)

	// Then write per-locale properties
	Stream.WriteUInt32( Props.m_LocaleProps.Size() );
	FOR_EACH_ARRAY( LocalePropsIter, Props.m_LocaleProps, SFGLocaleProps )
	{
		const SFGLocaleProps& LocaleProps = LocalePropsIter.GetValue();
		SaveLocaleProps( Stream, LocaleProps );
	}
}

void PopulateBitmapCodePoints( const SFGProps& Props, SFGLocaleProps& LocaleProps )
{
	const float	fCharWidth	= static_cast<float>( Max( 0, LocaleProps.m_BitmapabcA ) + LocaleProps.m_BitmapabcB + Max( 0, LocaleProps.m_BitmapabcC ) );
	const float fCharHeight	= static_cast<float>( Props.m_Height );
	const uint	CharsPerRow	= static_cast<uint>( LocaleProps.m_TextureWidth / ( fCharWidth + 1.0f ) );

	uint	RowChar	= 0;
	float	X		= 1.0f;
	float	Y		= 1.0f;

	FOR_EACH_ARRAY( CodePointPropsIter, LocaleProps.m_CodePointProps, SFGCodePointProps )
	{
		SFGCodePointProps&	CodePointProps	= CodePointPropsIter.GetValue();

		CodePointProps.m_U1			= ( X - .5f )					/ LocaleProps.m_TextureWidth;
		CodePointProps.m_U2			= ( X + fCharWidth + 0.5f )		/ LocaleProps.m_TextureWidth;
		CodePointProps.m_V1			= ( Y - .5f )					/ LocaleProps.m_TextureHeight;
		CodePointProps.m_V2			= ( Y + fCharHeight + 0.5f )	/ LocaleProps.m_TextureHeight;
		CodePointProps.m_A			= LocaleProps.m_BitmapabcA;
		CodePointProps.m_B			= LocaleProps.m_BitmapabcB;
		CodePointProps.m_C			= LocaleProps.m_BitmapabcC;
		ComputeWidth( CodePointProps );

		if( ++RowChar == CharsPerRow )
		{
			RowChar	= 0;
			X		= 1.0f;
			Y		+= fCharHeight + 1.0f;
		}
		else
		{
			X		+= fCharWidth + 1.0f;
		}
	}
}

void PassThroughBitmapFilenames( SFGLocaleProps& LocaleProps )
{
	LocaleProps.m_RuntimeImageFilename = LocaleProps.m_BitmapTextureName;
}

// As a reminder, here's what I do with filename wrangling:
// 0. Given a target font name (../Baked/Fonts/foo.fnp) and an
// image template name (../Intermediate/Textures/Fonts/foo.tga).
// Font name does not need any wrangling. The following rules
// are for image filename wrangling only:
// 1. Insert a locale suffix (e.g. -Latin) before the extension
// (../Intermediate/Textures/Fonts/foo-Latin.tga)
// 2. Insert _NODXT and _DXT5 suffixes for no-compress and antialiased
// font sheets respectively. Does not apply to bitmap fonts.
// (../Intermediate/Textures/Fonts/foo-Latin_NODXT.tga)
// 3. Replace the .tga extension with .dds unless the font is a no-compress
// font or a bitmap font.
// (../Intermediate/Textures/Fonts/foo-Latin_NODXT.dds)
// 4. Trim the leading ../Intermediate/ in the version of the filenames
// that get saved into the .fnp, to produce a relative path for runtime.
// (Textures/Fonts/foo-Latin_NODXT.dds)

void WrangleLocaleFilenames( SFGLocaleProps& LocaleProps, const SFGProps& Props )
{
	// Shouldn't be wrangling image filenames for bitmap fonts.
	ASSERT( !LocaleProps.m_BitmapFont );

	const SimpleString& ImageFilenameTemplate = Props.m_OutputImageTemplate;
	SimpleString Name;
	SimpleString Extension;
	const uint ExtensionIndex = ImageFilenameTemplate.Length() - 4;
	ImageFilenameTemplate.SplitInPlace( ExtensionIndex, Name, Extension );

	// 1. Add locale suffix (if applicable)
	if( LocaleProps.m_LocaleTag != "" )
	{
		Name = SimpleString::PrintF( "%s-%s", Name.CStr(), LocaleProps.m_LocaleTag.CStr() );
	}

	// 2. Insert suffixes for settings
	SimpleString SettingsName = Name;
	if( Props.m_NoCompress )
	{
		Name += "_NODXT";
	}
	else if( Props.m_Antialias )
	{
		Name += "_DXT5";
	}
	else
	{
		Name += "_DXT1";
	}

	// 3. Replace the runtime extension
	Extension = Props.m_NoCompress ? ".tga" : ".dds";

	// 4. Trim ../Intermediate/ for runtime
	SimpleString Intermediate;
	SimpleString Remainder;
	if( Name.Contains( "Intermediate" ) )
	{
		Name.SplitInPlace( 16, Intermediate, Remainder );
	}
	else
	{
		Remainder = Name;
	}

	LocaleProps.m_ImageFilename			= Name + ".tga";
	LocaleProps.m_RuntimeImageFilename	= Remainder + Extension;
}

void AddUnicodeRange( const unicode_t RangeLow, const unicode_t RangeHigh, Array<SFGCodePointProps>& CodePointProps )
{
	ASSERT( RangeLow <= RangeHigh );

	for( unicode_t CodePoint = RangeLow; CodePoint <= RangeHigh; ++CodePoint )
	{
		SFGCodePointProps& Props = CodePointProps.PushBack();
		Props.m_CodePoint = CodePoint;
	}
}

// NOTE: We only have one locale per font anymore, defined by "Script"
void AddLocales( SFGProps& Props )
{
	SFGLocaleProps& LocaleProps	= Props.m_LocaleProps.PushBack();

	STATICHASH( Script );
	const SimpleString Locale	= ConfigManager::GetString( sScript, "" );
	LocaleProps.m_LocaleContext	= Locale;

	MAKEHASH( Locale );
	STATICHASH( Tag );
	LocaleProps.m_LocaleTag		= ConfigManager::GetString( sTag, "", sLocale );
}

// Load glyph substitute configuration data into the target props struct.
void AddGlyphSubstitutes( SFGProps& Props )
{
	STATICHASH( NumSubstitutes );
	const uint NumSubstitutes = ConfigManager::GetInt( sNumSubstitutes );

	for( uint SubstituteIndex = 0; SubstituteIndex < NumSubstitutes; ++SubstituteIndex )
	{
		SGlyphSubstitute NewSubstitute;

		const SimpleString SubstituteDef = ConfigManager::GetSequenceString( "Substitute%d", SubstituteIndex, "" );
		MAKEHASH( SubstituteDef );

		STATICHASH( CodePoint );
		const uint			CodePoint		= ConfigManager::GetInt( sCodePoint, 0, sSubstituteDef );
		const SimpleString	CodePointString	= ConfigManager::GetString( sCodePoint, "", sSubstituteDef );
		NewSubstitute.m_CodePoint = Pick( CodePoint, CodePointString.GetCodePoint() );

		NewSubstitute.m_SubType = SGlyphSubstitute::EST_Icon;	// This is the only substitution I support anymore.

		STATICHASH( Image );
		NewSubstitute.m_IconName = ConfigManager::GetString( sImage, NULL, sSubstituteDef );

		STATICHASH( SpacingLeft );
		NewSubstitute.m_SpacingLeft = ConfigManager::GetInt( sSpacingLeft, 0, sSubstituteDef );

		STATICHASH( SpacingRight );
		NewSubstitute.m_SpacingRight = ConfigManager::GetInt( sSpacingRight, 0, sSubstituteDef );

		STATICHASH( HeightOffset );
		NewSubstitute.m_HeightOffset = ConfigManager::GetInt( sHeightOffset, 0, sSubstituteDef );

		STATICHASH( PositionAtBaseline );
		NewSubstitute.m_PositionAtBaseline = ConfigManager::GetBool( sPositionAtBaseline, false, sSubstituteDef );

		Props.m_GlyphSubstitutes.PushBack( NewSubstitute );
	}
}

void AddLanguages( SFGLocaleProps& LocaleProps )
{
	MAKEHASHFROM( Locale, LocaleProps.m_LocaleContext );

	STATICHASH( NumLanguages );
	const uint NumLanguages = ConfigManager::GetInt( sNumLanguages, 0, sLocale );

	if( NumLanguages == 0 )
	{
		// Add default language if none is specified.
		LocaleProps.m_Languages.PushBack( "English" );
	}
	else
	{
		for( uint LanguageIndex = 0; LanguageIndex < NumLanguages; ++LanguageIndex )
		{
			const SimpleString Language = ConfigManager::GetSequenceString( "Language%d", LanguageIndex, "", sLocale );
			LocaleProps.m_Languages.PushBack( Language );
		}
	}
}

void AddCodePoints( SFGLocaleProps& LocaleProps, const SFGProps& Props )
{
	MAKEHASHFROM( Locale, LocaleProps.m_LocaleContext );

	STATICHASH( NumCodePointRanges );
	const uint NumCodePointRanges = ConfigManager::GetInt( sNumCodePointRanges, 0, sLocale );

	if( NumCodePointRanges == 0 )
	{
		// Add default characters if none are specified.
		AddUnicodeRange( 0x20, 0x7f, LocaleProps.m_CodePointProps );	// Main ASCII character set
		AddUnicodeRange( 0xa0, 0xff, LocaleProps.m_CodePointProps );	// Extended character set from ISO/IEC 8859-1 (or code page 1252)
		AddUnicodeRange( 0x09, 0x09, LocaleProps.m_CodePointProps );	// Tab
	}
	else
	{
		for( uint RangeIndex = 0; RangeIndex < NumCodePointRanges; ++RangeIndex )
		{
			const SimpleString	RangeSingleCode	= ConfigManager::GetSequenceString( "CodePointRange%d", RangeIndex, "", sLocale );
			const SimpleString	RangeLowCode	= ConfigManager::GetSequenceString( "CodePointRange%dLow", RangeIndex, "", sLocale );
			const SimpleString	RangeHighCode	= ConfigManager::GetSequenceString( "CodePointRange%dHigh", RangeIndex, "", sLocale );

			if( RangeSingleCode == "" )
			{
				const unicode_t		RangeLow		= RangeLowCode.GetCodePoint();
				const unicode_t		RangeHigh		= RangeHighCode.GetCodePoint();

				AddUnicodeRange( RangeLow, RangeHigh, LocaleProps.m_CodePointProps );
			}
			else
			{
				const unicode_t		RangeSingle		= RangeSingleCode.GetCodePoint();

				AddUnicodeRange( RangeSingle, RangeSingle, LocaleProps.m_CodePointProps );
			}
		}
	}

	if( Props.m_BoldFace != "" )
	{
		LocaleProps.m_CodePointPropsBold = LocaleProps.m_CodePointProps;
	}

	if( Props.m_ItalicFace != "" )
	{
		LocaleProps.m_CodePointPropsItalic = LocaleProps.m_CodePointProps;
	}
}

void PopulateLocaleProps( SFGLocaleProps& LocaleProps, const SFGProps& Props )
{
	MAKEHASHFROM( Locale, LocaleProps.m_LocaleContext );

	// For backward compatibility and simplicity, dimensions can
	// be defined in the main section or overridden per locale.
	STATICHASH( Width );
	STATICHASH( Height );
	LocaleProps.m_TextureWidth	= ConfigManager::GetInt( sWidth, ConfigManager::GetInt( sWidth ), sLocale );
	LocaleProps.m_TextureHeight	= ConfigManager::GetInt( sHeight, ConfigManager::GetInt( sHeight ), sLocale );

	STATICHASH( BitmapFont );
	LocaleProps.m_BitmapFont = ConfigManager::GetBool( sBitmapFont, ConfigManager::GetBool( sBitmapFont ), sLocale );

	STATICHASH( Texture );
	LocaleProps.m_BitmapTextureName = ConfigManager::GetString( sTexture, ConfigManager::GetString( sTexture, "" ), sLocale );

	STATICHASH( abcA );
	LocaleProps.m_BitmapabcA = ConfigManager::GetInt( sabcA, ConfigManager::GetInt( sabcA ), sLocale );

	STATICHASH( abcB );
	LocaleProps.m_BitmapabcB = ConfigManager::GetInt( sabcB, ConfigManager::GetInt( sabcB ), sLocale );

	STATICHASH( abcC );
	LocaleProps.m_BitmapabcC = ConfigManager::GetInt( sabcC, ConfigManager::GetInt( sabcC ), sLocale );

	AddLanguages( LocaleProps );
	AddCodePoints( LocaleProps, Props );
}

void PopulateProps( SFGProps& Props )
{
	STATICHASH( Size );
	Props.m_PointSize = ConfigManager::GetInt( sSize, 12 );

	STATICHASH( Weight );
	Props.m_Weight = ConfigManager::GetInt( sWeight, FW_REGULAR );

	STATICHASH( Face );
	Props.m_Face = ConfigManager::GetString( sFace, "Arial" );

	STATICHASH( BoldFace );
	Props.m_BoldFace = ConfigManager::GetString( sBoldFace, "" );

	STATICHASH( ItalicFace );
	Props.m_ItalicFace = ConfigManager::GetString( sItalicFace, "" );

	STATICHASH( Antialias );
	Props.m_Antialias = ConfigManager::GetBool( sAntialias );

	STATICHASH( NoCompress );
	Props.m_NoCompress = ConfigManager::GetBool( sNoCompress );

	STATICHASH( Italics );
	Props.m_Italics = ConfigManager::GetBool( sItalics );

	STATICHASH( MinHeight );
	Props.m_Height = ConfigManager::GetInt( sMinHeight );

	STATICHASH( CapHeight );
	Props.m_CapHeight = ConfigManager::GetInt( sCapHeight );

	STATICHASH( AlphaScalar );
	Props.m_AlphaScalar = ConfigManager::GetFloat( sAlphaScalar, 1.0f );

	STATICHASH( AlphaExponent );
	Props.m_AlphaExponent = ConfigManager::GetFloat( sAlphaExponent, 1.0f );

	AddLocales( Props );
	FOR_EACH_ARRAY( LocaleIter, Props.m_LocaleProps, SFGLocaleProps )
	{
		PopulateLocaleProps( LocaleIter.GetValue(), Props );
	}

	AddGlyphSubstitutes( Props );
}

void DrawAndSaveImage( SFGLocaleProps& LocaleProps, SFGProps& Props )
{
	Surface LocaleSurface( LocaleProps.m_TextureWidth, LocaleProps.m_TextureHeight );
	DrawFont( LocaleSurface, LocaleProps, Props );

	// Save bitmaps as we process locales, but only save .fnp at the end.
	SaveTGA( FileStream( LocaleProps.m_ImageFilename.CStr(), FileStream::EFM_Write ), LocaleSurface, LocaleProps, Props );
}

int main( int argc, char* argv[] )
{
	if( argc != 4 )
	{
		PRINTF( "Syntax: FontGenerator <infile> <outfontfile> <outimagefile>\n" );
		return 0;
	}

	const char*	InFile			= argv[1];
	ASSERT( InFile );

	const char*	OutFontFile		= argv[2];
	ASSERT( OutFontFile );

	const char*	OutImageFile	= argv[3];	// NOTE: The actual output filename(s) will be modified per locale and according to settings.
	ASSERT( OutImageFile );

	ConfigManager::Load( FileStream( InFile, FileStream::EFM_Read ) );

	SimpleString InFilePath;
	SimpleString InFileName;
	FileUtil::SplitLeadingPath( InFile, InFilePath, InFileName );

	STATICHASH( NumIncludes );
	const uint NumIncludes = ConfigManager::GetInt( sNumIncludes );
	for( uint IncludeIndex = 0; IncludeIndex < NumIncludes; ++IncludeIndex )
	{
		const SimpleString IncludeFilename		= ConfigManager::GetSequenceString( "Include%d", IncludeIndex, "" );
		const SimpleString IncludeFullFilename	= InFilePath + IncludeFilename;
		ConfigManager::Load( FileStream( IncludeFullFilename.CStr(), FileStream::EFM_Read ) );
	}

	SFGProps Props;
	Props.m_OutputFontFilename	= OutFontFile;
	Props.m_OutputImageTemplate	= OutImageFile;
	PopulateProps( Props );
	ASSERT( Props.m_LocaleProps.Size() );

	FOR_EACH_ARRAY( LocaleIter, Props.m_LocaleProps, SFGLocaleProps )
	{
		SFGLocaleProps& LocaleProps = LocaleIter.GetValue();
		if( LocaleProps.m_BitmapFont )
		{
			PopulateBitmapCodePoints( Props, LocaleProps );
			PassThroughBitmapFilenames( LocaleProps );
		}
		else
		{
			WrangleLocaleFilenames( LocaleProps, Props );
			DrawAndSaveImage( LocaleProps, Props );
		}
	}

	SaveProps( FileStream( Props.m_OutputFontFilename.CStr(), FileStream::EFM_Write ), Props );

	return 0;
}

LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
