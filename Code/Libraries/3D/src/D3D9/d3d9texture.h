#ifndef D3D9TEXTURE_H
#define D3D9TEXTURE_H

#include "texturecommon.h"

struct IDirect3DDevice9;
struct IDirect3DTexture9;

class D3D9Texture : public TextureCommon
{
public:
	D3D9Texture( IDirect3DDevice9* D3DDevice );
	D3D9Texture( IDirect3DDevice9* D3DDevice, IDirect3DTexture9* Texture );
	virtual ~D3D9Texture();

	virtual void*			GetHandle();
	virtual SSamplerState*	GetSamplerState() { return NULL; }

	// Static + public for reuse in cubemap code, loads DXT-compressed image from DDS into a struct
	static void		StaticLoadDDS( const IDataStream& Stream, STextureData& OutTextureData );

private:
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData ) { StaticLoadDDS( Stream, OutTextureData ); }
	virtual void	CreateTexture( const STextureData& TextureData );

	IDirect3DTexture9*	m_Texture;
	IDirect3DDevice9*	m_D3DDevice;
};

#endif // D3D9TEXTURE_H
