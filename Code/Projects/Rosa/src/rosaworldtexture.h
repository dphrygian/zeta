#ifndef ROSAWORLDTEXTURE_H
#define ROSAWORLDTEXTURE_H

#include "texturecommon.h"

class RosaWorldTexture : public TextureCommon
{
public:
	RosaWorldTexture();
	virtual ~RosaWorldTexture();

	virtual void*			GetHandle()			{ return NULL; }
	virtual SSamplerState*	GetSamplerState()	{ return NULL; }

	const STextureData&	GetData() const	{ return m_Data; }
	STextureData&		GetData()		{ return m_Data; }

private:
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData );
	virtual void	CreateTexture( const STextureData& TextureData );

	STextureData	m_Data;
};

#endif // ROSAWORLDTEXTURE_H
