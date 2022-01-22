#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "map.h"
#include "hashedstring.h"

// ROSATODO: Replace this stuff, it's getting to be too project-aware
#define DEFAULT_TEXTURE	"Textures/default-a.tga"
#define DEFAULT_ALBEDO	"Textures/default-a.tga"
#define DEFAULT_NORMAL	"Textures/default-n.tga"
#define DEFAULT_SPEC	"Textures/default-s.tga"

class IRenderer;
class ITexture;

class TextureManager
{
public:
	TextureManager( IRenderer* Renderer );
	~TextureManager();

	// Should be sorted from least to most permanent
	enum ETextureLife
	{
		ETL_Null,
		ETL_World,		// For environments and characters, only persist for a level
		ETL_Permanent,	// For fonts, UI, etc.
	};

	struct SManagedTexture
	{
		SManagedTexture()
		:	m_Texture( NULL )
		,	m_Life( ETL_Null )
		{
		}

		ITexture*		m_Texture;
		ETextureLife	m_Life;
	};

	void		FreeTextures( ETextureLife Life );
	ITexture*	GetTexture( const char* Filename, const ETextureLife Life = ETL_Permanent, const bool NoMips = false );
	ITexture*	GetTextureNoMips( const char* Filename ) { return GetTexture( Filename, ETL_Permanent, true ); }
	ITexture*	GetCubemap( const SimpleString& CubemapDef, const ETextureLife Life = ETL_Permanent, const bool NoMips = false );
	ITexture*	GetCubemapNoMips( const SimpleString& CubemapDef ) { return GetCubemap( CubemapDef, ETL_Permanent, true ); }

private:
	Map<HashedString, SManagedTexture>	m_TextureTable;
	IRenderer*							m_Renderer;
};

#endif // TEXTUREMANAGER_H
