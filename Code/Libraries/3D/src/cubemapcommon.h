#ifndef CUBEMAPCOMMON_H
#define CUBEMAPCOMMON_H

#include "itexture.h"
#include "texturecommon.h"

struct SCubemapData
{
	SCubemapData()
	:	m_Textures()
	{
	}

	// Ordered X+, X-, Y+, Y-, Z+, Z-
	// (or right, left, front, back, up, down)
	STextureData	m_Textures[6];
};

class CubemapCommon : public ITexture
{
public:
	CubemapCommon();
	virtual ~CubemapCommon();

	virtual bool	IsCubemap() const { return true; }

	virtual void	CreateCubemap( const SCubemapData& CubemapData ) = 0;

	// NoMips is just a hint; DXT images may already have mips, and those will be loaded
	void			Initialize( const SimpleString& CubemapDef, const bool NoMips );

protected:
	// Pass through to texture classes
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData ) = 0;
};

#endif // CUBEMAPCOMMON_H
