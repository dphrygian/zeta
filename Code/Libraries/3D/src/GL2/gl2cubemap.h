#ifndef GL2CUBEMAP_H
#define GL2CUBEMAP_H

#include "cubemapcommon.h"
#include "gl2texture.h"
#include "gl2.h"
#include "renderstates.h"

class GL2Cubemap : public CubemapCommon
{
public:
	GL2Cubemap();
	GL2Cubemap( GLuint CubeTexture );
	virtual ~GL2Cubemap();

	virtual void*			GetHandle();
	virtual void			CreateCubemap( const SCubemapData& CubemapData );
	virtual SSamplerState*	GetSamplerState() { return &m_SamplerState; }

private:
	// Pass through to texture classes
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData ) { GL2Texture::StaticLoadDDS( Stream, OutTextureData ); }

	GLuint			m_CubeTexture;

	// Shadow sampler state per-texture in GL, because that's how GL manages sampler state.
	SSamplerState	m_SamplerState;
};

#endif // GL2TEXTURE_H
