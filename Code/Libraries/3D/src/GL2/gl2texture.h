#ifndef GL2TEXTURE_H
#define GL2TEXTURE_H

#include "texturecommon.h"
#include "gl2.h"
#include "renderstates.h"

class GL2Texture : public TextureCommon
{
public:
	GL2Texture();
	GL2Texture( GLuint Texture );
	virtual ~GL2Texture();

	virtual void*			GetHandle();
	virtual SSamplerState*	GetSamplerState() { return &m_SamplerState; }

	// Static + public for reuse in cubemap code, loads DXT-compressed image from DDS into a struct
	static void		StaticLoadDDS( const IDataStream& Stream, STextureData& OutTextureData );

private:
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData ) { StaticLoadDDS( Stream, OutTextureData ); }
	virtual void	CreateTexture( const STextureData& TextureData );

	GLuint			m_Texture;

	// Shadow sampler state per-texture in GL, because that's how GL manages sampler state.
	// NOTE: This means I should never have two GL2Texture objects representing the same
	// OpenGL texture object, because the state will be shadowed twice and get out of sync.
	SSamplerState	m_SamplerState;
};

#endif // GL2TEXTURE_H
