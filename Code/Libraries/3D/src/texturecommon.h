#ifndef TEXTURECOMMON_H
#define TEXTURECOMMON_H

#include "itexture.h"
#include "3d.h"
#include "array.h"

class IDataStream;

typedef Array<byte>			TTextureMip;
typedef Array<TTextureMip>	TTextureMipChain;

enum EImageFormat
{
	EIF_Unknown,
	EIF_ARGB8,
	EIF_ARGB32F,
	EIF_DXT1,
	EIF_DXT3,
	EIF_DXT5,
};

struct STextureData
{
	STextureData()
	:	m_Width( 0 )
	,	m_Height( 0 )
	,	m_Format( EIF_Unknown )
	,	m_MipChain()
	{
	}

	uint				m_Width;
	uint				m_Height;
	EImageFormat		m_Format;
	TTextureMipChain	m_MipChain;
};

class TextureCommon : public ITexture
{
public:
	TextureCommon();
	virtual ~TextureCommon();

	virtual bool	IsCubemap() const { return false; }

	// NoMips is just a hint; DXT images may already have mips, and those will be loaded
	void			Initialize( const SimpleString& Filename, const bool NoMips );

	// Static + public for reuse in cubemap code
	static uint		StaticCountMipLevels( const uint Width, const uint Height );
	static void		StaticMakeMip( STextureData& OutTextureData, const uint MipLevel );
	static void		StaticLoadBMP( const IDataStream& Stream, STextureData& OutTextureData, const bool NoMips );
	static void		StaticLoadTGA( const IDataStream& Stream, STextureData& OutTextureData, const bool NoMips );

	static void		StaticSaveTGA( const IDataStream& Stream, const STextureData& TextureData );

protected:
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData ) = 0;
	virtual void	CreateTexture( const STextureData& TextureData ) = 0;
};

#endif // TEXTURECOMMON_H
