#include "core.h"
#include "3d.h"
#include "fontmanager.h"
#include "irenderer.h"
#include "font.h"
#include "packstream.h"
#include "simplestring.h"
#include "configmanager.h"
#include "mathcore.h"

FontManager::FontManager( IRenderer* Renderer )
:	m_FontTable()
,	m_Renderer( Renderer )
{
}

FontManager::~FontManager()
{
	FreeFonts();
}

void FontManager::FreeFonts()
{
	FOR_EACH_MAP( MapIter, m_FontTable, HashedString, TFontArray )
	{
		TFontArray& FontArray = MapIter.GetValue();
		FOR_EACH_ARRAY( ArrayIter, FontArray, Font* )
		{
			Font* pFont = ArrayIter.GetValue();
			SafeDelete( pFont );
		}
	}
	m_FontTable.Clear();
}

// Return the smallest font that is equal to or larger than the target height
// This assumes font sheets are ordered smallest to largest, hence the reverse iteration!
Font* FontManager::GetFontFromArray( const TFontArray& FontArray, const float Height ) const
{
	DEVASSERT( FontArray.Size() );
	uint BestIndex = FontArray.Size() - 1;	// Start by assuming we'll use the largest font

	FOR_EACH_ARRAY_REVERSE( ArrayIter, FontArray, Font* )
	{
		Font* const pFont		= ArrayIter.GetValue();
		const float	FontHeight	= pFont->GetCapHeight();

		if( FontHeight < Height )
		{
			// Assuming font sheets are ordered smallest to largest, we won't get a better match now.
			break;
		}

		BestIndex = ArrayIter.GetIndex();
	}

	return FontArray[ BestIndex ];
}

Font* FontManager::GetFont( const SimpleString& Tag, const float Height )
{
	HashedString HashedTag = Tag;

	Map<HashedString, TFontArray>::Iterator FontIter = m_FontTable.Search( HashedTag );
	if( FontIter.IsValid() )
	{
		const TFontArray& FontArray = FontIter.GetValue();
		return GetFontFromArray( FontArray, Height );
	}
	else
	{
		TFontArray& FontArray = m_FontTable.Insert( HashedTag );

		// Map from tag to filename based on locale; default to the tag itself since
		// this is now used to look up *another* config context.
		const HashedString FontArrayHash = ConfigManager::GetLocalizedHash( HashedTag, HashedTag );

		STATICHASH( NumFontSheets );
		const uint NumFontSheets = ConfigManager::GetInheritedInt( sNumFontSheets, 0, FontArrayHash );
		DEVASSERT( NumFontSheets > 0 );

		FOR_EACH_INDEX( FontSheetIndex, NumFontSheets )
		{
			const SimpleString FontSheetName = ConfigManager::GetInheritedSequenceString( "FontSheet%d", FontSheetIndex, "", FontArrayHash );

			Font* const pFont = new Font;
			pFont->Initialize( PackStream( FontSheetName.CStr() ), m_Renderer );

			FontArray.PushBack( pFont );
		}

		return GetFontFromArray( FontArray, Height );
	}
}
