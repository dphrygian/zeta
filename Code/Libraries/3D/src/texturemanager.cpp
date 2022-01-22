#include "core.h"
#include "3d.h"
#include "texturemanager.h"
#include "irenderer.h"
#include "itexture.h"

#include <string.h>

TextureManager::TextureManager( IRenderer* Renderer )
:	m_TextureTable()
,	m_Renderer( Renderer )
{
}

TextureManager::~TextureManager()
{
	FreeTextures( ETL_Permanent );
}

void TextureManager::FreeTextures( ETextureLife Life )
{
	FOR_EACH_MAP_NOINCR( TextureIter, m_TextureTable, HashedString, SManagedTexture )
	{
		SManagedTexture& ManagedTexture = TextureIter.GetValue();
		if( ManagedTexture.m_Life <= Life )
		{
			SafeDelete( ManagedTexture.m_Texture );
			m_TextureTable.Remove( TextureIter );
		}
		else
		{
			++TextureIter;
		}
	}
}

ITexture* TextureManager::GetTexture( const char* Filename, const ETextureLife Life /*= ETL_Permanent*/, const bool NoMips /*= false*/ )
{
	XTRACE_FUNCTION;

	HashedString HashedFilename( Filename );

	SManagedTexture& ManagedTexture = m_TextureTable[ HashedFilename ];	// Creates the managed texture if it didn't exist before

	if( !ManagedTexture.m_Texture )
	{
		ManagedTexture.m_Texture = m_Renderer->CreateTexture( Filename, NoMips );
	}

	// Promote lifetime if requested higher
	if( ManagedTexture.m_Life < Life )
	{
		ManagedTexture.m_Life = Life;
	}

	return ManagedTexture.m_Texture;
}

ITexture* TextureManager::GetCubemap( const SimpleString& CubemapDef, const ETextureLife Life /*= ETL_Permanent*/, const bool NoMips /*= false*/ )
{
	XTRACE_FUNCTION;

	HashedString		HashedCubemapDef	= CubemapDef;
	SManagedTexture&	ManagedTexture		= m_TextureTable[ HashedCubemapDef ];	// Creates the managed texture if it didn't exist before

	if( !ManagedTexture.m_Texture )
	{
		ManagedTexture.m_Texture = m_Renderer->CreateCubemap( CubemapDef, NoMips );
	}

	// Promote lifetime if requested higher
	if( ManagedTexture.m_Life < Life )
	{
		ManagedTexture.m_Life = Life;
	}

	return ManagedTexture.m_Texture;
}
