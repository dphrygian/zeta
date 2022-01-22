#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include "map.h"
#include "array.h"
#include "hashedstring.h"

#define DEFAULT_FONT_TAG			"OpenSans-Tiny"
#define DEFAULT_MONOSPACED_FONT_TAG	"Cousine-Tiny"

class IRenderer;
class Font;
class SimpleString;

class FontManager
{
public:
	FontManager( IRenderer* Renderer );
	~FontManager();

	void	FreeFonts();
	Font*	GetFont( const SimpleString& Tag ) { return GetFont( Tag, 0.0f ); }	// Used when we aren't going to scale the font
	Font*	GetFont( const SimpleString& Tag, const float Height );

private:
	typedef Array<Font*> TFontArray;

	Font*	GetFontFromArray( const TFontArray& FontArray, const float Height ) const;

	Map<HashedString, TFontArray>	m_FontTable;
	IRenderer*						m_Renderer;
};

#endif // FONTMANAGER_H
